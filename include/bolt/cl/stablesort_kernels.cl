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

//  Good web references to read about stable sorting:
//  Parallel Merge Sort: http://www.drdobbs.com/parallel/parallel-merge-sort/229400239
//  Parallel Merge: http://www.drdobbs.com/parallel/parallel-merge/229204454

//  Good white papers to read about stable sorting:
//  Efficient parallel merge sort for fixed and variable length keys: 
//  http://www.idav.ucdavis.edu/publications/print_pub?pub_id=1085

//  Designing Efficient sorting algorithms for ManyCore GPUs: 
//  http://www.drdobbs.com/parallel/parallel-merge/229204454

#pragma OPENCL EXTENSION cl_amd_printf : enable

//  This implements a linear search routine to look for an 'insertion point' in a sequence, denoted
//  by a base pointer and a left and right index, with a  candidate value.  The comparison operator is 
//  passed as a functor parameter lessOp
//  This function returns an index that would be the appropriate index to use to insert the value
template< typename sType, typename StrictWeakOrdering >
uint lowerBoundSequential( global sType* data, uint left, uint right, sType searchVal, global StrictWeakOrdering* lessOp )
{
    //  The values firstIndex and lastIndex get modified within the loop, narrowing down the potential sequence
    uint firstIndex = left;
    uint lastIndex = right;
    
    //  This loops through [firstIndex, lastIndex)
    //  Since firstIndex and lastIndex will be different for every thread depending on the nested branch,
    //  this while loop will be divergent within a wavefront
    while( firstIndex < lastIndex )
    {
        sType dataVal = data[ firstIndex ];
        
        //  This branch will create divergent wavefronts
        if( (*lessOp)( dataVal, searchVal ) )
        {
            firstIndex = firstIndex+1;
        }
        else
        {
            break;
        }
    }
    
    return firstIndex;
}

//  This implements a binary search routine to look for an 'insertion point' in a sequence, denoted
//  by a base pointer and left and right index, with a particular candidate value.  The comparison operator is 
//  passed as a functor parameter lessOp
//  This function returns an index that would be the appropriate index to use to insert the value
template< typename sType, typename StrictWeakOrdering >
uint lowerBoundBinary( global sType* data, uint left, uint right, sType searchVal, global StrictWeakOrdering* lessOp )
{
    //  The values firstIndex and lastIndex get modified within the loop, narrowing down the potential sequence
    uint firstIndex = left;
    uint lastIndex = right;
    
    //  This loops through [firstIndex, lastIndex)
    //  Since firstIndex and lastIndex will be different for every thread depending on the nested branch,
    //  this while loop will be divergent within a wavefront
    while( firstIndex < lastIndex )
    {
        //  midIndex is the average of first and last, rounded down
        uint midIndex = ( firstIndex + lastIndex ) / 2;
        sType midValue = data[ midIndex ];
        
        //  This branch will create divergent wavefronts
        if( (*lessOp)( midValue, searchVal ) )
        {
            firstIndex = midIndex+1;
            // printf( "lowerBound: lastIndex[ %i ]=%i\n", get_local_id( 0 ), lastIndex );
        }
        else
        {
            lastIndex = midIndex;
            // printf( "lowerBound: firstIndex[ %i ]=%i\n", get_local_id( 0 ), firstIndex );
        }
    }
    
    return firstIndex;
}

//  This kernel implements merging of blocks of sorted data.  The input to this kernel most likely is
//  the output of blockInsertionSortTemplate.  It is expected that the source array contains multiple
//  blocks, each block is independently sorted.  The goal is to write into the output buffer half as 
//  many blocks, of double the size.  The even and odd blocks are stably merged together to form
//  a new sorted block of twice the size.  The algorithm is out-of-place.
template< typename sPtrType, typename dIterType, typename StrictWeakOrdering >
kernel void mergeTemplate( 
                global sPtrType* source_ptr,
                dIterType    source_iter, 
                global sPtrType* result_ptr,
                dIterType    result_iter, 
                const uint srcVecSize,
                const uint srcBlockSize,
                local sPtrType* lds,
                global StrictWeakOrdering* lessOp
            )
{
    size_t globalID     = get_global_id( 0 );
    size_t groupID      = get_group_id( 0 );
    size_t localID      = get_local_id( 0 );
    size_t wgSize       = get_local_size( 0 );

    //  Abort threads that are passed the end of the input vector
    if (globalID >= srcVecSize) return; // on SI this doesn't mess-up barriers

    //  For an element in sequence A, find the lowerbound index for it in sequence B
    uint blockNum = globalID / srcBlockSize;
    uint blockIndex = globalID % srcBlockSize;
    
    // printf( "mergeTemplate: blockNum[%i]=%i\n", blockNum, blockIndex );

    //  Pairs of even-odd blocks will be merged together 
    uint oddEvenBlockIndex = blockNum&~0x1;
    // uint leftBlockIndex = min( oddEvenBlockIndex * !(blockNum & 0x1)*srcBlockSize, srcVecSize );
    uint leftBlockIndex = globalID & ~((srcBlockSize<<1) - 1 );
    leftBlockIndex += (blockNum & 0x1) ? 0 : srcBlockSize;
    uint rightBlockIndex = min( leftBlockIndex + srcBlockSize, srcVecSize );
    
    if( localID == 0 )
    {
        if( blockNum&0x1 )
            printf( "mergeTemplate: block[ %i ] index[ %i ] leftBlockIndex[ %i ] <=> rightBlockIndex[ %i ]\n", blockNum, blockIndex, leftBlockIndex, rightBlockIndex );
        else
            printf( "mergeTemplate: block[ %i ] index[ %i ] leftBlockIndex[ %i ] <=> rightBlockIndex[ %i ]\n", blockNum, blockIndex, leftBlockIndex, rightBlockIndex );
    }
    
    //  For a particular element in the input array, find the lowerbound index for it in the search sequence given by leftBlockIndex & rightBlockIndex
    uint insertionIndex = lowerBoundBinary( source_ptr, leftBlockIndex, rightBlockIndex, source_ptr[ globalID ], lessOp ) - leftBlockIndex;

    //  The index of an element in the result sequence is the summation of it's indixes in the two input 
    //  sequences
    uint resultIndex = blockIndex + insertionIndex;
    // printf( "mergeTemplate: resultIndex[ %i ] = blockIndex[ %i ] + ( insertionIndex[ %i ] )\n", resultIndex, blockIndex, insertionIndex );

    result_ptr[ (oddEvenBlockIndex*srcBlockSize)+resultIndex ] = source_ptr[ globalID ];
    // printf( "mergeTemplate: leftResultIndex[ %i ]=%i + %i\n", leftResultIndex, blockIndex, leftInsertionIndex );
}

template< typename dPtrType, typename dIterType, typename StrictWeakOrdering >
kernel void blockInsertionSortTemplate( 
                global dPtrType* data_ptr,
                dIterType    data_iter, 
                const uint vecSize,
                local dPtrType* lds,
                global StrictWeakOrdering* lessOp
            )
{
    size_t gloId    = get_global_id( 0 );
    size_t groId    = get_group_id( 0 );
    size_t locId    = get_local_id( 0 );
    size_t wgSize   = get_local_size( 0 );

    //  Abort threads that are passed the end of the input vector
    if (gloId >= vecSize) return; // on SI this doesn't mess-up barriers

    data_iter.init( data_ptr );

    //  Make a copy of the entire input array into fast local memory
    dPtrType val = data_iter[ gloId ];
    lds[ locId ] = val;
    barrier( CLK_LOCAL_MEM_FENCE );

    //  Sorts a workgroup using a naive insertion sort
    //  The sort uses one thread within a workgroup to sort the entire workgroup
    if( locId == 0 )
    {
        //  The last workgroup may have an irregular size, so we calculate a per-block endIndex
        //  endIndex is essentially emulating a mod operator with subtraction and multiply
        size_t endIndex = vecSize - ( groId * wgSize );
        endIndex = min( endIndex, wgSize );

        // printf( "Debug: endIndex[%i]=%i\n", groId, endIndex );

        //  Indices are signed because the while loop will generate a -1 index inside of the max function
        for( int currIndex = 1; currIndex < endIndex; ++currIndex )
        {
            val = lds[ currIndex ];
            int scanIndex = currIndex;
            dPtrType ldsVal = lds[scanIndex - 1];
            while( scanIndex > 0 && (*lessOp)( val, ldsVal ) )
            {
                lds[ scanIndex ] = ldsVal;
                scanIndex = scanIndex - 1;
                ldsVal = lds[ max(0, scanIndex - 1) ];  // scanIndex-1 may be -1
            }
            lds[ scanIndex ] = val;
        }
    }
    barrier( CLK_LOCAL_MEM_FENCE );

    val = lds[ locId ];
    data_iter[ gloId ] = val;
}
