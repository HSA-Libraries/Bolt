
#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable 


template <int N>
kernel
void histogramAscendingRadixNTemplate(__global uint* unsortedData,
               __global uint* buckets,
               __global uint* histScanBuckets,
               uint shiftCount)
{
    const int RADIX_T     = N;
    const int RADICES_T   = (1 << RADIX_T);
    const int NUM_OF_ELEMENTS_PER_WORK_ITEM_T = RADICES_T; 
    const int MASK_T      = (1<<RADIX_T)  -1;

    size_t localId     = get_local_id(0);
    size_t globalId    = get_global_id(0);
    size_t groupId     = get_group_id(0);
    size_t groupSize   = get_local_size(0);
    size_t numOfGroups = get_num_groups(0);
    //uint   bucketPos   = groupId * RADICES_T * groupSize;

    for(int i = 0; i < RADICES_T; ++i)
    {
        //buckets[bucketPos + localId * RADICES_T + i] = 0;
        buckets[i * RADICES_T * numOfGroups + globalId] = 0;
    }
    barrier(CLK_GLOBAL_MEM_FENCE);

    /* Calculate thread-histograms */
    for(int i = 0; i < NUM_OF_ELEMENTS_PER_WORK_ITEM_T; ++i)
    {
        uint value = unsortedData[globalId * NUM_OF_ELEMENTS_PER_WORK_ITEM_T + i];
        value = (value >> shiftCount) & MASK_T;
        buckets[value * RADICES_T * numOfGroups + globalId ]++;
        barrier(CLK_GLOBAL_MEM_FENCE);
    }

    //barrier(CLK_GLOBAL_MEM_FENCE);

    //Start First step to scan
    /*int sum =0;
    for(int i = 0; i < groupSize; i++)
    {
        sum = sum + buckets[bucketPos + localId + groupSize*i];
    }
    histScanBuckets[localId*numOfGroups + groupId + 1] = sum;*/
}

template <int N>
kernel
void histogramDescendingRadixNTemplate(__global uint* unsortedData,
               __global uint* buckets,
               __global uint* histScanBuckets,
               uint shiftCount)
{
    const int RADIX_T     = N;
    const int RADICES_T   = (1 << RADIX_T);
    const int NUM_OF_ELEMENTS_PER_WORK_ITEM_T = RADICES_T; 
    const int MASK_T      = (1<<RADIX_T)  -1;
    size_t localId     = get_local_id(0);
    size_t globalId    = get_global_id(0);
    size_t groupId     = get_group_id(0);
    size_t groupSize   = get_local_size(0);
    size_t numOfGroups = get_num_groups(0);
    uint   bucketPos   = groupId * RADICES_T * groupSize;

    for(int i = 0; i < RADICES_T; ++i)
    {
        buckets[bucketPos + localId * RADICES_T + i] = 0;
    }
    barrier(CLK_GLOBAL_MEM_FENCE);

    /* Calculate thread-histograms */
    for(int i = 0; i < NUM_OF_ELEMENTS_PER_WORK_ITEM_T; ++i)
    {
        uint value = unsortedData[globalId * NUM_OF_ELEMENTS_PER_WORK_ITEM_T + i];
        value = (value >> shiftCount) & MASK_T;
        buckets[bucketPos + localId * RADICES_T + (RADICES_T - value -1)]++;
    }

    barrier(CLK_GLOBAL_MEM_FENCE);
    //Start First step to scan
    int sum =0;
    for(int i = 0; i < groupSize; i++)
        sum = sum + buckets[bucketPos + localId + groupSize*i];
    histScanBuckets[localId*numOfGroups + groupId + 1] = sum;
}

/*scanLocal is used for both ascending and descending*/
template <int N>
kernel 
void scanLocalTemplate(__global uint* buckets,
                  __global uint* histScanBuckets,
                  __local uint* localScanArray)
{
    const int RADIX_T     = N;
    const int RADICES_T   = (1 << RADIX_T);

    size_t localId     = get_local_id(0); 
    size_t numOfGroups = get_num_groups(0);
    size_t groupId     = get_group_id(0);
    size_t groupSize   = get_local_size(0);

    localScanArray[localId] = histScanBuckets[localId*numOfGroups + groupId];
    localScanArray[RADICES_T+localId] = 0;

    barrier(CLK_LOCAL_MEM_FENCE);
    for(int i = 0; i < RADICES_T; ++i)
    {
        uint bucketPos = groupId * RADICES_T * groupSize + i * RADICES_T + localId;
        uint temp = buckets[bucketPos];
        buckets[bucketPos] = localScanArray[RADICES_T+localId] + localScanArray[localId];
        localScanArray[RADICES_T+localId] += temp;
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
    //const int NUM_OF_ELEMENTS_PER_WORK_ITEM_T = RADICES_T; 
    const int MASK_T = (1<<RADIX_T)  -1;
    size_t groupId   = get_group_id(0);
    size_t localId   = get_local_id(0);
    size_t globalId  = get_global_id(0);
    size_t groupSize = get_local_size(0);
    size_t numOfGroups = get_num_groups(0);
    //uint bucketPos   = groupId * RADICES_T * groupSize;

    /* Premute elements to appropriate location */
    for(int i = 0; i < RADICES_T; ++i)
    {
        //uint value = unsortedData[globalId * RADICES_T + i];
        uint value = unsortedData[globalId * RADICES_T + i];
        value = (value >> shiftCount) & MASK_T;
        //uint index = scanedBuckets[bucketPos+localId * RADICES_T + value];
        uint index = scanedBuckets[value * RADICES_T * numOfGroups + globalId ];
        //barrier(CLK_GLOBAL_MEM_FENCE);
        sortedData[index] = unsortedData[globalId * RADICES_T + i];
        //scanedBuckets[bucketPos+localId * RADICES_T + value] = index + 1;
        scanedBuckets[value * RADICES_T * numOfGroups + globalId ] = index + 1;
        barrier(CLK_GLOBAL_MEM_FENCE);
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

    size_t groupId   = get_group_id(0);
    size_t localId   = get_local_id(0);
    size_t globalId  = get_global_id(0);
    size_t groupSize = get_local_size(0);
    uint bucketPos   = groupId * RADICES_T * groupSize;

    /* Premute elements to appropriate location */
    for(int i = 0; i < RADICES_T; ++i)
    {
        uint value = unsortedData[globalId * RADICES_T + i];
        value = (value >> shiftCount) & MASK_T;
        uint index = scanedBuckets[bucketPos+localId * RADICES_T + (RADICES_T -value-1)];
        sortedData[index] = unsortedData[globalId * RADICES_T + i];
        scanedBuckets[bucketPos+localId * RADICES_T + (RADICES_T -value-1)] = index + 1;
        barrier(CLK_LOCAL_MEM_FENCE);
    }
}

/****************Specialized kernels for signed integers****************/

template <int N>
kernel
void histogramSignedAscendingRadixNTemplate(__global uint* unsortedData,
               __global uint* buckets,
               __global uint* histScanBuckets,
               uint shiftCount)
{
    const int RADIX_T     = N;
    const int RADICES_T   = (1 << RADIX_T);
    const int NUM_OF_ELEMENTS_PER_WORK_ITEM_T = RADICES_T; 
    const int MASK_T      = ( 1 << ( RADIX_T - 1 ) ) - 1;

    size_t localId     = get_local_id(0);
    size_t globalId    = get_global_id(0);
    size_t groupId     = get_group_id(0);
    size_t groupSize   = get_local_size(0);
    size_t numOfGroups = get_num_groups(0);
    uint   bucketPos   = groupId * RADICES_T * groupSize;

    for(int i = 0; i < RADICES_T; ++i)
    {
        buckets[bucketPos + localId * RADICES_T + i] = 0;
    }
    barrier(CLK_GLOBAL_MEM_FENCE);

    /* Calculate thread-histograms */
    for(int i = 0; i < NUM_OF_ELEMENTS_PER_WORK_ITEM_T; ++i)
    {
        uint value = unsortedData[globalId * NUM_OF_ELEMENTS_PER_WORK_ITEM_T + i];
        value = (value >> shiftCount);
        uint signBit = value & (1<<(RADIX_T-1));
        value = ( ( ( value & MASK_T ) ^ MASK_T ) & MASK_T ) | signBit;
        buckets[bucketPos + localId * RADICES_T + value]++;
    }

    barrier(CLK_GLOBAL_MEM_FENCE);

    //Start First step to scan
    int sum =0;
    for(int i = 0; i < groupSize; i++)
    {
        sum = sum + buckets[bucketPos + localId + groupSize*i];
    }
    histScanBuckets[localId*numOfGroups + groupId + 1] = sum;
}

template <int N>
kernel
void histogramSignedDescendingRadixNTemplate(__global uint* unsortedData,
               __global uint* buckets,
               __global uint* histScanBuckets,
               uint shiftCount)
{
    const int RADIX_T     = N;
    const int RADICES_T   = (1 << RADIX_T);
    const int NUM_OF_ELEMENTS_PER_WORK_ITEM_T = RADICES_T; 
    const int MASK_T      = ( 1 << ( RADIX_T - 1 ) ) - 1;
    size_t localId		  = get_local_id(0);
    size_t globalId       = get_global_id(0);
    size_t groupId        = get_group_id(0);
    size_t groupSize      = get_local_size(0);
    size_t numOfGroups    = get_num_groups(0);
    uint   bucketPos      = groupId * RADICES_T * groupSize;

    for(int i = 0; i < RADICES_T; ++i)
    {
        buckets[bucketPos + localId * RADICES_T + i] = 0;
    }
    barrier(CLK_GLOBAL_MEM_FENCE);

    /* Calculate thread-histograms */
    for(int i = 0; i < NUM_OF_ELEMENTS_PER_WORK_ITEM_T; ++i)
    {
        uint value = unsortedData[globalId * NUM_OF_ELEMENTS_PER_WORK_ITEM_T + i];
        value = (value >> shiftCount);
        uint signBit = value & (1<<(RADIX_T-1));
        value = ( ( ( ( value & MASK_T ) ^ MASK_T ) & MASK_T ) | signBit );
        buckets[bucketPos + localId * RADICES_T + (RADICES_T - value -1)]++;
    }

    barrier(CLK_GLOBAL_MEM_FENCE);
    //Start First step to scan
    int sum =0;
    for(int i = 0; i < groupSize; i++)
        sum = sum + buckets[bucketPos + localId + groupSize*i];
    histScanBuckets[localId*numOfGroups + groupId + 1] = sum;
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
    //const int NUM_OF_ELEMENTS_PER_WORK_ITEM_T = RADICES_T; 
    const int MASK_T      = ( 1 << ( RADIX_T - 1 ) ) - 1;
    size_t groupId   = get_group_id(0);
    size_t localId   = get_local_id(0);
    size_t globalId  = get_global_id(0);
    size_t groupSize = get_local_size(0);
    uint bucketPos   = groupId * RADICES_T * groupSize;

    /* Premute elements to appropriate location */
    for(int i = 0; i < RADICES_T; ++i)
    {
        uint resultValue = unsortedData[globalId * RADICES_T + i];
        uint value;
        value = (resultValue >> shiftCount);
        uint signBit = value & (1<<(RADIX_T-1));
        value = ( ( ( ( value & MASK_T ) ^ MASK_T ) & MASK_T ) | signBit );
        uint index = scanedBuckets[bucketPos+localId * RADICES_T + value];
        sortedData[index] = resultValue;
        scanedBuckets[bucketPos+localId * RADICES_T + value] = index + 1;
        barrier(CLK_LOCAL_MEM_FENCE);
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

    size_t groupId   = get_group_id(0);
    size_t localId   = get_local_id(0);
    size_t globalId  = get_global_id(0);
    size_t groupSize = get_local_size(0);
    uint bucketPos   = groupId * RADICES_T * groupSize;

    /* Premute elements to appropriate location */
    for(int i = 0; i < RADICES_T; ++i)
    {
        uint resultValue = unsortedData[globalId * RADICES_T + i];
        uint value = (resultValue >> shiftCount);
        uint signBit = value & (1<<(RADIX_T-1));
        value = ( ( ( ( value & MASK_T ) ^ MASK_T ) & MASK_T ) | signBit );
        uint index = scanedBuckets[bucketPos+localId * RADICES_T + (RADICES_T -value-1)];
        sortedData[index] = unsortedData[globalId * RADICES_T + i];
        scanedBuckets[bucketPos+localId * RADICES_T + (RADICES_T -value-1)] = index + 1;
        barrier(CLK_LOCAL_MEM_FENCE);
    }
}
/****************************End of Signed integers templates********************************/

////////////////TO BE DELETED//////////////////////////////////////////////////////

kernel
void histogramAscendingRadixNInstantiated(uint radix, uint mask, __global uint* unsortedData,
               __global uint* buckets,
               __global uint* histScanBuckets,
               uint shiftCount)
{
    const int RADIX_T     = radix;
    const int RADICES_T   = (1 << RADIX_T);
    const int NUM_OF_ELEMENTS_PER_WORK_ITEM_T = RADICES_T; 
    const int MASK_T      = mask;

    size_t localId     = get_local_id(0);
    size_t globalId    = get_global_id(0);
    size_t groupId     = get_group_id(0);
    size_t groupSize   = get_local_size(0);
    size_t numOfGroups = get_num_groups(0);
    uint   bucketPos   = groupId * RADICES_T * groupSize;

    for(int i = 0; i < RADICES_T; ++i)
    {
        buckets[bucketPos + localId * RADICES_T + i] = 0;
    }
    barrier(CLK_GLOBAL_MEM_FENCE);

    /* Calculate thread-histograms */
    for(int i = 0; i < NUM_OF_ELEMENTS_PER_WORK_ITEM_T; ++i)
    {
        uint value = unsortedData[globalId * NUM_OF_ELEMENTS_PER_WORK_ITEM_T + i];
        value = (value >> shiftCount) & MASK_T;
        //printf("value = %x",value);
        buckets[bucketPos + localId * RADICES_T + value]++;
    }

    barrier(CLK_GLOBAL_MEM_FENCE);

    //Start First step to scan
    int sum =0;
    for(int i = 0; i < groupSize; i++)
    {
        sum = sum + buckets[bucketPos + localId + groupSize*i];
    }
    histScanBuckets[localId*numOfGroups + groupId + 1] = sum;
}


kernel
void histogramDescendingRadixNInstantiated(uint radix, uint mask, __global uint* unsortedData,
               __global uint* buckets,
               __global uint* histScanBuckets,
               uint shiftCount)
{
    const int RADIX_T     = radix;
    const int RADICES_T   = (1 << RADIX_T);
    const int NUM_OF_ELEMENTS_PER_WORK_ITEM_T = RADICES_T; 
    const int MASK_T      = mask;
    size_t localId     = get_local_id(0);
    size_t globalId    = get_global_id(0);
    size_t groupId     = get_group_id(0);
    size_t groupSize   = get_local_size(0);
    size_t numOfGroups = get_num_groups(0);
    uint   bucketPos   = groupId * RADICES_T * groupSize;

    for(int i = 0; i < RADICES_T; ++i)
    {
        buckets[bucketPos + localId * RADICES_T + i] = 0;
    }
    barrier(CLK_GLOBAL_MEM_FENCE);

    /* Calculate thread-histograms */
    for(int i = 0; i < NUM_OF_ELEMENTS_PER_WORK_ITEM_T; ++i)
    {
        uint value = unsortedData[globalId * NUM_OF_ELEMENTS_PER_WORK_ITEM_T + i];
        value = (value >> shiftCount) & MASK_T;
        buckets[bucketPos + localId * RADICES_T + (RADICES_T - value -1)]++;
    }

    barrier(CLK_GLOBAL_MEM_FENCE);
    //Start First step to scan
    int sum =0;
    for(int i = 0; i < groupSize; i++)
        sum = sum + buckets[bucketPos + localId + groupSize*i];
    histScanBuckets[localId*numOfGroups + groupId + 1] = sum;
}

/*scanLocal is used for both ascending and descending*/

kernel 
void scanLocalRadixNInstantiated(uint radix, __global uint* buckets,
                  __global uint* histScanBuckets,
                  __local uint* localScanArray)
{
    const int RADIX_T     = radix;
    const int RADICES_T   = (1 << RADIX_T);

    size_t localId     = get_local_id(0); 
    size_t numOfGroups = get_num_groups(0);
    size_t groupId     = get_group_id(0);
    size_t groupSize   = get_local_size(0);

    localScanArray[localId] = histScanBuckets[localId*numOfGroups + groupId];
    localScanArray[RADICES_T+localId] = 0;

    barrier(CLK_LOCAL_MEM_FENCE);
    for(int i = 0; i < RADICES_T; ++i)
    {
        uint bucketPos = groupId * RADICES_T * groupSize + i * RADICES_T + localId;
        uint temp = buckets[bucketPos];
        buckets[bucketPos] = localScanArray[RADICES_T+localId] + localScanArray[localId];
        localScanArray[RADICES_T+localId] += temp;
    }
}


kernel
void permuteAscendingRadixNInstantiated(uint radix, uint mask, __global uint* unsortedData,
             __global uint* scanedBuckets,
             uint shiftCount,
             __global uint* sortedData)
{
    const int RADIX_T     = radix;
    const int RADICES_T   = (1 << RADIX_T);
    //const int NUM_OF_ELEMENTS_PER_WORK_ITEM_T = RADICES_T; 
    const int MASK_T      = mask;
    size_t groupId   = get_group_id(0);
    size_t localId   = get_local_id(0);
    size_t globalId  = get_global_id(0);
    size_t groupSize = get_local_size(0);
    uint bucketPos   = groupId * RADICES_T * groupSize;

    /* Premute elements to appropriate location */
    for(int i = 0; i < RADICES_T; ++i)
    {
        uint value = unsortedData[globalId * RADICES_T + i];
        value = (value >> shiftCount) & MASK_T;
        uint index = scanedBuckets[bucketPos+localId * RADICES_T + value];
        sortedData[index] = unsortedData[globalId * RADICES_T + i];
        scanedBuckets[bucketPos+localId * RADICES_T + value] = index + 1;
        barrier(CLK_LOCAL_MEM_FENCE);
    }
}


kernel
void permuteDescendingRadixNInstantiated(uint radix, uint mask, __global uint* unsortedData,
             __global uint* scanedBuckets,
             uint shiftCount,
             __global uint* sortedData)
{
    const int RADIX_T     = radix;
    const int RADICES_T   = (1 << RADIX_T);
    const int MASK_T      = mask;

    size_t groupId   = get_group_id(0);
    size_t localId   = get_local_id(0);
    size_t globalId  = get_global_id(0);
    size_t groupSize = get_local_size(0);
    uint bucketPos   = groupId * RADICES_T * groupSize;

    /* Premute elements to appropriate location */
    for(int i = 0; i < RADICES_T; ++i)
    {
        uint value = unsortedData[globalId * RADICES_T + i];
        value = (value >> shiftCount) & MASK_T;
        uint index = scanedBuckets[bucketPos+localId * RADICES_T + (RADICES_T -value-1)];
        sortedData[index] = unsortedData[globalId * RADICES_T + i];
        scanedBuckets[bucketPos+localId * RADICES_T + (RADICES_T -value-1)] = index + 1;
        barrier(CLK_LOCAL_MEM_FENCE);
    }
}

