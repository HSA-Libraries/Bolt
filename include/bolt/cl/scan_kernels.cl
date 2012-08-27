/*
 * ScanLargeArrays : Scan is done for each block and the sum of each
 * block is stored in separate array (sumBuffer). SumBuffer is scanned
 * and results are added to every value of next corresponding block to
 * compute the scan of a large array.(not limited to 2*MAX_GROUP_SIZE)
 * Scan uses a balanced tree algorithm. See Belloch, 1990 "Prefix Sums
 * and Their Applications"
 * @param output output data
 * @param input  input data
 * @param block  local memory used in the kernel
 * @param sumBuffer  sum of blocks
 * @param length length of the input data
 */

template< typename Type, typename BinaryFunction >
kernel void perBlockAddition( 
                global Type* output,
                global Type* input,
                global BinaryFunction* binaryOp
                )
{
    int globalId = get_global_id( 0 );
    int groupId = get_group_id( 0 );
    int localId = get_local_id( 0 );

    Type value[ 1 ];
    Type testOut = output[ globalId ];

    /* Only 1 thread of a group will read from global buffer */
    if( localId == 0 )
    {
        value[ 0 ] = input[ groupId ];
    }
    barrier( CLK_LOCAL_MEM_FENCE );

    testOut = (*binaryOp)( testOut, value[ 0 ] );
    output[ globalId ] = testOut;
}

template< typename Type, typename BinaryFunction >
kernel void intraBlockInclusiveScan( 
                global Type* output, 
                global Type* input, 
                local Type* lds, 
                const uint inputLength,
                global BinaryFunction* binaryOp    // Functor operation to apply on each step
                )
{
    int tid = get_local_id(0);

    int offset = 1;

    /* Cache the computational window in shared memory */
    lds[2*tid]     = input[2*tid];
    lds[2*tid + 1] = input[2*tid + 1];

    /* build the sum in place up the tree */
    for(int d = inputLength>>1; d > 0; d >>=1)
    {
        barrier(CLK_LOCAL_MEM_FENCE);

        if(tid<d)
        {
            int ai = offset*(2*tid + 1) - 1;
            int bi = offset*(2*tid + 2) - 1;

            lds[bi] += lds[ai];
        }
        offset *= 2;
    }

    /* scan back down the tree */

    /* clear the last element */
    if(tid == 0)
    {
        lds[inputLength - 1] = 0;
    }

    /* traverse down the tree building the scan in the place */
    for(int d = 1; d < inputLength ; d *= 2)
    {
        offset >>=1;
        barrier(CLK_LOCAL_MEM_FENCE);

        if(tid < d)
        {
            int ai = offset*(2*tid + 1) - 1;
            int bi = offset*(2*tid + 2) - 1;

            Type t = lds[ai];
            lds[ai] = lds[bi];
            lds[bi] += t;
        }
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    /*write the results back to global memory */
    output[2*tid]     = lds[2*tid];
    output[2*tid + 1] = lds[2*tid + 1];
}

template< typename Type, typename BinaryFunction >
kernel void perBlockInclusiveScan(
                global Type* output,
                global Type* input,
                local  Type* lds,
                const uint ldsSize,
                global BinaryFunction* binaryOp,    // Functor operation to apply on each step
                global Type* scanBuffer)            // Passed to 2nd kernel; the of each block
{
    int tid = get_local_id(0);
    int gid = get_global_id(0);
    int bid = get_group_id(0);

    int offset = 1;

    /* Cache the computational window in shared memory */
    lds[2*tid]     = input[2*gid];
    lds[2*tid + 1] = input[2*gid + 1];

    /* build the sum in place up the tree */
    for(int d = ldsSize>>1; d > 0; d >>=1)
    {
        barrier(CLK_LOCAL_MEM_FENCE);

        if(tid<d)
        {
            int ai = offset*(2*tid + 1) - 1;
            int bi = offset*(2*tid + 2) - 1;

            lds[bi] += lds[ai];
        }
        offset *= 2;
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    int group_id = get_group_id(0);

    /* store the value in sum buffer before making it to 0 */
    scanBuffer[bid] = lds[ldsSize - 1];

    barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);

    /* scan back down the tree */

    /* clear the last element */
    lds[ldsSize - 1] = 0;

    /* traverse down the tree building the scan in the place */
    for(int d = 1; d < ldsSize ; d *= 2)
    {
        offset >>=1;
        barrier(CLK_LOCAL_MEM_FENCE);

        if(tid < d)
        {
            int ai = offset*(2*tid + 1) - 1;
            int bi = offset*(2*tid + 2) - 1;

            Type t = lds[ai];
            lds[ai] = lds[bi];
            lds[bi] += t;
        }
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    /*write the results back to global memory */

    if(group_id == 0)
    {
        output[2*gid]     = lds[2*tid];
        output[2*gid + 1] = lds[2*tid + 1];
    }
    else
    {
        output[2*gid]     = lds[2*tid];
        output[2*gid + 1] = lds[2*tid + 1];
    }
}

