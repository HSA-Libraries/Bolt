#include "matrix_utils.h"

extern cl::Kernel compileOpenCLFile(const std::string &kernelCodeString, const std::string kernelName);

extern bool update_trz_oclcpp(cl::Kernel k, H3& dH, const cl::Buffer& I1, int ipitch, const cl::Buffer& wI2, int wpitch, float sigma, float gradThresh, const  utils::Rect& roi );

extern void printLOC_OclCpp();