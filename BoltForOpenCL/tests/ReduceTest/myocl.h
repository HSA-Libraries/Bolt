#define __CL_ENABLE_EXCEPTIONS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#include "CL/cl.hpp"

enum t_DeviceType  {e_Cpu, e_Gpu, e_All};

// Init OCL Platform, Context, etc.  
struct MyOclContext {
	cl::Context      _context;
	cl::Device  	 _device;
	cl::CommandQueue _queue;
};

extern void printDevice(const cl::Device &d);
extern void printContext(const cl::Context &c);
extern MyOclContext initOcl(cl_int clDeviceType, int deviceIndex=0, int verbose=0) ;
extern cl::Kernel compileKernelCpp(const MyOclContext &ocl, const char *kernelFile, const char *kernelName, std::string compileOpt);

