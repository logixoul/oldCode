#include "precompiled.h"
#include "solve_poisson.h"
#include "CLEngine.h"
#include "cfg1.h"
#include "stuff.h"
#include "stefanfw.h"

template<class T>
CLBuffer2D gbuf(Array2D<T> arr) {
	size_t N = arr.NumBytes();

	cl::Buffer buffer(CLEngine::context, CL_MEM_READ_WRITE, N);

	CLEngine::queue.enqueueWriteBuffer(buffer, CL_FALSE, 0, N, arr.data);
	return CLBuffer2D(arr, buffer);
}

GLuint pbo; // tmp global

static void createPbo(ivec2 sz, int elementSize) {
	glGenBuffers(1, &pbo);
	glBindBuffer(GL_ARRAY_BUFFER, pbo);
	//specifying the buffer size
	// todo: what should the last 2 args be?
	glBufferData(GL_ARRAY_BUFFER, sz.x * sz.y * elementSize, NULL, GL_DYNAMIC_COPY);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, NULL);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

template<class T>
CLBuffer2D gbuf(gl::TextureRef tex) {
	//size_t N = tex.getWidth() * tex.getHeight() * sizeof(T);

	auto sz = ivec2(tex->getActualWidth(), tex->getActualHeight());
	bind(tex);
	createPbo(sz, sizeof(T));
	// was: CL_MEM_WRITE_ONLY
	CLErrorCheck err;
	cl_int err2;
	//auto clCreateFromGLBuffer = (clGetGLContextInfoKHR_fn)clGetExtensionFunctionAddressForPlatform(CLEngine::default_platform(), "clCreateFromGLBuffer");
	//auto mem = clCreateFromGLBuffer(CLEngine::context(), CL_MEM_READ_WRITE, pbo, &err2);
	auto mem = cl::BufferGL(CLEngine::context, CL_MEM_READ_WRITE, pbo, NULL);
	//err = err2;
	cl::Buffer buffer = cl::Buffer(mem);
	glFinish();
	// todo: release
	clEnqueueAcquireGLObjects(CLEngine::queue(), 1, &mem(), 0, 0, NULL);
	return CLBuffer2D(tex, sizeof(T), buffer);
}

static void streamPboToTex(gl::TextureRef tex) {
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
	glBindTexture(GL_TEXTURE_2D, tex->getId());
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex->getActualWidth(), tex->getActualHeight(), GL_RED, GL_FLOAT, NULL);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

static const std::string r2cKernel = MULTILINE(
	void kernel r2cKernel(global const float* A, global float2* B) {
	int gid = get_global_id(0);
	B[gid] = (float2)(A[gid], 0.0f);
}
);

static const std::string c2rKernel = MULTILINE(
	void kernel c2rKernel(global const float2* A, global float* B) {
	int gid = get_global_id(0);
	B[gid] = A[gid].x;
}
);

/*cl::Buffer r2c(cl::Buffer inr, ivec2 sz) {
	static cl::make_kernel<cl::Buffer&, cl::Buffer&> myKernel(
		CLEngine::instance->makeProgram(r2cKernel), "r2cKernel");
	int N = sizeof(vec2)*sz.x*sz.y;
	cl::Buffer inc(CLEngine::instance->context, CL_MEM_READ_WRITE, N);
	cl::EnqueueArgs eargs(CLEngine::instance->queue, cl::NullRange, cl::NDRange(sz.x * sz.y), cl::NullRange);
	myKernel(eargs, inr, inc);
	return inc;
}*/

static CLBuffer2D r2c(CLBuffer2D inr) {
	static auto myKernel = CLEngine::makeKernel(r2cKernel, "r2cKernel");
	int N = sizeof(vec2)*inr.area;
	cl::Buffer inc(CLEngine::context, CL_MEM_READ_WRITE, N);
	myKernel.setArg(0, inr());
	myKernel.setArg(1, inc());
	CLEngine::queue.enqueueNDRangeKernel(myKernel,
		cl::NullRange, cl::NDRange(inr.area), cl::NullRange);
	return CLBuffer2D(inr.Size(), sizeof(float) * 2, inc);
}

static cl::Buffer c2r(CLBuffer2D inc) {
	static auto myKernel = CLEngine::makeKernel(c2rKernel, "c2rKernel");
	int N = sizeof(float)*inc.area;
	cl::Buffer inr(CLEngine::context, CL_MEM_READ_WRITE, N);
	myKernel.setArg(0, inc());
	myKernel.setArg(1, inr());
	CLEngine::queue.enqueueNDRangeKernel(myKernel,
		cl::NullRange, cl::NDRange(inc.area), cl::NullRange);
	return inr;
}

static CLBuffer2D getInvLaplacef(ivec2 size)
{
	Array2D<vec2> greensFunc(size, vec2());
	auto center = size / 2;
	float eps = cfg1::getOpt("eps [e]", 0.15, 'e', [&]() { return expRange(mouseY, 0.001, 0.15); });
	forxy(greensFunc) {
		ivec2 p2 = p;
		if (p2.x >= center.x) p2.x -= size.x;
		if (p2.y >= center.y) p2.y -= size.y;
		auto dist = length(vec2(p2));
		if (dist == 0.0f)
			greensFunc(p).x = log(eps);
		else
			greensFunc(p).x = log(dist);
	}
	forxy(greensFunc) {
		greensFunc(p) /= sqrt(greensFunc.area);
	}
	auto greensFuncBuf = gbuf<vec2>(greensFunc);
	return CLEngine::fft(greensFuncBuf(), greensFunc.Size());
}

Array2D<float> solvePoissonFft(gl::TextureRef Img)
{
	static CLBuffer2D invLaplacef;
	static ivec2 sz;
	if (true || sz != Img->getSize()) {
		invLaplacef = getInvLaplacef(Img->getSize());
		sz = Img->getSize();
	}
	auto ImgTexR = Img;
	auto ImgBufR = gbuf<float>(ImgTexR);
	auto ImgBufC = r2c(ImgBufR); // up to this point - 1.9GB usage
	CLEngine::fft(ImgBufC(), sz);
	auto ImgBufF = ImgBufC;
	ImgBufF = CLEngine::compmulArrays(ImgBufF, invLaplacef);
	CLEngine::ifft(ImgBufF(), sz);
	auto solutionBufC = ImgBufF;
	auto solutionBufR = c2r(solutionBufC);
	auto solutionR = Array2D<float>(sz);
	CLEngine::queue.enqueueReadBuffer(solutionBufR, CL_TRUE, 0, solutionR.NumBytes(), solutionR.data);
	vector<cl::Memory> vec{ ImgBufR() };
	CLEngine::queue.enqueueReleaseGLObjects(&vec);
	//streamPboToTex(ImgTexR);
	glDeleteBuffers(1, &pbo);
	return solutionR;
}
