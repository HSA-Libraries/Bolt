#pragma once
#if !defined( BOLT_DEVICE_VECTOR_H )
#define BOLT_DEVICE_VECTOR_H

#define __CL_ENABLE_EXCEPTIONS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#if defined(__APPLE__) || defined(__MACOSX)
    #include <OpenCL/cl.hpp>
#else
    #include <CL/cl.hpp>
#endif
#include <iterator>
// #include <boost/iterator/iterator_facade.hpp>

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
        *   \details A device_vector is an abstract data type that provides random access to a flat, sequential region of memory that is performant for 
        *   for the device in question to use.  This may imply different memories for diffferent devices.  For instance, discrete class graphics
        *   devices, this is most likely video memory, for APU devices this may imply zero-copy memory and for classic CPU devices this may imply
        *   standard host memory.
        *   \sa http://www.sgi.com/tech/stl/Vector.html
        */
        template< typename T >
        class device_vector
        {
            ::cl::Buffer m_devMemory;

        public:

            //  Useful typedefs specific to this container
            typedef T value_type;
            typedef ptrdiff_t difference_type;
            typedef difference_type distance_type;
            typedef size_t size_type;

            typedef T* pointer;
            typedef const T* const_pointer;

            typedef T& reference;
            typedef const T& const_reference;

            /*! \brief A random access iterator in the classic sense
            *   \sa http://www.sgi.com/tech/stl/Iterators.html
            *   \sa http://www.sgi.com/tech/stl/RandomAccessIterator.html
            */
            class iterator: public std::iterator< std::random_access_iterator_tag, T >
            {
            };

            /*! \brief A constant random access iterator in the classic sense
            *   \sa http://www.sgi.com/tech/stl/Iterators.html
            *   \sa http://www.sgi.com/tech/stl/RandomAccessIterator.html
            */
            class const_iterator: public std::iterator< std::random_access_iterator_tag, const T >
            {
            };

            /*! \brief A reverse random access iterator in the classic sense
            *   \sa http://www.sgi.com/tech/stl/ReverseIterator.html
            *   \sa http://www.sgi.com/tech/stl/RandomAccessIterator.html
            */
            class reverse_iterator: public std::reverse_iterator< iterator >
            {
            };

            /*! \brief A constant random access iterator in the classic sense
            *   \sa http://www.sgi.com/tech/stl/ReverseIterator.html
            *   \sa http://www.sgi.com/tech/stl/RandomAccessIterator.html
            */
            class const_reverse_iterator: public std::reverse_iterator< const_iterator >
            {
            };

            /*! \brief A default constructor that creates an empty device_vector
            */
            device_vector( )
            {};

            /*! \brief A copy constructor that initializes a new device_vector in a non-overlapping range
            *   \param rhs The device_vector to copy from
            */
            device_vector( const device_vector& rhs )
            {};

            /*! \brief A constructor that will create a new device_vector with the specified number of elements, with a specified initial value
            *   \param N The number of elements of the new device_vector
            *   \param value The value that new elements will be initialized with
            */
            device_vector( size_type N, const value_type& value = value_type( ) )
            {}

            template< typename InputIterator >
            device_vector( const InputIterator begin, const InputIterator end )
            {};

            device_vector( const ::cl::Buffer& rhs )
            {};

            device_vector& operator=( const device_vector& rhs)
            {
                if( this == &rhs )
                    return *this;
            }

            //  Member functions
            void resize( size_type N, const value_type& val );
            size_type size( void ) const;
            size_type max_size( void ) const;
            void reserve( size_type n );
            size_type capacity( void ) const;
            void shrink_to_fit( void );

            reference operator[]( size_type n );
            const_reference operator[]( size_type n ) const;

            iterator begin( void );
            const_iterator begin( void ) const;
            const_iterator cbegin( void ) const;
            reverse_iterator rbegin( void );
            const_reverse_iterator rbegin( void ) const;
            const_reverse_iterator crbegin( void ) const;

            iterator end( void );
            const_iterator end( void ) const;
            const_iterator cend( void ) const;
            reverse_iterator rend( void );
            const_reverse_iterator rend( void ) const;
            const_reverse_iterator crend( void ) const;

            reference front( void );
            const_reference front( void ) const;

            reference back( void );
            const_reference back( void ) const;

            pointer data( void );
            const_pointer data( void ) const;

            void clear( void );

            bool empty( void ) const;

            void push_back( const value_type& value );
            void pop_back( void );

            void swap( device_vector& vec );

            iterator erase( iterator index );
            iterator erase( iterator begin, iterator end );

            iterator insert( iterator index, const value_type& value );
            void insert( iterator index, size_type N, const value_type& value );
            template< typename InputIterator >
            void insert( iterator index, InputIterator begin, InputIterator end );

            void assign( size_type N, const value_type& value );
            template< typename InputIterator >
            void assign( InputIterator begin, InputIterator end );

            ~device_vector( )
            {};

        };

    }
}

#endif