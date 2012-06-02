#include "matrix_utils.h"


extern bool update_trz_cpuserial( H3& dH, const utils::Matrix<float>& I1,  const utils::Matrix<float>& wI2, float sigma, float gradThresh, const  utils::Rect& roi );

namespace fpu_orig {
extern void hessianTRZ( double outH[][4],
		double outb[],
		const utils::Matrix<float>& I1, 
		const utils::Matrix<float>& wI2,
		const utils::Rect& roi,
		float gradThresh,
		float Xc, float Yc,
		float sigma ) ;
};

