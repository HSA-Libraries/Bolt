/* ============================================================

Copyright (c) 2009-2010 Advanced Micro Devices, Inc.  All rights reserved.
 
Redistribution and use of this material is permitted under the following 
conditions:
 
Redistributions must retain the above copyright notice and all terms of this 
license.
 
In no event shall anyone redistributing or accessing or using this material 
commence or participate in any arbitration or legal action relating to this 
material against Advanced Micro Devices, Inc. or any copyright holders or 
contributors. The foregoing shall survive any expiration or termination of 
this license or any agreement or access or use related to this material. 

ANY BREACH OF ANY TERM OF THIS LICENSE SHALL RESULT IN THE IMMEDIATE REVOCATION 
OF ALL RIGHTS TO REDISTRIBUTE, ACCESS OR USE THIS MATERIAL.

THIS MATERIAL IS PROVIDED BY ADVANCED MICRO DEVICES, INC. AND ANY COPYRIGHT 
HOLDERS AND CONTRIBUTORS "AS IS" IN ITS CURRENT CONDITION AND WITHOUT ANY 
REPRESENTATIONS, GUARANTEE, OR WARRANTY OF ANY KIND OR IN ANY WAY RELATED TO 
SUPPORT, INDEMNITY, ERROR FREE OR UNINTERRUPTED OPERA TION, OR THAT IT IS FREE 
FROM DEFECTS OR VIRUSES.  ALL OBLIGATIONS ARE HEREBY DISCLAIMED - WHETHER 
EXPRESS, IMPLIED, OR STATUTORY - INCLUDING, BUT NOT LIMITED TO, ANY IMPLIED 
WARRANTIES OF TITLE, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, 
ACCURACY, COMPLETENESS, OPERABILITY, QUALITY OF SERVICE, OR NON-INFRINGEMENT. 
IN NO EVENT SHALL ADVANCED MICRO DEVICES, INC. OR ANY COPYRIGHT HOLDERS OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, REVENUE, DATA, OR PROFITS; OR 
BUSINESS INTERRUPTION) HOWEVER CAUSED OR BASED ON ANY THEORY OF LIABILITY 
ARISING IN ANY WAY RELATED TO THIS MATERIAL, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE. THE ENTIRE AND AGGREGATE LIABILITY OF ADVANCED MICRO DEVICES, 
INC. AND ANY COPYRIGHT HOLDERS AND CONTRIBUTORS SHALL NOT EXCEED TEN DOLLARS 
(US $10.00). ANYONE REDISTRIBUTING OR ACCESSING OR USING THIS MATERIAL ACCEPTS 
THIS ALLOCATION OF RISK AND AGREES TO RELEASE ADVANCED MICRO DEVICES, INC. AND 
ANY COPYRIGHT HOLDERS AND CONTRIBUTORS FROM ANY AND ALL LIABILITIES, 
OBLIGATIONS, CLAIMS, OR DEMANDS IN EXCESS OF TEN DOLLARS (US $10.00). THE 
FOREGOING ARE ESSENTIAL TERMS OF THIS LICENSE AND, IF ANY OF THESE TERMS ARE 
CONSTRUED AS UNENFORCEABLE, FAIL IN ESSENTIAL PURPOSE, OR BECOME VOID OR 
DETRIMENTAL TO ADVANCED MICRO DEVICES, INC. OR ANY COPYRIGHT HOLDERS OR 
CONTRIBUTORS FOR ANY REASON, THEN ALL RIGHTS TO REDISTRIBUTE, ACCESS OR USE 
THIS MATERIAL SHALL TERMINATE IMMEDIATELY. MOREOVER, THE FOREGOING SHALL 
SURVIVE ANY EXPIRATION OR TERMINATION OF THIS LICENSE OR ANY AGREEMENT OR 
ACCESS OR USE RELATED TO THIS MATERIAL.

NOTICE IS HEREBY PROVIDED, AND BY REDISTRIBUTING OR ACCESSING OR USING THIS 
MATERIAL SUCH NOTICE IS ACKNOWLEDGED, THAT THIS MATERIAL MAY BE SUBJECT TO 
RESTRICTIONS UNDER THE LAWS AND REGULATIONS OF THE UNITED STATES OR OTHER 
COUNTRIES, WHICH INCLUDE BUT ARE NOT LIMITED TO, U.S. EXPORT CONTROL LAWS SUCH 
AS THE EXPORT ADMINISTRATION REGULATIONS AND NATIONAL SECURITY CONTROLS AS 
DEFINED THEREUNDER, AS WELL AS STATE DEPARTMENT CONTROLS UNDER THE U.S. 
MUNITIONS LIST. THIS MATERIAL MAY NOT BE USED, RELEASED, TRANSFERRED, IMPORTED,
EXPORTED AND/OR RE-EXPORTED IN ANY MANNER PROHIBITED UNDER ANY APPLICABLE LAWS, 
INCLUDING U.S. EXPORT CONTROL LAWS REGARDING SPECIFICALLY DESIGNATED PERSONS, 
COUNTRIES AND NATIONALS OF COUNTRIES SUBJECT TO NATIONAL SECURITY CONTROLS. 
MOREOVER, THE FOREGOING SHALL SURVIVE ANY EXPIRATION OR TERMINATION OF ANY 
LICENSE OR AGREEMENT OR ACCESS OR USE RELATED TO THIS MATERIAL.

NOTICE REGARDING THE U.S. GOVERNMENT AND DOD AGENCIES: This material is 
provided with "RESTRICTED RIGHTS" and/or "LIMITED RIGHTS" as applicable to 
computer software and technical data, respectively. Use, duplication, 
distribution or disclosure by the U.S. Government and/or DOD agencies is 
subject to the full extent of restrictions in all applicable regulations, 
including those found at FAR52.227 and DFARS252.227 et seq. and any successor 
regulations thereof. Use of this material by the U.S. Government and/or DOD 
agencies is acknowledgment of the proprietary rights of any copyright holders 
and contributors, including those of Advanced Micro Devices, Inc., as well as 
the provisions of FAR52.227-14 through 23 regarding privately developed and/or 
commercial computer software.

This license forms the entire agreement regarding the subject matter hereof and 
supersedes all proposals and prior discussions and writings between the parties 
with respect thereto. This license does not affect any ownership, rights, title,
or interest in, or relating to, this material. No terms of this license can be 
modified or waived, and no breach of this license can be excused, unless done 
so in a writing signed by all affected parties. Each term of this license is 
separately enforceable. If any term of this license is determined to be or 
becomes unenforceable or illegal, such term shall be reformed to the minimum 
extent necessary in order for this license to remain in effect in accordance 
with its terms as modified by such reformation. This license shall be governed 
by and construed in accordance with the laws of the State of Texas without 
regard to rules on conflicts of law of any state or jurisdiction or the United 
Nations Convention on the International Sale of Goods. All disputes arising out 
of this license shall be subject to the jurisdiction of the federal and state 
courts in Austin, Texas, and all defenses are hereby waived concerning personal 
jurisdiction and venue of these courts.

============================================================ */

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

__kernel
void blockAddition(__global float* input, __global float* output)
{	
	int globalId = get_global_id(0);
	int groupId = get_group_id(0);
	int localId = get_local_id(0);

	__local float value[1];

	/* Only 1 thread of a group will read from global buffer */
	if(localId == 0)
	{
		value[0] = input[groupId];
	}
	barrier(CLK_LOCAL_MEM_FENCE);

	output[globalId] += value[0];
}


__kernel 
void prefixSum(__global float *output, __global float *input, __local  float *block, const uint length)
{
	int tid = get_local_id(0);
	
	int offset = 1;

    /* Cache the computational window in shared memory */
	block[2*tid]     = input[2*tid];
	block[2*tid + 1] = input[2*tid + 1];	

    /* build the sum in place up the tree */
	for(int d = length>>1; d > 0; d >>=1)
	{
		barrier(CLK_LOCAL_MEM_FENCE);
		
		if(tid<d)
		{
			int ai = offset*(2*tid + 1) - 1;
			int bi = offset*(2*tid + 2) - 1;
			
			block[bi] += block[ai];
		}
		offset *= 2;
	}

    /* scan back down the tree */

    /* clear the last element */
	if(tid == 0)
	{
		block[length - 1] = 0;
	}

    /* traverse down the tree building the scan in the place */
	for(int d = 1; d < length ; d *= 2)
	{
		offset >>=1;
		barrier(CLK_LOCAL_MEM_FENCE);
		
		if(tid < d)
		{
			int ai = offset*(2*tid + 1) - 1;
			int bi = offset*(2*tid + 2) - 1;
			
			float t = block[ai];
			block[ai] = block[bi];
			block[bi] += t;
		}
	}
	
	barrier(CLK_LOCAL_MEM_FENCE);

    /*write the results back to global memory */
	output[2*tid]     = block[2*tid];
	output[2*tid + 1] = block[2*tid + 1];
}

__kernel 
void ScanLargeArrays(__global float *output,
               		__global float *input,
              		 __local  float *block,	 // Size : block_size
					const uint block_size,	 // size of block				
              		const uint length,	 	 // no of elements 
					 __global float *sumBuffer)  // sum of blocks
			
{
	int tid = get_local_id(0);
	int gid = get_global_id(0);
	int bid = get_group_id(0);
	
	int offset = 1;

    /* Cache the computational window in shared memory */
	block[2*tid]     = input[2*gid];
	block[2*tid + 1] = input[2*gid + 1];	

    /* build the sum in place up the tree */
	for(int d = block_size>>1; d > 0; d >>=1)
	{
		barrier(CLK_LOCAL_MEM_FENCE);
		
		if(tid<d)
		{
			int ai = offset*(2*tid + 1) - 1;
			int bi = offset*(2*tid + 2) - 1;
			
			block[bi] += block[ai];
		}
		offset *= 2;
	}

	barrier(CLK_LOCAL_MEM_FENCE);

	int group_id = get_group_id(0);
		
    /* store the value in sum buffer before making it to 0 */ 	
	sumBuffer[bid] = block[block_size - 1];

	barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
	
    /* scan back down the tree */

    /* clear the last element */
	block[block_size - 1] = 0;	

    /* traverse down the tree building the scan in the place */
	for(int d = 1; d < block_size ; d *= 2)
	{
		offset >>=1;
		barrier(CLK_LOCAL_MEM_FENCE);
		
		if(tid < d)
		{
			int ai = offset*(2*tid + 1) - 1;
			int bi = offset*(2*tid + 2) - 1;
			
			float t = block[ai];
			block[ai] = block[bi];
			block[bi] += t;
		}
	}
	
	barrier(CLK_LOCAL_MEM_FENCE);	

    /*write the results back to global memory */

	if(group_id == 0)
	{	
		output[2*gid]     = block[2*tid];
		output[2*gid + 1] = block[2*tid + 1];
	}
	else
	{
		output[2*gid]     = block[2*tid];
		output[2*gid + 1] = block[2*tid + 1];
	}
	

}

