// TransformTest.cpp : Defines the entry point for the console application.
//
//BOLT Header files
#include <bolt/cl/clcode.h>
#include <bolt/cl/sort.h>
#include <bolt/cl/functional.h>
#include <bolt/cl/control.h>

//STD Header files
#include <iostream>
#include <algorithm>  // for testing against STL functions.
#include <vector>
#include "myocl.h"



bool myfunction (int i,int j) { return (i<j); }





template <typename T>    
struct MyFunctor { 
    T a; 
    T b; 
    bool operator() (const MyFunctor& lhs, const MyFunctor& rhs) { 
        return (lhs.a < rhs.a);
    } 
    MyFunctor(const MyFunctor &other) 
        : a(other.a), b(other.b) { } 
    MyFunctor() 
        : a(0), b(0) { } 
    MyFunctor(T& _in) 
        : a(_in), b(_in) { } 
}; 
BOLT_CREATE_TYPENAME(MyFunctor<int>);
BOLT_CREATE_CLCODE(MyFunctor<int>, "template<typename T> struct MyFunctor { T a; T b; bool operator() (const MyFunctor& lhs, const MyFunctor& rhs) { return (lhs.a < rhs.a); } };");
BOLT_CREATE_TYPENAME(MyFunctor<float>);
BOLT_CREATE_CLCODE(MyFunctor<float>, "template<typename T> struct MyFunctor { T a; T b; bool operator() (const MyFunctor& lhs, const MyFunctor& rhs) { return (lhs.a < rhs.a); } };");

template <typename stdType>
void UserDefinedFunctorSortTestOfLength(size_t length)
{
    typedef MyFunctor<stdType> myfunctor;

    std::vector<myfunctor> stdInput(length);
    std::vector<myfunctor> boltInput(length);
    std::vector<myfunctor>::iterator it; 

    size_t i;
    for (i=0;i<length;i++)
    {
        boltInput[i].a = (stdType)(length - i +2);
        stdInput[i].a  = (stdType)(length - i +2);
    }
    
    bolt::cl::sort(boltInput.begin(), boltInput.end(),myfunctor());
    std::sort(stdInput.begin(), stdInput.end(),myfunctor());
    for (i=0; i<length; i++)
    {
        if(stdInput[i].a == boltInput[i].a)
            continue;
        else
            break;
    }
    if (i==length)
        std::cout << "Test Passed" <<std::endl;
    else 
        std::cout << "Test Failed i = " << i <<std::endl;
}

template <typename T> 
struct MyType { 
    T a; 
    bool operator < (const MyType& other) const { 
        return (a < other.a);
    }
    MyType(const MyType &other) 
        : a(other.a) { } 
    MyType() 
        : a(0) { } 
    MyType(T& _in) 
        : a(_in) { } 
}; 
BOLT_CREATE_TYPENAME(MyType<int>);
BOLT_CREATE_CLCODE(MyType<int>, "template <typename T> struct MyType { T a; bool operator < (const MyType& other) const { return (a < other.a); } };");
BOLT_CREATE_TYPENAME(MyType<float>);
BOLT_CREATE_CLCODE(MyType<float>, "template <typename T> struct MyType { T a; bool operator < (const MyType& other) const { return (a < other.a); } };");

template <typename stdType>
void UserDefinedObjectSortTestOfLength(size_t length)
{
    typedef MyType<stdType> mytype;

    std::vector<mytype> stdInput(length);
    std::vector<mytype> boltInput(length);
    std::vector<mytype>::iterator it; 

    size_t i;
    for (i=0;i<length;i++)
    {
        boltInput[i].a = (stdType)(length - i +2);
        stdInput[i].a  = (stdType)(length - i +2);
    }
    
    bolt::cl::sort(boltInput.begin(), boltInput.end());
    std::sort(stdInput.begin(), stdInput.end());
    for (i=0; i<length; i++)
    {
        if(stdInput[i].a == boltInput[i].a)
            continue;
        else
            break;
    }
    if (i==length)
        std::cout << "Test Passed" <<std::endl;
    else 
        std::cout << "Test Failed i = " << i <<std::endl;
}


template <typename T>
void BasicSortTest(std::vector<T> stdInput, std::vector<T> boltInput)
{
    std::vector<T>::iterator it; 
    size_t length,  i;
    it = std::vector<T>::iterator();
    length = stdInput.size();
    bolt::cl::sort(boltInput.begin(), boltInput.end());
    std::sort(stdInput.begin(), stdInput.end());
    for (i=0; i<length; i++)
    {
        if(stdInput[i] == boltInput[i])
            continue;
        else
            break;
    }
    if (i==length)
        std::cout << "Test Passed" <<std::endl;
    else 
        std::cout << "Test Failed i = " << i <<std::endl;
}

template <typename T>
void BasicSortTestOfLength(size_t length)
{
    std::vector<T> stdInput(length);
    std::vector<T> boltInput(length);
    std::vector<T>::iterator it; 
    size_t i;
    for (i=0;i<length;i++)
    {
        boltInput[i]= (T)(length - i +2);
        stdInput[i]= (T)(length - i +2);
    }
    
    bolt::cl::sort(boltInput.begin(), boltInput.end());
    std::sort(stdInput.begin(), stdInput.end());
    for (i=0; i<length; i++)
    {
        if(stdInput[i] == boltInput[i])
            continue;
        else
            break;
    }
    if (i==length)
        std::cout << "Test Passed" <<std::endl;
    else 
        std::cout << "Test Failed i = " << i <<std::endl;
}


int main(int argc, char* argv[])
{
    int myints[] = {32,71,12,45,26,80,53,33};
    float myfloats[] = {32.3f,71.1f,12.4f,45.9f,26.98f,80.23f,53.12f,33.7f};
    double mydoubles[] = {32.3,71.1,12.4,45.9,26.98,80.23,53.12,33.7};
    std::vector<int> intStdvector (myints, myints+8);               // 32 71 12 45 26 80 53 33
    std::vector<int> intBoltvector (myints, myints+8);               // 32 71 12 45 26 80 53 33
    std::vector<float> floatStdvector (myfloats, myfloats+8);               // 32 71 12 45 26 80 53 33
    std::vector<float> floatBoltvector (myfloats, myfloats+8);               // 32 71 12 45 26 80 53 33
    std::vector<double> doubleStdvector (mydoubles, mydoubles+8);               // 32 71 12 45 26 80 53 33
    std::vector<double> doubleBoltvector (mydoubles, mydoubles+8);               // 32 71 12 45 26 80 53 33

    std::vector<int>::iterator it;


#if 1
    BasicSortTest(intStdvector, intBoltvector); 
    BasicSortTest(floatStdvector, floatBoltvector); 
    //BasicSortTest(doubleStdvector, doubleBoltvector); 
    UserDefinedObjectSortTestOfLength<int>(256);   
    UserDefinedObjectSortTestOfLength<int>(512);    
    UserDefinedObjectSortTestOfLength<int>(1024);    
    UserDefinedObjectSortTestOfLength<int>(2048);    
    UserDefinedObjectSortTestOfLength<int>(1048576); 
    UserDefinedObjectSortTestOfLength<float>(256);   
    UserDefinedObjectSortTestOfLength<float>(512);    
    UserDefinedObjectSortTestOfLength<float>(1024);    
    UserDefinedObjectSortTestOfLength<float>(2048);    
    UserDefinedObjectSortTestOfLength<float>(1048576);

    UserDefinedFunctorSortTestOfLength<int>(256);   
    UserDefinedFunctorSortTestOfLength<int>(512);    
    UserDefinedFunctorSortTestOfLength<int>(1024);    
    UserDefinedFunctorSortTestOfLength<int>(2048);    
    UserDefinedFunctorSortTestOfLength<int>(1048576); 
    UserDefinedFunctorSortTestOfLength<float>(256);   
    UserDefinedFunctorSortTestOfLength<float>(512);    
    UserDefinedFunctorSortTestOfLength<float>(1024);    
    UserDefinedFunctorSortTestOfLength<float>(2048);  
    UserDefinedFunctorSortTestOfLength<float>(1048576); 

    BasicSortTestOfLength<int>(256);
    BasicSortTestOfLength<int>(512);
    BasicSortTestOfLength<int>(1024);
    BasicSortTestOfLength<int>(2048);
    BasicSortTestOfLength<int>(1048576);
    BasicSortTestOfLength<float>(256);
    BasicSortTestOfLength<float>(512);
    BasicSortTestOfLength<float>(1024);
    BasicSortTestOfLength<float>(2048);
    BasicSortTestOfLength<float>(1048576);
    //BasicSortTestOfLength<double>(100);
    //BasicSortTestOfLength<double>(1024);
    //BasicSortTestOfLength<double>(2048);

#endif 
#if 0
	MyOclContext ocl = initOcl(CL_DEVICE_TYPE_GPU, 0);
	bolt::cl::control c(ocl._queue);  // construct control structure from the queue.
#endif
    std::cout << "Test Completed" << std::endl; 
    getchar();
	return 0;
}

