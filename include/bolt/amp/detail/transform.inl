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

///////////////////////////////////////////////////////////////////////////////
// AMP Transform
//////////////////////////////////////////////////////////////////////////////

#if !defined( AMP_TRANSFORM_INL )
#define AMP_TRANSFORM_INL
#define WAVEFRONT_SIZE 64

#ifdef BOLT_ENABLE_PROFILING
#include "bolt/AsyncProfiler.h"
//AsyncProfiler aProfiler("transform");
#endif

#include <algorithm>
#include <type_traits>
#include "bolt/amp/bolt.h"
#include "bolt/amp/device_vector.h"

namespace bolt
{
    namespace amp
    {
        //////////////////////////////////////////
        //  Transform overloads
        //////////////////////////////////////////
        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename OutputIterator, typename BinaryFunction> 
        void transform( bolt::amp::control& ctl,
                       InputIterator first1,
                       InputIterator last1,
                       InputIterator first2,
                       OutputIterator result, 
                       BinaryFunction f )
        {
            detail::transform_detect_random_access( ctl,
                                                   first1,
                                                   last1,
                                                   first2,
                                                   result,
                                                   f,                                                    
                                                   std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename OutputIterator, typename BinaryFunction> 
        void transform( InputIterator first1,
                        InputIterator last1,
                        InputIterator first2,
                        OutputIterator result, 
                        BinaryFunction f )
        {
            detail::transform_detect_random_access( control::getDefault(),
                                                    first1,
                                                    last1,
                                                    first2,
                                                    result,
                                                    f,                                                     
                                                    std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename OutputIterator, typename UnaryFunction>
        void transform( bolt::amp::control& ctl,
                        InputIterator first1,
                        InputIterator last1,
                        OutputIterator result, 
                        UnaryFunction f )
        {
            detail::transform_unary_detect_random_access( ctl,
                                                          first1,
                                                          last1,
                                                          result,
                                                          f,                                                           
                                                          std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename OutputIterator, typename UnaryFunction> 
        void transform( InputIterator first1,
                        InputIterator last1,
                        OutputIterator result, 
                        UnaryFunction f )
        {
            detail::transform_unary_detect_random_access( control::getDefault(),
                                                          first1,
                                                          last1,
                                                          result,
                                                          f,                                                           
                                                          std::iterator_traits< InputIterator >::iterator_category( ) );
        };

    };//end of namespace amp
};//end of namespace bolt
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace bolt 
{    
    namespace amp 
    {
        namespace detail 
        {
            // Wrapper that uses default control class, iterator interface
            template<typename InputIterator, typename OutputIterator, typename BinaryFunction> 
            void transform_detect_random_access( bolt::amp::control& ctl,
                                                 const InputIterator& first1,
                                                 const InputIterator& last1,
                                                 const InputIterator& first2, 
                                                 const OutputIterator& result,
                                                 const BinaryFunction& f,
                                                 std::input_iterator_tag )
            {
                //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data 
                //  to a temporary buffer.  Should we?
                static_assert( false, "Bolt only supports random access iterator types" );
            }

            template<typename InputIterator, typename OutputIterator, typename BinaryFunction> 
            void transform_detect_random_access( bolt::amp::control& ctl,
                                                 const InputIterator& first1,
                                                 const InputIterator& last1,
                                                 const InputIterator& first2, 
                                                 const OutputIterator& result,
                                                 const BinaryFunction& f,
                                                 std::random_access_iterator_tag )
            {
                transform_pick_iterator( ctl, first1, last1, first2, result, f  );
            }

            // Wrapper that uses default control class, iterator interface
            template<typename InputIterator, typename OutputIterator, typename UnaryFunction> 
            void transform_unary_detect_random_access( bolt::amp::control& ctl,
                                                       const InputIterator& first1,
                                                       const InputIterator& last1, 
                                                       const OutputIterator& result,
                                                       const UnaryFunction& f,
                                                       std::input_iterator_tag )
            {
                //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data 
                //  to a temporary buffer.  Should we?
                static_assert( false, "Bolt only supports random access iterator types" );
            }

            template<typename InputIterator, typename OutputIterator, typename UnaryFunction> 
            void transform_unary_detect_random_access( bolt::amp::control& ctl,
                                                       const InputIterator& first1,
                                                       const InputIterator& last1, 
                                                       const OutputIterator& result,
                                                       const UnaryFunction& f,
                                                       std::random_access_iterator_tag )
            {
                transform_unary_pick_iterator( ctl, first1, last1, result, f  );
            }


            /*! \brief This template function overload is used to seperate device_vector iterators from all other iterators
                \detail This template is called by the non-detail versions of transform, it already assumes random access
             *  iterators.  This overload is called strictly for non-device_vector iterators
            */
            template<typename InputIterator, typename OutputIterator, typename BinaryFunction> 
            typename std::enable_if< 
                         !(std::is_base_of<typename device_vector<typename std::iterator_traits<InputIterator>::value_type>::iterator,InputIterator>::value &&
                           std::is_base_of<typename device_vector<typename std::iterator_traits<OutputIterator>::value_type>::iterator,OutputIterator>::value),
                     void >::type
            transform_pick_iterator( bolt::amp::control &ctl,
                                     const InputIterator& first1,
                                     const InputIterator& last1,
                                     const InputIterator& first2, 
                                     const OutputIterator& result,
                                     const BinaryFunction& f)
            {
                typedef std::iterator_traits<InputIterator>::value_type iType;
                typedef std::iterator_traits<OutputIterator>::value_type oType;
                size_t sz = (last1 - first1); 
                if (sz == 0)
                    return;

                // Use host pointers memory since these arrays are only read once - no benefit to copying.

                // Map the input iterator to a device_vector
                //device_vector< iType > dvInput( first1, last1, ctl );
                device_vector< iType, concurrency::array_view > dvInput( first1, last1, false, ctl );

                //device_vector< iType > dvInput2( first2, sz, true, ctl );
                device_vector< iType, concurrency::array_view > dvInput2( first2, sz, false, ctl );

                // Map the output iterator to a device_vector
                //device_vector< oType > dvOutput( result, sz, false, ctl );
                device_vector< oType, concurrency::array_view > dvOutput( result, sz, true, ctl );

                transform_enqueue( ctl, dvInput.begin( ), dvInput.end( ), dvInput2.begin( ), dvOutput.begin( ), f  );

                // This should immediately map/unmap the buffer
                dvOutput.data( );
            }

            // This template is called by the non-detail versions of transform, it already assumes random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator
            template<typename DVInputIterator, typename DVOutputIterator, typename BinaryFunction> 
            typename std::enable_if< 
                          (std::is_base_of<typename device_vector<typename std::iterator_traits<DVInputIterator>::value_type>::iterator,DVInputIterator>::value &&
                           std::is_base_of<typename device_vector<typename std::iterator_traits<DVOutputIterator>::value_type>::iterator,DVOutputIterator>::value),
                     void >::type
            transform_pick_iterator( bolt::amp::control &ctl,
                                     const DVInputIterator& first1,
                                     const DVInputIterator& last1,
                                     const DVInputIterator& first2, 
                                     const DVOutputIterator& result,
                                     const BinaryFunction& f )
            {
                transform_enqueue( ctl, first1, last1, first2, result, f  );
            }

            /*! \brief This template function overload is used to seperate device_vector iterators from all other iterators
                \detail This template is called by the non-detail versions of transform, it already assumes random access
             *  iterators.  This overload is called strictly for non-device_vector iterators
            */
            template<typename InputIterator, typename OutputIterator, typename UnaryFunction> 
            typename std::enable_if< 
                         !(std::is_base_of<typename device_vector<typename std::iterator_traits<InputIterator>::value_type>::iterator,InputIterator>::value &&
                           std::is_base_of<typename device_vector<typename std::iterator_traits<OutputIterator>::value_type>::iterator,OutputIterator>::value),
                     void >::type
            transform_unary_pick_iterator( bolt::amp::control &ctl,
                                           const InputIterator& first,
                                           const InputIterator& last, 
                                           const OutputIterator& result,
                                           const UnaryFunction& f)
            {
                typedef std::iterator_traits<InputIterator>::value_type iType;
                typedef std::iterator_traits<OutputIterator>::value_type oType;
                size_t sz = (last - first); 
                if (sz == 0)
                    return;

                // Use host pointers memory since these arrays are only read once - no benefit to copying.

                // Map the input iterator to a device_vector
                //device_vector< iType > dvInput( first, last, ctl );
                device_vector< iType, concurrency::array_view > dvInput( first, last, false, ctl );

                // Map the output iterator to a device_vector
                //device_vector< oType > dvOutput( result, sz, false, ctl );
                device_vector< oType, concurrency::array_view > dvOutput( result, sz, true, ctl );

                transform_unary_enqueue( ctl, dvInput.begin( ), dvInput.end( ), dvOutput.begin( ), f );

                // This should immediately map/unmap the buffer
                dvOutput.data( );
            }

            // This template is called by the non-detail versions of transform, it already assumes random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator
            template<typename DVInputIterator, typename DVOutputIterator, typename UnaryFunction> 
            typename std::enable_if< 
                          (std::is_base_of<typename device_vector<typename std::iterator_traits<DVInputIterator>::value_type>::iterator,DVInputIterator>::value &&
                           std::is_base_of<typename device_vector<typename std::iterator_traits<DVOutputIterator>::value_type>::iterator,DVOutputIterator>::value),
                     void >::type
            transform_unary_pick_iterator( bolt::amp::control &ctl,
                                           const DVInputIterator& first,
                                           const DVInputIterator& last, 
                                           const DVOutputIterator& result,
                                           const UnaryFunction& f )
            {
                transform_unary_enqueue( ctl, first, last, result, f  );
            };

            template< typename DVInputIterator, typename DVOutputIterator, typename BinaryFunction > 
            void transform_enqueue( bolt::amp::control &ctl,
                                    const DVInputIterator& first1,
                                    const DVInputIterator& last1,
                                    const DVInputIterator& first2, 
                                    const DVOutputIterator& result,
                                    const BinaryFunction& f)
            {
               typedef std::iterator_traits< DVInputIterator >::value_type iType;
               typedef std::iterator_traits< DVOutputIterator >::value_type oType;

               const unsigned int arraySize =  static_cast< unsigned int >( std::distance( first1, last1 ) );
               unsigned int wavefrontMultiple = arraySize;
               const unsigned int lowerBits = ( arraySize & ( WAVEFRONT_SIZE -1 ) );

               if( lowerBits )
               {
                   wavefrontMultiple &= ~lowerBits;
                   wavefrontMultiple += WAVEFRONT_SIZE;
               }

               concurrency::array_view<iType,1> inputV1 (first1->getBuffer());
               concurrency::array_view<iType,1> inputV2 (first2->getBuffer());
               concurrency::array_view<oType,1> resultV(result->getBuffer());
               concurrency::extent< 1 > inputExtent( wavefrontMultiple );

               concurrency::parallel_for_each(ctl.getAccelerator().default_view, inputExtent, [=](concurrency::index<1> idx) mutable restrict(amp)
               {
                   unsigned int globalId = idx[0];
                   if( globalId >= wavefrontMultiple )  
                       return;
                   resultV[idx[0]] = f(inputV1[globalId], inputV2[globalId]);
               });

            };

            template< typename DVInputIterator, typename DVOutputIterator, typename UnaryFunction > 
            void transform_unary_enqueue(bolt::amp::control &ctl,
                                         const DVInputIterator& first,
                                         const DVInputIterator& last, 
                                         const DVOutputIterator& result,
                                         const UnaryFunction& f)
            {

               typedef std::iterator_traits< DVInputIterator >::value_type iType;
               typedef std::iterator_traits< DVOutputIterator >::value_type oType;

               const unsigned int arraySize =  static_cast< unsigned int >( std::distance( first, last ) );
               unsigned int wavefrontMultiple = arraySize;
               const unsigned int lowerBits = ( arraySize & ( WAVEFRONT_SIZE -1 ) );

               if( lowerBits )
               {
                   wavefrontMultiple &= ~lowerBits;
                   wavefrontMultiple += WAVEFRONT_SIZE;
               }

               concurrency::array_view<iType,1> inputV (first->getBuffer());
               concurrency::array_view<oType,1> resultV(result->getBuffer());
               concurrency::extent< 1 > inputExtent( wavefrontMultiple );

               concurrency::parallel_for_each(ctl.getAccelerator().default_view, inputExtent, [=](concurrency::index<1> idx) mutable restrict(amp)
               {
                   unsigned int globalId = idx[0];
                   if( globalId >= wavefrontMultiple )  
                       return;
                   resultV[globalId] = f(inputV[globalId]);
               });
               
            }


        };//end of namespace detail
    }; //end of namespace amp
}; //end of namespace bolt

#endif // AMP_TRANSFORM_INL