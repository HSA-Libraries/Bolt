/***************************************************************************                                                                                     
*   Copyright 2012 - 2013 Advanced Micro Devices, Inc.                                     
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

#include <bolt/cl/device_vector.h>
#include <bolt/cl/inner_product.h>
#include <bolt/cl/functional.h>
#include <bolt/cl/control.h>

#include <numeric>
#include "common/myocl.h"

void testDeviceVector()
{
    const int aSize = 64;
    std::vector<int> hA(aSize), hB(aSize);
    bolt::cl::device_vector<int> dA(aSize), dB(aSize);

    for(int i=0; i<aSize; i++) {
        hA[i] = hB[i] = dB[i] = dA[i] = i;
    };

    int hSum = std::inner_product(hA.begin(), hA.end(), hB.begin(), 1);

	int sum = bolt::cl::inner_product(dA.begin(), dA.end(), dB.begin(), 1,bolt::cl::plus<int>(),bolt::cl::multiplies<int>());
};
