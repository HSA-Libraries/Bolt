#include <boltCL/transform_reduce.h>

#include "hessian_boltcl.h"



// Bolt-for-OCL has restriction that we can't include pointers in the functors - the functor is initialized on the host and read on the device, and OpenCL doesn't provide a mechanism to make this work:
//   * 6.9 p)   Structures cannot contain OpenCL objects (buffers, images).
//   * Host and device pointers are not guaranteed to be the same - the handoff is handled through the cl::buffer object.  When the runtime launches the kernel, it provides an address in GPU-space to the kernel.
//   * The solution is SVM, where host and device have same pointer.  Then we can create a structure with a pointer; host can determine that pointer and write it to the structure; then host can directly use the pointer.


bool update_trz_boltfcl(cl::Kernel k, H3& dH, const cl::Buffer& I1, int ipitch, const cl::Buffer& wI2, int wpitch, float sigma, float gradThresh, const  utils::Rect& roi )
{

	//HessianTransform w( I1, wI2, sigma, gradThresh, validRgn );

	printf ("NOT IMPLEMENTED\n");


	return false;
}



