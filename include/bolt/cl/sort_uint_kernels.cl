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
    int       localBuckets[16] = {0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0};
    size_t globalId    = get_global_id(0);
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
        buckets[i * RADICES_T * numOfGroups + globalId ] = localBuckets[i];
    }
}

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

