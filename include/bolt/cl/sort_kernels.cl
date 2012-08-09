#pragma OPENCL EXTENSION cl_amd_printf : enable

template <typename T,  typename Compare>
kernel
void sortTemplate(global T * theArray, 
                 const uint stage, 
                 const uint passOfStage,
                 global Compare *userComp,  
                 local T *scratch)
{
    uint threadId = get_global_id(0);
    uint pairDistance = 1 << (stage - passOfStage);
    uint blockWidth   = 2 * pairDistance;
    uint temp; 
    uint leftId = (threadId % pairDistance) 
                   + (threadId / pairDistance) * blockWidth;

    uint rightId = leftId + pairDistance;
    //printf("gid=%d,lID=%d,rID=%d\n",threadId,leftId,rightId);
    uint leftElement = theArray[leftId];
    uint rightElement = theArray[rightId];
    uint element;
    uint sameDirectionBlockWidth = 1 << stage;
    
    if((threadId/sameDirectionBlockWidth) % 2 == 1)
    {   
        temp = rightId;
        rightId = leftId;
        leftId = temp;
    }

    uint greater;
    uint lesser;
    element = (*userComp)(leftElement, rightElement);

    if(element == leftElement)
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
    //printf("    theArray[%d]=%d  theArray[%d]=%d\n",leftId,theArray[leftId],rightId,theArray[rightId]);
}



