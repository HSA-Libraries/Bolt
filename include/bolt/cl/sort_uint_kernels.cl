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
#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable 

#if 1
/*This kernel is giving good performance. With group size as groupSize*16*/
template <int N>
kernel
void histogramAscendingRadixNTemplate(__global uint* unsortedData,
               __global uint* buckets,
               uint shiftCount)
{
    const int RADIX_T     = N;
    const int RADICES_T   = (1 << RADIX_T);
    const int NUM_OF_ELEMENTS_PER_WORK_ITEM_T = RADICES_T; 
    const int MASK_T      = (1<<RADIX_T)  -1;
    uint4     MASK_T_4    = (uint4)MASK_T;
    uint4     shiftCount_4 = (uint4)shiftCount;
    int       localBuckets[16] = {0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0/*,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0*/};
    size_t globalId    = get_global_id(0);
    size_t globalSize  = get_global_size(0);
    size_t numOfGroups = get_num_groups(0);
    uint4  value4_0, value4_1, value4_2, value4_3;
    /* Calculate thread-histograms */
    //for(int i = 0; i < NUM_OF_ELEMENTS_PER_WORK_ITEM_T/4; i++)
    //{
        //uint value = unsortedData[globalId * NUM_OF_ELEMENTS_PER_WORK_ITEM_T + i];
        value4_0 =  vload4(0, &unsortedData[globalId * NUM_OF_ELEMENTS_PER_WORK_ITEM_T]);
        value4_1 =  vload4(1, &unsortedData[globalId * NUM_OF_ELEMENTS_PER_WORK_ITEM_T]);
        value4_2 =  vload4(2, &unsortedData[globalId * NUM_OF_ELEMENTS_PER_WORK_ITEM_T]);
        value4_3 =  vload4(3, &unsortedData[globalId * NUM_OF_ELEMENTS_PER_WORK_ITEM_T]);
        value4_0 = (value4_0 >> shiftCount_4) & MASK_T_4;
        value4_1 = (value4_1 >> shiftCount_4) & MASK_T_4;
        value4_2 = (value4_2 >> shiftCount_4) & MASK_T_4;
        value4_3 = (value4_3 >> shiftCount_4) & MASK_T_4;
        localBuckets[value4_0.x]++;
        localBuckets[value4_0.y]++;
        localBuckets[value4_0.z]++;
        localBuckets[value4_0.w]++;
        localBuckets[value4_1.x]++;
        localBuckets[value4_1.y]++;
        localBuckets[value4_1.z]++;
        localBuckets[value4_1.w]++;
        localBuckets[value4_2.x]++;
        localBuckets[value4_2.y]++;
        localBuckets[value4_2.z]++;
        localBuckets[value4_2.w]++;
        localBuckets[value4_3.x]++;
        localBuckets[value4_3.y]++;
        localBuckets[value4_3.z]++;
        localBuckets[value4_3.w]++;
    //}

    for(int i = 0; i < NUM_OF_ELEMENTS_PER_WORK_ITEM_T; ++i)
    {
        buckets[i * globalSize + globalId ] = localBuckets[i];
    }
}
#endif 

#if 0
template <int N>
kernel
void histogramAscendingRadixNTemplate(__global uint* unsortedData,
               __global uint* buckets,
               uint shiftCount)
{
    const int RADIX_T     = N;
    const int RADICES_T   = (1 << RADIX_T);
    const int NUM_OF_ELEMENTS_PER_WORK_ITEM_T = RADICES_T; 
    const int MASK_T      = (1<<RADIX_T)  -1;
    int       localBuckets[64] = {0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0};
    size_t globalId    = get_global_id(0);
    size_t globalSize    = get_global_size(0);
    size_t numOfGroups = get_num_groups(0);

    /* Calculate thread-histograms */
    for(int i = 0; i < NUM_OF_ELEMENTS_PER_WORK_ITEM_T; ++i)
    {
        uint value = unsortedData[globalId * NUM_OF_ELEMENTS_PER_WORK_ITEM_T + i];
        value = (value >> shiftCount) & MASK_T;
        localBuckets[value]++;
    }

    for(int i = 0; i < NUM_OF_ELEMENTS_PER_WORK_ITEM_T; ++i)
    {
        buckets[i *globalSize + globalId ] = localBuckets[i];
    }
}
#endif

template <int N>
kernel
void histogramDescendingRadixNTemplate(__global uint* unsortedData,
               __global uint* buckets,
               uint shiftCount)
{
    const int RADIX_T     = N;
    const int RADICES_T   = (1 << RADIX_T);
    const int NUM_OF_ELEMENTS_PER_WORK_ITEM_T = RADICES_T; 
    const int MASK_T      = (1<<RADIX_T)  -1;
    int       localBuckets[16] = {0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0};
    size_t globalId    = get_global_id(0);
    size_t numOfGroups = get_num_groups(0);

    /* Calculate thread-histograms */
    for(int i = 0; i < NUM_OF_ELEMENTS_PER_WORK_ITEM_T; ++i)
    {
        uint value = unsortedData[globalId * NUM_OF_ELEMENTS_PER_WORK_ITEM_T + i];
        value = (value >> shiftCount) & MASK_T;
        localBuckets[RADICES_T - value - 1]++;
    }

    for(int i = 0; i < NUM_OF_ELEMENTS_PER_WORK_ITEM_T; ++i)
    {
        buckets[i * RADICES_T * numOfGroups + globalId ] = localBuckets[i];
    }
}

template <int N>
kernel
void permuteAscendingRadixNTemplate(__global uint* unsortedData,
             __global uint* scanedBuckets,
             uint shiftCount,
             __global uint* sortedData)
{
    const int RADIX_T     = N;
    const int RADICES_T   = (1 << RADIX_T);
    const int MASK_T = (1<<RADIX_T)  -1;

    size_t globalId  = get_global_id(0);
    size_t numOfGroups = get_num_groups(0);
    const int NUM_OF_ELEMENTS_PER_WORK_GROUP_T = numOfGroups << N;
    int  localIndex[16];

    /*Load the index to local memory*/
    for(int i = 0; i < RADICES_T; ++i)
        localIndex[i] = scanedBuckets[i*NUM_OF_ELEMENTS_PER_WORK_GROUP_T + globalId];
    /* Permute elements to appropriate location */
    for(int i = 0; i < RADICES_T; ++i)
    {
        uint value = unsortedData[globalId * RADICES_T + i];
        uint maskedValue = (value >> shiftCount) & MASK_T;
        uint index = localIndex[maskedValue];
        sortedData[index] = value;
        localIndex[maskedValue] = index + 1;
    }
}

template <int N>
kernel
void permuteDescendingRadixNTemplate(__global uint* unsortedData,
             __global uint* scanedBuckets,
             uint shiftCount,
             __global uint* sortedData)
{
    const int RADIX_T     = N;
    const int RADICES_T   = (1 << RADIX_T);
    const int MASK_T      = (1<<RADIX_T)  -1;

    size_t globalId  = get_global_id(0);
    size_t numOfGroups = get_num_groups(0);
    const int NUM_OF_ELEMENTS_PER_WORK_GROUP_T = numOfGroups << N;
    int  localIndex[16];

    /*Load the index to local memory*/
    for(int i = 0; i < RADICES_T; ++i)
        localIndex[i] = scanedBuckets[(RADICES_T - i - 1)*NUM_OF_ELEMENTS_PER_WORK_GROUP_T + globalId];
    /*Permute elements to appropriate location */
    for(int i = 0; i < RADICES_T; ++i)
    {
        uint value = unsortedData[globalId * RADICES_T + i];
        uint maskedValue = (value >> shiftCount) & MASK_T;
        uint index = localIndex[maskedValue];
        sortedData[index] = value;
        localIndex[maskedValue] = index + 1;
    }
}

/****************Specialized kernels for signed integers****************/

template <int N>
kernel
void histogramSignedAscendingRadixNTemplate(__global uint* unsortedData,
               __global uint* buckets,
               uint shiftCount)
{
    const int RADIX_T     = N;
    const int RADICES_T   = (1 << RADIX_T);
    const int NUM_OF_ELEMENTS_PER_WORK_ITEM_T = RADICES_T; 
    const int MASK_T      = ( 1 << ( RADIX_T - 1 ) ) - 1;
    int       localBuckets[16] = {0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0};
    size_t globalId    = get_global_id(0);
    size_t numOfGroups = get_num_groups(0);

    /* Calculate thread-histograms */
    for(int i = 0; i < NUM_OF_ELEMENTS_PER_WORK_ITEM_T; ++i)
    {
        uint value = unsortedData[globalId * NUM_OF_ELEMENTS_PER_WORK_ITEM_T + i];
        value = (value >> shiftCount);
        uint signBit = value & (1<<(RADIX_T-1));
        value = ( ( ( value & MASK_T ) ^ MASK_T ) & MASK_T ) | signBit;
        localBuckets[value]++;
    }

    for(int i = 0; i < NUM_OF_ELEMENTS_PER_WORK_ITEM_T; ++i)
    {
        buckets[i * RADICES_T * numOfGroups + globalId ] = localBuckets[i];
    }
}

template <int N>
kernel
void histogramSignedDescendingRadixNTemplate(__global uint* unsortedData,
               __global uint* buckets,
               uint shiftCount)
{
    const int RADIX_T     = N;
    const int RADICES_T   = (1 << RADIX_T);
    const int NUM_OF_ELEMENTS_PER_WORK_ITEM_T = RADICES_T; 
    const int MASK_T      = ( 1 << ( RADIX_T - 1 ) ) - 1;
    int       localBuckets[16] = {0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0};
    size_t globalId    = get_global_id(0);
    size_t numOfGroups = get_num_groups(0);

    /* Calculate thread-histograms */
    for(int i = 0; i < NUM_OF_ELEMENTS_PER_WORK_ITEM_T; ++i)
    {
        uint value = unsortedData[globalId * NUM_OF_ELEMENTS_PER_WORK_ITEM_T + i];
        value = (value >> shiftCount);
        uint signBit = value & (1<<(RADIX_T-1));
        value = ( ( ( ( value & MASK_T ) ^ MASK_T ) & MASK_T ) | signBit );
        localBuckets[RADICES_T - value - 1]++;
    }

    for(int i = 0; i < NUM_OF_ELEMENTS_PER_WORK_ITEM_T; ++i)
    {
        buckets[i * RADICES_T * numOfGroups + globalId ] = localBuckets[i];
    }
}

template <int N>
kernel
void permuteSignedAscendingRadixNTemplate(__global uint* unsortedData,
             __global uint* scanedBuckets,
             uint shiftCount,
             __global uint* sortedData)
{
    const int RADIX_T     = N;
    const int RADICES_T   = (1 << RADIX_T);
    const int MASK_T      = ( 1 << ( RADIX_T - 1 ) ) - 1;

    size_t globalId  = get_global_id(0);
    size_t numOfGroups = get_num_groups(0);
    const int NUM_OF_ELEMENTS_PER_WORK_GROUP_T = numOfGroups << N;
    int  localIndex[16];

    /*Load the index to local memory*/
    for(int i = 0; i < RADICES_T; ++i)
        localIndex[i] = scanedBuckets[i*NUM_OF_ELEMENTS_PER_WORK_GROUP_T + globalId];
    /* Premute elements to appropriate location */
    for(int i = 0; i < RADICES_T; ++i)
    {
        uint resultValue = unsortedData[globalId * RADICES_T + i];
        uint value;
        value = (resultValue >> shiftCount);
        uint signBit = value & (1<<(RADIX_T-1));
        value = ( ( ( ( value & MASK_T ) ^ MASK_T ) & MASK_T ) | signBit );
        uint index = localIndex[value];
        sortedData[index] = resultValue;
        localIndex[value] = index + 1;
    }
}

template <int N>
kernel
void permuteSignedDescendingRadixNTemplate(__global uint* unsortedData,
             __global uint* scanedBuckets,
             uint shiftCount,
             __global uint* sortedData)
{
    const int RADIX_T     = N;
    const int RADICES_T   = (1 << RADIX_T);
    const int MASK_T      = ( 1 << ( RADIX_T - 1 ) ) - 1;

    size_t globalId  = get_global_id(0);
    size_t numOfGroups = get_num_groups(0);
    const int NUM_OF_ELEMENTS_PER_WORK_GROUP_T = numOfGroups << N;
    int  localIndex[16];

    /*Load the index to local memory*/
    for(int i = 0; i < RADICES_T; ++i)
        localIndex[i] = scanedBuckets[(RADICES_T - i - 1)*NUM_OF_ELEMENTS_PER_WORK_GROUP_T + globalId];

    /* Premute elements to appropriate location */
    for(int i = 0; i < RADICES_T; ++i)
    {
        uint resultValue = unsortedData[globalId * RADICES_T + i];
        uint value = (resultValue >> shiftCount);
        uint signBit = value & (1<<(RADIX_T-1));
        value = ( ( ( ( value & MASK_T ) ^ MASK_T ) & MASK_T ) | signBit );
        uint index = localIndex[value];
        sortedData[index] = resultValue;
        localIndex[ value ]++;
    }
}
/****************************End of Signed Integers templates********************************/

