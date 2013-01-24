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

// PairTest.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include <bolt/cl/pair.h>
#include <bolt/cl/functional.h>
#include <bolt/cl/control.h>
#include <gtest/gtest.h>
#include <cassert>

#include <iostream>
#include <algorithm>  // for testing against STL functions.
#include <numeric>
#include "common/myocl.h"

///////////////////////////////////////////
/// Pair Manipulation Test
///////////////////////////////////////////


void pairManipulationTests()
{

    bolt::cl::pair<int,int> x,y;

    // test operator ==
    x.first = x.second = y.first = y.second = 0;
    EXPECT_EQ(true, x == y);
    EXPECT_EQ(true, y == x);

    // test individual value manipulation
    x.first  = int(1);
    y.second = int(2);
    EXPECT_EQ((int)1, x.first);
    EXPECT_EQ((int)2, y.second);

    // test copy constructor
    typedef bolt::cl::pair<int,int> P;
    P p1;
    P p2(p1);
    EXPECT_EQ(p1.first,  p2.first);
    EXPECT_EQ(p1.second, p2.second);

    // test copy from std::pair constructor
    std::pair<int,int> sp(p1.first, p1.second);
    EXPECT_EQ(p1.first,  sp.first);
    EXPECT_EQ(p1.second, sp.second);

    // test initialization
    P p3 = p2;
    EXPECT_EQ(p2.first,  p3.first);
    EXPECT_EQ(p2.second, p3.second);

    // test initialization from std::pair
    P p4 = sp;
    EXPECT_EQ(sp.first,  p4.first);
    EXPECT_EQ(sp.second, p4.second);

    // test copy from pair
    p4.first  = 2;
    p4.second = 3;

    P p5;
    p5 = p4;
    EXPECT_EQ(p4.first,  p5.first);
    EXPECT_EQ(p4.second, p5.second);
    // test copy from std::pair
    sp.first  =4;
    sp.second =5;


    P p6;
    p6 = sp;
    EXPECT_EQ(sp.first,  p6.first);
    EXPECT_EQ(sp.second, p6.second);


    // test initialization from make_pair
    P p7 = bolt::cl::make_pair( 6, 7);
    EXPECT_EQ( 6, p7.first);
    EXPECT_EQ( 7, p7.second);


    // test copy from make_pair
    p7 = bolt::cl::make_pair( 8, 9);
    EXPECT_EQ( 8, p7.first);
    EXPECT_EQ( 9, p7.second);

    std::cout<<"Pair manipulation completed"<<std::endl;

}

///////////////////////////////////////////////
// Pair Comparison
///////////////////////////////////////////////

void pairComparisonTests()
{
    bolt::cl::pair<int,int> x,y;


    // test operator ==
    x.first = x.second = y.first = y.second = int(0);
    EXPECT_EQ(true, x == y);
    EXPECT_EQ(true, y == x);


    x.first = y.first = y.second = int(0);
    x.second = int(1);
    EXPECT_EQ(false, x == y);
    EXPECT_EQ(false, y == x);


    // test operator<
    x.first  = int(0); x.second = int(0);
    y.first  = int(0); y.second = int(0);
    EXPECT_EQ(false, x < y);
    EXPECT_EQ(false, y < x);


    x.first  = int(0); x.second = int(1);
    y.first  = int(2); y.second = int(3);
    EXPECT_EQ(true,  x < y);
    EXPECT_EQ(false, y < x);


    x.first  = int(0); x.second = int(0);
    y.first  = int(0); y.second = int(1);
    EXPECT_EQ(true,  x < y);
    EXPECT_EQ(false, y < x);


    x.first  = int(0); x.second = int(1);
    y.first  = int(0); y.second = int(2);
    EXPECT_EQ(true,  x < y);
    EXPECT_EQ(false, y < x);


    // test operator!=
    x.first = y.first = y.second = int(0);
    x.second = int(1);
    EXPECT_EQ(true, x != y);
    EXPECT_EQ(true, y != x);


    x.first = x.second = y.first = y.second = int(0);
    EXPECT_EQ(false, x != y);
    EXPECT_EQ(false, y != x);


    // test operator>
    x.first  = int(0); x.second = int(0);
    y.first  = int(0); y.second = int(0);
    EXPECT_EQ(false, x > y);
    EXPECT_EQ(false, y > x);


    x.first  = int(2); x.second = int(3);
    y.first  = int(0); y.second = int(1);
    EXPECT_EQ(true,  x > y);
    EXPECT_EQ(false, y > x);


    x.first  = int(0); x.second = int(1);
    y.first  = int(0); y.second = int(0);
    EXPECT_EQ(true,  x > y);
    EXPECT_EQ(false, y > x);


    x.first  = int(0); x.second = int(2);
    y.first  = int(0); y.second = int(1);
    EXPECT_EQ(true,  x > y);
    EXPECT_EQ(false, y > x);




    // test operator <=
    x.first = x.second = y.first = y.second = int(0);
    EXPECT_EQ(true, x <= y);
    EXPECT_EQ(true, y <= x);


    x.first = y.first = y.second = int(0);
    x.second = int(1);
    EXPECT_EQ(false, x <= y);


    x.first  = int(0); x.second = int(1);
    y.first  = int(2); y.second = int(3);
    EXPECT_EQ(true,  x <= y);
    EXPECT_EQ(false, y <= x);


    x.first  = int(0); x.second = int(0);
    y.first  = int(0); y.second = int(1);
    EXPECT_EQ(true,  x <= y);
    EXPECT_EQ(false, y <= x);


    x.first  = int(0); x.second = int(1);
    y.first  = int(0); y.second = int(2);
    EXPECT_EQ(true,  x <= y);
    EXPECT_EQ(false, y <= x);




    // test operator >=
    x.first = x.second = y.first = y.second = int(0);
    EXPECT_EQ(true, x >= y);
    EXPECT_EQ(true, y >= x);


    x.first = x.second = y.first = int(0);
    y.second = int(1);
    EXPECT_EQ(false, x >= y);


    x.first  = int(2); x.second = int(3);
    y.first  = int(0); y.second = int(1);
    EXPECT_EQ(true,  x >= y);
    EXPECT_EQ(false, y >= x);


    x.first  = int(0); x.second = int(1);
    y.first  = int(0); y.second = int(0);
    EXPECT_EQ(true,  x >= y);
    EXPECT_EQ(false, y >= x);


    x.first  = int(0); x.second = int(2);
    y.first  = int(0); y.second = int(1);
    EXPECT_EQ(true,  x >= y);
    EXPECT_EQ(false, y >= x);

    std::cout<<"Comparison test completed"<<std::endl;


}




int _tmain(int argc, _TCHAR* argv[])
{

    pairComparisonTests();
    pairManipulationTests();
    std::cout<<"All done"<<std::endl;
    return 0;
}

