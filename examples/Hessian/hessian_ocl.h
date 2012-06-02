#pragma once

#include "matrix_utils.h"

extern int zeroCopy;

extern bool update_trz_ocl(const MyOclContextC &ocl, cl_kernel k, H3& dH, const cl_mem I1, int ipitch, const cl_mem wI2, int wpitch, float sigma, float gradThresh, const  utils::Rect& roi );


