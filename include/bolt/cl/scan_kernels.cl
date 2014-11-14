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
#pragma OPENCL EXTENSION cl_amd_printf : enable
//#define USE_AMD_HSA 1

#if USE_AMD_HSA

/******************************************************************************
 *  HSA Kernel
 *****************************************************************************/
template< typename iType, typename oType, typename initType, typename BinaryFunction >
kernel void HSA_Scan(
	global oType    *output,
	global iType    *input,
	initType        init,
	const uint      numElements,
	const uint      numIterations,
	local oType     *lds,
	global BinaryFunction* binaryOp,
	global oType    *intermediateScanArray,
	global int      *dev2host,
	global int      *host2dev,
	int             exclusive)
{
	size_t gloId = get_global_id( 0 );
	size_t groId = get_group_id( 0 );
	size_t locId = get_local_id( 0 );
	size_t wgSize = get_local_size( 0 );

	// report P1 completion
	intermediateScanArray[ groId ] = input[ groId ];
	dev2host[ groId ] = 1;



	// wait for P2 completion
	for (size_t i = 0; i < 10000; i++ )
	{
		mem_fence(CLK_GLOBAL_MEM_FENCE);
		//printf("DEV: interScan[%i]=%i ( %i, %i )", groId, intermediateScanArray[groId], dev2host[groId], host2dev[groId]);
		if ( host2dev[ groId] == 2 )
		{ // host reported P2 completion
			// report P3 completion
			dev2host[ groId ] = 3;
			break;
		}
	}
}






/******************************************************************************
 *  Not Using HSA
 *****************************************************************************/
#else

#define NUM_ITER 16
#define MIN(X,Y) X<Y?X:Y;
#define MAX(X,Y) X>Y?X:Y;
/******************************************************************************
 *  Kernel 2
 *****************************************************************************/
template< typename iPtrType, typename iIterType, typename oPtrType, typename oIterType, typename BinaryFunction, typename initType >
kernel void perBlockAddition( 
				global oPtrType* output_ptr,
				oIterType    output_iter, 
				global iPtrType* input_ptr,
				iIterType    input_iter, 
				global typename iIterType::value_type* preSumArray,
				local typename iIterType::value_type* lds,
				const uint vecSize,
				const uint load_per_wg,
				global BinaryFunction* binaryOp,
				initType identity )
{
	
// 1 thread per element
	size_t gloId = get_global_id( 0 );
	size_t groId = get_group_id( 0 );
	size_t locId = get_local_id( 0 );
	size_t wgSize = get_local_size( 0 );
	output_iter.init( output_ptr );
	input_iter.init( input_ptr );

 
	typename iIterType::value_type scanResult;
	const uint input_offset = groId * load_per_wg + locId;

	typename iIterType::value_type sum, temp;
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
			  scanResult = input_iter[input_offset + i -1];
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
		  scanResult = input_iter[input_offset + i];
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
				typename iIterType::value_type y = lds[ locId - offset ];
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

/******************************************************************************
 *  Kernel 1
 *****************************************************************************/
template< typename iPtrType, typename initType, typename BinaryFunction >
kernel void intraBlockInclusiveScan(
				global iPtrType* preSumArray, 
				initType identity,
				const uint vecSize,
				local iPtrType* lds,
				const uint workPerThread,
				global BinaryFunction* binaryOp
				)
{
	size_t gloId = get_global_id( 0 );
	size_t locId = get_local_id( 0 );
	size_t wgSize = get_local_size( 0 );
	uint mapId  = gloId * workPerThread;
	// do offset of zero manually
	uint offset;
	iPtrType workSum;
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
				iPtrType y = preSumArray[mapId+offset];
				workSum = (*binaryOp)( workSum, y );
			}
		}
	}
	barrier( CLK_LOCAL_MEM_FENCE );
	iPtrType scanSum = workSum;
	lds[ locId ] = workSum;
	offset = 1;
  // scan in lds
	for( offset = offset*1; offset < wgSize; offset *= 2 )
	{
		barrier( CLK_LOCAL_MEM_FENCE );
		if (mapId < vecSize)
		{
			if (locId >= offset)
			{
				iPtrType y = lds[ locId - offset ];
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
		iPtrType y = lds[locId-1];
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
			iPtrType y  = preSumArray[ mapId + offset ] ;
			iPtrType y1 = (*binaryOp)(y, workSum);
			preSumArray[ mapId + offset ] = y1;
			workSum = y1;

		} // thread in bounds
		else if((mapId + offset) < vecSize){
		   iPtrType y  = preSumArray[ mapId + offset ] ;
		   preSumArray[ mapId + offset ] = (*binaryOp)(y, workSum);
		   workSum = preSumArray[ mapId + offset ];
		}

	} // for 


} // end kernel


/******************************************************************************
 *  Kernel 0
 *****************************************************************************/
template< typename iPtrType, typename iIterType, typename initType, typename BinaryFunction >
kernel void perBlockInclusiveScan(
				global iPtrType* input_ptr,
				iIterType    input_iter, 
				initType identity,
				const uint vecSize,
				local typename iIterType::value_type* lds,
				global BinaryFunction* binaryOp,
				global typename iIterType::value_type* preSumArray,
				const uint load_per_wg) 
{
// 2 thread per element
	size_t gloId = get_global_id( 0 );
	size_t groId = get_group_id( 0 );
	size_t locId = get_local_id( 0 );
	size_t wgSize = get_local_size( 0 )  ;

	const uint input_offset = groId * load_per_wg + locId;
	input_iter.init( input_ptr );

	typename iIterType::value_type local_sum;
	if((input_offset) < vecSize)
		local_sum = input_iter[input_offset];

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
			typename iIterType::value_type temp = input_iter[input_offset + i];
			local_sum = (*binaryOp)(local_sum, temp);
	   }
	}
	lds[locId] = local_sum;

	typename iIterType::value_type sum = lds[locId];
	for( size_t offset = 1; offset < (wgSize); offset *= 2 )
	{
		barrier( CLK_LOCAL_MEM_FENCE );
		if (locId >= offset)
		{
			typename iIterType::value_type y = lds[ locId - offset ];
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

// not using HSA
#endif
