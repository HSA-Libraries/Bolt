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


// No Boundary Check Kernel
template <typename Type, typename Generator>
kernel
void generateNoBoundaryCheckTemplate( global Type* Z,
			const int length,
			global Generator *gen)
{
    int gx = get_global_id(0);
    Z[gx] = (*gen)();
}


// Yes Boundary Generate Kernel
template <typename Type, typename Generator>
kernel
void generateTemplate( global Type* Z,
			const int length,
			global Generator *gen)
{
    int gx = get_global_id(0);

    if (gx >= length)
    {
        return;
    }
    else
    {
        Z[gx] = (*gen)();
    }
}


// Single thread performs in-order generation
template <typename Type, typename Generator>
kernel
void generateSingleThreadTemplate( global Type* Z,
			const int length,
			global Generator *gen)
{
    for (int i = 0; i < length; i++) {            
        Z[i] = (*gen)();
    }
}