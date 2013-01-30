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

#pragma once
#if !defined( CLCODE_H )
#define CLCODE_H

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

/*! 
 * Return a string with the specified function F, and also create code that is fed to the host compiler.
 */
#define BOLT_CODE_STRING( ... )  #__VA_ARGS__; __VA_ARGS__;

/*! \brief Macro that wraps around arbitrary text, and creates a string out of it
*   \detailed This macro is helpful to write inline OpenCL programs, as it avoids wrapping every line of OpenCL 
*   line with "XXX"\n
*   \return The contents of the macro wrapped in an ASCII string
*/
#define STRINGIFY_CODE2( ... ) #__VA_ARGS__
#define STRINGIFY_CODE( ... ) STRINGIFY_CODE2( __VA_ARGS__ )

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
 * \brief Experimental code, the definition of a type trait to return a string representing fully specialized 
 * templated types

 * Defining a new type trait for templated types.  This is meant to be specialized with the specific templated type 
 * through BOLT_CREATE_TEMPLATE_TYPENAME.  template_TypeName returns a string that represents a fully specialized 
 * templated type

 * \tparam Container A template template parameter, which takes a container like std::vector
 * \tparam TemplateTypeValue The type that specializes the container, such as int
 * \todo If this does not pan out, remove
 */
template< template< class > class Container, class TemplateTypeValue >
struct template_TypeName
{
    /*!
     * \brief This method asserts if it is called.  The get() method should be specialized with the macros in this 
     * file to return a fully specified templated type
     */
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
        // static_assert( false, "Bolt< error >:
        // No code is associated with this type.
        // Use BOLT_CREATE_CLCODE or BOLT_FUNCTOR" );
    }
};

/*!
 * \brief Experimental code, the definition of a type trait to return a string representing the definition of a type

 * Defining a new type trait for templated types.  This is meant to be specialized with BOLT_CREATE_TEMPLATE_CLCODE.
 * It returns the definition of the templated code.  It should not be fully specialized, the template definition.

 * \tparam Container A template template parameter, which takes a container like std::vector
 * \tparam TemplateTypeValue The type that specializes the container, such as int
 * \todo If this does not pan out, remove
 */
template< template< class > class Container, class TemplateTypeValue >
struct template_clCode
{
    static std::string get( )
    {
        return "";
        //static_assert( false, "Bolt< error >: No code is associated with this type.  Use BOLT_CREATE_CLCODE or BOLT_FUNCTOR" );
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
    template<> struct TypeName< Type > { static std::string get() { return #Type; }};

/*!
 * A convenience macro to create the the fully specialized template type trait 
 *
 * \param container A template template parameter, which takes a container like std::vector
 * \param value_type The type that specializes the container, such as int
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

#define BOLT_CREATE_TEMPLATE_TYPENAME( container, value_type ) \
    template<> struct template_TypeName< container, value_type > \
    { \
        static std::string get() \
        { \
            return #container "<" #value_type ">"; \
        } \
    }; 

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
#define BOLT_CREATE_CLCODE( Type, CODE_STRING ) \
    template<> struct ClCode< Type > { static std::string get() { return CODE_STRING; }};

/*!
 * \brief Experimental code, this macro specializes the template_clCode type trait to handle a new templated type.
 * This macro associates arbitrary code with a fully specified template type
 * \param Container A template template parameter, which takes a container like std::vector
 * \param value_type The type that specializes the container, such as int
 * \param CODE_STRING The arbitrarily complex definition of the type, used for register a template type
 * \todo If this does not pan out, remove
 */
 #define BOLT_CREATE_TEMPLATE_CLCODE( container, value_type, CODE_STRING) \
    template<> struct template_clCode< container, value_type > { static std::string get() { return CODE_STRING; }};

/*!
 * \brief This macro specializes the ClCode type trait to handle a new fully specialized templated type
 * This macro associates a type definition with a fully specified template type.  The type definition
 * is templated code
 * \param CONTAINER A template template parameter, such as std::vector
 * \param OLDTYPE A type that has already been registered with the container template definition
 * \param NEWTYPE A new type to register with the template definition
 */
#define BOLT_TEMPLATE_REGISTER_NEW_TYPE( CONTAINER, OLDTYPE, NEWTYPE ) \
            BOLT_CREATE_TYPENAME( CONTAINER<NEWTYPE> ) \
            BOLT_CREATE_CLCODE( CONTAINER<NEWTYPE>, ClCode< CONTAINER<OLDTYPE> >::get( ) )

/*!
 * Creates a string and a regular version of the functor F, and automatically defines the ClCode trait to associate 
 * the code string with the specified class T. 
 * \param Type A fully specified type name
 * \param ... Arbitrary type definition to associate with the type.  See \ref ClCodeTraits
 */
#define BOLT_FUNCTOR( TYPE, ... ) \
                    __VA_ARGS__; \
                    BOLT_CREATE_TYPENAME( TYPE ); \
                    BOLT_CREATE_CLCODE( TYPE, #__VA_ARGS__ "\n" );

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

#endif