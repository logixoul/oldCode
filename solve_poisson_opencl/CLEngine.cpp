#include "precompiled.h"
#include "CLEngine.h"

cl::Context CLEngine::context;
cl::Program CLEngine::compmulProgram;
cl::CommandQueue CLEngine::queue;
cl::Device CLEngine::m_device;
cl::Platform CLEngine::m_platform;

static const std::string CL_GL_SHARING_EXT = "cl_khr_gl_sharing";

static bool checkExtnAvailability(cl::Device pDevice, std::string pName)
{
	bool ret_val = true;
	try {
		// find extensions required
		std::string exts = pDevice.getInfo<CL_DEVICE_EXTENSIONS>();
		std::stringstream ss(exts);
		std::string item;
		int found = -1;
		while (std::getline(ss, item, ' ')) {
			if (item == pName) {
				found = 1;
				break;
			}
		}
		if (found == 1) {
			//std::cout << "Found CL_GL_SHARING extension: " << item << std::endl;
			ret_val = true;
		}
		else {
			//std::cout << "CL_GL_SHARING extension not found\n";
			ret_val = false;
		}
	}
	catch (cl::Error err) {
		std::cout << err.what() << "(" << err.err() << ")" << std::endl;
	}
	return ret_val;
}

static vector<cl::Device> filterBySharingExtension(vector<cl::Device> const& in) {
	vector<cl::Device> out;
	for (auto& device: in) {
		if (device() == 0) // why is this needed?
			continue;
		if (checkExtnAvailability(device, CL_GL_SHARING_EXT)) {
			out.push_back(device);
		}
	}
	return out;
}

vector<cl_context_properties> CLEngine::getContextPropertyVector(cl::Platform platform) {
	auto pData = gl::context()->getPlatformData();
	auto pDataWin = (gl::PlatformDataMsw*)gl::context()->getPlatformData().get();
	/*cl_context_properties props[] =
	{
		//OpenGL context
		CL_GL_CONTEXT_KHR,   (cl_context_properties)pDataWin->mGlrc,
		//HDC used to create the OpenGL context
		CL_WGL_HDC_KHR,     (cl_context_properties)pDataWin->mDc,
		//OpenCL platform
		CL_CONTEXT_PLATFORM, (cl_context_properties)default_platform(),
		0
	};*/
	vector<cl_context_properties> props =
	{
		//OpenGL context
		CL_GL_CONTEXT_KHR,   (cl_context_properties)wglGetCurrentContext(),
		//HDC used to create the OpenGL context
		CL_WGL_HDC_KHR,     (cl_context_properties)wglGetCurrentDC(),
		//OpenCL platform
		CL_CONTEXT_PLATFORM, (cl_context_properties)platform(),
		0
	};
	return props;
}

static vector<cl::Device> getClDevicesForGLContext(vector<cl_context_properties> const& clContextPropertyVector, cl::Platform& platform) {
	clGetGLContextInfoKHR_fn clGetGLContextInfo = (clGetGLContextInfoKHR_fn)clGetExtensionFunctionAddressForPlatform(platform(), "clGetGLContextInfoKHR");
	std::vector<cl_device_id> devsC(1);
	clGetGLContextInfo(clContextPropertyVector.data(), CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, sizeof(cl_device_id), devsC.data(), NULL);
	std::vector<cl::Device> devs(1);
	for (auto& devC : devsC) {
		if (devC == 0)
			continue;
		devs.push_back(cl::Device(devC));
	}
	return devs;
}

struct Pair {
	cl::Platform platform;
	cl::Device device;
};

void CLEngine::init()
{
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);

	vector<Pair> options;
	int bestPerf = 0; // https://stackoverflow.com/a/33488953
	for (auto& platform : platforms) {
		auto props = getContextPropertyVector(platform);
		std::vector<cl::Device> devices = getClDevicesForGLContext(props, platform);
		devices = filterBySharingExtension(devices);
		for (auto& device : devices) {
			options.push_back({ platform, device });
		}
	}

	cout << "Supported OpenCL devices:" << endl;
	for (int i = 0; i < options.size(); i++) {
		auto& option = options[i];
		cout << i << ": " << option.device.getInfo<CL_DEVICE_NAME>() <<endl;
	}
	cout << "Please enter the number of your choice: " << flush;
	int userChoice;
	cin >> userChoice;
	auto& option = options[userChoice];
	m_platform = option.platform;
	m_device = option.device;

	cout << "Using platform: " << m_platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

	//device = all_devices[0];
	cout << "Using device: " << m_device.getInfo<CL_DEVICE_NAME>() << std::endl;
	auto props = getContextPropertyVector(m_platform);
	context = cl::Context(m_device, props.data());

	queue = cl::CommandQueue(context, m_device);

	compmulProgram = makeProgram(compmulKernel);
}

void CLEngine::runClfftPlan(clfft::Plan plan, cl_mem in, cl_mem out, clfftDirection dir) {
	CLErrorCheck err;
	cl_int err2;
	err = clfftEnqueueTransform(plan(), dir, 1, &queue(), 0, NULL, NULL, &in, NULL, NULL);
}
