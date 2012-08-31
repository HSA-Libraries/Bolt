#pragma once
#if !defined( BOLT_DEVICE_VECTOR_H )
#define BOLT_DEVICE_VECTOR_H

#include <iterator>
#include <type_traits>
#include <numeric>
#include <bolt/cl/bolt.h>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/reverse_iterator.hpp>
#include <boost/shared_array.hpp>

/*! \file device_vector.h
 * Public header file for the device_container class
 */

/*! \brief Defining namespace for the Bolt project
    */
namespace bolt
{
    /*! \brief Namespace that captures OpenCL related data types and functions
     */
    namespace cl
    {
        /*! \addtogroup Containers
         */

        /*! \addtogroup Device
        *   \ingroup Containers
        *   Containers that guarantee sequential and linear access to memory "close" to the device
        */

        /*! \brief This defines the OpenCL version of a device_vector
        *   \ingroup Device
        *   \details A device_vector is an abstract data type that provides random access to a flat, sequential region of memory that is performant 
        *   for the device in question to use.  This may imply different memories for diffferent devices.  For instance, discrete class graphics
        *   devices, this is most likely video memory, for APU devices this may imply zero-copy memory and for classic CPU devices this may imply
        *   standard host memory.
        *   \sa http://www.sgi.com/tech/stl/Vector.html
        */
        template< typename T >
        class device_vector
        {

            /*! \brief Class used with shared_ptr<> as a custom deleter, to unmap a buffer that has been mapped with 
            *   device_vector data() method
            */
            template< typename Container >
            class UnMapBufferFunctor
            {
                Container& m_Container;

            public:
                //  Basic constructor requires a reference to the container and a positional element
                UnMapBufferFunctor( Container& rhs ): m_Container( rhs )
                {}

                void operator( )( const void* pBuff )
                {
                    V_OPENCL( m_Container.m_commQueue.enqueueUnmapMemObject( m_Container.m_devMemory, const_cast< void* >( pBuff ) ),
                            "shared_ptr failed to unmap host memory back to device memory" );
                }
            };

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
            *   The location of an element of the container may not actually reside in system memory, but rather device
            *   memory which may be in a partitioned memory space.  Access of a reference of the container results in 
            *   a mapping and unmapping operation of device memory
            *   \note The container element reference is implemented as a proxy object
            *   \warning This operation may be slow, depending on the location of the element
            */
            class reference
            {
                ::cl::Buffer m_devMemory;
                ::cl::CommandQueue m_commQueue;
                size_type m_index;

            public:
                reference( const ::cl::Buffer& devMem, const ::cl::CommandQueue& cq, size_type index ): m_devMemory( devMem ), m_commQueue( cq ), m_index( index )
                {}

                //  Automatic type conversion operator to turn the reference object into a value_type
                operator value_type( ) const
                {
                    cl_int l_Error = CL_SUCCESS;
                    naked_pointer result = reinterpret_cast< naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, true, CL_MAP_READ, m_index * sizeof( value_type ), sizeof( value_type ), NULL, NULL, &l_Error ) );
                    V_OPENCL( l_Error, "device_vector failed map device memory to host memory for operator[]" );

                    value_type valTmp = *result;

                    V_OPENCL( m_commQueue.enqueueUnmapMemObject( m_devMemory, result ), "device_vector failed to unmap host memory back to device memory" );

                    return valTmp;
                }

                reference& operator=( const value_type& rhs )
                {
                    cl_int l_Error = CL_SUCCESS;
                    naked_pointer result = reinterpret_cast< naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, true, CL_MAP_WRITE_INVALIDATE_REGION, m_index * sizeof( value_type ), sizeof( value_type ), NULL, NULL, &l_Error ) );
                    V_OPENCL( l_Error, "device_vector failed map device memory to host memory for operator[]" );

                    *result = rhs;

                    V_OPENCL( m_commQueue.enqueueUnmapMemObject( m_devMemory, result ), "device_vector failed to unmap host memory back to device memory" );

                    return *this;
                }

            /*! \brief A get accessor function to return the encapsulated device buffer
            *   This member function allows access to the Buffer object, which can be retrieved through a reference or an iterator.
            *   This is necessary to allow library functions to set the encapsulated Buffer object as a kernel argument.  
            *   \note This get function could be implemented in the iterator, but the reference object is usually a temporary rvalue so 
            *   this location seems less intrusive to the design of the vector class.
            */
                ::cl::Buffer getBuffer( ) const
                {
                    return m_devMemory;
                }
            };

            /*! \brief A non-writeable copy of an element of the container
            *   Constant references are optimized to return only a value_type, because it can be guaranteed that
            *   the value will not be modified, so no need for mapping operations
            *   \note A const_reference actually returns a value, not a reference.
            */
            typedef const value_type const_reference;

            /*! \brief Base class provided to encapsulate all the common functionality for constant
            *   and non-constant iterators
            *   \sa http://www.sgi.com/tech/stl/Iterators.html
            *   \sa http://www.sgi.com/tech/stl/RandomAccessIterator.html
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
            *   \sa http://www.sgi.com/tech/stl/ReverseIterator.html
            *   \sa http://www.sgi.com/tech/stl/RandomAccessIterator.html
            */
            class reverse_iterator: public boost::reverse_iterator< iterator >
            {
            };

            /*! \brief A constant random access iterator in the classic sense
            *   \sa http://www.sgi.com/tech/stl/ReverseIterator.html
            *   \sa http://www.sgi.com/tech/stl/RandomAccessIterator.html
            */
            class const_reverse_iterator: public boost::reverse_iterator< const_iterator >
            {
            };

            /*! \brief A default constructor that creates an empty device_vector
            *   \param ctl An Bolt control class used to perform copy operations; a default is used if not supplied by the user
            */
            device_vector( /* cl_mem_flags flags = CL_MEM_READ_WRITE,*/ const control& ctl = control::getDefault( ) ): m_Size( 0 ), m_commQueue( ctl.commandQueue( ) ), m_Flags( CL_MEM_READ_WRITE )
            {
                static_assert( std::is_pod< value_type >::value, "device_vector only supports POD (plain old data) types" );
            }

            /*! \brief A constructor that will create a new device_vector with the specified number of elements, with a specified initial value
            *   \param newSize The number of elements of the new device_vector
            *   \param value The value that new elements will be initialized with
            *   \param flags A bitfield that takes the OpenCL memory flags to help specify where the device_vector should allocate memory
            *   \param ctl An Bolt control class used to perform copy operations; a default is used if not supplied by the user
            *   \warning The ::cl::CommandQueue is not a STD reserve( ) parameter
            *   \note This constructor relies on the ::cl::Buffer object to throw on error
            */
            device_vector( size_type newSize, const value_type& value = value_type( ), cl_mem_flags flags = CL_MEM_READ_WRITE, const control& ctl = control::getDefault( ) ): m_Size( newSize ), m_commQueue( ctl.commandQueue( ) ), m_Flags( flags )
            {
                static_assert( std::is_pod< value_type >::value, "device_vector only supports POD (plain old data) types" );

                //  We want to use the context from the passed in commandqueue to initialize our buffer
                cl_int l_Error = CL_SUCCESS;
                ::cl::Context l_Context = m_commQueue.getInfo< CL_QUEUE_CONTEXT >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to query for the context of the ::cl::CommandQueue object" );

                if( m_Size > 0 )
                {
                    m_devMemory = ::cl::Buffer( l_Context, CL_MEM_READ_WRITE, m_Size * sizeof( value_type ) );

                    std::vector< ::cl::Event > fillEvent( 1 );
                    V_OPENCL( m_commQueue.enqueueFillBuffer< value_type >( m_devMemory, value, 0, newSize * sizeof( value_type ), NULL, &fillEvent.front( ) ), 
                        "device_vector failed to fill the internal buffer with the requested pattern");

                    //  Not allowed to return until the fill operation is finished
                    V_OPENCL( m_commQueue.enqueueWaitForEvents( fillEvent ), "device_vector failed to wait for an event" );
                }
            }

            /*! \brief A constructor that will create a new device_vector using a range specified by the user
            *   \param begin An iterator pointing at the beginning of the range
            *   \param end An iterator pointing at the end of the range
            *   \param flags A bitfield that takes the OpenCL memory flags to help specify where the device_vector should allocate memory
            *   \param ctl An Bolt control class used to perform copy operations; a default is used if not supplied by the user
            *   \note This constructor relies on the ::cl::Buffer object to throw on error 
            *   \note The last nameless parameter should be ignored; it helps distinguish this constructor from the size_type constructor for primitive types
            */
            template< typename InputIterator >
            device_vector( const InputIterator begin, const InputIterator end, cl_mem_flags flags = CL_MEM_READ_WRITE, const control& ctl = control::getDefault( ),
                typename std::enable_if< std::is_class< InputIterator >::value >::type* = 0 ): m_commQueue( ctl.commandQueue( ) ), m_Flags( flags )
            {
                static_assert( std::is_convertible< value_type, typename std::iterator_traits< InputIterator >::value_type >::value, "device_vector value_type does not match iterator value_type" );
                static_assert( std::is_pod< value_type >::value, "device_vector only supports POD (plain old data) types" );

                //  We want to use the context from the passed in commandqueue to initialize our buffer
                cl_int l_Error = CL_SUCCESS;
                ::cl::Context l_Context = m_commQueue.getInfo< CL_QUEUE_CONTEXT >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to query for the context of the ::cl::CommandQueue object" );

                m_Size = std::distance( begin, end );
                m_devMemory = ::cl::Buffer( l_Context, flags, m_Size * sizeof( value_type ), reinterpret_cast< value_type* >( &*begin ) );

                if( !(m_Flags & CL_MEM_USE_HOST_PTR) )
                {
                    naked_pointer ptrBuffer = reinterpret_cast< naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, CL_TRUE, CL_MAP_WRITE_INVALIDATE_REGION, 0 , m_Size * sizeof( value_type ), NULL, NULL, &l_Error ) );
                    V_OPENCL( l_Error, "device_vector failed to map device memory to host memory" );

#if( _WIN32 )
                    std::copy( begin, end, stdext::checked_array_iterator< naked_pointer >( ptrBuffer, m_Size ) );
#else
                    std::copy( begin, end, ptrBuffer );
#endif
                    V_OPENCL( m_commQueue.enqueueUnmapMemObject( m_devMemory, ptrBuffer ), "device_vector failed to unmap host memory back to device memory" );
                }
            };

            /*! \brief A constructor that will create a new device_vector using a pre-initialized buffer supplied by the user
            *   \param rhs A pre-existing ::cl::Buffer supplied by the user
            *   \param ctl An Bolt control class used to perform copy operations; a default is used if not supplied by the user
            */
            device_vector( const ::cl::Buffer& rhs, const control& ctl = control::getDefault( ) ): m_devMemory( rhs ), m_commQueue( ctl.commandQueue( ) )
            {
                static_assert( std::is_pod< value_type >::value, "device_vector only supports POD (plain old data) types" );

                m_Size = capacity( );

                cl_int l_Error = CL_SUCCESS;
                m_Flags = m_devMemory.getInfo< CL_MEM_FLAGS >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to query for the memory flags of the ::cl::Buffer object" );
            };

            //  Member functions

            /*! \brief Change the number of elements in device_vector to reqSize
            *   If the new requested size is less than the original size, the data is truncated and lost.  If the new size is greater than the original
            *   size, the extra paddign will be initialized with the value specified by the user.
            *   \param reqSize The requested size of the device_vector in elements
            *   \param val All new elements will be initialized with this new value
            *   \note capacity( ) may exceed n, but will not be less than n
            *   \warning If the device_vector has to reallocate, all previous iterators, references and pointers are invalidated
            *   \warning The ::cl::CommandQueue is not a STD reserve( ) parameter
            */

            void resize( size_type reqSize, const value_type& val )
            {
                size_type cap = capacity( );

                if( reqSize == cap )
                    return;

                if( reqSize > max_size( ) )
                    throw ::cl::Error( CL_MEM_OBJECT_ALLOCATION_FAILURE , "The amount of memory requested exceeds what is available" );

                cl_int l_Error = CL_SUCCESS;

                ::cl::Context l_Context = m_devMemory.getInfo< CL_MEM_CONTEXT >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to query for the context of the ::cl::Buffer object" );

                cl_mem_flags l_memFlags = m_devMemory.getInfo< CL_MEM_FLAGS >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to query for the flags of the memory buffer" );

                size_type l_reqSize = reqSize * sizeof( value_type );
                ::cl::Buffer l_tmpBuffer( l_Context, l_memFlags, l_reqSize, NULL, &l_Error );

                size_type l_srcSize = m_devMemory.getInfo< CL_MEM_SIZE >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to request the size of the ::cl::Buffer object" );

                std::vector< ::cl::Event > copyEvent( 1 );
                l_Error = m_commQueue.enqueueCopyBuffer( m_devMemory, l_tmpBuffer, 0, 0, l_srcSize, NULL, &copyEvent.front( ) );
                V_OPENCL( l_Error, "device_vector failed to copy data to the new ::cl::Buffer object" );

                //  If the new buffer size is greater than the old, then the new elements need to be initialized to the value specified on the
                //  function parameter
                if( l_reqSize > l_srcSize )
                {
                    std::vector< ::cl::Event > fillEvent( 1 );
                    l_Error = m_commQueue.enqueueFillBuffer< value_type >( m_devMemory, val, l_srcSize, l_reqSize - l_srcSize, &copyEvent, &fillEvent.front( ) );
                    V_OPENCL( l_Error, "device_vector failed to fill the new data with the provided pattern" );

                    //  Not allowed to return until the copy operation is finished
                    l_Error = m_commQueue.enqueueWaitForEvents( fillEvent );
                    V_OPENCL( l_Error, "device_vector failed to wait for fill event" );
                }
                else
                {
                    //  Not allowed to return until the copy operation is finished
                    l_Error = m_commQueue.enqueueWaitForEvents( copyEvent );
                    V_OPENCL( l_Error, "device_vector failed to wait for copy event" );
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

            /*! \brief Return the maximum number of elements possible to allocate on the associated device
            *   \return The maximum amount of memory possible to allocate, counted in elements
            */
            size_type max_size( void ) const
            {
                cl_int l_Error = CL_SUCCESS;

                ::cl::Device l_Device = m_commQueue.getInfo< CL_QUEUE_DEVICE >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to query for the device of the command queue" );

                size_type l_MaxSize  = l_Device.getInfo< CL_DEVICE_MAX_MEM_ALLOC_SIZE >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to query device for the maximum memory size" );

                return l_MaxSize / sizeof( value_type );
            }

            /*! \brief Request a change in the capacity of the device_vector
            *   If reserve completes successfully, this device_vector object guarantees that the it can store the requested amount
            *   of elements without another reallocation, until the device_vector size exceeds n.
            *   \param n The requested size of the device_vector in elements
            *   \note capacity( ) may exceed n, but will not be less than n
            *   \note Contents are preserved, and the size( ) of the vector is not affected
            *   \warning if the device_vector has to reallocate, all previous iterators, references and pointers are invalidated
            *   \warning The ::cl::CommandQueue is not a STD reserve( ) parameter
            */
            void reserve( size_type reqSize )
            {
                if( reqSize <= capacity( ) )
                    return;

                if( reqSize > max_size( ) )
                    throw ::cl::Error( CL_MEM_OBJECT_ALLOCATION_FAILURE , "The amount of memory requested exceeds what is available" );

                if( m_Size == 0 )
                {
                    ::cl::Buffer l_tmpBuffer( CL_MEM_READ_WRITE, reqSize * sizeof( value_type ) );
                    m_devMemory = l_tmpBuffer;
                    return;
                }

                cl_int l_Error = CL_SUCCESS;

                ::cl::Context l_Context = m_devMemory.getInfo< CL_MEM_CONTEXT >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to query for the context of the ::cl::Buffer object" );

                cl_mem_flags l_memFlags = m_devMemory.getInfo< CL_MEM_FLAGS >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to query for the flags of the memory buffer" );

                size_type l_size = reqSize * sizeof( value_type );
                //  Can't user host_ptr because l_size is guranteed to be bigger
                ::cl::Buffer l_tmpBuffer( l_Context, l_memFlags, l_size, NULL, &l_Error );
                V_OPENCL( l_Error, "device_vector can not create an temporary internal OpenCL buffer" );

                size_type l_srcSize = m_devMemory.getInfo< CL_MEM_SIZE >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to request the size of the ::cl::Buffer object" );

                std::vector< ::cl::Event > copyEvent( 1 );
                V_OPENCL( m_commQueue.enqueueCopyBuffer( m_devMemory, l_tmpBuffer, 0, 0, l_srcSize, NULL, &copyEvent.front( ) ), 
                    "device_vector failed to copy from buffer to buffer " );

                //  Not allowed to return until the copy operation is finished
                V_OPENCL( m_commQueue.enqueueWaitForEvents( copyEvent ), "device_vector failed to wait on an event object" );

                //  Operator= should call retain/release appropriately
                m_devMemory = l_tmpBuffer;
            }

            /*! \brief Return the maximum possible number of elements without reallocation
            *   \note Capacity() differs from size(), in that capacity() returns the number of elements that \b could be stored
            *   in the memory currently allocated.
            *   \return The size of the memory held by device_vector, counted in elements
            */
            size_type capacity( void ) const
            {
                size_type l_memSize  = 0;
                cl_int l_Error = CL_SUCCESS;

                if( m_Size == 0 )
                    return m_Size;

                l_memSize = m_devMemory.getInfo< CL_MEM_SIZE >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to request the size of the ::cl::Buffer object" );

                return l_memSize / sizeof( value_type );
            }

            /*! \brief Shrink the capacity( ) of this device_vector to just fit its elements
            *   This makes the size( ) of the vector equal to its capacity( )
            *   \note Contents are preserved
            *   \warning if the device_vector has to reallocate, all previous iterators, references and pointers are invalidated
            *   \warning The ::cl::CommandQueue is not a STD reserve( ) parameter
            */
            void shrink_to_fit( )
            {
                if( m_Size > capacity( ) )
                    throw ::cl::Error( CL_MEM_OBJECT_ALLOCATION_FAILURE , "device_vector size can not be greater than capacity( )" );

                if( m_Size == capacity( ) )
                    return;

                cl_int l_Error = CL_SUCCESS;

                ::cl::Context l_Context = m_devMemory.getInfo< CL_MEM_CONTEXT >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to query for the context of the ::cl::Buffer object" );

                cl_mem_flags l_memFlags = m_devMemory.getInfo< CL_MEM_FLAGS >( &l_Error );
                V_OPENCL( l_Error, "device_vector failed to query for the flags of the memory buffer" );

                size_type l_newSize = m_Size * sizeof( value_type );
                ::cl::Buffer l_tmpBuffer( l_Context, l_memFlags, l_newSize, NULL, &l_Error );
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
            }

            /*! \brief Retrieves the value stored at index n
            *   \return Returns a proxy reference object, to control when device memory gets mapped
            */
            reference operator[]( size_type n )
            {
                return reference( m_devMemory, m_commQueue, n );
            }

            /*! \brief Retrieves a constant value stored at index n
            *   \return Returns a const_reference, which is not a proxy object
            */
            const_reference operator[]( size_type n ) const
            {
                cl_int l_Error = CL_SUCCESS;

                naked_pointer ptrBuff = reinterpret_cast< naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, true, CL_MAP_READ, n * sizeof( value_type), sizeof( value_type), NULL, NULL, &l_Error ) );
                V_OPENCL( l_Error, "device_vector failed map device memory to host memory for operator[]" );

                const_reference tmpRef = *ptrBuff;

                l_Error = m_commQueue.enqueueUnmapMemObject( m_devMemory, ptrBuff );
                V_OPENCL( l_Error, "device_vector failed to unmap host memory back to device memory" );

                return tmpRef;
            }

            /*! \brief Retrieves an iterator for this container that points at the beginning element
            *   \return A device_vector< value_type >::iterator
            */
            iterator begin( void )
            {
                return iterator( *this, 0 );
            }

            /*! \brief Retrieves an iterator for this container that points at the beginning constant element
            *   No operation through this iterator may modify the contents of the referenced container
            *   \return A device_vector< value_type >::const_iterator
            */
            const_iterator begin( void ) const
            {
                return const_iterator( *this, 0 );
            }

            /*! \brief Retrieves an iterator for this container that points at the beginning constant element
            *   No operation through this iterator may modify the contents of the referenced container
            *   \note This method may return a constant iterator from a non-constant container
            *   \return A device_vector< value_type >::const_iterator
            */
            const_iterator cbegin( void ) const
            {
                return const_iterator( *this, 0 );
            }

            reverse_iterator rbegin( void )
            {
                return reverse_iterator( );
            }

            const_reverse_iterator rbegin( void ) const
            {
                return const_reverse_iterator( );
            }

            const_reverse_iterator crbegin( void ) const
            {
                return const_reverse_iterator( );
            }

            /*! \brief Retrieves an iterator for this container that points at the last element
            *   \return A device_vector< value_type >::iterator
            */
            iterator end( void )
            {
                return iterator( *this, m_Size );
            }

            /*! \brief Retrieves an iterator for this container that points at the last constant element
            *   No operation through this iterator may modify the contents of the referenced container
            *   \return A device_vector< value_type >::const_iterator
            */
            const_iterator end( void ) const
            {
                return const_iterator( *this, m_Size );
            }

            /*! \brief Retrieves an iterator for this container that points at the last constant element
            *   No operation through this iterator may modify the contents of the referenced container
            *   \note This method may return a constant iterator from a non-constant container
            *   \return A device_vector< value_type >::const_iterator
            */
            const_iterator cend( void ) const
            {
                return const_iterator( *this, m_Size );
            }

            reverse_iterator rend( void )
            {
                return reverse_iterator( );
            }

            const_reverse_iterator rend( void ) const
            {
                return const_reverse_iterator( );
            }

            const_reverse_iterator crend( void ) const
            {
                return const_reverse_iterator( );
            }

            /*! \brief Retrieves the value stored at index 0
            *   \note This returns a proxy object, to control when device memory gets mapped
            */
            reference front( void )
            {
                return reference( m_devMemory, m_commQueue, 0 );
            }

            /*! \brief Retrieves the value stored at index 0
            *   \return Returns a const_reference, which is not a proxy object
            */
            const_reference front( void ) const
            {
                cl_int l_Error = CL_SUCCESS;

                const_naked_pointer ptrBuff = reinterpret_cast< const_naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, true, CL_MAP_READ, 0, sizeof( value_type ), NULL, NULL, &l_Error ) );
                V_OPENCL( l_Error, "device_vector failed map device memory to host memory for operator[]" );

                const_reference tmpRef = *ptrBuff;

                l_Error = m_commQueue.enqueueUnmapMemObject( m_devMemory, ptrBuff );
                V_OPENCL( l_Error, "device_vector failed to unmap host memory back to device memory" );

                return tmpRef;
            }

            /*! \brief Retrieves the value stored at index size( ) - 1
            *   \note This returns a proxy object, to control when device memory gets mapped
            */
            reference back( void )
            {
                return reference( m_devMemory, m_commQueue, m_Size - 1 );
            }

            /*! \brief Retrieves the value stored at index size( ) - 1
            *   \return Returns a const_reference, which is not a proxy object
            */
            const_reference back( void ) const
            {
                cl_int l_Error = CL_SUCCESS;

                const_naked_pointer ptrBuff = reinterpret_cast< const_naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, true, CL_MAP_READ, 
                    (m_Size - 1) * sizeof( value_type ), sizeof( value_type ), NULL, NULL, &l_Error ) );
                V_OPENCL( l_Error, "device_vector failed map device memory to host memory for operator[]" );

                const_reference tmpRef = *ptrBuff;

                l_Error = m_commQueue.enqueueUnmapMemObject( m_devMemory, ptrBuff );
                V_OPENCL( l_Error, "device_vector failed to unmap host memory back to device memory" );

                return tmpRef;
            }

            pointer data( void )
            {
                cl_int l_Error = CL_SUCCESS;

                naked_pointer ptrBuff = reinterpret_cast< naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, true, CL_MAP_READ | CL_MAP_WRITE, 
                    0, m_Size * sizeof( value_type ), NULL, NULL, &l_Error ) );
                V_OPENCL( l_Error, "device_vector failed map device memory to host memory for operator[]" );

                pointer sp( ptrBuff, UnMapBufferFunctor< device_vector< value_type > >( *this ) );
                return sp;
            }

            const_pointer data( void ) const
            {
                cl_int l_Error = CL_SUCCESS;

                const_naked_pointer ptrBuff = reinterpret_cast< const_naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, true, CL_MAP_READ, 
                    0, m_Size * sizeof( value_type ), NULL, NULL, &l_Error ) );
                V_OPENCL( l_Error, "device_vector failed map device memory to host memory for operator[]" );

                const_pointer sp( ptrBuff, UnMapBufferFunctor< const device_vector< value_type > >( *this ) );
                return sp;
            }

            /*! \brief Removes all elements (makes the device_vector empty)
            *   \note All previous iterators, references and pointers are invalidated
            */
            void clear( void )
            {
                //  Only way to release the Buffer resource is to explicitly call the destructor
                //m_devMemory.~Buffer( );

                //  TODO:  Allocate a temp empty buffer on the stack, because of a double release problem with explicitly
                //  calling the Wrapper destructor with cl.hpp version 1.2
                ::cl::Buffer tmp;
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
                    throw ::cl::Error( CL_MEM_OBJECT_ALLOCATION_FAILURE , "device_vector size can not be greater than capacity( )" );

                //  Need to grow the vector to push new value
                //  Vectors will double their capacity on push_back if the array is not big enough
                if( m_Size == capacity( ) )
                {
                    m_Size ? reserve( m_Size * 2 ) : reserve( 1 );
                }

                cl_int l_Error = CL_SUCCESS;

                naked_pointer result = reinterpret_cast< naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, true, CL_MAP_WRITE_INVALIDATE_REGION, 
                    m_Size * sizeof( value_type), sizeof( value_type ), NULL, NULL, &l_Error ) );
                V_OPENCL( l_Error, "device_vector failed map device memory to host memory for push_back" );
                *result = value;

                l_Error = m_commQueue.enqueueUnmapMemObject( m_devMemory, result );
                V_OPENCL( l_Error, "device_vector failed to unmap host memory back to device memory" );

                ++m_Size;
            }

            /*! \brief Removes the last element, but does not return it
            */
            void pop_back( void )
            {
                if( m_Size > 0 )
                {
                    --m_Size;
                }
            }

            /*! \brief Swaps the contents of two device_vectors in an efficient manner
             *  \param vec The device_vector to swap with
            */
            void swap( device_vector& vec )
            {
                ::cl::Buffer    swapTmp( m_devMemory );
                m_devMemory = vec.m_devMemory;
                vec.m_devMemory = swapTmp;

                size_type sizeTmp = m_Size;
                m_Size = vec.m_Size;
                vec.m_Size = sizeTmp;
            }

            /*! \brief Removes an element
             *  \param index The iterator position in which to remove the element
            *   \return The iterator position after the deleted element
            */
            iterator erase( const_iterator index )
            {
                if( &index.m_Container != this )
                    throw ::cl::Error( CL_INVALID_ARG_VALUE , "Iterator is not from this container" );

                iterator l_End = end( );
                if( index.m_index >= l_End.m_index )
                    throw ::cl::Error( CL_INVALID_ARG_INDEX , "Iterator is pointing past the end of this container" );

                size_type sizeRegion = l_End.m_index - index.m_index;

                cl_int l_Error = CL_SUCCESS;
                naked_pointer ptrBuff = reinterpret_cast< naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, true, CL_MAP_READ | CL_MAP_WRITE, 
                        index.m_index * sizeof( value_type ), sizeRegion * sizeof( value_type ), NULL, NULL, &l_Error ) );
                V_OPENCL( l_Error, "device_vector failed map device memory to host memory for operator[]" );

                ::memmove( ptrBuff, ptrBuff + 1, (sizeRegion - 1)*sizeof( value_type ) );

                l_Error = m_commQueue.enqueueUnmapMemObject( m_devMemory, ptrBuff );
                V_OPENCL( l_Error, "device_vector failed to unmap host memory back to device memory" );

                --m_Size;

                size_type newIndex = (m_Size < index.m_index) ? m_Size : index.m_index;
                return iterator( *this, newIndex );
            }

            /*! \brief Removes a range of elements
             *  \param begin The iterator position signifiying the beginning of the range
             *  \param end The iterator position signifying the end of the range (exclusive)
            *   \return The iterator position after the deleted range
            */
            iterator erase( const_iterator first, const_iterator last )
            {
                if(( &first.m_Container != this ) && ( &last.m_Container != this ) )
                    throw ::cl::Error( CL_INVALID_ARG_VALUE , "Iterator is not from this container" );

                if( last.m_index > m_Size )
                    throw ::cl::Error( CL_INVALID_ARG_INDEX , "Iterator is pointing past the end of this container" );

                if( (first == begin( )) && (last == end( )) )
                {
                    clear( );
                    return iterator( *this, m_Size );
                }

                iterator l_End = end( );
                size_type sizeMap = l_End.m_index - first.m_index;

                cl_int l_Error = CL_SUCCESS;
                naked_pointer ptrBuff = reinterpret_cast< naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, true, CL_MAP_READ | CL_MAP_WRITE, 
                        first.m_index * sizeof( value_type ), sizeMap * sizeof( value_type ), NULL, NULL, &l_Error ) );
                V_OPENCL( l_Error, "device_vector failed map device memory to host memory for operator[]" );

                size_type sizeErase = last.m_index - first.m_index;
                ::memmove( ptrBuff, ptrBuff + sizeErase, (sizeMap - sizeErase)*sizeof( value_type ) );

                l_Error = m_commQueue.enqueueUnmapMemObject( m_devMemory, ptrBuff );
                V_OPENCL( l_Error, "device_vector failed to unmap host memory back to device memory" );

                m_Size -= sizeErase;

                size_type newIndex = (m_Size < last.m_index) ? m_Size : last.m_index;
                return iterator( *this, newIndex );
            }

            /*! \brief Insert a new element into the container
             *  \param index The iterator position to insert a copy of the element
             *  \param value The element to insert
            *   \return The position of the new element
            *   \note Only iterators before the insertion point remain valid after the insertion.
            *   \note If the container must grow to contain the new value, all iterators and references are invalidated
            */
            iterator insert( const_iterator index, const value_type& value )
            {
                if( &index.m_Container != this )
                    throw ::cl::Error( CL_INVALID_ARG_VALUE , "Iterator is not from this container" );

                if( index.m_index > m_Size )
                    throw ::cl::Error( CL_INVALID_ARG_INDEX , "Iterator is pointing past the end of this container" );

                if( index.m_index == m_Size )
                {
                    push_back( value );
                    return iterator( *this, index.m_index );
                }

                //  Need to grow the vector to insert a new value
                //  TODO:  What is an appropriate growth strategy for GPU memory allocation?  Exponential growth does not seem 
                //  right at first blush
                if( m_Size == capacity( ) )
                {
                    reserve( m_Size + 10 );
                }

                size_type sizeMap = (m_Size - index.m_index) + 1;

                cl_int l_Error = CL_SUCCESS;
                naked_pointer ptrBuff = reinterpret_cast< naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, true, CL_MAP_READ | CL_MAP_WRITE, 
                        index.m_index * sizeof( value_type ), sizeMap * sizeof( value_type ), NULL, NULL, &l_Error ) );
                V_OPENCL( l_Error, "device_vector failed map device memory to host memory for operator[]" );

                //  Shuffle the old values 1 element down
                ::memmove( ptrBuff + 1, ptrBuff, (sizeMap - 1)*sizeof( value_type ) );

                //  Write the new value in its place
                *ptrBuff = value;

                l_Error = m_commQueue.enqueueUnmapMemObject( m_devMemory, ptrBuff );
                V_OPENCL( l_Error, "device_vector failed to unmap host memory back to device memory" );

                ++m_Size;

                return iterator( *this, index.m_index );
            }

            /*! \brief Inserts n copies of the new element into the container
             *  \param index The iterator position to insert n copies of the element
             *  \param n The number of copies of element
             *  \param value The element to insert
             *   \note Only iterators before the insertion point remain valid after the insertion.
             *   \note If the container must grow to contain the new value, all iterators and references are invalidated
             */
            void insert( const_iterator index, size_type n, const value_type& value )
            {
                if( &index.m_Container != this )
                    throw ::cl::Error( CL_INVALID_ARG_VALUE , "Iterator is not from this container" );

                if( index.m_index > m_Size )
                    throw ::cl::Error( CL_INVALID_ARG_INDEX , "Iterator is pointing past the end of this container" );

                //  Need to grow the vector to insert a new value
                //  TODO:  What is an appropriate growth strategy for GPU memory allocation?  Exponential growth does not seem 
                //  right at first blush
                if( ( m_Size + n ) > capacity( ) )
                {
                    reserve( m_Size + n );
                }

                size_type sizeMap = (m_Size - index.m_index) + n;

                cl_int l_Error = CL_SUCCESS;
                naked_pointer ptrBuff = reinterpret_cast< naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, true, CL_MAP_READ | CL_MAP_WRITE, 
                        index.m_index * sizeof( value_type ), sizeMap * sizeof( value_type ), NULL, NULL, &l_Error ) );
                V_OPENCL( l_Error, "device_vector failed map device memory to host memory for operator[]" );

                //  Shuffle the old values n element down
                ::memmove( ptrBuff + n, ptrBuff, (sizeMap - n)*sizeof( value_type ) );

                //  Copy the new value n times in the buffer
                for( size_type i = 0; i < n; ++i )
                {
                    ptrBuff[ i ] = value;
                }

                l_Error = m_commQueue.enqueueUnmapMemObject( m_devMemory, ptrBuff );
                V_OPENCL( l_Error, "device_vector failed to unmap host memory back to device memory" );

                m_Size += n;
            }

            template< typename InputIterator >
            void insert( const_iterator index, InputIterator begin, InputIterator end )
            {
                if( &index.m_Container != this )
                    throw ::cl::Error( CL_INVALID_ARG_VALUE , "Iterator is not from this container" );

                if( index.m_index > m_Size )
                    throw ::cl::Error( CL_INVALID_ARG_INDEX , "Iterator is pointing past the end of this container" );

                //  Need to grow the vector to insert a new value
                //  TODO:  What is an appropriate growth strategy for GPU memory allocation?  Exponential growth does not seem 
                //  right at first blush
                size_type n = std::distance( begin, end );
                if( ( m_Size + n ) > capacity( ) )
                {
                    reserve( m_Size + n );
                }
                size_type sizeMap = (m_Size - index.m_index) + n;

                cl_int l_Error = CL_SUCCESS;
                naked_pointer ptrBuff = reinterpret_cast< naked_pointer >( m_commQueue.enqueueMapBuffer( m_devMemory, true, CL_MAP_READ | CL_MAP_WRITE, 
                        index.m_index * sizeof( value_type ), sizeMap * sizeof( value_type ), NULL, NULL, &l_Error ) );
                V_OPENCL( l_Error, "device_vector failed map device memory to host memory for iterator insert" );

                //  Shuffle the old values n element down
                ::memmove( ptrBuff + n, ptrBuff, (sizeMap - n)*sizeof( value_type ) );

#if( _WIN32 )
                std::copy( begin, end, stdext::checked_array_iterator< naked_pointer >( ptrBuff, n ) );
#else
                std::copy( begin, end, ptrBuff );
#endif

                l_Error = m_commQueue.enqueueUnmapMemObject( m_devMemory, ptrBuff );
                V_OPENCL( l_Error, "device_vector failed to unmap host memory back to device memory" );

                m_Size += n;
            }

            /*! \brief Assigns newSize copies of element value
             *  \param newSize The new size of the device_vector
             *  \param value The value of the element that will be replicated newSize times
            *   \warning All previous iterators, references and pointers are invalidated
            */
            void assign( size_type newSize, const value_type& value )
            {
                if( newSize > m_Size )
                {
                    reserve( newSize );
                }
                m_Size = newSize;

                cl_int l_Error = CL_SUCCESS;

                std::vector< ::cl::Event > fillEvent( 1 );
                l_Error = m_commQueue.enqueueFillBuffer< value_type >( m_devMemory, value, 0, m_Size * sizeof( value_type ), NULL, &fillEvent.front( ) );
                V_OPENCL( l_Error, "device_vector failed to fill the new data with the provided pattern" );

                //  Not allowed to return until the copy operation is finished
                l_Error = m_commQueue.enqueueWaitForEvents( fillEvent );
                V_OPENCL( l_Error, "device_vector failed to wait for fill event" );
            }

            /*! \brief Assigns a range of values to device_vector, replacing all previous elements
             *  \param begin The iterator position signifiying the beginning of the range
             *  \param end The iterator position signifying the end of the range (exclusive)
            *   \warning All previous iterators, references and pointers are invalidated
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
                l_Error = m_commQueue.enqueueUnmapMemObject( m_devMemory, ptrBuffer );
                V_OPENCL( l_Error, "device_vector failed to unmap host memory back to device memory" );
            }

        private:
            ::cl::Buffer m_devMemory;
            ::cl::CommandQueue m_commQueue;
            size_type m_Size;
            cl_mem_flags m_Flags;
        };

    }
}

#endif
