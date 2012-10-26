/***************************************************************************                                                                                     
*   Copyright 2012 Advanced Micro Devices, Inc.                                     
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
#include <bolt/cl/reduce.h>

#include <numeric>
#include "common/myocl.h"

void testDeviceVector()
{
    const int aSize = 1000;
    std::vector<int> hA(aSize);
    bolt::cl::device_vector<int> dA(aSize);

    for(int i=0; i<aSize; i++) {
        hA[i] = i;
        dA[i] = i;
    };

    int hSum = std::accumulate(hA.begin(), hA.end(), 0);

    int sum = bolt::cl::reduce(dA.begin(), dA.end(), 0);
};