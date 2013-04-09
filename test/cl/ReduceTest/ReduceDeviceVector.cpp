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

#include <bolt/cl/device_vector.h>
#include <bolt/cl/reduce.h>
#include <bolt/cl/control.h>
#include <numeric>
#include "common/myocl.h"

void testDeviceVector()
{
    const int aSize = 1000;
    std::vector<int> hA(aSize);
    bolt::cl::device_vector<int> dA(aSize);

    for(int i=0; i<aSize; i++) {
        hA[i] = i;
        dA[i] = i;
    };

    int hSum = std::accumulate(hA.begin(), hA.end(), 0);
    int sum = bolt::cl::reduce(dA.begin(), dA.end(), 0);
    
};

BOLT_FUNCTOR(UDD,
struct UDD { 

     int a; 
    int b;
    bool operator() (const UDD& lhs, const UDD& rhs) { 
        return ((lhs.a+lhs.b) > (rhs.a+rhs.b));
    } 
    bool operator < (const UDD& other) const { 
        return ((a+b) < (other.a+other.b));
    }
    bool operator > (const UDD& other) const { 
        return ((a+b) > (other.a+other.b));
    }
    bool operator == (const UDD& other) const { 
        return ((a+b) == (other.a+other.b));
    }

    UDD operator + (const UDD &rhs) const {
                UDD tmp = *this;
                tmp.a = tmp.a + rhs.a;
                tmp.b = tmp.b + rhs.b;
                return tmp;
    }
    
    UDD() 
        : a(0),b(0) { } 
    UDD(int _in) 
        : a(_in), b(_in +1)  { } 
}; 
);
BOLT_CREATE_TYPENAME( bolt::cl::device_vector< UDD >::iterator );
BOLT_CREATE_CLCODE( bolt::cl::device_vector< UDD >::iterator, bolt::cl::deviceVectorIteratorTemplate );
BOLT_CREATE_TYPENAME(bolt::cl::plus<UDD>);


void testTBB()
{
    const int aSize = 1<<24;
    std::vector<int> stdInput(aSize);
    std::vector<int> tbbInput(aSize);


    for(int i=0; i<aSize; i++) {
        stdInput[i] = 2;
        tbbInput[i] = 2;
    };

    int hSum = std::accumulate(stdInput.begin(), stdInput.end(), 2);
    bolt::cl::control ctl = bolt::cl::control::getDefault();

    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);
    int sum = bolt::cl::reduce(ctl, tbbInput.begin(), tbbInput.end(), 2);

    if(hSum == sum)
        printf ("\nTBB Test case PASSED %d %d\n", hSum, sum);
    else
        printf ("\nTBB Test case FAILED\n");

};
void testdoubleTBB()
{
  const int aSize = 1<<24;
    std::vector<double> stdInput(aSize);
    std::vector<double> tbbInput(aSize);


    for(int i=0; i<aSize; i++) {
        stdInput[i] = 3.0;
        tbbInput[i] = 3.0;
    };

    double hSum = std::accumulate(stdInput.begin(), stdInput.end(), 1.0);
    bolt::cl::control ctl = bolt::cl::control::getDefault();
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);
    double sum = bolt::cl::reduce(ctl, tbbInput.begin(), tbbInput.end(), 1.0);
    if(hSum == sum)
        printf ("\nTBB Test case PASSED %lf %lf\n", hSum, sum);
    else
        printf ("\nTBB Test case FAILED\n");
}

void testUDDTBB()
{

    const int aSize = 1<<19;
    std::vector<UDD> stdInput(aSize);
    std::vector<UDD> tbbInput(aSize);

    UDD initial;
    initial.a = 1;
    initial.b = 2;

    for(int i=0; i<aSize; i++) {
        stdInput[i].a = 2;
        stdInput[i].b = 3;
        tbbInput[i].a = 2;
        tbbInput[i].b = 3;

    };
    bolt::cl::plus<UDD> add;
    UDD hSum = std::accumulate(stdInput.begin(), stdInput.end(), initial,add);
    bolt::cl::control ctl = bolt::cl::control::getDefault();
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);
    UDD sum = bolt::cl::reduce(ctl, tbbInput.begin(), tbbInput.end(), initial, add);
    if(hSum == sum)
        printf ("\nUDDTBB Test case PASSED %d %d %d %d\n", hSum.a, sum.a, hSum.b, sum.b);
    else
        printf ("\nUDDTBB Test case FAILED\n");
        
}

void testTBBDevicevector()
{
    const int aSize = 1024;
    std::vector<int> stdInput(aSize);
    bolt::cl::device_vector<int> tbbInput(aSize, 0);


    for(int i=0; i<aSize; i++) {
        stdInput[i] = i;
        tbbInput[i] = i;
    };

    int hSum = std::accumulate(stdInput.begin(), stdInput.end(), 0);
    bolt::cl::control ctl = bolt::cl::control::getDefault();
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);
    int sum = bolt::cl::reduce(ctl, tbbInput.begin(), tbbInput.end(), 0);
    if(hSum == sum)
        printf ("\nTBBDevicevector Test case PASSED %d %d\n", hSum, sum);
    else
        printf ("\nTBBDevicevector Test case FAILED*********\n");


};