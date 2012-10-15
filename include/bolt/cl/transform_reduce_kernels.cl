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

template <typename T,  typename unary_function, typename binary_function>
kernel
void transform_reduceTemplate(
    global T* input, 
    const int length,

    global unary_function *transformFunctor,

	const T init,
    global binary_function *reduceFunctor,

    global T* result,
    local T *scratch
)
{
    int gx = get_global_id (0);

    T accumulator = init;

    // Loop sequentially over chunks of input vector
    while (gx < length) {
        T element = (input[gx]);
        element = (*transformFunctor)(element);

        accumulator = (*reduceFunctor)(accumulator, element);;
        gx += get_global_size(0);
    }

    // Perform parallel reduction through shared memory:
    int local_index = get_local_id(0);
    scratch[local_index] = accumulator;
    barrier(CLK_LOCAL_MEM_FENCE);
    

    for(int offset = get_local_size(0) / 2;
        offset > 0;
        offset = offset / 2) {
        bool test = local_index < offset;
        if (test) {
            int other = scratch[local_index + offset];
            int mine  = scratch[local_index];
            scratch[local_index] = (*reduceFunctor)(mine, other);;
      
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if (local_index == 0) {
        result[get_group_id(0)] = scratch[0];
    }
};
