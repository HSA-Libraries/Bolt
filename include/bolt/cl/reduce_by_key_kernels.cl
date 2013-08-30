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
#pragma OPENCL EXTENSION cl_amd_printf : enable


/******************************************************************************
 *  Kernel 0
 *****************************************************************************/
template<
    typename kType,
    typename kIterType,
    typename BinaryPredicate,
    typename BinaryFunction >
__kernel void OffsetCalculation(
    global kType *ikeys, //input keys
    kIterType keys,
    global int *output2, //offsetKeys
    const uint vecSize,
    global BinaryPredicate *binaryPred,
    global BinaryFunction *binaryFunct)
{

    keys.init( ikeys );

    size_t gloId = get_global_id( 0 );
    size_t groId = get_group_id( 0 );
    size_t locId = get_local_id( 0 );
    size_t wgSize = get_local_size( 0 );

	 if (gloId >= vecSize) return;

    kType key, prev_key;

    if(gloId > 0){
      key = keys[ gloId ];
	  prev_key = keys[ gloId - 1];
	  if((*binaryPred)(key, prev_key))
	    output2[ gloId ] = 0;
	  else
		output2[ gloId ] = 1;
	}
	else{
		 output2[ gloId ] = 0;
	}
}

/******************************************************************************
 *  Kernel 1
 *****************************************************************************/
template<
    typename vType,
    typename vIterType,
    typename oType,
    typename voType,
    typename BinaryFunction >
__kernel void perBlockScanByKey(
    global int *keys,
    global vType *ivals, //input values
    vIterType vals,
    global voType *output, // offsetValues
    const uint vecSize,
    local int *ldsKeys,
    local oType *ldsVals,
    global BinaryFunction *binaryFunct,
    global int *keyBuffer,
    global oType *valBuffer)
{

    vals.init( ivals );

    size_t gloId = get_global_id( 0 );
    size_t groId = get_group_id( 0 );
    size_t locId = get_local_id( 0 );
    size_t wgSize = get_local_size( 0 );

    // if exclusive, load gloId=0 w/ init, and all others shifted-1
    int key;
    oType val;

    if(gloId < vecSize){
      key = keys[ gloId ];
      val = vals[ gloId ];
      ldsKeys[ locId ] = key;
      ldsVals[ locId ] = val;
    }
    // Computes a scan within a workgroup
    // updates vals in lds but not keys
    oType sum = val;
    for( size_t offset = 1; offset < wgSize; offset *= 2 )
    {
        barrier( CLK_LOCAL_MEM_FENCE );
        int key2 = ldsKeys[locId - offset];
        if (locId >= offset && key == key2)
        {
            oType y = ldsVals[ locId - offset ];
            sum = (*binaryFunct)( sum, y );
        }
        barrier( CLK_LOCAL_MEM_FENCE );
        ldsVals[ locId ] = sum;
    }
    barrier( CLK_LOCAL_MEM_FENCE ); // needed for large data types
    //  Abort threads that are passed the end of the input vector
    if (gloId >= vecSize) return;

    // Each work item writes out its calculated scan result, relative to the beginning
    // of each work group
	int key2 = -1;
	if (gloId < vecSize -1 )
		key2 = keys[gloId + 1];
	if(key != key2)
       output[ gloId ] = sum;

    if (locId == 0)
    {
        keyBuffer[ groId ] = ldsKeys[ wgSize-1 ];
        valBuffer[ groId ] = ldsVals[ wgSize-1 ];
    }
}


/******************************************************************************
 *  Kernel 2
 *****************************************************************************/
template<
    typename oType,
    typename BinaryFunction >
__kernel void intraBlockInclusiveScanByKey(
    global int *keySumArray,
    global oType *preSumArray,
    global oType *postSumArray,
    const uint vecSize,
    local int *ldsKeys,
    local oType *ldsVals,
    const uint workPerThread,
    global BinaryFunction  *binaryFunct )
{
    size_t groId = get_group_id( 0 );
    size_t gloId = get_global_id( 0 );
    size_t locId = get_local_id( 0 );
    size_t wgSize = get_local_size( 0 );
    uint mapId  = gloId * workPerThread;

    // do offset of zero manually
    uint offset;
    int key;
    oType workSum;

    if (mapId < vecSize)
    {
        int prevKey;

        // accumulate zeroth value manually
        offset = 0;
        key = keySumArray[ mapId+offset ];
        workSum = preSumArray[ mapId+offset ];
        postSumArray[ mapId+offset ] = workSum;

        //  Serial accumulation
        for( offset = offset+1; offset < workPerThread; offset += 1 )
        {
            prevKey = key;
            key = keySumArray[ mapId+offset ];
            if (mapId+offset<vecSize )
            {
                oType y = preSumArray[ mapId+offset ];
                if ( key == prevKey )
                {
                    workSum = (*binaryFunct)( workSum, y );
                }
                else
                {
                    workSum = y;
                }
                postSumArray[ mapId+offset ] = workSum;
            }
        }
    }
    barrier( CLK_LOCAL_MEM_FENCE );

    oType scanSum = workSum;
    offset = 1;
    // load LDS with register sums
    ldsVals[ locId ] = workSum;
    ldsKeys[ locId ] = key;
    // scan in lds
    for( offset = offset*1; offset < wgSize; offset *= 2 )
    {
        barrier( CLK_LOCAL_MEM_FENCE );
        if (mapId < vecSize)
        {
            if (locId >= offset  )
            {
                oType y = ldsVals[ locId - offset ];
                int key1 = ldsKeys[ locId ];
                int key2 = ldsKeys[ locId-offset ];
                if ( key1 == key2 )
                {
                   scanSum = (*binaryFunct)( scanSum, y );
                }
                else
                   scanSum = ldsVals[ locId ];
             }

        }
        barrier( CLK_LOCAL_MEM_FENCE );
        ldsVals[ locId ] = scanSum;
    } // for offset
    barrier( CLK_LOCAL_MEM_FENCE );

    // write final scan from pre-scan and lds scan
    for( offset = 0; offset < workPerThread; offset += 1 )
    {
        barrier( CLK_GLOBAL_MEM_FENCE );

        if (mapId < vecSize && locId > 0)
        {
            oType y = postSumArray[ mapId+offset ];
            int key1 = keySumArray[ mapId+offset ]; // change me
            int key2 = ldsKeys[ locId-1 ];
            if ( key1 == key2 )
            {
                oType y2 = ldsVals[locId-1];
                y = (*binaryFunct)( y, y2 );
            }
            postSumArray[ mapId+offset ] = y;
        } // thread in bounds
    } // for

} // end kernel


/******************************************************************************
 *  Kernel 3
 *****************************************************************************/
template<
    typename oType,
    typename BinaryFunction >
__kernel void perBlockAdditionByKey(
    global int *keySumArray, //InputBuffer
    global oType *postSumArray, //InputBuffer
	global int *keys,
    global oType *output, //offsetValArray
    const uint vecSize,
    global BinaryFunction *binaryFunct)
{
    size_t gloId = get_global_id( 0 );
    size_t groId = get_group_id( 0 );
    size_t locId = get_local_id( 0 );

    //  Abort threads that are passed the end of the input vector
    if( gloId >= vecSize )
        return;

    // accumulate prefix
    int key1 = keySumArray[ groId-1 ];
    int key2 = keys[ gloId ];
	int key3 = -1;
	if(gloId < vecSize -1 )
	  key3 =  keys[ gloId + 1];
    if (groId > 0 && key1 == key2 && key2 != key3)
    {
	    oType scanResult = output[ gloId ];
        oType postBlockSum = postSumArray[ groId-1 ];
        oType newResult = (*binaryFunct)( scanResult, postBlockSum );
        output[ gloId ] = newResult;

    }
}


/******************************************************************************
 *  Kernel 4
 *****************************************************************************/
 template<
    typename kType,
    typename kIterType,
    typename koType,
    typename koIterType,
    typename voType,
    typename voIterType >
__kernel void keyValueMapping(
    global kType *ikeys,
    kIterType keys,
    global koType *ikeys_output,
    koIterType keys_output,
    global voType *ivals_output,
    voIterType vals_output,
    global int *offsetArray,
    global voType *offsetValArray,
    const uint vecSize,
    int numSections)
{
    keys.init( ikeys );
    keys_output.init( ikeys_output );
    vals_output.init( ivals_output );


    size_t gloId = get_global_id( 0 );
	size_t locId = get_local_id( 0 );

    //  Abort threads that are passed the end of the input vector
    if( gloId >= vecSize )
        return;

	numSections = *(offsetArray+vecSize-1) + 1;
    if(gloId < (vecSize-1) && offsetArray[ gloId ] != offsetArray[ gloId +1])
    {
		keys_output[ offsetArray [ gloId ]] = keys[ gloId];
        vals_output[ offsetArray [ gloId ]] = offsetValArray [ gloId];
    }

  if( gloId == (vecSize-1) )
  {
        keys_output[ numSections - 1 ] = keys[ gloId ]; //Copying the last key directly. Works either ways
        vals_output[ numSections - 1 ] = offsetValArray [ gloId ];
	    offsetArray [ gloId ] = numSections;
  }

}
