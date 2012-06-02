#define TBB_VECTOR 1

extern bool update_trz_tbb( H3& dH, const utils::Matrix<float>& I1,  const utils::Matrix<float>& wI2, float sigma, float gradThresh, const  utils::Rect& roi );
extern bool update_trz_tbb_lamda( H3& dH, const utils::Matrix<float>& I1,  const utils::Matrix<float>& wI2, float sigma, float gradThresh, const  utils::Rect& roi );
