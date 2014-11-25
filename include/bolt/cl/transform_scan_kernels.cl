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
// #pragma OPENCL EXTENSION cl_amd_printf : enable

/******************************************************************************
 *  Kernel 0
 *****************************************************************************/

//__attribute__((reqd_work_group_size(KERNEL0WORKGROUPSIZE,1,1)))
template< typename iValueType, typename iIterType, typename oValueType, typename initType, typename UnaryFunction, typename BinaryFunction >
__kernel void perBlockTransformScan(
                global iValueType* input_ptr,
                iIterType input_iter,
                initType identity,
                const uint vecSize,
                local oValueType* lds,
                global UnaryFunction* unaryOp,
                global BinaryFunction* binaryOp,
                global oValueType* preSumArray,
				const uint load_per_wg ) 
{
   // 2 thread per element
	size_t gloId = get_global_id( 0 );
	size_t groId = get_group_id( 0 );
	size_t locId = get_local_id( 0 );
	size_t wgSize = get_local_size( 0 )  ;

	const uint input_offset = groId * load_per_wg + locId;
	input_iter.init( input_ptr );

	typename iIterType::value_type val;
	oValueType local_sum, temp;
	if((input_offset) < vecSize)
	{
		val = input_iter[input_offset];
		local_sum = (*unaryOp)(val);
	}

#if EXCLUSIVE
	if(gloId == 0)
	{
		local_sum = (*binaryOp)(local_sum, identity);
	}
#endif
	for (uint i = wgSize; i < load_per_wg; i += wgSize)
	{
	   if((input_offset + i) < vecSize)
	   {
			val = input_iter[input_offset + i];
			temp = (*unaryOp)(val);
			local_sum = (*binaryOp)(local_sum, temp);
	   }
	}
	lds[locId] = local_sum;

	oValueType sum = lds[locId];
	for( size_t offset = 1; offset < (wgSize); offset *= 2 )
	{
		barrier( CLK_LOCAL_MEM_FENCE );
		if (locId >= offset)
		{
			oValueType y = lds[ locId - offset ];
			sum = (*binaryOp)( sum, y );
		}
		barrier( CLK_LOCAL_MEM_FENCE );
		lds[ locId ] = sum;
	}
	if (locId == 0)
	{
		preSumArray[groId] = lds[wgSize-1];
	}
}


/******************************************************************************
 *  Kernel 1
 *****************************************************************************/
//__attribute__((reqd_work_group_size(KERNEL1WORKGROUPSIZE,1,1)))
template< typename Type, typename BinaryFunction >
__kernel void intraBlockInclusiveScan(
                global Type* preSumArray, 
                const uint vecSize,
                local Type* lds,
                const uint workPerThread,
                global BinaryFunction* binaryOp
                )
{
    size_t groId = get_group_id( 0 );
    size_t gloId = get_global_id( 0 );
    size_t locId = get_local_id( 0 );
    size_t wgSize = get_local_size( 0 );
    uint mapId  = gloId * workPerThread;

    // do offset of zero manually
    uint offset;
    Type workSum;
    if (mapId < vecSize)
    {
        // accumulate zeroth value manually
        offset = 0;
        workSum = preSumArray[mapId+offset];
        //  Serial accumulation
        for( offset = offset+1; offset < workPerThread; offset += 1 )
        {
            if (mapId+offset<vecSize)
            {
                Type y = preSumArray[mapId+offset];
                workSum = (*binaryOp)( workSum, y );
            }
        }
    }
    barrier( CLK_LOCAL_MEM_FENCE );
    Type scanSum = workSum;
    lds[ locId ] = workSum;
    offset = 1;
    // load LDS with register sums
    // scan in lds
    for( offset = offset*1; offset < wgSize; offset *= 2 )
    {
        barrier( CLK_LOCAL_MEM_FENCE );
        if (mapId < vecSize)
        {
            if (locId >= offset)
            {
                Type y = lds[ locId - offset ];
                scanSum = (*binaryOp)( scanSum, y );
            }
        }
        barrier( CLK_LOCAL_MEM_FENCE );
        lds[ locId ] = scanSum;
    } // for offset
    barrier( CLK_LOCAL_MEM_FENCE );
    // write final scan from pre-scan and lds scan
     workSum = preSumArray[mapId];
     if(locId > 0){
        Type y = lds[locId-1];
        workSum = (*binaryOp)(workSum, y);
        preSumArray[ mapId] = workSum;
     }
     else{
       preSumArray[ mapId] = workSum;
    }

    for( offset = 1; offset < workPerThread; offset += 1 )
    {
        barrier( CLK_GLOBAL_MEM_FENCE );
		if ((mapId + offset) < vecSize && locId > 0)
        {
            Type y  = preSumArray[ mapId + offset ] ;
            Type y1 = (*binaryOp)(y, workSum);
            preSumArray[ mapId + offset ] = y1;
            workSum = y1;

        } // thread in bounds
        else if((mapId + offset) < vecSize){
           Type y  = preSumArray[ mapId + offset ] ;
           preSumArray[ mapId + offset ] = (*binaryOp)(y, workSum);
           workSum = preSumArray[ mapId + offset ];
        }
    } // for 
} // end kernel


/******************************************************************************
 *  Kernel 2
 *****************************************************************************/
//__attribute__((reqd_work_group_size(KERNEL2WORKGROUPSIZE,1,1)))
template< typename iValueType, typename iIterType, typename oValueType, typename oIterType, typename UnaryFunction, 
           typename initType, typename BinaryFunction >
__kernel void perBlockAddition( 
                global oValueType* output_ptr,
                oIterType output_iter,
                global iValueType* input_ptr,
                iIterType input_iter,
                global oValueType* preSumArray,
                local oValueType* lds,
                const uint vecSize,
				const uint load_per_wg,
                global UnaryFunction* unaryOp,
                global BinaryFunction* binaryOp,
                initType identity)
{
  // 1 thread per element
	size_t gloId = get_global_id( 0 );
	size_t groId = get_group_id( 0 );
	size_t locId = get_local_id( 0 );
	size_t wgSize = get_local_size( 0 );
	output_iter.init( output_ptr );
	input_iter.init( input_ptr );

    const uint input_offset = groId * load_per_wg + locId;
	typename iIterType::value_type val;
	oValueType sum, temp, scanResult;


	for (uint i = 0; i < load_per_wg; i += wgSize)
	{
#if EXCLUSIVE
		if(gloId == 0 && i == 0 )
		{
			  scanResult = identity;
		}
		else
		{
		   if((input_offset + i) < vecSize)
		   {
			  val = input_iter[input_offset + i -1];
			  scanResult = (*unaryOp)(val);
		   }
		   if(groId > 0 && i == 0 && locId == 0)
		   {
			  scanResult = preSumArray[groId-1];
		   }
		   if(locId == 0 && i > 0)
		   {
			 temp = lds[wgSize-1];
			 scanResult = (*binaryOp)(scanResult, temp);
		   }
		}
#else	
	   if((input_offset + i) < vecSize)
	   {
	      val = input_iter[input_offset + i];
		  scanResult = (*unaryOp)(val);
	   }
	   if(groId > 0 && i == 0 && locId == 0)
	   {
		  temp = preSumArray[groId-1];
		  scanResult = (*binaryOp)(scanResult, temp);
	   }
	   if(locId == 0 && i > 0)
	   {
		   temp = lds[wgSize-1];
		   scanResult = (*binaryOp)(scanResult, temp);
	   }
#endif
	   barrier( CLK_LOCAL_MEM_FENCE );
	   lds[locId] = scanResult;
	   sum = lds[locId];
	   for( size_t offset = 1; offset < (wgSize); offset *= 2 )
	   { 
			barrier( CLK_LOCAL_MEM_FENCE );
			if (locId >= offset)
			{
				oValueType y = lds[ locId - offset ];
				sum = (*binaryOp)( sum, y );
			}
			barrier( CLK_LOCAL_MEM_FENCE );
			lds[ locId ] = sum;
		}
		if((input_offset + i) < vecSize)
			 output_iter[input_offset + i] = sum;

		barrier( CLK_LOCAL_MEM_FENCE );
	}


}
