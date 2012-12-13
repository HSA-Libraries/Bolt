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

#pragma OPENCL EXTENSION cl_amd_printf : enable
//#pragma OPENCL EXTENSION cl_khr_fp64 : enable 


template <typename T, typename Compare>
kernel
void selectionSortLocalTemplate(global const T * in, 
                           global T       * out, 
                           global Compare * userComp, 
                           local  T       * scratch, 
                           const int        buffSize)
{
  int          i  = get_local_id(0); // index in workgroup
  int numOfGroups = get_num_groups(0); // index in workgroup
  int groupID     = get_group_id(0);
  int         wg  = get_local_size(0); // workgroup size = block size
  int n;
  bool compareResult;
  
  int offset = groupID * wg;
  int same=0;
  in  += offset; 
  out += offset;
  n = (groupID == (numOfGroups-1))? (buffSize - wg*(numOfGroups-1)) : wg;
  
  if(i < n)
  {
      T iData = in[i];
      scratch[i] = iData;
      barrier(CLK_LOCAL_MEM_FENCE);
  
      int pos = 0;
      for (int j=0;j<n;j++)
      {
          T jData = scratch[j];
          if((*userComp)(jData, iData)) 
              pos++;
          else 
          {
              if((*userComp)(iData, jData))
                  continue;
              else 
              {
                  // iData and jData are same
                  same++;
              }
          }
      }
      for (int j=0; j< same; j++)      
         out[pos + j] = iData;
  }
  return;
}

template <typename T, typename Compare>
kernel
void selectionSortFinalTemplate(global const T * in, 
                           global T       * out, 
                           global Compare * userComp,
                           local  T       * scratch, 
                           const int        buffSize)
{
  int          i  = get_local_id(0); // index in workgroup
  int numOfGroups = get_num_groups(0); // index in workgroup
  int groupID     = get_group_id(0);
  int         wg  = get_local_size(0); // workgroup size = block size
  int pos = 0, same = 0;
  int remainder;
  int offset = get_group_id(0) * wg;

  T iData = in[groupID*wg + i];
  if((offset + i ) >= buffSize)
      return;
  
  remainder = buffSize - wg*(numOfGroups-1);
  
  for(int j=0; j<numOfGroups-1; j++ )
  {
     for(int k=0; k<wg; k++)
     {
        T jData = in[j*wg + k];
        if(((*userComp)(iData, jData)))
           break;
        else
        {
           //Increment only if the value is not the same. 
           //Two elements are same if (*userComp)(jData, iData)  and (*userComp)(iData, jData) are both false
           if( ((*userComp)(jData, iData)) )
              pos++;
           else 
              same++;
        }
     }
  }
  
  for(int k=0; k<remainder; k++)
  {
     T jData = in[(numOfGroups-1)*wg + k];
        if(((*userComp)(iData, jData)))
           break;
        else
        {
           //Don't increment if the value is the same. 
           //Two elements are same if (*userComp)(jData, iData)  and (*userComp)(iData, jData) are both false
           if(((*userComp)(jData, iData)))
              pos++;
           else 
              same++;
        }
  }  
  for (int j=0; j< same; j++)      
      out[pos + j] = iData;  
}


template <typename T, typename Compare>
kernel
void sortTemplate(global T * theArray, 
                 const uint stage,
                 const uint passOfStage,
                 global Compare *userComp)
{
    uint threadId = get_global_id(0);
    uint pairDistance = 1 << (stage - passOfStage);
    uint blockWidth   = 2 * pairDistance;
    uint temp;
    uint leftId = (threadId % pairDistance) 
                       + (threadId / pairDistance) * blockWidth;
    bool compareResult;
    
    uint rightId = leftId + pairDistance;
    
    T greater, lesser;
    T leftElement = theArray[leftId];
    T rightElement = theArray[rightId];
    
    uint sameDirectionBlockWidth = 1 << stage;
    
    if((threadId/sameDirectionBlockWidth) % 2 == 1)
    {
        temp = rightId;
        rightId = leftId;
        leftId = temp;
    }

    compareResult = (*userComp)(leftElement, rightElement);

    if(compareResult)
    {
        greater = rightElement;
        lesser  = leftElement;
    }
    else
    {
        greater = leftElement;
        lesser  = rightElement;
    }
    theArray[leftId]  = lesser;
    theArray[rightId] = greater;

}


