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
#if !defined( BOLT_DEVICE_VECTOR_H )
#define BOLT_DEVICE_VECTOR_H

#include <iterator>
#include <type_traits>
#include <numeric>
#include <amp.h>
#include <exception> // For exception class
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/reverse_iterator.hpp>
#include <boost/shared_array.hpp>

/*! \file device_vector.h
 * Header file for the device_container class
 */

/*! \brief Defining namespace for the Bolt project
    */
namespace bolt
{
    /*! \brief Namespace that AMP related data types and functions
     */
    namespace amp
    {
        /*! \addtogroup Containers
         */

        /*! \addtogroup Device
        *   \ingroup Containers
        *   Containers that guarantee sequential and linear access to memory "close" to the device
        */

        /*! \brief This defines the AMP version of a device_vector
        *   \ingroup Device
        *   \details A device_vector is an abstract data type that provides random access to a flat, sequential region of memory that is performant
        *   for the device.  This can imply different memories for different devices.  For discrete class graphics,
        *   devices, this is most likely video memory; for APU devices, this can imply zero-copy memory; for CPU devices, this can imply
        *   standard host memory.
        *   \sa http://www.sgi.com/tech/stl/Vector.html
        */
        template< typename T >
        class device_vector
        {

            /*! \brief Class used with shared_ptr<> as a custom deleter, to unmap a buffer that has been mapped with
            *   device_vector data() method
            */
            /*template< typename Container >
            class UnMapBufferFunctor
            {
                Container& m_Container;

            public:
                //  Basic constructor requires a reference to the container and a positional element
                UnMapBufferFunctor( Container& rhs ): m_Container( rhs )
                {}

                void operator( )( const void* pBuff )
                {
                    ::cl::Event unmapEvent;

                    V_OPENCL( m_Container.m_commQueue.enqueueUnmapMemObject( m_Container.m_devMemory, const_cast< void* >( pBuff ), NULL, &unmapEvent ),
                            "shared_ptr failed to unmap host memory back to device memory" );
                    V_OPENCL( unmapEvent.wait( ), "failed to wait for unmap event" );
                }
            };*/

            typedef T* naked_pointer;
            typedef const T* const_naked_pointer;

        public:

            //  Useful typedefs specific to this container
            typedef T value_type;
            typedef ptrdiff_t difference_type;
            typedef difference_type distance_type;
            typedef size_t size_type;

            typedef boost::shared_array< value_type > pointer;
            typedef boost::shared_array< const value_type > const_pointer;

            /*! \brief A writeable element of the container
            *   The location of an element of the container may not actually reside in system memory, but rather in device
            *   memory, which may be in a partitioned memory space.  Access to a reference of the container results in
            *   a mapping and unmapping operation of device memory.
            *   \note The container element reference is implemented as a proxy object.
            *   \warning Use of this class can be slow: each operation on it results in a map/unmap sequence.
            */

            template< typename Container >
            class reference_base
            {
            public:
                reference_base( Container& rhs, size_type index ): m_Container( rhs ), m_index( index )
                {
                }

                //  Automatic type conversion operator to turn the reference object into a value_type
                operator value_type( ) const
                {
                    value_type &result =  m_Container.m_devMemory[(int)m_index];
                    return result;
                }

                reference_base< Container >& operator=( const value_type& rhs )
                {
                    m_Container.m_devMemory[(int)m_index] = rhs;
                    return *this;
                }

                /*! \brief A get accessor function to return the encapsulated device buffer for const objects.
                *   This member function allows access to the Buffer object, which can be retrieved through a reference or an iterator.
                *   This is necessary to allow library functions to set the encapsulated buffer object as a kernel argument.
                *   \note This get function could be implemented in the iterator, but the reference object is usually a temporary rvalue, so
                *   this location seems less intrusive to the design of the vector class.
                */
                const Concurrency::array<T,1> getBuffer( ) const
                {
                    return m_Container.m_devMemory;
                }

                /*! \brief A get accessor function to return the encapsulated device buffer for non-const objects.
                *   This member function allows access to the Buffer object, which can be retrieved through a reference or an iterator.
                *   This is necessary to allow library functions to set the encapsulated buffer object as a kernel argument.
                *   \note This get function can be implemented in the iterator, but the reference object is usually a temporary rvalue, so
                *   this location seems less intrusive to the design of the vector class.
                */
                Concurrency::array<T,1>& getBuffer( )
                {
                    return m_Container.m_devMemory;
                }

                /*! \brief A get accessor function to return the encapsulated device_vector.
                */
                Container& getContainer( ) const
                {
                    return m_Container;
                }

                size_type getIndex() const
                {
                    return m_index;
                }

            private:
                Container& m_Container;
                size_type m_index;
            };

            /*! \brief Typedef to create the non-constant reference.
            */

            typedef reference_base< device_vector< value_type > > reference;

            /*! \brief A non-writeable copy of an element of the container.
            *   Constant references are optimized to return a value_type, since it is certain that
            *   the value will not be modified
            *   \note A const_reference actually returns a value, not a reference.
            */
            typedef const reference_base< device_vector< value_type > > const_reference;

            //  Handy for the reference class to get at the wrapped ::cl objects
            friend class reference;

            /*! \brief Base class provided to encapsulate all the common functionality for constant
            *   and non-constant iterators.
            *   \sa http://www.sgi.com/tech/stl/Iterators.html
            *   \sa http://www.sgi.com/tech/stl/RandomAccessIterator.html
            *   \bug operator[] with device_vector iterators result in a compile-time error when accessed for reading.
            *   Writing with operator[] appears to be OK.  Workarounds: either use the operator[] on the device_vector
            *   container, or use iterator arithmetic instead, such as *(iter + 5) for reading from the iterator.
            */
            template< typename Container >
            class iterator_base: public boost::iterator_facade< iterator_base< Container >, value_type, std::random_access_iterator_tag, typename device_vector::reference >
            {
            public:

                //  Basic constructor requires a reference to the container and a positional element
                iterator_base( Container& rhs, size_type index ): m_Container( rhs ), m_index( index )
                {}

                //  This copy constructor allows an iterator to convert into a const_iterator, but not vica versa
                template< typename OtherContainer >
                iterator_base( const iterator_base< OtherContainer >& rhs ): m_Container( rhs.m_Container ), m_index( rhs.m_index )
                {}

                //  This copy constructor allows an iterator to convert into a const_iterator, but not vica versa
                //template< typename Container >
                iterator_base< Container >& operator= ( const iterator_base< Container >& rhs )
                {
                    m_Container = rhs.m_Container;
                    m_index = rhs.m_index;
                    return *this;
                }

                iterator_base< Container >& operator+= ( const difference_type & n )
                {
                    advance( n );
                    return *this;
                }

                const iterator_base< Container > operator+ ( const difference_type & n ) const
                {
                    iterator_base< Container > result(*this);
                    result.advance(n);
                    return result;
                }

                int getIndex() const
                {
                    return m_index;
                }

            private:
                //  Implementation detail of boost.iterator
                friend class boost::iterator_core_access;

                //  Handy for the device_vector erase methods
                friend class device_vector< value_type >;

                //  Used for templatized copy constructor and the templatized equal operator
                template < typename > friend class iterator_base;

                void advance( difference_type n )
                {
                    m_index += n;
                }

                void increment( )
                {
                    advance( 1 );
                }

                void decrement( )
                {
                    advance( -1 );
                }

                difference_type distance_to( const iterator_base< Container >& rhs ) const
                {
                    return ( rhs.m_index - m_index );
                }

                template< typename OtherContainer >
                bool equal( const iterator_base< OtherContainer >& rhs ) const
                {
                    bool sameIndex = rhs.m_index == m_index;
                    bool sameContainer = (&m_Container == &rhs.m_Container );

                    return ( sameIndex && sameContainer );
                }

                reference dereference( ) const
                {
                    return m_Container[ m_index ];
                }

                Container& m_Container;
                size_type m_index;
            };

            /*! \brief Typedef to create the non-constant iterator
            */
            typedef iterator_base< device_vector< value_type > > iterator;

            /*! \brief Typedef to create the constant iterator
            */
            typedef iterator_base< const device_vector< value_type > > const_iterator;

            /*! \brief A reverse random access iterator in the classic sense
            *   \todo Need to implement reverse_iterators
            *   \sa http://www.sgi.com/tech/stl/ReverseIterator.html
            *   \sa http://www.sgi.com/tech/stl/RandomAccessIterator.html
            */
            class reverse_iterator: public boost::reverse_iterator< iterator >
            {
            };

            /*! \brief A constant random access iterator in the classic sense
            *   \todo Need to implement const_reverse_iterator
            *   \sa http://www.sgi.com/tech/stl/ReverseIterator.html
            *   \sa http://www.sgi.com/tech/stl/RandomAccessIterator.html
            */
            class const_reverse_iterator: public boost::reverse_iterator< const_iterator >
            {
            };

            /*! \brief A default constructor that creates an empty device_vector
            *   \param ctl An Bolt control class used to perform copy operations; a default is used if not supplied by the user
            *   \todo Find a way to be able to unambiguously specify memory flags for this constructor, that is not
            *   confused with the size constructor below.
            */

            device_vector( ): m_Size( 0 ), m_devMemory(0)
            {
                static_assert( !std::is_polymorphic< value_type >::value, "AMD C++ template extensions do not support the virtual keyword yet" );
            }

            /*! \brief A constructor that creates a new device_vector with the specified number of elements, with a specified initial value.
            *   \param newSize The number of elements of the new device_vector
            *   \param value The value with which to initialize new elements.
            *   \param flags A bitfield that takes the OpenCL memory flags to help specify where the device_vector allocates memory.
            *   \param init Boolean value to indicate whether to initialize device memory from host memory.
            *   \param ctl A Bolt control class for copy operations; a default is used if not supplied by the user.
            *   \warning The ::cl::CommandQueue is not an STD reserve( ) parameter.
            */

            device_vector( size_type newSize, const value_type& value = value_type( ),
                bool init = true ): m_Size( newSize ), m_devMemory((int)m_Size)
            {
                static_assert( !std::is_polymorphic< value_type >::value, "AMD C++ template extensions do not support the virtual keyword yet" );

                //  We want to use the context from the passed in commandqueue to initialize our buffer
                //cl_int l_Error = CL_SUCCESS;
                //::cl::Context l_Context = m_commQueue.getInfo< CL_QUEUE_CONTEXT >( &l_Error );
                //V_OPENCL( l_Error, "device_vector failed to query for the context of the ::cl::CommandQueue object" );

                if( m_Size > 0 )
                {
                    if( init )
                    {
                        Concurrency::extent<1> ext(m_devMemory.get_extent());
                        Concurrency::array_view<T> m_devMemoryAV = m_devMemory.section(ext);
                        Concurrency::parallel_for_each(m_devMemoryAV.extent, 
                        [=,&m_devMemoryAV](Concurrency::index<1> idx) restrict(amp)
                        {
                            m_devMemoryAV[idx] = value;
                        }
                        );
                    }
                }
            }
#if 0
            /*! \brief A constructor that creates a new device_vector using a range specified by the user.
            *   \param begin An iterator pointing at the beginning of the range.
            *   \param end An iterator pointing at the end of the range.
            *   \param flags A bitfield that takes the OpenCL memory flags to help specify where the device_vector allocates memory.
            *   \param init Boolean value to indicate whether to initialize device memory from host memory.
            *   \param ctl A Bolt control class used to perform copy operations; a default is used if not supplied by the user.
            *   \note Ignore the enable_if<> parameter; it prevents this constructor from being called with integral types.
            */
            template< typename InputIterator >
            device_vector( const InputIterator begin, size_type newSize, cl_mem_flags flags = CL_MEM_READ_WRITE,
                bool init = true, const control& ctl = control::getDefault( ),
                typename std::enable_if< !std::is_integral< InputIterator >::value >::type* = 0 ): m_Size( newSize ), m_commQueue( ctl.commandQueue( ) ), m_Flags( flags )
            {
                static_assert( std::is_convertible< value_type, typename std::iterator_traits< InputIterator >::value_type >::value,
                    "iterator value_type does not convert to device_vector value_type" );
                static_assert( !std::is_polymorphic< value_type >::value, "AMD C++ template extensions do not support the virtual keyword yet" );

                //  We want to use the context from the passed in commandqueue to initialize our buffer
                cl_int l_Error = CL_SUCCESS;
                ::cl::Context l_Context = m_commQueue.getInfo< CL_QUEUE_CONTEXT >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to query for the context of the ::cl::CommandQueue object" );

                if( m_Flags & CL_MEM_USE_HOST_PTR )
                {
                    m_devMemory = ::cl::Buffer( l_Context, m_Flags, m_Size * sizeof( value_type ),
                        reinterpret_cast< value_type* >( const_cast< value_type* >( &*begin ) ) );
                }
                else
                {
                    m_devMemory = ::cl::Buffer( l_Context, m_Flags, m_Size * sizeof( value_type ) );

                    if( init )
                    {
                        ::cl::copy( begin, begin+m_Size, m_devMemory );
                    }
                }
            };

            /*! \brief A constructor that creates a new device_vector using a range specified by the user.
            *   \param begin An iterator pointing at the beginning of the range.
            *   \param end An iterator pointing at the end of the range.
            *   \param flags A bitfield that takes the OpenCL memory flags to help specify where the device_vector allocates memory.
            *   \param ctl A Bolt control class for copy operations; a default is used if not supplied by the user.
            *   \note Ignore the enable_if<> parameter; it prevents this constructor from being called with integral types.
            */
            template< typename InputIterator >
            device_vector( const InputIterator begin, const InputIterator end, cl_mem_flags flags = CL_MEM_READ_WRITE, const control& ctl = control::getDefault( ),
                typename std::enable_if< !std::is_integral< InputIterator >::value >::type* = 0 ): m_commQueue( ctl.commandQueue( ) ), m_Flags( flags )
            {
                static_assert( std::is_convertible< value_type, typename std::iterator_traits< InputIterator >::value_type >::value,
                    "iterator value_type does not convert to device_vector value_type" );
                static_assert( !std::is_polymorphic< value_type >::value, "AMD C++ template extensions do not support the virtual keyword yet" );

                //  We want to use the context from the passed in commandqueue to initialize our buffer
                cl_int l_Error = CL_SUCCESS;
                ::cl::Context l_Context = m_commQueue.getInfo< CL_QUEUE_CONTEXT >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to query for the context of the ::cl::CommandQueue object" );

                m_Size = std::distance( begin, end );

                if( m_Flags & CL_MEM_USE_HOST_PTR )
                {
                    m_devMemory = ::cl::Buffer( l_Context, m_Flags, m_Size * sizeof( value_type ),
                        reinterpret_cast< value_type* >( const_cast< value_type* >( &*begin ) ) );
                }
                else
                {
                    m_devMemory = ::cl::Buffer( l_Context, m_Flags, m_Size * sizeof( value_type ) );

                    ::cl::copy( begin, end, m_devMemory );
                }
            };

            /*! \brief A constructor that creates a new device_vector using a pre-initialized buffer supplied by the user.
            *   \param rhs A pre-existing ::cl::Buffer supplied by the user.
            *   \param ctl A Bolt control class for copy operations; a default is used if not supplied by the user.
            */
            device_vector( const ::cl::Buffer& rhs, const control& ctl = control::getDefault( ) ): m_devMemory( rhs ), m_commQueue( ctl.commandQueue( ) )
            {
                static_assert( !std::is_polymorphic< value_type >::value, "AMD C++ template extensions do not support the virtual keyword yet" );

                m_Size = capacity( );

                cl_int l_Error = CL_SUCCESS;
                m_Flags = m_devMemory.getInfo< CL_MEM_FLAGS >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to query for the memory flags of the ::cl::Buffer object" );
            };

            //  Member functions

            /*! \brief Change the number of elements in device_vector to reqSize.
            *   If the new requested size is less than the original size, the data is truncated and lost.  If the
			*   new size is greater than the original
            *   size, the extra paddign will be initialized with the value specified by the user.
            *   \param reqSize The requested size of the device_vector in elements.
            *   \param val All new elements are initialized with this new value.
            *   \note capacity( ) may exceed n, but is not less than n.
            *   \warning If the device_vector must reallocate, all previous iterators, references, and pointers are invalidated.
            *   \warning The ::cl::CommandQueue is not a STD reserve( ) parameter
            */
#endif
            void resize( size_type reqSize, const value_type& val = value_type( ) )
            {
                size_type cap = capacity( );

                if( reqSize == cap )
                    return;

                Concurrency::array<T,1> l_tmpBuffer(reqSize);
                if( m_Size > 0 )
                {
                    //1622 Arrays are logically considered to be value types in that when an array is copied to another array,
                    //a deep copy is performed. Two arrays never point to the same data.
                    //m_Size data elements are copied
                    //Concurrency::array_view<T,1> tmpAV(m_devMemory);

					m_devMemory.copy_to(l_tmpBuffer);

                    if( reqSize > m_Size )
                    {
						extent<1> copyRange(reqSize - m_Size);
						index<1>  startPoint(m_Size);
						Concurrency::array_view<T,1> l_tmpBufferSection = l_tmpBuffer.section(startPoint, copyRange);
                        parallel_for_each(l_tmpBufferSection.extents, [&](index<1> idx)
                        {
                            l_tmpBufferSection[idx] = val;
                        });
						/*Ask kalyan for setting data to 0 */
                        //::cl::Event fillEvent;
                        //l_Error = m_commQueue.enqueueFillBuffer< value_type >( l_tmpBuffer, val, l_srcSize, l_reqSize - l_srcSize, &copyEvent, &fillEvent );
                        //V_OPENCL( l_Error, "device_vector failed to fill the new data with the provided pattern" );

                        //  Not allowed to return until the copy operation is finished
                        //l_Error = fillEvent.wait( );
                        //V_OPENCL( l_Error, "device_vector failed to wait for fill event" );
                    }
                    else
                    {
                        ;//  Not allowed to return until the copy operation is finished
                        //l_Error = m_commQueue.enqueueWaitForEvents( copyEvent );
                        //V_OPENCL( l_Error, "device_vector failed to wait for copy event" );
                    }
                }
                else
                {
					parallel_for_each(l_tmpBuffer.extents, [&](index<1> idx)
					{
						l_tmpBufferSection[idx] = val;
					});
                }

                //  Remember the new size
                m_Size = reqSize;

                //  Operator= should call retain/release appropriately
                m_devMemory = l_tmpBuffer;
            }

            /*! \brief Return the number of known elements
            *   \note size( ) differs from capacity( ), in that size( ) returns the number of elements between begin() & end()
            *   \return Number of valid elements
            */
            size_type size( void ) const
            {
                return m_Size;
            }

            /*! \brief Return the maximum number of elements possible to allocate on the associated device.
            *   \return The maximum amount of memory possible to allocate, counted in elements.
            */
            /*size_type max_size( void ) const
            {
                cl_int l_Error = CL_SUCCESS;

                ::cl::Device l_Device = m_commQueue.getInfo< CL_QUEUE_DEVICE >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to query for the device of the command queue" );

                size_type l_MaxSize  = l_Device.getInfo< CL_DEVICE_MAX_MEM_ALLOC_SIZE >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to query device for the maximum memory size" );

                return l_MaxSize / sizeof( value_type );
            }*/

            /*! \brief Request a change in the capacity of the device_vector.
            *   If reserve completes successfully, this device_vector object guarantees that the it can store the requested amount
            *   of elements without another reallocation, until the device_vector size exceeds n.
            *   \param n The requested size of the device_vector in elements
            *   \note capacity( ) may exceed n, but will not be less than n.
            *   \note Contents are preserved, and the size( ) of the vector is not affected.
            *   \warning if the device_vector must reallocate, all previous iterators, references, and pointers are invalidated.
            *   \warning The ::cl::CommandQueue is not a STD reserve( ) parameter
            */
            void reserve( size_type reqSize )
            {
#if 0
                if( reqSize <= capacity( ) )
                    return;

                /*if( reqSize > max_size( ) )
                    throw ::cl::Error( CL_MEM_OBJECT_ALLOCATION_FAILURE , "The amount of memory requested exceeds what is available" );
                */
                //  We want to use the context from the passed in commandqueue to initialize our buffer
                cl_int l_Error = CL_SUCCESS;
                ::cl::Context l_Context = m_commQueue.getInfo< CL_QUEUE_CONTEXT >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to query for the context of the ::cl::CommandQueue object" );

                if( m_Size == 0 )
                {
                    ::cl::Buffer l_tmpBuffer( l_Context, m_Flags, reqSize * sizeof( value_type ) );
                    m_devMemory = l_tmpBuffer;
                    return;
                }

                size_type l_size = reqSize * sizeof( value_type );
                //  Can't user host_ptr because l_size is guranteed to be bigger
                ::cl::Buffer l_tmpBuffer( l_Context, m_Flags, l_size, NULL, &l_Error );
                V_OPENCL( l_Error, "device_vector can not create an temporary internal OpenCL buffer" );

                size_type l_srcSize = m_devMemory.getInfo< CL_MEM_SIZE >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to request the size of the ::cl::Buffer object" );

                ::cl::Event copyEvent;
                V_OPENCL( m_commQueue.enqueueCopyBuffer( m_devMemory, l_tmpBuffer, 0, 0, l_srcSize, NULL, &copyEvent ),
                    "device_vector failed to copy from buffer to buffer " );

                //  Not allowed to return until the copy operation is finished
                V_OPENCL( copyEvent.wait( ), "device_vector failed to wait on an event object" );

                //  Operator= should call retain/release appropriately
                m_devMemory = l_tmpBuffer;
#endif
            }

            /*! \brief Return the maximum possible number of elements without reallocation.
            *   \note Capacity() differs from size(), in that capacity() returns the number of elements that \b could be stored
            *   in the memory currently allocated.
            *   \return The size of the memory held by device_vector, counted in elements.
            */
            size_type capacity( void ) const
            {
                Concurrency::extent<1> ext = m_devMemory.get_extent();
                return ext.size();
            }

            /*! \brief Shrink the capacity( ) of this device_vector to just fit its elements.
            *   This makes the size( ) of the vector equal to its capacity( ).
            *   \note Contents are preserved.
            *   \warning if the device_vector must reallocate, all previous iterators, references, and pointers are invalidated.
            *   \warning The ::cl::CommandQueue is not a STD reserve( ) parameter.
            */
            void shrink_to_fit( )
            {
#if 0
                if( m_Size > capacity( ) )
                    throw ::cl::Error( CL_MEM_OBJECT_ALLOCATION_FAILURE , "device_vector size can not be greater than capacity( )" );

                if( m_Size == capacity( ) )
                    return;

                //  We want to use the context from the passed in commandqueue to initialize our buffer
                cl_int l_Error = CL_SUCCESS;
                ::cl::Context l_Context = m_commQueue.getInfo< CL_QUEUE_CONTEXT >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to query for the context of the ::cl::CommandQueue object" );

                size_type l_newSize = m_Size * sizeof( value_type );
                ::cl::Buffer l_tmpBuffer( l_Context, m_Flags, l_newSize, NULL, &l_Error );
                V_OPENCL( l_Error, "device_vector can not create an temporary internal OpenCL buffer" );

                size_type l_srcSize = m_devMemory.getInfo< CL_MEM_SIZE >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to request the size of the ::cl::Buffer object" );

                std::vector< ::cl::Event > copyEvent( 1 );
                l_Error = m_commQueue.enqueueCopyBuffer( m_devMemory, l_tmpBuffer, 0, 0, l_newSize, NULL, &copyEvent.front( ) );
                V_OPENCL( l_Error, "device_vector failed to copy data to the new ::cl::Buffer object" );

                //  Not allowed to return until the copy operation is finished
                l_Error = m_commQueue.enqueueWaitForEvents( copyEvent );
                V_OPENCL( l_Error, "device_vector failed to wait for copy event" );

                //  Operator= should call retain/release appropriately
                m_devMemory = l_tmpBuffer;
#endif
            }

            /*! \brief Retrieves the value stored at index n.
            *   \return Returns a proxy reference object, to control when device memory gets mapped.
            */
            reference operator[]( size_type n )
            {
                return reference( *this, n );
            }

            /*! \brief Retrieves a constant value stored at index n.
            *   \return Returns a const_reference, which is not a proxy object.
            */
            const_reference operator[]( size_type n ) const
            {
                //cl_int l_Error = CL_SUCCESS;

                //naked_pointer ptrBuff = reinterpret_cast< naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, true, CL_MAP_READ, n * sizeof( value_type), sizeof( value_type), NULL, NULL, &l_Error ) );
                //V_OPENCL( l_Error, "device_vector failed map device memory to host memory for operator[]" );

                const_reference tmpRef = m_devMemory[n];

                /*::cl::Event unmapEvent;
                l_Error = m_commQueue.enqueueUnmapMemObject( m_devMemory, ptrBuff, NULL, &unmapEvent );
                V_OPENCL( l_Error, "device_vector failed to unmap host memory back to device memory" );
                V_OPENCL( unmapEvent.wait( ), "failed to wait for unmap event" );*/

                return tmpRef;
            }

            /*! \brief Retrieves an iterator for this container that points at the beginning element.
            *   \return A device_vector< value_type >::iterator.
            */
            iterator begin( void )
            {
                return iterator( *this, 0 );
            }

            /*! \brief Retrieves an iterator for this container that points at the beginning constant element.
            *   No operation through this iterator may modify the contents of the referenced container.
            *   \return A device_vector< value_type >::const_iterator
            */
            const_iterator begin( void ) const
            {
                return const_iterator( *this, 0 );
            }

            /*! \brief Retrieves an iterator for this container that points at the beginning constant element.
            *   No operation through this iterator may modify the contents of the referenced container.
            *   \note This method may return a constant iterator from a non-constant container.
            *   \return A device_vector< value_type >::const_iterator.
            */
            const_iterator cbegin( void ) const
            {
                return const_iterator( *this, 0 );
            }

            reverse_iterator rbegin( void )
            {
                static_assert( false, "Reverse iterators are not yet implemented" );
                return reverse_iterator( );
            }

            const_reverse_iterator rbegin( void ) const
            {
                static_assert( false, "Reverse iterators are not yet implemented" );
                return const_reverse_iterator( );
            }

            const_reverse_iterator crbegin( void ) const
            {
                static_assert( false, "Reverse iterators are not yet implemented" );
                return const_reverse_iterator( );
            }

            /*! \brief Retrieves an iterator for this container that points at the last element.
            *   \return A device_vector< value_type >::iterator.
            */
            iterator end( void )
            {
                return iterator( *this, m_Size );
            }

            /*! \brief Retrieves an iterator for this container that points at the last constant element.
            *   No operation through this iterator may modify the contents of the referenced container.
            *   \return A device_vector< value_type >::const_iterator.
            */
            const_iterator end( void ) const
            {
                return const_iterator( *this, m_Size );
            }

            /*! \brief Retrieves an iterator for this container that points at the last constant element.
            *   No operation through this iterator may modify the contents of the referenced container.
            *   \note This method may return a constant iterator from a non-constant container.
            *   \return A device_vector< value_type >::const_iterator.
            */
            const_iterator cend( void ) const
            {
                return const_iterator( *this, m_Size );
            }

            reverse_iterator rend( void )
            {
                static_assert( false, "Reverse iterators are not yet implemented" );
                return reverse_iterator( );
            }

            const_reverse_iterator rend( void ) const
            {
                static_assert( false, "Reverse iterators are not yet implemented" );
                return const_reverse_iterator( );
            }

            const_reverse_iterator crend( void ) const
            {
                static_assert( false, "Reverse iterators are not yet implemented" );
                return const_reverse_iterator( );
            }

            /*! \brief Retrieves the value stored at index 0.
            *   \note This returns a proxy object, to control when device memory gets mapped.
            */
            reference front( void )
            {
                return reference( m_devMemory, 0 );
            }

            /*! \brief Retrieves the value stored at index 0.
            *   \return Returns a const_reference, which is not a proxy object.
            */
            const_reference front( void ) const
            {

                const_reference tmpRef = m_devMemory[0];

                /*::cl::Event unmapEvent;
                l_Error = m_commQueue.enqueueUnmapMemObject( m_devMemory, ptrBuff, NULL, &unmapEvent );
                V_OPENCL( l_Error, "device_vector failed to unmap host memory back to device memory" );
                V_OPENCL( unmapEvent.wait( ), "failed to wait for unmap event" );*/

                return tmpRef;
            }

            /*! \brief Retrieves the value stored at index size( ) - 1.
            *   \note This returns a proxy object, to control when device memory gets mapped.
            */
            reference back( void )
            {
                return reference( m_devMemory, m_Size - 1 );
            }

            /*! \brief Retrieves the value stored at index size( ) - 1.
            *   \return Returns a const_reference, which is not a proxy object.
            */
            const_reference back( void ) const
            {
                /*cl_int l_Error = CL_SUCCESS;

                const_naked_pointer ptrBuff = reinterpret_cast< const_naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, true, CL_MAP_READ,
                    (m_Size - 1) * sizeof( value_type ), sizeof( value_type ), NULL, NULL, &l_Error ) );
                V_OPENCL( l_Error, "device_vector failed map device memory to host memory for operator[]" );

                const_reference tmpRef = *ptrBuff;*/
                const_reference tmpRef = m_devMemory[m_Size - 1];
                /*::cl::Event unmapEvent;
                l_Error = m_commQueue.enqueueUnmapMemObject( m_devMemory, ptrBuff, NULL, &unmapEvent );
                V_OPENCL( l_Error, "device_vector failed to unmap host memory back to device memory" );
                V_OPENCL( unmapEvent.wait( ), "failed to wait for unmap event" );*/

                return tmpRef;
            }

            //Do I need the boost::shared_array
            pointer data( void )
            {
                /*cl_int l_Error = CL_SUCCESS;

                naked_pointer ptrBuff = reinterpret_cast< naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, true, CL_MAP_READ | CL_MAP_WRITE,
                    0, m_Size * sizeof( value_type ), NULL, NULL, &l_Error ) );

                V_OPENCL( l_Error, "device_vector failed map device memory to host memory for operator[]" );

                pointer sp( ptrBuff, UnMapBufferFunctor< device_vector< value_type > >( *this ) );
                */
                pointer sp( m_devMemory.data( ) );
                return  sp;
            }

            const_pointer data( void ) const
            {
                /*cl_int l_Error = CL_SUCCESS;

                const_naked_pointer ptrBuff = reinterpret_cast< const_naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, true, CL_MAP_READ,
                    0, m_Size * sizeof( value_type ), NULL, NULL, &l_Error ) );
                V_OPENCL( l_Error, "device_vector failed map device memory to host memory for operator[]" );

                const_pointer sp( ptrBuff, UnMapBufferFunctor< const device_vector< value_type > >( *this ) );
                return sp;*/
                return m_devMemory.data();
            }

            /*! \brief Removes all elements (makes the device_vector empty).
            *   \note All previous iterators, references and pointers are invalidated.
            */
            void clear( void )
            {
                //  Only way to release the Buffer resource is to explicitly call the destructor
                // m_devMemory.~Buffer( );

                //  Allocate a temp empty buffer on the stack, because of a double release problem with explicitly
                //  calling the Wrapper destructor with cl.hpp version 1.2.
                Concurrency::array<T,1> tmp(0);
                m_devMemory = tmp;
                m_Size = 0;
            }

            /*! \brief Test whether the container is empty
            *   \return Returns true if size( ) == 0
            */
            bool empty( void ) const
            {
                return m_Size ? false: true;
            }

            /*! \brief Appends a copy of the value to the container
             *  \param value The element to append
            */
            void push_back( const value_type& value )
            {
                if( m_Size > capacity( ) )
                    throw std::exception( "device_vector size can not be greater than capacity( )" );

                //  Need to grow the vector to push new value.
                //  Vectors double their capacity on push_back if the array is not big enough.
                if( m_Size == capacity( ) )
                {
                    m_Size ? reserve( m_Size * 2 ) : reserve( 1 );
                }

                m_devMemory[(int)m_Size] = value;
                ++m_Size;
            }

            /*! \brief Removes the last element, but does not return it.
            */
            void pop_back( void )
            {
                if( m_Size > 0 )
                {
                    --m_Size;
                }
            }

            /*! \brief Swaps the contents of two device_vectors in an efficient manner.
             *  \param vec The device_vector to swap with.
            */
            void swap( device_vector& vec )
            {
                if( this == &vec )
                    return;

                Concurreny::array<T,1>  &swapBuffer( m_devMemory );
                m_devMemory = vec.m_devMemory;
                vec.m_devMemory = swapBuffer;

                /*::cl::CommandQueue    swapQueue( m_commQueue );
                m_commQueue = vec.m_commQueue;
                vec.m_commQueue = swapQueue;*/

                size_type sizeTmp = m_Size;
                m_Size = vec.m_Size;
                vec.m_Size = sizeTmp;

                /*cl_mem_flags flagsTmp = m_Flags;
                m_Flags = vec.m_Flags;
                vec.m_Flags = flagsTmp;*/
            }

            /*! \brief Removes an element.
             *  \param index The iterator position in which to remove the element.
            *   \return The iterator position after the deleted element.
            */
            iterator erase( const_iterator index )
            {
                if( &index.m_Container != this )
                    throw std::exception( "Iterator is not from this container" );

                iterator l_End = end( );
                if( index.m_index >= l_End.m_index )
                    throw std::exception( "Iterator is pointing past the end of this container" );

                size_type sizeRegion = l_End.m_index - index.m_index;

                /*cl_int l_Error = CL_SUCCESS;
                naked_pointer ptrBuff = reinterpret_cast< naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, true, CL_MAP_READ | CL_MAP_WRITE,
                        index.m_index * sizeof( value_type ), sizeRegion * sizeof( value_type ), NULL, NULL, &l_Error ) );
                V_OPENCL( l_Error, "device_vector failed map device memory to host memory for operator[]" );*/
                naked_pointer ptrBuff = m_devMemory.data();
                 naked_pointer ptrBuffTemp = ptrBuff + index.m_index;
                ::memmove( ptrBuffTemp, ptrBuffTemp + 1, (sizeRegion - 1)*sizeof( value_type ) );

                /*::cl::Event unmapEvent;
                l_Error = m_commQueue.enqueueUnmapMemObject( m_devMemory, ptrBuff, NULL, &unmapEvent );
                V_OPENCL( l_Error, "device_vector failed to unmap host memory back to device memory" );
                V_OPENCL( unmapEvent.wait( ), "failed to wait for unmap event" );*/

                --m_Size;

                size_type newIndex = (m_Size < index.m_index) ? m_Size : index.m_index;
                return iterator( *this, newIndex );
            }

            /*! \brief Removes a range of elements.
             *  \param begin The iterator position signifiying the beginning of the range.
             *  \param end The iterator position signifying the end of the range (exclusive).
            *   \return The iterator position after the deleted range.
            */
            iterator erase( const_iterator first, const_iterator last )
            {
                if(( &first.m_Container != this ) && ( &last.m_Container != this ) )
                    throw std::exception( "Iterator is not from this container" );

                if( last.m_index > m_Size )
                    throw std::exception( "Iterator is pointing past the end of this container" );

                if( (first == begin( )) && (last == end( )) )
                {
                    clear( );
                    return iterator( *this, m_Size );
                }

                iterator l_End = end( );
                size_type sizeMap = l_End.m_index - first.m_index;

                //cl_int l_Error = CL_SUCCESS;
                /*naked_pointer ptrBuff = reinterpret_cast< naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, true, CL_MAP_READ | CL_MAP_WRITE,
                        first.m_index * sizeof( value_type ), sizeMap * sizeof( value_type ), NULL, NULL, &l_Error ) );
                V_OPENCL( l_Error, "device_vector failed map device memory to host memory for operator[]" );*/

                naked_pointer ptrBuff = m_devMemory.data();
                ptrBuff = ptrBuff + first.m_index;
                size_type sizeErase = last.m_index - first.m_index;
                ::memmove( ptrBuff, ptrBuff + sizeErase, (sizeMap - sizeErase)*sizeof( value_type ) );

                /*::cl::Event unmapEvent;
                l_Error = m_commQueue.enqueueUnmapMemObject( m_devMemory, ptrBuff, NULL, &unmapEvent );
                V_OPENCL( l_Error, "device_vector failed to unmap host memory back to device memory" );
                V_OPENCL( unmapEvent.wait( ), "failed to wait for unmap event" );*/

                m_Size -= sizeErase;

                size_type newIndex = (m_Size < last.m_index) ? m_Size : last.m_index;
                return iterator( *this, newIndex );
            }

            /*! \brief Insert a new element into the container.
             *  \param index The iterator position to insert a copy of the element.
             *  \param value The element to insert.
            *   \return The position of the new element.
            *   \note Only iterators before the insertion point remain valid after the insertion.
            *   \note If the container must grow to contain the new value, all iterators and references are invalidated.
            */
            iterator insert( const_iterator index, const value_type& value )
            {
                if( &index.m_Container != this )
                    throw std::exception( "Iterator is not from this container" );

                if( index.m_index > m_Size )
                    throw std::exception( "Iterator is pointing past the end of this container" );

                if( index.m_index == m_Size )
                {
                    push_back( value );
                    return iterator( *this, index.m_index );
                }

                //  Need to grow the vector to insert a new value.
                //  TODO:  What is an appropriate growth strategy for GPU memory allocation?  Exponential growth does not seem
                //  right at first blush.
                if( m_Size == capacity( ) )
                {
                    reserve( m_Size + 10 );
                }

                size_type sizeMap = (m_Size - index.m_index) + 1;

                /*cl_int l_Error = CL_SUCCESS;
                naked_pointer ptrBuff = reinterpret_cast< naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, true, CL_MAP_READ | CL_MAP_WRITE,
                        index.m_index * sizeof( value_type ), sizeMap * sizeof( value_type ), NULL, NULL, &l_Error ) );
                V_OPENCL( l_Error, "device_vector failed map device memory to host memory for operator[]" );*/
                naked_pointer ptrBuff = m_devMemory.data();
                ptrBuff = ptrBuff + index.m_index;
                //  Shuffle the old values 1 element down
                ::memmove( ptrBuff + 1, ptrBuff, (sizeMap - 1)*sizeof( value_type ) );

                //  Write the new value in its place
                *ptrBuff = value;

                /*::cl::Event unmapEvent;
                l_Error = m_commQueue.enqueueUnmapMemObject( m_devMemory, ptrBuff, NULL, &unmapEvent );
                V_OPENCL( l_Error, "device_vector failed to unmap host memory back to device memory" );
                V_OPENCL( unmapEvent.wait( ), "failed to wait for unmap event" );*/

                ++m_Size;

                return iterator( *this, index.m_index );
            }

            /*! \brief Inserts n copies of the new element into the container.
             *  \param index The iterator position to insert n copies of the element.
             *  \param n The number of copies of element.
             *  \param value The element to insert.
             *   \note Only iterators before the insertion point remain valid after the insertion.
             *   \note If the container must grow to contain the new value, all iterators and references are invalidated.
             */
            void insert( const_iterator index, size_type n, const value_type& value )
            {
                if( &index.m_Container != this )
                    throw std::exception(  "Iterator is not from this container" );

                if( index.m_index > m_Size )
                    throw std::exception(  "Iterator is pointing past the end of this container" );

                //  Need to grow the vector to insert a new value.
                //  TODO:  What is an appropriate growth strategy for GPU memory allocation?  Exponential growth does not seem
                //  right at first blush.
                if( ( m_Size + n ) > capacity( ) )
                {
                    reserve( m_Size + n );
                }

                size_type sizeMap = (m_Size - index.m_index) + n;

                /*cl_int l_Error = CL_SUCCESS;
                naked_pointer ptrBuff = reinterpret_cast< naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, true, CL_MAP_READ | CL_MAP_WRITE,
                        index.m_index * sizeof( value_type ), sizeMap * sizeof( value_type ), NULL, NULL, &l_Error ) );
                V_OPENCL( l_Error, "device_vector failed map device memory to host memory for operator[]" );*/
                naked_pointer ptrBuff = m_devMemory.data();
                ptrBuff = ptrBuff + index.m_index;
                //  Shuffle the old values n element down.
                ::memmove( ptrBuff + n, ptrBuff, (sizeMap - n)*sizeof( value_type ) );

                //  Copy the new value n times in the buffer.
                for( size_type i = 0; i < n; ++i )
                {
                    ptrBuff[ i ] = value;
                }

                /*::cl::Event unmapEvent;
                l_Error = m_commQueue.enqueueUnmapMemObject( m_devMemory, ptrBuff, NULL, &unmapEvent );
                V_OPENCL( l_Error, "device_vector failed to unmap host memory back to device memory" );
                V_OPENCL( unmapEvent.wait( ), "failed to wait for unmap event" );*/

                m_Size += n;
            }

            template< typename InputIterator >
            void insert( const_iterator index, InputIterator begin, InputIterator end )
            {
                if( &index.m_Container != this )
                    throw std::exception(  "Iterator is not from this container" );

                if( index.m_index > m_Size )
                    throw std::exception(  "Iterator is pointing past the end of this container" );

                //  Need to grow the vector to insert a new value.
                //  TODO:  What is an appropriate growth strategy for GPU memory allocation?  Exponential growth does not seem
                //  right at first blush.
                size_type n = std::distance( begin, end );
                if( ( m_Size + n ) > capacity( ) )
                {
                    reserve( m_Size + n );
                }
                size_type sizeMap = (m_Size - index.m_index) + n;

                /*cl_int l_Error = CL_SUCCESS;
                naked_pointer ptrBuff = reinterpret_cast< naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, true, CL_MAP_READ | CL_MAP_WRITE,
                        index.m_index * sizeof( value_type ), sizeMap * sizeof( value_type ), NULL, NULL, &l_Error ) );
                V_OPENCL( l_Error, "device_vector failed map device memory to host memory for iterator insert" );*/
                naked_pointer ptrBuff = m_devMemory.data() + index.m_index;
                //  Shuffle the old values n element down.
                ::memmove( ptrBuff + n, ptrBuff, (sizeMap - n)*sizeof( value_type ) );

#if( _WIN32 )
                std::copy( begin, end, stdext::checked_array_iterator< naked_pointer >( ptrBuff, n ) );
#else
                std::copy( begin, end, ptrBuff );
#endif

                /*::cl::Event unmapEvent;
                l_Error = m_commQueue.enqueueUnmapMemObject( m_devMemory, ptrBuff, NULL, &unmapEvent );
                V_OPENCL( l_Error, "device_vector failed to unmap host memory back to device memory" );
                V_OPENCL( unmapEvent.wait( ), "failed to wait for unmap event" );*/

                m_Size += n;
            }
#if 0
            /*! \brief Assigns newSize copies of element value.
             *  \param newSize The new size of the device_vector.
             *  \param value The value of the element that is replicated newSize times.
            *   \warning All previous iterators, references, and pointers are invalidated.
            */
            void assign( size_type newSize, const value_type& value )
            {
                if( newSize > m_Size )
                {
                    reserve( newSize );
                }
                m_Size = newSize;

                cl_int l_Error = CL_SUCCESS;

                ::cl::Event fillEvent;
                l_Error = m_commQueue.enqueueFillBuffer< value_type >( m_devMemory, value, 0, m_Size * sizeof( value_type ), NULL, &fillEvent );
                V_OPENCL( l_Error, "device_vector failed to fill the new data with the provided pattern" );

                //  Not allowed to return until the copy operation is finished.
                l_Error = fillEvent.wait( );
                V_OPENCL( l_Error, "device_vector failed to wait for fill event" );
            }

            /*! \brief Assigns a range of values to device_vector, replacing all previous elements.
             *  \param begin The iterator position signifiying the beginning of the range.
             *  \param end The iterator position signifying the end of the range (exclusive).
            *   \warning All previous iterators, references, and pointers are invalidated.
            */
            template< typename InputIterator >
            void assign( InputIterator begin, InputIterator end )
            {
                size_type l_Count = std::distance( begin, end );

                if( l_Count > m_Size )
                {
                    reserve( l_Count );
                }
                m_Size = l_Count;

                cl_int l_Error = CL_SUCCESS;

                naked_pointer ptrBuffer = reinterpret_cast< naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, CL_TRUE, CL_MAP_WRITE_INVALIDATE_REGION, 0 , m_Size * sizeof( value_type ), NULL, NULL, &l_Error ) );
                V_OPENCL( l_Error, "device_vector failed map device memory to host memory for push_back" );

#if( _WIN32 )
                std::copy( begin, end, stdext::checked_array_iterator< naked_pointer >( ptrBuffer, m_Size ) );
#else
                std::copy( begin, end, ptrBuffer );
#endif
                ::cl::Event unmapEvent;
                l_Error = m_commQueue.enqueueUnmapMemObject( m_devMemory, ptrBuffer, NULL, &unmapEvent );
                V_OPENCL( l_Error, "device_vector failed to unmap host memory back to device memory" );
                V_OPENCL( unmapEvent.wait( ), "failed to wait for unmap event" );
            }
#endif
        private:
            Concurrency::array<T,1> m_devMemory;
            size_type m_Size;
        };

    }
}

#endif
