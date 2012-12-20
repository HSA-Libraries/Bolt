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

/*! \addtogroup miscellaneous
 */

/*! \addtogroup ClCode ClCode: Traits and Helper Macros
 *   \ingroup miscellaneous
 *   \{
 */

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
template <typename T>
struct TypeName
{
    static std::string get()
    {
        static_assert( false, "Bolt< error >: Unknown typename; define missing TypeName with Bolt provided macro's" );
    }
};

//---
template <typename T>
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
#define BOLT_CREATE_TYPENAME(T) \
    template<> struct TypeName<T> { static std::string get() { return #T; }};

/*!
 * Creates the ClCode trait that associates the specified type \p T with the string \p CODE_STRING.
 * \param T : Class. 
 * \param CODE_STRING : Code string to associate with T's ClCode trait.
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
#define BOLT_CREATE_CLCODE(T,CODE_STRING) \
    template<> struct ClCode<T> { static std::string get() { return CODE_STRING; }};

/*!
 * Creates a string and a regular version of the functor F, and automatically defines the ClCode trait to associate 
 * the code string with the specified class T. 
 * \param T : Class.
 * \param FUNCTION : Function definition.  See \ref ClCodeTraits
 */
#define BOLT_FUNCTOR( T, ... ) __VA_ARGS__; BOLT_CREATE_TYPENAME( T ); BOLT_CREATE_CLCODE( T, #__VA_ARGS__ );

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

#endif