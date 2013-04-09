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
template< typename TypeNameType >
struct TypeName
{
    static std::string get()
    {
        static_assert( false, "Bolt< error >: Unknown typename; define missing TypeName with Bolt provided macro's" );
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
#define BOLT_CREATE_TYPENAME( Type ) \
    template<> struct TypeName< Type > { static std::string get( ) { return #Type; } };

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
#define BOLT_CREATE_CLCODE(Type,CODE_STRING) \
    template<> struct ClCode< Type > {	static std::vector< std::string > dependencies;\
                                        static void addDependency(std::string s) { dependencies.push_back(s); }; \
                                        static std::string getDependingCodeString() { \
                                            std::string c;\
                                            for (std::vector<std::string>::iterator i = dependencies.begin(); i != dependencies.end(); i++) { c = c + *i; } \
                                            return c; \
                                        };\
                                        static std::string getCodeString() { return CODE_STRING; }; \
                                        static std::string get() { return getDependingCodeString() + getCodeString(); }; };\
__declspec( selectany ) std::vector<std::string> ClCode< Type >::dependencies; 

/*!
 * \brief This macro specializes a template with a new type using the template definition of a previously defined
 * type
 * \detail This is a convenience macro to specialize a template for a new type, using the generic template 
 * definition from a previosly defined type
 * \param CONTAINER A template template parameter, such as std::vector without the type specified
 * \param OLDTYPE A type that has already been registered with the container template definition
 * \param NEWTYPE A new type to register with the template definition
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
 */
#define BOLT_TEMPLATE_REGISTER_NEW_ITERATOR( CONTAINER, OLDTYPE, NEWTYPE ) \
            BOLT_CREATE_TYPENAME( CONTAINER< NEWTYPE >::iterator ) \
            BOLT_CREATE_CLCODE( CONTAINER< NEWTYPE >::iterator, ClCode< CONTAINER< OLDTYPE >::iterator >::get( ) )

#define BOLT_CREATE_DEFINE( DefineName,D,... ) struct DefineName {}; BOLT_CREATE_CLCODE( DefineName,   std::string("#ifndef ") + std::string(#D) + std::string(" \n")\
                                                                                                  + std::string("#define ") + std::string(#D) + std::string(" ") + std::string(#__VA_ARGS__) + std::string(" \n")\
                                                                                                  + std::string("#endif \n"));

/*!
 * Creates a string and a regular version of the functor F, and automatically defines the ClCode trait to associate 
 * the code string with the specified class T. 
 * \param Type A fully specified type name
 * \param ... Arbitrary type definition to associate with the type.  See \ref ClCodeTraits
 */
#define BOLT_FUNCTOR( T, ... ) __VA_ARGS__; \
                               BOLT_CREATE_TYPENAME( T ); \
                               BOLT_CREATE_CLCODE( T,     std::string("#ifndef ") + std::string("BOLT_FUNCTOR_") + std::string(#T) + std::string(" \n")\
                                                        + std::string("#define ") + std::string("BOLT_FUNCTOR_") + std::string(#T) + std::string(" \n")\
                                                        + std::string(#__VA_ARGS__) + std::string(" \n")\
                                                        + std::string("#endif \n"));
                               

#define BOLT_TEMPLATE_FUNCTOR1( CONTAINER, TYPE1, ... ) \
                    __VA_ARGS__; \
                    BOLT_CREATE_TYPENAME( CONTAINER<TYPE1> ) \
                    BOLT_CREATE_CLCODE( CONTAINER<TYPE1>, #__VA_ARGS__ )

#define BOLT_TEMPLATE_FUNCTOR2( CONTAINER, TYPE1, TYPE2, ... ) \
                    __VA_ARGS__; \
                    BOLT_CREATE_TYPENAME( CONTAINER<TYPE1> ) \
                    BOLT_CREATE_CLCODE( CONTAINER<TYPE1>, #__VA_ARGS__ ) \
                    BOLT_TEMPLATE_REGISTER_NEW_TYPE( CONTAINER, TYPE1, TYPE2 )

#define BOLT_TEMPLATE_FUNCTOR3( CONTAINER, TYPE1, TYPE2, TYPE3, ... ) \
                    __VA_ARGS__; \
                    BOLT_CREATE_TYPENAME( CONTAINER<TYPE1> ) \
                    BOLT_CREATE_CLCODE( CONTAINER<TYPE1>, #__VA_ARGS__ ) \
                    BOLT_TEMPLATE_REGISTER_NEW_TYPE( CONTAINER, TYPE1, TYPE2 ) \
                    BOLT_TEMPLATE_REGISTER_NEW_TYPE( CONTAINER, TYPE1, TYPE3 )

#define BOLT_TEMPLATE_FUNCTOR4( CONTAINER, TYPE1, TYPE2, TYPE3, TYPE4, ... ) \
                    __VA_ARGS__; \
                    BOLT_CREATE_TYPENAME( CONTAINER<TYPE1> ) \
                    BOLT_CREATE_CLCODE( CONTAINER<TYPE1>, #__VA_ARGS__ ) \
                    BOLT_TEMPLATE_REGISTER_NEW_TYPE( CONTAINER, TYPE1, TYPE2 ) \
                    BOLT_TEMPLATE_REGISTER_NEW_TYPE( CONTAINER, TYPE1, TYPE3 ) \
                    BOLT_TEMPLATE_REGISTER_NEW_TYPE( CONTAINER, TYPE1, TYPE4 )

 #define BOLT_CREATE_CODE_SNIPPET(Name,...) \
    __VA_ARGS__;\
    struct Name {};\
    BOLT_CREATE_CLCODE(Name,  std::string("#ifndef ") + std::string(#Name) +std::string(" \n") \
                            + std::string("#define ") + std::string(#Name) + std::string(" \n") \
                            + std::string(#__VA_ARGS__) + std::string(" \n") \
                            + std::string("#endif \n"));

#define BOLT_ADD_DEPENDENCY(Type,DependingType) ClCode<Type>::addDependency(ClCode<DependingType>::get());

#endif
