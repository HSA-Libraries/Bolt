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

//#include "common/stdafx.h"
#include <vector>
//#include <array>
#include "bolt/cl/bolt.h"
//#include "bolt/cl/scan.h"
#include "bolt/cl/functional.h"
#include "bolt/cl/reduce_by_key.h"
#include "bolt/unicode.h"
#include "bolt/miniDump.h"

#include <gtest/gtest.h>
//#include <boost/shared_array.hpp>

#include <boost/program_options.hpp>



template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator1,
    typename OutputIterator2,
    typename BinaryFunction>
void
gold_reduce_by_key(
    InputIterator1 keys_first,
    InputIterator1 keys_last,
    InputIterator2 values_first,
    OutputIterator1 keys_output,
    OutputIterator2 values_output,
    BinaryFunction binary_op)
{
    typedef std::iterator_traits< InputIterator1 >::value_type kType;
    typedef std::iterator_traits< InputIterator2 >::value_type vType;
    typedef std::iterator_traits< OutputIterator1 >::value_type koType;
    typedef std::iterator_traits< OutputIterator2 >::value_type voType;
       static_assert( std::is_convertible< vType, voType >::value,
        "InputIterator2 and OutputIterator's value types are not convertible." );

    // do zeroeth element
    *values_output = *values_first;
    *keys_output = *keys_first;
    unsigned int count = 1;
    // rbk oneth element and beyond

    values_first++;
    for ( InputIterator1 key = (keys_first+1); key != keys_last; key++)
    {
        // load keys
        kType currentKey  = *(key);
        kType previousKey = *(key-1);

        // load value
        voType currentValue = *values_first; 
        voType previousValue = *values_output; //Sure?: Damn sure

        previousValue = *values_output;
        // within segment
        if (currentKey == previousKey)
        {
            //std::cout << "continuing segment" << std::endl;
            voType r = binary_op( previousValue, currentValue);
            *values_output = r;
            *keys_output = currentKey; 
            
        }
        else // new segment
        {
            values_output++;
            keys_output++;            
            *values_output = currentValue;
            *keys_output = currentKey;
            count++; //To count the number of elements in the output array
        }
        values_first++;
    }

    //return result;
    //Print values here: Temporary, things gonna change after pair()
    
    std::cout<<"Number of ele = "<<count<<std::endl;




}

int _tmain(int argc, _TCHAR* argv[])
{
 
    int length = 256;
    std::vector< int > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 1;
    std::vector< int > refInput( length );
    std::vector< int > input( length );
    for (int i = 0; i < length; i++)
    {
        if(std::rand()%3 == 1) key++;
        keys[i] = key;
        refInput[i] = std::rand()%4;
        input[i] = refInput[i];
    }

    // input and output vectors for device and reference
    
    std::vector< int > koutput( length ); 
    std::vector< int > voutput( length ); 
    //std::vector< int > refInput( length );
    std::vector< int > krefOutput( length );
    std::vector< int > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0);
    // call reduce_by_key

    bolt::cl::equal_to<int> binary_predictor;
    bolt::cl::plus<int> binary_operator;

    bolt::cl::reduce_by_key( keys.begin(), keys.end(), input.begin(), koutput.begin(), voutput.begin(), binary_predictor, binary_operator);
    for(unsigned int i = 0; i < 256 ; i++)
    {
        std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }
    gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(), vrefOutput.begin(),std::plus<int>());



    return 0;
}