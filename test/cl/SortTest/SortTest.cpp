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

// A Data structure defining a less than operator
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

// A Data structure defining a Functor

template <typename T>    
struct MyFunctor { 
    T a; 
    T b; 
    bool operator() (const MyFunctor& lhs, const MyFunctor& rhs) { 
        return (lhs.a > rhs.a);
    } 
    bool operator < (const MyFunctor& other) const { 
        return (a < other.a);
    }
    bool operator > (const MyFunctor& other) const { 
        return (a > other.a);
    }
    MyFunctor(const MyFunctor &other) 
        : a(other.a), b(other.b) { } 
    MyFunctor() 
        : a(0), b(0) { } 
    MyFunctor(T& _in) 
        : a(_in), b(_in) { } 
}; 
BOLT_CREATE_TYPENAME(MyFunctor<int>);
BOLT_CREATE_CLCODE(MyFunctor<int>, "template<typename T> struct MyFunctor { T a; T b; \n\nbool operator() (const MyFunctor& lhs, const MyFunctor& rhs) { return (lhs.a > rhs.a); }   \n\nbool operator < (const MyFunctor& other) const { return (a < other.a); }   \n\nbool operator > (const MyFunctor& other) const { return (a > other.a);}  \n\n}; \n\n");
BOLT_CREATE_TYPENAME(MyFunctor<float>);
BOLT_CREATE_CLCODE(MyFunctor<float>, "template<typename T> struct MyFunctor { T a; T b; \n\nbool operator() (const MyFunctor& lhs, const MyFunctor& rhs) { return (lhs.a > rhs.a); }   \n\nbool operator < (const MyFunctor& other) const { return (a < other.a); }   \n\nbool operator > (const MyFunctor& other) const { return (a > other.a);}  \n\n}; \n\n");

template <typename T>
bool FUNCTION (T &i,T &j) { return (i<j); }

template <typename stdType>
void UserDefinedLambdaSortTestOfLength(size_t length)
{
    std::vector<stdType> stdInput(length);
    std::vector<stdType> boltInput(length);
    std::vector<stdType>::iterator it; 
    auto func = [](const stdType & a, const stdType & b) {  return a < b;  };

    size_t i;
    for (i=0;i<length;i++)
    {
        boltInput[i]= (stdType)(length - i +2);
        stdInput[i]= (stdType)(length - i +2);
    }
    
    bolt::cl::sort(boltInput.begin(), boltInput.end(), func," [](const stdType & a, const stdType & b) {  return a < b;  };");
    std::sort(stdInput.begin(), stdInput.end(),func); 
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

template <typename stdType>
void UserDefinedFunctionSortTestOfLength(size_t length)
{
    std::vector<stdType> stdInput(length);
    std::vector<stdType> boltInput(length);

    typedef bool (*MyFunction)(stdType &i,stdType &j);
    size_t i;
    for (i=0;i<length;i++)
    {
        boltInput[i] = (stdType)(length - i +2);
        stdInput[i]  = (stdType)(length - i +2);
    }
    MyFunction function = FUNCTION<stdType>;
    std::string functionString("bool FUNCTION(" + std::string(typeid(stdType).name()) + " in1, " + std::string(typeid(stdType).name()) + " in2) { return (in1 < in2); }");
    bolt::cl::sort(boltInput.begin(), boltInput.end(), functionString);
    std::sort(stdInput.begin(), stdInput.end(), function);
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

BOLT_CREATE_TYPENAME(bolt::cl::greater< MyFunctor<int> >);
BOLT_CREATE_TYPENAME(bolt::cl::greater< MyFunctor<float> >);
BOLT_CREATE_TYPENAME(bolt::cl::greater< MyFunctor<double> >);
template <typename stdType>
void UserDefinedBoltFunctorSortTestOfLength(size_t length)
{
    typedef MyFunctor<stdType> myfunctor;

    std::vector<myfunctor> stdInput(length);
    std::vector<myfunctor> boltInput(length);

    size_t i;
    for (i=0;i<length;i++)
    {
        boltInput[i].a = (stdType)(length - i +2);
        stdInput[i].a  = (stdType)(length - i +2);
    }
    
    bolt::cl::sort(boltInput.begin(), boltInput.end(),bolt::cl::greater<myfunctor>());
    std::sort(stdInput.begin(), stdInput.end(),bolt::cl::greater<myfunctor>());

    for (i=0; i<length; i++)
    {
        if (stdInput[i].a == boltInput[i].a)
            continue;
        else
            break;
    }
    if (i==length)
        std::cout << "Test Passed" <<std::endl;
    else 
        std::cout << "Test Failed i = " << i <<std::endl;
}

template <typename stdType>
void UserDefinedFunctorSortTestOfLength(size_t length)
{
    typedef MyFunctor<stdType> myfunctor;

    std::vector<myfunctor> stdInput(length);
    std::vector<myfunctor> boltInput(length);

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
        if (stdInput[i].a == boltInput[i].a)
            continue;
        else
            break;
    }
    if (i==length)
        std::cout << "Test Passed" <<std::endl;
    else 
        std::cout << "Test Failed i = " << i <<std::endl;
}

template <typename stdType>
void UserDefinedObjectSortTestOfLength(size_t length)
{
    typedef MyType<stdType> mytype;

    std::vector<mytype> stdInput(length);
    std::vector<mytype> boltInput(length);

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

#if 0
	MyOclContext ocl = initOcl(CL_DEVICE_TYPE_GPU, 0);
	bolt::cl::control c(ocl._queue);  // construct control structure from the queue.
#endif
        
#if 1
    UserDefinedBoltFunctorSortTestOfLength<int>(256);   
    UserDefinedBoltFunctorSortTestOfLength<int>(512);    
    UserDefinedBoltFunctorSortTestOfLength<int>(1024);    
    UserDefinedBoltFunctorSortTestOfLength<int>(2048);    
    UserDefinedBoltFunctorSortTestOfLength<int>(1048576); 
    UserDefinedBoltFunctorSortTestOfLength<float>(256);   
    UserDefinedBoltFunctorSortTestOfLength<float>(512);
    UserDefinedBoltFunctorSortTestOfLength<float>(1024);
    UserDefinedBoltFunctorSortTestOfLength<float>(2048);
    UserDefinedBoltFunctorSortTestOfLength<float>(1048576);
#if 0    
    UserDefinedFunctorSortTestOfLength<double>(256);   
    UserDefinedFunctorSortTestOfLength<double>(512);    
    UserDefinedFunctorSortTestOfLength<double>(1024);    
    UserDefinedFunctorSortTestOfLength<double>(2048);  
    UserDefinedFunctorSortTestOfLength<double>(1048576); 
#endif
#endif


#if 1
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
#if 0    
    UserDefinedFunctorSortTestOfLength<double>(256);   
    UserDefinedFunctorSortTestOfLength<double>(512);    
    UserDefinedFunctorSortTestOfLength<double>(1024);    
    UserDefinedFunctorSortTestOfLength<double>(2048);  
    UserDefinedFunctorSortTestOfLength<double>(1048576); 
#endif
#endif

#if 1
    UserDefinedFunctionSortTestOfLength<int>(256);   
    UserDefinedFunctionSortTestOfLength<int>(512);    
    UserDefinedFunctionSortTestOfLength<int>(1024);    
    UserDefinedFunctionSortTestOfLength<int>(2048);    
    UserDefinedFunctionSortTestOfLength<int>(1048576); 
    UserDefinedFunctionSortTestOfLength<float>(256);   
    UserDefinedFunctionSortTestOfLength<float>(512);    
    UserDefinedFunctionSortTestOfLength<float>(1024);    
    UserDefinedFunctionSortTestOfLength<float>(2048);  
    UserDefinedFunctionSortTestOfLength<float>(1048576); 
#if 0
    UserDefinedFunctionSortTestOfLength<double>(256);   
    UserDefinedFunctionSortTestOfLength<double>(512);    
    UserDefinedFunctionSortTestOfLength<double>(1024);    
    UserDefinedFunctionSortTestOfLength<double>(2048);  
    UserDefinedFunctionSortTestOfLength<double>(1048576); 
#endif
#endif

#if 1
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
#if 0
    UserDefinedObjectSortTestOfLength<double>(256);   
    UserDefinedObjectSortTestOfLength<double>(512);    
    UserDefinedObjectSortTestOfLength<double>(1024);    
    UserDefinedObjectSortTestOfLength<double>(2048);    
    UserDefinedObjectSortTestOfLength<double>(1048576);
#endif
#endif

#if 1
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
#if 0
    BasicSortTestOfLength<double>(256);
    BasicSortTestOfLength<double>(512);
    BasicSortTestOfLength<double>(1024);
    BasicSortTestOfLength<double>(2048);
    BasicSortTestOfLength<double>(1048576);
#endif
#endif 

    std::cout << "Test Completed" << std::endl; 
    getchar();
	return 0;
}

