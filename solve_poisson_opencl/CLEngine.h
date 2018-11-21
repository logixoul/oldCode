#pragma once
#include "util.h"
#include "precompiled.h"
#include "clfft.h"

// todo: rm dupe
struct CLErrorCheck {
	void operator=(cl_int res) {
		if (res != CL_SUCCESS) {
			cout << "res = " << res << endl;
			throw std::runtime_error("foo");
		}
	}
};

class CLBuffer2D {
public:
	int area;
	int w, h;
	int sizeofT;
	int NumBytes() const {
		return area * sizeofT;
	}
	cl::Buffer impl;
	ci::ivec2 Size() const { return ci::ivec2(w, h); }
	CLBuffer2D() {
	}
	template<class T>
	CLBuffer2D(Array2D<T> const& src, cl::Buffer impl) {
		sizeofT = sizeof(T);
		w = src.w;
		h = src.h;
		area = src.area;
		this->impl = impl;
	}
	CLBuffer2D(gl::TextureRef const& src, int sizeofT, cl::Buffer impl) {
		this->sizeofT = sizeofT;
		w = src->getWidth();
		h = src->getHeight();
		area = w * h;
		this->impl = impl;
	}
	CLBuffer2D(ivec2 sz, int sizeofT, cl::Buffer impl) {
		this->sizeofT = sizeofT;
		w = sz.x;
		h = sz.y;
		area = w * h;
		this->impl = impl;
	}
	cl::Buffer operator()() {
		return impl;
	}
};

static const std::string compmulKernel = MULTILINE(
	float2 compmul(float2 p, float2 q) {
		float a = p.x, b = p.y;
		float c = q.x, d = q.y;
		float ac = a * c;
		float bd = b * d;
		float2 out;
		out.x = ac - bd;
		out.y = (a + b) * (c + d) - ac - bd;
		return out;
	}
	void kernel compmulKernel(global const float2* A, global const float2* B, global float2* C) {
		int gid = get_global_id(0);
		C[gid] = compmul(A[gid], B[gid]);
	}
);

class CLEngine {
public:
	static cl::Context context;
	static cl::Program compmulProgram;
	static cl::CommandQueue queue;
	static cl::Device m_device;
	static cl::Platform m_platform;

	static vector<cl_context_properties> getContextPropertyVector(cl::Platform platform);

	static void init();

	static CLBuffer2D compmulArrays(CLBuffer2D a, CLBuffer2D b) {
		ivec2 sz = a.Size();
		int N = sizeof(vec2)*sz.x*sz.y;
		cl::Buffer resultBuffer(context, CL_MEM_READ_WRITE, N);
		
		static cl::make_kernel<cl::Buffer&, cl::Buffer&, cl::Buffer&> myKernel(compmulProgram, "compmulKernel");
		// Todo: why NullRange
		cl::EnqueueArgs eargs(queue, cl::NullRange, cl::NDRange(sz.x * sz.y), cl::NullRange);
		myKernel(eargs, a(), b(), resultBuffer);

		return CLBuffer2D(sz, sizeof(vec2), resultBuffer);
	}

	static CLBuffer2D fftImpl(cl::Buffer& inc, ivec2 sz, clfftDirection dir) {
		size_t N = sz.x * sz.y * sizeof(vec2);

		cl::Buffer res(context, CL_MEM_READ_WRITE, N);

		// todo: make this use a cache instead of being static.
		static clfft::Plan plan(sz, queue, context);

		runClfftPlan(plan, inc(), res(), dir);

		return CLBuffer2D(sz, sizeof(vec2), inc);
	}

	static void runClfftPlan(clfft::Plan plan, cl_mem in, cl_mem out, clfftDirection dir);
	
	static CLBuffer2D fft(cl::Buffer& inc, ivec2 sz) {
		return fftImpl(inc, sz, CLFFT_FORWARD);
	}
	static CLBuffer2D ifft(cl::Buffer& inc, ivec2 sz) {
		return fftImpl(inc, sz, CLFFT_BACKWARD);
	}

	static cl::Program makeProgram(std::string const& program_code) {
		cl::Program::Sources sources;

		sources.push_back(std::make_pair<const char*, size_t>(program_code.c_str(), program_code.length()));

		auto program = cl::Program(context, sources);
		std::vector<cl::Device> deviceArg = { m_device };
		try {
			program.build(deviceArg);
		} catch(cl::Error) {
			console() << " Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(m_device) << endl;
			throw std::runtime_error("");
		}
		return program;
	}

	static cl::Kernel makeKernel(std::string const& programCode, std::string const& kernelName) {
		return cl::Kernel(makeProgram(programCode), kernelName.c_str());
	}
};

