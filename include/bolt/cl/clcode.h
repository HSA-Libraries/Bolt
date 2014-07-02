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

#pragma once
#if !defined( BOLT_CL_CLCODE_H )
#define BOLT_CL_CLCODE_H

#include <string>

/*! \file clcode.h
    \brief Defines macros to help the user create code with C++ semantics for OpenCL kernels
*/

/*! \addtogroup miscellaneous
 */

/*! \addtogroup ClCode ClCode: Traits and Helper Macros
 *  \ingroup miscellaneous
 *  \{
 */

/*! \brief Macro that wraps around arbitrary text, and creates a string out of it
*   \detailed This macro is helpful to write inline OpenCL programs, as it avoids wrapping every line of OpenCL
*   line with "XXX"\n
*   \return The contents of the macro wrapped in an ASCII string
*/
#define STRINGIFY_CODE2( ... ) #__VA_ARGS__
#define STRINGIFY_CODE( ... ) STRINGIFY_CODE2( __VA_ARGS__ )

/*!
 * Return a string with the specified function F, and also create code that is fed to the host compiler.
 */
#define BOLT_CODE_STRING( ... )  STRINGIFY_CODE( __VA_ARGS__ ); __VA_ARGS__;

/*!
 * /brief Types that are defined within the bolt::cl namespace in host code should also be defined within the same
 * namespace within the kernel code
 * /detail This is a convenience macro, intended to only be used by the Bolt library itself, to wrap internal types
 * in the appropriate bolt namespaces
 */
#define BOLT_HOST_DEVICE_DEFINITION( ... ) "namespace bolt { namespace cl {\n" STRINGIFY_CODE( __VA_ARGS__ ) "\n } } \n" ; __VA_ARGS__;

/*!
 * Bolt uses the TypeName trait to determine the string name of function object parameters when
 * generating OpenCL(TM) code that instantiates the required templates.  For example,
 * if the user calls the Bolt \p transpose library with a functor \p MyClass, Bolt
 * instantiates the transform's template function with the class \p MyClass.
 *
 * This is the default implementation for the TypeName trait, which must be overriden with
 * a class specialization to return a string name of the class.
 * The default TypeName returns an error string that causes the OpenCL(TM) kernel compilation to fail.
 *
 * The \ref BOLT_CREATE_TYPENAME is a convenience macro that helps create a TypeName trait.  An example:
 *
 * \code
 * class MyClass { ... };
 *
 * // Create TypeName trait for "MyClass" manually:
 * template<> struct TypeName<MyClass> { static std::string get() { return "MyClass"; }};
 *
 * // Same as above, but using the BOLT_CREATE_TYPENAME convenience macro.
 * BOLT_CREATE_TYPENAME(MyClass);
 * \endcode
 *
 * Note that the TypeName trait must be defined exactly once for each functor class used
 * in a Bolt API.
 */
/*
template<typename T>
struct foobar : std::false_type
{ };

*/
template< typename TypeNameType >
struct TypeName
{
    static std::string get()
    {
        static_assert( sizeof(TypeNameType) == 0  , "Bolt< error >: Unknown typename; define missing TypeName with Bolt provided macro's" );
//	return "";

    }
};

/*!
 * \brief The definition of a type trait for the definition of a type, which could be arbitrarily complex

 * Defining a new type trait to return code associated with a type.  This is meant to be specialized with the
 * BOLT_CREATE_CLCODE.  A type may have an arbitrarily complex definition (templated or not) and this returns the
 * definition for the fully specified type.

 * \tparam Type A fully specified Type
 * \todo If this does not pan out, remove
 */
template< typename Type >
struct ClCode
{
    static std::string get()
    {
        return "";
    }
};

/*!
 * A convenience macro to create the TypeName trait for a specified class \p T.
 *
 * \param T : Class for which TypeName trait will be defined.
 *
 * \code
 * // Example that shows use of BOLT_CREATE_TYPENAME macro:
 * class MyClass { ... };
 *
 * // Associate string "MyClass" with class MyClass
 * BOLT_CREATE_TYPENAME(MyClass);
 * \endcode
 *
 * See TypeName for more information.
 */
#define BOLT_CREATE_TYPENAME( ... ) \
    template<> struct TypeName< __VA_ARGS__ > { static std::string get( ) { return #__VA_ARGS__; } };

/*!
 * Creates the ClCode trait that associates the specified type \p T with the string \p CODE_STRING.
 * \param Type A fully specified type name
 * \param CODE_STRING The definition of the type, which could be a template definition
 *
 * An example:
 * \code
 * // Manually create code string inline and associate with class IsOdd:
 * BOLT_CREATE_CLCODE(IsOdd, "struct IsOdd { bool operator(int val) { return val & 0x1; }; };"
 * \endcode
 *
 * Another approach is to define the code string in a file (so host and string are identical), then
 * pass the string read from the file to BOLT_CREATE_CLCODE. (See \ref clCodeFromFile)
 */
#if defined(WIN32)
#define BOLT_CREATE_CLCODE(Type,CODE_STRING) \
    template<> struct ClCode< Type > {	static std::vector< std::string > dependencies;\
                                        static void addDependency(std::string s) { dependencies.push_back(s); }; \
                                        static std::string getDependingCodeString() { \
                                            std::string c;\
                                            for (std::vector< std::string >::iterator i = dependencies.begin(); i != dependencies.end(); i++) { c = c + *i; } \
                                            return c; \
                                        };\
                                        static std::string getCodeString() { return CODE_STRING; }; \
                                        static std::string get() { return getDependingCodeString() + getCodeString(); }; };\
 __declspec( selectany ) std::vector< std::string > ClCode< Type >::dependencies;
#else
#define BOLT_CREATE_CLCODE(Type,CODE_STRING) \
    template<> struct ClCode< Type > {	static std::vector< std::string > dependencies;\
                                        static void addDependency(std::string s) { dependencies.push_back(s); }; \
                                        static std::string getDependingCodeString() { \
                                            std::string c;\
                                            for (std::vector< std::string >::iterator i = dependencies.begin(); i != dependencies.end(); i++) { c = c + *i; } \
                                            return c; \
                                        };\
                                        static std::string getCodeString() { return CODE_STRING; }; \
                                        static std::string get() { return getDependingCodeString() + getCodeString(); }; };\
__attribute__((weak))  std::vector< std::string > ClCode< Type >::dependencies;


#endif
/*!
 * \brief This macro specializes a template with a new type using the template definition of a previously defined
 * type
 * \detail This is a convenience macro to specialize a template for a new type, using the generic template
 * definition from a previosly defined type
 * \param CONTAINER A template template parameter, such as std::vector without the type specified
 * \param OLDTYPE A type that has already been registered with the container template definition
 * \param NEWTYPE A new type to register with the template definition
 * \details An example:
 * \code
 *
 *  #include <bolt/cl/transform.h>
 *
 *  // Create a new user-defined type UDD on host and copy to kernel
 *  BOLT_FUNCTOR( UDD,
 *    class UDD
 *    {
 *       ...
 *
 *    };
 *  );
 *
 *  // Create an instance of bolt::cl::negate of type UDD
 *  BOLT_TEMPLATE_REGISTER_NEW_TYPE( bolt::cl::negate, int, UDD );
 *
 *  int main( )
 *  {
 *    ...
 *
 *    bolt::cl::device_vector< UDD > Src( 10, UDD( ) );
 *    bolt::cl::device_vector< UDD > Dest( 10 );
 *
 *    bolt::cl::transform( Src.begin( ), Src.end( ), Dest.begin( ), bolt::cl::negate< UDD >( ) );
 *
 *  }
 * \endcode
*/
#define BOLT_TEMPLATE_REGISTER_NEW_TYPE( CONTAINER, OLDTYPE, NEWTYPE ) \
            BOLT_CREATE_TYPENAME( CONTAINER< NEWTYPE > ) \
            BOLT_CREATE_CLCODE( CONTAINER< NEWTYPE >, ClCode< CONTAINER< OLDTYPE > >::get( ) )

/*!
 * \brief This macro specializes a template iterator with a new type using the template definition of a previously
 * defined iterator type
 * \detail This is a convenience macro to specialize an iterator for a new type, using the generic template
 * definition from a previosly defined iterator
 * \param CONTAINER A template template parameter, such as std::vector without the type specified
 * \param OLDTYPE A type that has already been registered with the container template definition
 * \param NEWTYPE A new type to register with the template definition
 * \details An example:
 * \code
 *
 *  #include <bolt/cl/transform.h>
 *
 *  // Create a new user-defined type UDD on host and copy to kernel
 *  BOLT_FUNCTOR( UDD,
 *    class UDD
 *    {
 *       ...
 *
 *    };
 *  );
 *
 *  // Create a new negation functor for UDD
 *  BOLT_FUNCTOR( UDDNegate,
 *    class UDDNegate
 *    {
 *       ...
 *
 *    };
 *  );
 *
 *  // Create an UDD instance of device_vector
 *  BOLT_TEMPLATE_REGISTER_NEW_ITERATOR( bolt::cl::device_vector, int, UDD );
 *
 *  int main( )
 *  {
 *    ...
 *
 *    bolt::cl::device_vector< UDD > Src( 10, UDD( ) );
 *    bolt::cl::device_vector< UDD > Dest( 10 );
 *
 *    UDDNegate udd_negate_functor;
 *    bolt::cl::transform( Src.begin( ), Src.end( ), Dest.begin( ), udd_negate_functor );
 *
 *  }
 * \endcode
 */
#define BOLT_TEMPLATE_REGISTER_NEW_ITERATOR( CONTAINER, OLDTYPE, NEWTYPE ) \
            BOLT_CREATE_TYPENAME( CONTAINER< NEWTYPE >::iterator ) \
            BOLT_CREATE_CLCODE( CONTAINER< NEWTYPE >::iterator, ClCode< CONTAINER< OLDTYPE >::iterator >::get( ) )

/*!
 * \brief This macro defines a macro on the kernel code.
 * \param DefineName Name of the macro. Can be used with other Bolt macros.
 * \param D The macro
 * \param ... Value of the macro ( Arbitrary type definition to associate with the type )  See \ref ClCodeTraits
 */
#define BOLT_CREATE_DEFINE( DefineName,D,... ) struct DefineName {}; BOLT_CREATE_CLCODE( DefineName,   std::string("#ifndef ") + std::string(#D) + std::string(" \n")\
                                                                                                  + std::string("#define ") + std::string(#D) + std::string(" ") + std::string(#__VA_ARGS__) + std::string(" \n")\
                                                                                                  + std::string("#endif \n"));


/*!
 * \brief This macro creates a TypeName and the ClCode data structure for the transform iterator associated 
 *        with FUNCTOR and DATA_TYPE. The use of this macro is necessary if transform iterator is used in the algorithm
 * \detail This macro creates a TypeName and the ClCode data structure for the transform iterator associated 
 *        with FUNCTOR and DATA_TYPE. The use of this macro is necessary if transform iterator is used in the algorithm
 * \param FUNCTOR This defines the unary function for the transform iterator. FUNCTOR should be previously defined 
 *                using BOLT_FUNCTOR. The FUNCTOR is used to transform the objects of type DATA_TYPE. The FUNCTOR 
 *                should also define an associated type result_type.
 * \param DATATYPE This is the type of the data which the transform iterator points to and the unary function FUNCTOR 
                   operates on.
 * \details An example:
 * \code
 *
 *  #include <bolt/cl/iterator/transform_iterator.h>
 *  #include <bolt/cl/transform.h
 *  // Create a new user-defined type UDD on host and copy to kernel
 *  BOLT_FUNCTOR( UDD,
 *    struct UDD
 *    {
 *        int i;
 *        float f;
 *        ...
 *    };
 *  );
 *
 *  // Create a new square functor takes int and return int
 *  BOLT_FUNCTOR(square,
 *    struct square
 *    {
 *        int operator() (const int x)  const { return x + 2; }
 *        typedef int result_type;
 *    };
 *  );
 *
 *  // Create a new negation functor for UDD
 *  BOLT_FUNCTOR( UDDNegate,
 *    class UDDNegate
 *    {
 *       ...
 *
 *    };
 *  );
 *
 * 
 *  // Create a new transform iterator with square as a functor and UDD as a data type.
 *  BOLT_TEMPLATE_REGISTER_NEW_TRANSFORM_ITERATOR( square, int);
 *
 *  // Create a new transform iterator with square as a functor and UDD as a data type.
 *  BOLT_FUNCTOR(squareUDD,
 *      struct squareUDD
 *      {
 *          UDD operator() (const UDD& x)  const 
 *          { 
 *              UDD tmp;
 *              tmp.i = x.i * x.i;
 *              tmp.f = x.f * x.f;
 *              return tmp;
 *          }
 *          typedef UDD result_type;
 *      };
 *  );
 *
 * BOLT_TEMPLATE_REGISTER_NEW_TRANSFORM_ITERATOR( square, UDD);
 *
 *  int main( )
 *  {
 *    ...
 *    {
 *       typedef bolt::BCKND::transform_iterator< square, bolt::BCKND::device_vector< int >::iterator> dv_trf_itr;
 *       typedef bolt::BCKND::transform_iterator< square, std::vector< int >::const_iterator>          sv_trf_itr;
 *
 *       std::vector< int > svInVec( length );
 *       bolt::cl::device_vector< int > dvInVec( length );
 *       std::vector< int > svOutVec( length );
 *       bolt::cl::device_vector< int > dvOutVec( length );
 *       
 *       // Initialize svInVec and dvInVec  ...
 *
 *       //Declare the required functors
 *       UDDNegate udd_negate_functor;
 *       bolt::cl::negate<int>() neg;
 *
 *       sv_trf_itr sv_trf_begin (svInVec.begin(), sq), sv_trf_end (svInVec.end(), sq);
 *       dv_trf_itr dv_trf_begin (dvInVec.begin(), sq), dv_trf_end (dvInVec.end(), sq);
 *
 *      bolt::cl::transform( sv_trf_begin, sv_trf_end, svOutVec.begin(), neg );
 *      bolt::cl::transform( dv_trf_begin, dv_trf_end, dvOutVec.begin(), neg );
 *   }
 *   {
 *      typedef bolt::BCKND::transform_iterator< squareUDD, bolt::BCKND::device_vector< UDD >::iterator> dv_udd_trf_itr;
 *      typedef bolt::BCKND::transform_iterator< squareUDD, std::vector< UDD >::const_iterator>          sv_udd_trf_itr;
 *
 *       std::vector< UDD > svInVec( length );
 *       bolt::cl::device_vector< UDD > dvInVec( length );
 *       std::vector< UDD > svOutVec( length );
 *       bolt::cl::device_vector< UDD > dvOutVec( length );
 *       
 *       // Initialize svInVec and dvInVec  ...
 *
 *       //Declare the required functors
 *       UDDNegate udd_negate_functor;
 *       squareUDD sq_udd;
 *
 *       sv_udd_trf_itr sv_trf_begin (svInVec.begin(), sq_udd), sv_trf_end (svInVec.end(), sq_udd);
 *       dv_udd_trf_itr dv_trf_begin (dvInVec.begin(), sq_udd), dv_trf_end (dvInVec.end(), sq_udd);
 *
 *      bolt::cl::transform( sv_trf_begin, sv_trf_end, svOutVec.begin(), udd_negate_functor );
 *      bolt::cl::transform( dv_trf_begin, dv_trf_end, dvOutVec.begin(), udd_negate_functor );
 *   }
 *
 *  }
 * \endcode
 */
#if defined(WIN32)
#define BOLT_TEMPLATE_REGISTER_NEW_TRANSFORM_ITERATOR( FUNCTOR, DATATYPE) \
            BOLT_CREATE_TYPENAME( bolt::cl::transform_iterator< FUNCTOR, bolt::cl::device_vector< DATATYPE >::iterator > );\
            template<> struct ClCode< bolt::cl::transform_iterator< FUNCTOR, bolt::cl::device_vector< DATATYPE >::iterator > > {	static std::vector< std::string > dependencies;\
                                        static void addDependency(std::string s) { dependencies.push_back(s); }; \
                                        static std::string getDependingCodeString() { \
                                            std::string c;\
                                            for (std::vector< std::string >::iterator i = dependencies.begin(); i != dependencies.end(); i++) { c = c + *i; } \
                                            return c; \
                                        };\
                                        static std::string getCodeString() { return ClCode<DATATYPE>::get() + ClCode<FUNCTOR>::get() + bolt::cl::deviceTransformIteratorTemplate; }; \
                                        static std::string get() { return getDependingCodeString() + getCodeString(); }; };\
                                        __declspec( selectany ) std::vector< std::string > ClCode< bolt::cl::transform_iterator< FUNCTOR, bolt::cl::device_vector< DATATYPE >::iterator > >::dependencies; 
#else
#define BOLT_TEMPLATE_REGISTER_NEW_TRANSFORM_ITERATOR( FUNCTOR, DATATYPE) \
            BOLT_CREATE_TYPENAME( bolt::cl::transform_iterator< FUNCTOR, bolt::cl::device_vector< DATATYPE >::iterator > );\
            template<> struct ClCode< bolt::cl::transform_iterator< FUNCTOR, bolt::cl::device_vector< DATATYPE >::iterator > > {	static std::vector< std::string > dependencies;\
                                        static void addDependency(std::string s) { dependencies.push_back(s); }; \
                                        static std::string getDependingCodeString() { \
                                            std::string c;\
                                            for (std::vector< std::string >::iterator i = dependencies.begin(); i != dependencies.end(); i++) { c = c + *i; } \
                                            return c; \
                                        };\
                                        static std::string getCodeString() { return ClCode<DATATYPE>::get() + ClCode<FUNCTOR>::get() + bolt::cl::deviceTransformIteratorTemplate; }; \
                                        static std::string get() { return getDependingCodeString() + getCodeString(); }; };\
                                        __attribute__((weak))  std::vector< std::string > ClCode< bolt::cl::transform_iterator< FUNCTOR, bolt::cl::device_vector< DATATYPE >::iterator > >::dependencies; 
#endif

/*!
 * \brief A Permutation iterator is associated with an ELEMENT_ITERATOR and INDEX_ITERATOR. 
 *         This Macro creates a ClCode and TypeName for the permutation iterator.
 * \detail A Permutation iterator is associated with an ELEMENT_ITERATOR and INDEX_ITERATOR. 
 *         This Macro creates a ClCode and TypeName for the permutation iterator. Note that the data type 
 *         associated with the element and index iterators must be declared within the BOLT_FUNCTOR macro.
 *         Only device vector iterators can be used with permutation iterators. Both the iterators has to be either 
 *         std iterators or device vector iterators. Combination of the two is not supported.
 * 
 * \param ELEMENT_ITERATOR This specifies the iterator for elements
 * \param INDEX_ITERATOR This specifies the iterator for the index elements
 * \details An example:
 * \code
 *
 *  #include <bolt/cl/iterator/premutation_iterator.h>
 *  #include <bolt/cl/transform.h
 *
 *  // Create a new user-defined type UDD on host and copy to kernel
 *  BOLT_FUNCTOR( UDD,
 *    struct UDD
 *    {
 *        int i;
 *        float f;
 *        ...
 *    };
 *  );
 *
 *  // Create a new permutation iterator with both index and element iterators as device_vector iterators for int data type.
 *  BOLT_TEMPLATE_REGISTER_NEW_PERMUTATION_ITERATOR( bolt::cl::device_vector<int>::iterator, bolt::cl::device_vector<int>::iterator);
 *  // Create a new permutation iterator with both index and element iterators as device_vector iterators for UDD data type.
 *  BOLT_TEMPLATE_REGISTER_NEW_PERMUTATION_ITERATOR( bolt::cl::device_vector<UDD>::iterator, bolt::cl::device_vector<UDD>::iterator);
 *  
 *  // Create a new transform iterator with square as a functor and UDD as a data type.

 *
 *  int main( )
 *  {
 *    ...
 *    {
 *       const int length = 1<<10;
 *       std::vector< int > svIndexVec( length );
 *       std::vector< int > svElementVec( length );
 *       std::vector< int > svOutVec( length );
 *       bolt::BCKND::device_vector< int > dvIndexVec( length );
 *       bolt::BCKND::device_vector< int > dvElementVec( length );        
 *       bolt::BCKND::device_vector< int > dvOutVec( length );
 *
 *       typedef std::vector< int >::const_iterator                  sv_itr;
 *       typedef bolt::BCKND::device_vector< int >::iterator         dv_itr;
 *       typedef bolt::BCKND::permutation_iterator< sv_itr, 
 *                                                  sv_itr>          sv_perm_itr;
 *       typedef bolt::BCKND::permutation_iterator< dv_itr, 
 *                                                  dv_itr>          dv_perm_itr;
 *
 *       //Create Iterators
 *       dv_perm_itr dv_perm_begin (dvElementVec.begin(), dvIndexVec.begin()), dv_perm_end (dvElementVec.end(), dvIndexVec.end());
 *
 *       // Generate the inputs for the input vectors svIndexVec, svElementVec, dvIndexVec, dvElementVec.
 *       // Call the transform routine with permutation iterators
 *       bolt::cl::transform(dv_perm_begin, dv_perm_end, dvOutVec.begin(), add3);
 *   }
 *  }
 * \endcode
 */
#if defined(WIN32)
#define BOLT_TEMPLATE_REGISTER_NEW_PERMUTATION_ITERATOR( ELEMENT_ITERATOR, INDEX_ITERATOR) \
            BOLT_CREATE_TYPENAME( bolt::cl::permutation_iterator< ELEMENT_ITERATOR, INDEX_ITERATOR > );\
            template<> struct ClCode< bolt::cl::permutation_iterator< ELEMENT_ITERATOR, INDEX_ITERATOR > > {	static std::vector< std::string > dependencies;\
                                        static void addDependency(std::string s) { dependencies.push_back(s); }; \
                                        static std::string getDependingCodeString() { \
                                            std::string c;\
                                            for (std::vector< std::string >::iterator i = dependencies.begin(); i != dependencies.end(); i++) { c = c + *i; } \
                                            return c; \
                                        };\
                                        static std::string getCodeString() { return ClCode<ELEMENT_ITERATOR>::get() + ClCode<INDEX_ITERATOR>::get() + bolt::cl::devicePermutationIteratorTemplate; }; \
                                        static std::string get() { return getDependingCodeString() + getCodeString(); }; };\
                                        __declspec( selectany ) std::vector< std::string > ClCode< bolt::cl::permutation_iterator< ELEMENT_ITERATOR, INDEX_ITERATOR > >::dependencies; 
#else
#define BOLT_TEMPLATE_REGISTER_NEW_PERMUTATION_ITERATOR( ELEMENT_ITERATOR, INDEX_ITERATOR) \
            BOLT_CREATE_TYPENAME( bolt::cl::permutation_iterator< ELEMENT_ITERATOR, INDEX_ITERATOR > );\
            template<> struct ClCode< bolt::cl::permutation_iterator< ELEMENT_ITERATOR, INDEX_ITERATOR > > {	static std::vector< std::string > dependencies;\
                                        static void addDependency(std::string s) { dependencies.push_back(s); }; \
                                        static std::string getDependingCodeString() { \
                                            std::string c;\
                                            for (std::vector< std::string >::iterator i = dependencies.begin(); i != dependencies.end(); i++) { c = c + *i; } \
                                            return c; \
                                        };\
                                        static std::string getCodeString() { return ClCode<ELEMENT_ITERATOR>::get() + ClCode<INDEX_ITERATOR>::get() + bolt::cl::devicePermutationIteratorTemplate; }; \
                                        static std::string get() { return getDependingCodeString() + getCodeString(); }; };\
                                        __attribute__((weak))  std::vector< std::string > ClCode< bolt::cl::permutation_iterator< ELEMENT_ITERATOR, INDEX_ITERATOR > >::dependencies; 
#endif


/*!
 * Creates a string and a regular version of the functor F, and automatically defines the ClCode trait to associate
 * the code string with the specified class T.
 * \param Type A fully specified type name
 * \param ... Arbitrary type definition to associate with the type.  See \ref ClCodeTraits
 * \details An example:
 * \code
 *
 *  #include <bolt/cl/transform.h>
 *
 *  // Create a new cube root functor on host and copy to kernel
 *  BOLT_FUNCTOR( CbrtFunctor,
 *    class CbrtFunctor
 *    {
 *       ...
 *
 *    };
 *  );
 *
 *  int main( )
 *  {
 *    ...
 *
 *    bolt::cl::device_vector< double > Src( 10, rand( ) );
 *    bolt::cl::device_vector< double > Dest( 10 );
 *
 *    CbrtFunctor cbrt_func;
 *    bolt::cl::transform( Src.begin( ), Src.end( ), Dest.begin( ), cbrt_func );
 *
 *  }
 * \endcode
 */
#define BOLT_FUNCTOR( T, ... ) __VA_ARGS__; \
                               BOLT_CREATE_TYPENAME( T ); \
                               BOLT_CREATE_CLCODE( T,     std::string("#ifndef ") + std::string("BOLT_FUNCTOR_") + std::string(#T) + std::string(" \n")\
                                                        + std::string("#define ") + std::string("BOLT_FUNCTOR_") + std::string(#T) + std::string(" \n")\
                                                        + std::string(#__VA_ARGS__) + std::string(" \n")\
                                                        + std::string("#endif \n"));


/*!
 * Helper macro to define and specialize a template type for Bolt.  The code given as the macro vararg
 * should be a template container that requires 1 template paramter.  The type passed into the TYPE1
 * argument is used to fully specialize the template container into a concrete type for Bolt.
 * \param CONTAINER Name  to associate with the code given in the macro variable argument
 * \param TYPE1 Type used to fully specialize the template code given in the macro variable argument
 * \param ... Arbitrary user defined code; expected to be a 1 argument template type
 * \details An example:
 * \code
 *
 *  #include <bolt/cl/transform.h>
 *
 *  // Create a new user-defined type UDD on host and copy to kernel
 *  // In this example, BOLT_TEMPLATE_FUNCTOR1 enables integer templatization of UDD - UDD<int>
 *  BOLT_TEMPLATE_FUNCTOR1( UDD, int,
 *    template<typename T>
 *    struct UDD
 *    {
 *
 *      ....
 *
 *    };
 *  );
 *
 *  BOLT_TEMPLATE_FUNCTOR1( UDDNegate, int,
 *    template<typename T>
 *    struct UDDNegate
 *    {
 *      UDD< T > operator( ) ( const UDD< T > &x, const UDD< T > &y ) const
 *      {
 *
 *         ...
 *
 *      }
 *    };
 *  );
 *
 *  BOLT_CREATE_TYPENAME( bolt::cl::device_vector< UDD< int > >::iterator );
 *  BOLT_CREATE_CLCODE( bolt::cl::device_vector< UDD< int > >::iterator,
 *                      bolt::cl::deviceVectorIteratorTemplate );
 *
 *  int main( )
 *  {
 *    ...
 *
 *    bolt::cl::device_vector< UDD< int > > Src( 10, UDD< int >( ) );
 *    bolt::cl::device_vector< UDD< int > > Dest( 10 );
 *
 *    UDDNegate<int> udd_negate;
 *    bolt::cl::transform( Src.begin( ), Src.end( ), Dest.begin( ), udd_negate );
 *
 *  }
 * \endcode
 */
#define BOLT_TEMPLATE_FUNCTOR1( CONTAINER, TYPE1, ... ) \
                    __VA_ARGS__; \
                    BOLT_CREATE_TYPENAME( CONTAINER<TYPE1> ) \
                    BOLT_CREATE_CLCODE( CONTAINER<TYPE1>, #__VA_ARGS__ )

/*!
 * Helper macro to define and specialize a template type for Bolt.  The code given as the macro vararg
 * should be a template container that requires 1 template paramter.  The types passed into TYPE1 & TYPE2
 * are used to fully specialize the template container into concrete types for Bolt use with two different types
 * \param CONTAINER Name  to associate with the code given in the macro variable argument
 * \param TYPE1 Type used to fully specialize the template code given in the macro variable argument
 * \param TYPE2 Second type used to fully specialize the template code, independant of 1st type
 * \param ... Arbitrary user defined code; expected to be a 1 argument template type
 */
#define BOLT_TEMPLATE_FUNCTOR2( CONTAINER, TYPE1, TYPE2, ... ) \
                    __VA_ARGS__; \
                    BOLT_CREATE_TYPENAME( CONTAINER<TYPE1> ) \
                    BOLT_CREATE_CLCODE( CONTAINER<TYPE1>, #__VA_ARGS__ ) \
                    BOLT_TEMPLATE_REGISTER_NEW_TYPE( CONTAINER, TYPE1, TYPE2 )

/*!
 * Helper macro to define and specialize a template type for Bolt.  The code given as the macro vararg
 * should be a template container that requires 1 template paramter.  The types passed into TYPE1 & TYPE2 & TYPE3
 * are used to fully specialize the template container into concrete types for Bolt use with two different types
 * \param CONTAINER Name  to associate with the code given in the macro variable argument
 * \param TYPE1 Type used to fully specialize the template code given in the macro variable argument
 * \param TYPE2 Second type used to fully specialize the template code , independant of other types
 * \param TYPE3 Third type used to fully specialize the template code, independant of other types
 * \param ... Arbitrary user defined code; expected to be a 1 argument template type
 */
#define BOLT_TEMPLATE_FUNCTOR3( CONTAINER, TYPE1, TYPE2, TYPE3, ... ) \
                    __VA_ARGS__; \
                    BOLT_CREATE_TYPENAME( CONTAINER<TYPE1> ) \
                    BOLT_CREATE_CLCODE( CONTAINER<TYPE1>, #__VA_ARGS__ ) \
                    BOLT_TEMPLATE_REGISTER_NEW_TYPE( CONTAINER, TYPE1, TYPE2 ) \
                    BOLT_TEMPLATE_REGISTER_NEW_TYPE( CONTAINER, TYPE1, TYPE3 )

/*!
 * Helper macro to define and specialize a template type for Bolt.  The code given as the macro vararg
 * should be a template container that requires 1 template paramter.  The types passed into TYPE1 & TYPE2 & TYPE3 & type4
 * are used to fully specialize the template container into concrete types for Bolt use with two different types
 * \param CONTAINER Name  to associate with the code given in the macro variable argument
 * \param TYPE1 Type used to fully specialize the template code given in the macro variable argument
 * \param TYPE2 Second type used to fully specialize the template code , independant of other types
 * \param TYPE3 Third type used to fully specialize the template code, independant of other types
 * \param TYPE4 Fourth type used to fully specialize the template code, independant of other types
 * \param ... Arbitrary user defined code; expected to be a 1 argument template type
 */
#define BOLT_TEMPLATE_FUNCTOR4( CONTAINER, TYPE1, TYPE2, TYPE3, TYPE4, ... ) \
                    __VA_ARGS__; \
                    BOLT_CREATE_TYPENAME( CONTAINER<TYPE1> ) \
                    BOLT_CREATE_CLCODE( CONTAINER<TYPE1>, #__VA_ARGS__ ) \
                    BOLT_TEMPLATE_REGISTER_NEW_TYPE( CONTAINER, TYPE1, TYPE2 ) \
                    BOLT_TEMPLATE_REGISTER_NEW_TYPE( CONTAINER, TYPE1, TYPE3 ) \
                    BOLT_TEMPLATE_REGISTER_NEW_TYPE( CONTAINER, TYPE1, TYPE4 )

/*!
 * Registers code used on both host and device.  It is used to help define complex
 * relationships between user defined types, such as when one type contains instances of another.
 * BOLT_CREATE_CODE_SNIPPET defines new types and BOLT_ADD_DEPENDENCY is used to specify the
 * relationship to Bolt how the definition of one type relies on the definition of the other.
 * \param Name An identifier to associate with the trailing code
 * \param ... Arbitrary user defined code
 */
#define BOLT_CREATE_CODE_SNIPPET( Name, ... ) \
    __VA_ARGS__;\
    struct Name {};\
    BOLT_CREATE_CLCODE(Name,  std::string("#ifndef ") + std::string(#Name) +std::string(" \n") \
                            + std::string("#define ") + std::string(#Name) + std::string(" \n") \
                            + std::string(#__VA_ARGS__) + std::string(" \n") \
                            + std::string("#endif \n"));

/*!
 * This macro is used to specify an A-depends-on-B type relationship between types.  Bolt will ensure that
 * whenever the definition of the A type is needed, the definition for the B type is also included
 * \param Type The A type
 * \param DependingType The B type
 * \details An example:
 * \code
 *
 *  #define EPS CL_FLT_EPSILON
 *
 *  // Copies macro the to kernel code.
 *  BOLT_CREATE_DEFINE( BOLT_EPSILON, EPS, CL_FLT_EPSILON );
 *
 *  // A simple functor to multiply by e
 *  BOLT_FUNCTOR( MultiplyE,
 *    struct MultiplyE
 *    {
 *      double operator( )( double x )
 *      {
 *          return x * EPS;
 *      };
 *    };
 *  );
 *
 *  int main( )
 *  {
 *
 *    bolt::cl::counting_iterator< double > Ci( 1 );
 *    bolt::cl::device_vector< double > Dest( 10 );
 *
 *    BOLT_ADD_DEPENDENCY( MultiplyE, BOLT_EPSILON );
 *
 *    bolt::cl::transform( Ci, Ci + 10 , Dest.begin( ), MultiplyE( ) );
 *
 *    return 0;
 *  }
 *  
 * \endcode
 */
#define BOLT_ADD_DEPENDENCY( Type, DependingType ) ClCode<Type>::addDependency(ClCode<DependingType>::get());

#endif

