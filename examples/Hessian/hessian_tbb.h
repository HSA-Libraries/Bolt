/***************************************************************************                                                                                     
*   � 2012,2014 Advanced Micro Devices, Inc. All rights reserved.                                     
*                                                                                    
*   Licensed under the Apache License, Version 2.0 (the "License");   
*   you may not use this file except in compliance with the License.                 
*   You may obtain a copy of the License at                                          
*                                                                                    
*       http://www.apache.org/licenses/LICENSE-2.0                      
*                                                                                    
*   Unless required by applicable law or agreed to in writing, software              
*   distributed under the License is distributed on an "AS IS" BASIS,              
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.         
*   See the License for the specific language governing permissions and              
*   limitations under the License.                                                   

***************************************************************************/                                                                                     

#define TBB_VECTOR 1

extern bool update_trz_tbb( H3& dH, const utils::Matrix<float>& I1,  const utils::Matrix<float>& wI2, float sigma, float gradThresh, const  utils::Rect& roi );
extern bool update_trz_tbb_lamda( H3& dH, const utils::Matrix<float>& I1,  const utils::Matrix<float>& wI2, float sigma, float gradThresh, const  utils::Rect& roi );
