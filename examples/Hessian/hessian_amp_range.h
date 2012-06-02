#include <amp.h>



extern bool update_trz_boltforamp_range( H3& dH, matrix_type& I1View,  matrix_type& wI2View, float sigma, float gradThresh, const  utils::Rect& roi );
extern bool update_trz_boltforamp_range_cpu( H3& dH, const utils::Matrix<float> &I1, const utils::Matrix<float> &wI2, matrix_type& I1View,  matrix_type& wI2View, float sigma, float gradThresh, const  utils::Rect& roi );

