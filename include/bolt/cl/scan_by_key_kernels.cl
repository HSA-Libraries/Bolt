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
    typename vType,
    typename oType,
    typename initType,
    typename BinaryPredicate,
    typename BinaryFunction >
__kernel void perBlockScanByKey(
    global kType *keys,
    global vType *vals,
    global oType *output, // input
    initType init,
    const uint vecSize,
    local kType *ldsKeys,
    local oType *ldsVals,
    global BinaryPredicate *binaryPred,
    global BinaryFunction *binaryFunct,
    global kType *keyBuffer,
    global oType *valBuffer,
    int exclusive) // do exclusive scan ?
{
    size_t gloId = get_global_id( 0 );
    size_t groId = get_group_id( 0 );
    size_t locId = get_local_id( 0 );
    size_t wgSize = get_local_size( 0 );
    //printf("gid=%i, lTid=%i, gTid=%i\n", groId, locId, gloId);

    // if exclusive, load gloId=0 w/ init, and all others shifted-1
    kType key;
    oType val;
	  if (gloId < vecSize){
       if (exclusive)
       {
          if (gloId > 0)
          { // thread>0
              key = keys[ gloId-1 ];
              val = vals[ gloId-1 ];
              ldsKeys[ locId ] = key;
              ldsVals[ locId ] = val;
          }
          else
          { // thread=0
              val = init;
              ldsVals[ locId ] = val;
            // key stays null, this thread should never try to compare it's key
            // nor should any thread compare it's key to ldsKey[ 0 ]
            // I could put another key into lds just for whatevs
            // for now ignore this
          }
       }
       else
       {
          //vType inVal = vals[gloId];
          //val = (oType) (*unaryOp)(inVal);
          key = keys[ gloId ];
          val = vals[ gloId ];
          ldsKeys[ locId ] = key;
          ldsVals[ locId ] = val;
       }
     }
    // Computes a scan within a workgroup
    // updates vals in lds but not keys
    oType sum = val;
    for( size_t offset = 1; offset < wgSize; offset *= 2 )
    {
        barrier( CLK_LOCAL_MEM_FENCE );
        if (locId >= offset )
        {
            kType key2 = ldsKeys[ locId - offset];
            if( (*binaryPred)( key, key2 )  )
            {
                oType y = ldsVals[ locId - offset ];
                sum = (*binaryFunct)( sum, y );
            }
        }
        barrier( CLK_LOCAL_MEM_FENCE );
        ldsVals[ locId ] = sum;
    }
    
  	barrier( CLK_LOCAL_MEM_FENCE ); // needed for large data types
	  //  Abort threads that are passed the end of the input vector
    if (gloId >= vecSize) return; 

    // Each work item writes out its calculated scan result, relative to the beginning
    // of each work group
    kType curkey, prekey;
    if(exclusive && gloId > 0)
    {
       curkey = keys[gloId];
       prekey = keys[gloId-1];
       if((*binaryPred)( curkey, prekey  ))
          output[ gloId ] =  (*binaryFunct)( init, sum ); 
       else
          output[ gloId ] = init;
    }
    else
         output[ gloId ] = sum;

    
    if (locId == 0)
    {
        keyBuffer[ groId ] = ldsKeys[ wgSize-1 ];
        valBuffer[ groId ] = ldsVals[ wgSize-1 ];
    }
}


/******************************************************************************
 *  Kernel 1
 *****************************************************************************/
template<
    typename kType,
    typename oType,
    typename BinaryPredicate,
    typename BinaryFunction >
__kernel void intraBlockInclusiveScanByKey(
    global kType *keySumArray,
    global oType *preSumArray,
    global oType *postSumArray,
    const uint vecSize,
    local kType *ldsKeys,
    local oType *ldsVals,
    const uint workPerThread,
    global BinaryPredicate *binaryPred,
    global BinaryFunction  *binaryFunct )
{
    size_t groId = get_group_id( 0 );
    size_t gloId = get_global_id( 0 );
    size_t locId = get_local_id( 0 );
    size_t wgSize = get_local_size( 0 );
    uint mapId  = gloId * workPerThread;

    // do offset of zero manually
    uint offset;
    kType key;
    oType workSum;
    if (mapId < vecSize)
    {
        kType prevKey;

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
                if ( (*binaryPred)(key, prevKey ) )
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
                kType key1 = ldsKeys[ locId ];
                kType key2 = ldsKeys[ locId-offset ];
                if ( (*binaryPred)( key1, key2 ) )
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
            kType key1 = keySumArray[ mapId+offset ]; // change me
            kType key2 = ldsKeys[ locId-1 ];
            if ( (*binaryPred)( key1, key2 ) )
            {
                oType y2 = ldsVals[locId-1];
                y = (*binaryFunct)( y, y2 );
            }
            postSumArray[ mapId+offset ] = y;
        } // thread in bounds
    } // for 
} // end kernel


/******************************************************************************
 *  Kernel 2
 *****************************************************************************/
template<
    typename kType,
    typename oType,
    typename BinaryPredicate,
    typename BinaryFunction >
__kernel void perBlockAdditionByKey(
    global kType *keySumArray,
    global oType *postSumArray,
    global kType *keys,
    global oType *output,
    const uint vecSize,
    global BinaryPredicate *binaryPred,
    global BinaryFunction *binaryFunct)
{
    size_t gloId = get_global_id( 0 );
    size_t groId = get_group_id( 0 );
    size_t locId = get_local_id( 0 );

    //  Abort threads that are passed the end of the input vector
    if( gloId >= vecSize )
        return;
        
    oType scanResult = output[ gloId ];

    // accumulate prefix
    kType key1 = keySumArray[ groId-1 ];
    kType key2 = keys[ gloId ];
    if (groId > 0 && (*binaryPred)( key1, key2 ) )
    {
        oType postBlockSum = postSumArray[ groId-1 ];
        oType newResult = (*binaryFunct)( scanResult, postBlockSum );
        output[ gloId ] = newResult;
    }
}
