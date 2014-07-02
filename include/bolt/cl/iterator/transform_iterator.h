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

// (C) Copyright David Abrahams 2002.
// (C) Copyright Jeremy Siek    2002.
// (C) Copyright Thomas Witt    2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file BOOST_LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOLT_TRANSFORM_ITERATOR_H
#define BOLT_TRANSFORM_ITERATOR_H

#include <type_traits>

#include <bolt/cl/iterator/iterator_adaptor.h>
#include <bolt/cl/iterator/iterator_facade.h>
#include <bolt/cl/iterator/iterator_traits.h>
#include <bolt/cl/device_vector.h>


namespace bolt
{
namespace cl
{

  struct transform_iterator_tag
      : public fancy_iterator_tag
        {  };

      /*! \addtogroup fancy_iterators
       */

      /*! \addtogroup CL-TransformIterator
      *   \ingroup fancy_iterators
      *   \{
      */

      /*! transform iterator adapts an iterator by modifying the operator* to apply a function object to the 
       *                     result of dereferencing the iterator and returning the result..
       *
       *
       *
       *  \details The following example demonstrates how to use a \p transform_iterator.
       *
       *  \code
       *  #include <bolt/cl/iterator/transform_iterator.h>
       *  #include <bolt/cl/transform.h>
       *  #include <bolt/cl/functional.h>
       *  //For OpenCL, Transform Iterator macro should be created using the macro BOLT_TEMPLATE_REGISTER_NEW_TRANSFORM_ITERATOR
       *  //The example here uses a device_vector iterators.
       *
       *  BOLT_FUNCTOR(UDD, 
       *    struct UDD
       *    {
       *        int i;
       *        float f;
       *  
       *	    UDD operator = (const int rhs) 
       *        {
       *            UDD _result;
       *            _result.i = i + rhs;
       *            _result.f = f + (float)rhs;
       *            return _result;
       *        }
       *
       *	    UDD operator + (const UDD &rhs) const
       *        {
       *            UDD _result;
       *            _result.i = this->i + rhs.i;
       *            _result.f = this->f + rhs.f;
       *            return _result;
       *        }
       *        UDD()
       *            : i(0), f(0) { }
       *        UDD(int _in)
       *            : i(_in), f((float)(_in+2) ){ }
       *     };
       *   );
       *
       *
       *  BOLT_FUNCTOR(UDDadd_3,
       *      struct UDDadd_3
       *      {
       *          UDD operator() (const UDD &x) const
       *  		  { 
       *  			UDD temp;
       *  			temp.i = x.i + 3;
       *  			temp.f = x.f + 3.0f;
       *  			return temp; 
       *  		  }
       *          typedef UDD result_type;
       *          //Note that the result_type needs to be defined and should be type-defined to the  
       *          //return type of operator () overload.
       *      };
       *  );
       *  
       *  
       *  
       *  BOLT_TEMPLATE_REGISTER_NEW_TRANSFORM_ITERATOR( UDDadd_3, UDD);
       *  BOLT_TEMPLATE_REGISTER_NEW_ITERATOR( bolt::cl::device_vector, int, UDD);
       *  BOLT_TEMPLATE_REGISTER_NEW_TYPE(bolt::cl::plus, int, UDD );
       *  
       *  int main() {
       *    // Create device_vectors
       *    bolt::cl::device_vector< UDD > dvInVec1( 5 );
       *    bolt::cl::device_vector< UDD > dvInVec2( 5 );
       *    bolt::cl::device_vector< UDD > dvDestVec( 5, 0 );
       *    UDDadd_3 add3;
       *
       *    typedef bolt::BCKND::transform_iterator< UDDadd_3, bolt::BCKND::device_vector< UDD >::iterator> dv_trf_itr_add3;
       *    dv_trf_itr_add3 dv_trf_begin (dvInVec1.begin(), add3), dv_trf_end (dvInVec1.end(), add3);
       *    // Fill values
       *    dvInVec1[ 0 ] = 10 ; dvInVec1[ 1 ] = 15 ; dvInVec1[ 2 ] = 20 ;
       *    dvInVec1[ 3 ] = 25 ; dvInVec1[ 4 ] = 30 ;
       *    dvInVec2[ 0 ] = 10 ; dvInVec2[ 1 ] = 15 ; dvInVec2[ 2 ] = 20 ;
       *    dvInVec2[ 3 ] = 25 ; dvInVec2[ 4 ] = 30 ;       
       *
       *    ...
       *    bolt::cl::transform(dv_trf_begin,
       *                        dv_trf_end,
       *                        dvInVec2.begin( ),
       *                        dvDestVec.begin( ),
       *                        bolt::cl::plus< int >( ) );
       *
       *  }
       *  \endcode
       */
  template <class UnaryFunction, class Iterator, class Reference = use_default, class Value = use_default>
  class transform_iterator;

  namespace detail
  {
    // Compute the iterator_adaptor instantiation to be used for transform_iterator
    template <class UnaryFunc, class Iterator, class Reference, class Value>
    struct transform_iterator_base
    {
     private:
        // By default, dereferencing the iterator yields the same as
        // the function.
        typedef typename bolt::cl::detail::ia_dflt_help <
            Reference
          , std::result_of< const UnaryFunc(typename std::iterator_traits<Iterator>::reference) >
        >::type reference;

        // To get the default for Value: remove any reference on the
        // result type, but retain any constness to signal
        // non-writability.  Note that if we adopt Thomas' suggestion
        // to key non-writability *only* on the Reference argument,
        // we'd need to strip constness here as well.
        typedef typename bolt::cl::detail::ia_dflt_help<
            Value
          , std::remove_reference<reference>
        >::type cv_value_type;

     public:
        typedef iterator_adaptor<
            transform_iterator<UnaryFunc, Iterator, Reference, Value>
          , Iterator
          , cv_value_type
          , use_default    // Leave the traversal category alone
          , reference
          , std::ptrdiff_t
        > type;
    };
  }// namespace detail

  template <class UnaryFunc, class Iterator, class Reference, class Value>
  class transform_iterator
    : public bolt::cl::detail::transform_iterator_base<UnaryFunc, Iterator, Reference, Value>::type
  {
    typedef typename
    bolt::cl::detail::transform_iterator_base<UnaryFunc, Iterator, Reference, Value>::type
    super_t;

    friend class iterator_core_access;

  public:
    typedef transform_iterator_tag                                   iterator_category;
    typedef typename bolt::cl::iterator_category<Iterator>::type     memory_system;
    typedef super_t                                                  iterator_base_class;
    //
    typedef UnaryFunc                                                unary_func;
    typedef typename std::iterator_traits<Iterator>::value_type      value_type;
    typedef std::ptrdiff_t                                           difference_type;
    typedef typename std::iterator_traits<Iterator>::pointer         pointer;
    typedef transform_iterator<unary_func, typename bolt::cl::device_vector<value_type>::iterator>  device_transform_iterator;
    transform_iterator() { }

    transform_iterator(Iterator const& x, UnaryFunc f)
      : super_t(x), m_f(f) { }

    explicit transform_iterator(Iterator const& x)
      : super_t(x)
    {
    }

    template <
        class OtherUnaryFunction
      , class OtherIterator
      , class OtherReference
      , class OtherValue>
    transform_iterator( transform_iterator<OtherUnaryFunction, OtherIterator, OtherReference, OtherValue> const& t
                      , typename bolt::cl::enable_if_convertible<OtherIterator, Iterator>::type* = 0
                      , typename bolt::cl::enable_if_convertible<OtherUnaryFunction, UnaryFunc>::type* = 0 )
      : super_t(t.base()), m_f(t.functor())
   { }


        value_type* getPointer()
        {
            Iterator base_iterator = this->base_reference();
            return &(*base_iterator);
        }

        const value_type* getPointer() const
        {
            Iterator base_iterator = this->base_reference();
            return &(*base_iterator);
        }

        UnaryFunc functor() const
        { return m_f; }

        struct Payload
        {
            int m_Index;
            int m_Ptr1[ 3 ];  // Represents device pointer, big enough for 32 or 64bit
            UnaryFunc       m_f;
        };

        /*TODO - RAVI Probably I can acheive this using friend class device_vector. But the problem would be
                 multiple defintions of functions like advance()*/
        template<typename Container >
        Container& getContainer( ) const
        {
            return this->base().getContainer( );
        }

        const Payload  gpuPayload( ) const
        {
            Payload payload = { 0/*m_Index*/, { 0, 0, 0 } };
            return payload;
        }

        /*TODO - This should throw a compilation error if the Iterator is of type std::vector*/
        const difference_type gpuPayloadSize( ) const
        {
            cl_int l_Error = CL_SUCCESS;
            //::cl::Device which_device;
            //l_Error  = m_it.getContainer().m_commQueue.getInfo(CL_QUEUE_DEVICE,&which_device );
            //TODO - fix the device bits
            cl_uint deviceBits = 32;// = which_device.getInfo< CL_DEVICE_ADDRESS_BITS >( );
            //  Size of index and pointer
            cl_uint szUF = sizeof(UnaryFunc);
            szUF = (szUF+3) &(~3);
            difference_type payloadSize = sizeof( int ) + ( deviceBits >> 3 ) + szUF;

            //  64bit devices need to add padding for 8 byte aligned pointer
            if( deviceBits == 64 )
                payloadSize += 4;

            return payloadSize;
        }

        int setKernelBuffers(int arg_num, ::cl::Kernel &kernel) const
        {
            /*Next set the Argument Iterator*/
            arg_num = this->base().setKernelBuffers(arg_num, kernel);
            return arg_num;
        }
  private:
    typename super_t::reference dereference() const
    { return m_f(*this->base()); }

    UnaryFunc m_f;
  };

  template <class UnaryFunc, class Iterator>
  transform_iterator<UnaryFunc, Iterator>
  make_transform_iterator(Iterator it, UnaryFunc fun)
  {
      return transform_iterator<UnaryFunc, Iterator>(it, fun);
  }

  // Version which allows explicit specification of the UnaryFunc
  // type.
  //
  // This generator is not provided if UnaryFunc is a function
  // pointer type, because it's too dangerous: the default-constructed
  // function pointer in the iterator be 0, leading to a runtime
  // crash.
  template <class UnaryFunc, class Iterator>
  typename std::enable_if<
      std::is_class<UnaryFunc>::value   // We should probably find a cheaper test than is_class<>
    , transform_iterator<UnaryFunc, Iterator>
  >::type
  make_transform_iterator(Iterator it)
  {
      return transform_iterator<UnaryFunc, Iterator>(it, UnaryFunc());
  }

   //  This string represents the device side definition of the Transform Iterator template
    static std::string deviceTransformIteratorTemplate =
        bolt::cl::deviceVectorIteratorTemplate +
        std::string("#if !defined(BOLT_CL_TRANSFORM_ITERATOR) \n#define BOLT_CL_TRANSFORM_ITERATOR \n") +
        STRINGIFY_CODE(
            namespace bolt { namespace cl { \n
            template< typename UnaryFunc, typename Iterator > \n
            class transform_iterator \n
            { \n
                public:    \n
                    typedef int iterator_category;        \n
                    typedef typename UnaryFunc::result_type value_type; \n
                    typedef typename Iterator::value_type base_type; \n
                    typedef int size_type; \n

                    transform_iterator( size_type init ): m_StartIndex( init ), m_Ptr( 0 ) \n
                    {} \n

                    void init( global base_type* ptr )\n
                    { \n
                        m_Ptr = ptr; \n
                    } \n

                    value_type operator[]( size_type threadID ) const \n
                    { \n
                        base_type tmp = m_Ptr[ m_StartIndex + threadID ]; \n
                        return m_f(tmp);\n
                    } \n

                    value_type operator*( ) const \n
                    { \n
                        base_type tmp = m_Ptr[ m_StartIndex + threadID ]; \n
                        return m_f( tmp ); \n
                    } \n

                    size_type m_StartIndex; \n
                    global base_type* m_Ptr; \n
                    UnaryFunc          m_f; \n
            }; \n
            } } \n
        )
        +  std::string("#endif \n");

} // namespace cl
} // namespace bolt

#endif // BOLT_TRANSFORM_ITERATOR_H
