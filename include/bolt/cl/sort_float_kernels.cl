/***************************************************************************                                                                                     
*   © 2012,2014 Advanced Micro Devices, Inc. All rights reserved.                                     
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

#define WG_SIZE                 256
#define ELEMENTS_PER_WORK_ITEM  4
#define RADICES                 16
#define GROUP_LDS_BARRIER barrier(CLK_LOCAL_MEM_FENCE)
#define GROUP_MEM_FENCE mem_fence(CLK_LOCAL_MEM_FENCE)
#define m_n             x
#define m_nWGs          y
#define m_startBit      z
#define m_nBlocksPerWG  w

#define CHECK_BOUNDARY


__kernel 
__attribute__((reqd_work_group_size(WG_SIZE,1,1)))
void
flipFloatInstantiated(__global uint * restrict data, int4 cb_data)
{
    __local uint lmem[WG_SIZE];
    uint lIdx = get_local_id(0);    
    uint wgIdx  = get_group_id(0);
    
    const int n    = cb_data.m_n;
    uint numBlocks = cb_data.m_nBlocksPerWG * ELEMENTS_PER_WORK_ITEM;

    int offset = (WG_SIZE*numBlocks)*wgIdx; 

    for(int i=0;i<numBlocks;i++)
    {
        int addr = offset + i*WG_SIZE;
        uint value = ((addr + lIdx) < n) ?data[addr + lIdx]: 0;

        unsigned int mask = -int(value >> 31) | 0x80000000;
        value ^= mask;
        if((addr + lIdx) < n)
            data[addr + lIdx] = value;
    }
}

__kernel 
__attribute__((reqd_work_group_size(WG_SIZE,1,1)))
void
inverseFlipFloatInstantiated(__global uint * restrict data, int4 cb_data)
{
    __local uint lmem[WG_SIZE];
    uint lIdx = get_local_id(0);    
    uint wgIdx  = get_group_id(0);

    const int n    = cb_data.m_n;
    uint numBlocks = cb_data.m_nBlocksPerWG * ELEMENTS_PER_WORK_ITEM;

    int offset = (WG_SIZE*numBlocks)*wgIdx; 

    for(int i=0;i<numBlocks;i++)
    {
        int addr = offset + i*WG_SIZE;
        uint value = ((addr + lIdx) < n) ?data[addr + lIdx]: 0;
        unsigned int mask = ((value >> 31) - 1) | 0x80000000;
        value ^= mask;
        if((addr + lIdx) < n)
            data[addr + lIdx] = value;
    }
}
