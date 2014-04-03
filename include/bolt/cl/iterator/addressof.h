#ifndef BOLT_ADDRESSOF_H
#define BOLT_ADDRESSOF_H
#include <bolt/cl/device_vector.h>
#include <bolt/cl/iterator/transform_iterator.h>
#include <bolt/cl/iterator/counting_iterator.h>
#include <bolt/cl/iterator/constant_iterator.h>
namespace bolt{
namespace cl{

    template <typename Iterator>
    typename Iterator::value_type * addressof(Iterator itr)
    {
        return std::addressof(*itr);
    }

    template <typename UnaryFunction, typename Iterator>
    typename bolt::cl::transform_iterator<UnaryFunction, Iterator>::pointer 
        addressof(typename bolt::cl::transform_iterator<UnaryFunction, Iterator> itr)
    {
        typedef typename bolt::cl::transform_iterator<UnaryFunction, Iterator>::pointer pointer;
        pointer ptr = itr;
        return ptr;
    }

    template <typename value_type>
    typename bolt::cl::counting_iterator<value_type>::pointer 
        addressof(typename bolt::cl::counting_iterator<value_type> itr)
    {
        typedef typename bolt::cl::counting_iterator<value_type>::pointer pointer;
        pointer ptr = itr;
        return itr;
    }

    template <typename value_type>
    typename bolt::cl::constant_iterator<value_type>::pointer 
        addressof(typename bolt::cl::constant_iterator<value_type> itr)
    {
        typedef typename bolt::cl::constant_iterator<value_type>::pointer pointer;
        pointer ptr = itr;
        return itr;
    }

    
    /*******************Create device side Iterators **********************/

    template <typename Iterator, typename DeviceIterator>
    const transform_iterator<typename Iterator::unary_func, DeviceIterator>
    create_device_itr(bolt::cl::transform_iterator_tag, Iterator itr, DeviceIterator dev_itr)
    {
        typedef typename Iterator::unary_func unary_func;
        return transform_iterator<unary_func, DeviceIterator> (dev_itr, itr.functor());
    }   

    template <typename Iterator, typename DeviceIterator>
    const typename bolt::cl::device_vector<typename Iterator::value_type>::iterator 
    create_device_itr(std::random_access_iterator_tag, Iterator itr, DeviceIterator dev_itr)
    {
        return dev_itr;
    }

    template <typename Iterator, typename DeviceIterator>
    const constant_iterator<typename Iterator::value_type> 
    create_device_itr(bolt::cl::constant_iterator_tag, Iterator itr, DeviceIterator dev_itr)
    {
        return itr;
    }

    template <typename Iterator, typename DeviceIterator>
    const counting_iterator<typename Iterator::value_type> 
    create_device_itr(bolt::cl::counting_iterator_tag, Iterator itr, DeviceIterator dev_itr)
    {
        return itr;
    }
    
    /******************* Create Mapped Iterators **********************/

    template <typename Iterator, typename T>
    transform_iterator<typename Iterator::unary_func, T*>
    create_mapped_iterator(bolt::cl::transform_iterator_tag, Iterator &itr, T* ptr)
    {
        typedef typename Iterator::unary_func unary_func;
        return transform_iterator<unary_func, T*> (ptr, itr.functor());
    }   

    template <typename Iterator, typename T>
    T*
    create_mapped_iterator(bolt::cl::device_vector_tag, Iterator &itr, T* ptr)
    {
        return ptr + itr.m_Index;
    }

    /*TODO - The current constant and counting iterator implementations are buggy. 
      They create an OpenCL device buffer even if the iterator is to be used on host only.
   */
    template <typename Iterator, typename T>
    const constant_iterator<typename Iterator::value_type> &
    create_mapped_iterator(bolt::cl::constant_iterator_tag, Iterator &itr, T* ptr)
    {
        return itr;
    }
    
    template <typename Iterator, typename T>
    const counting_iterator<typename Iterator::value_type> &
    create_mapped_iterator(bolt::cl::counting_iterator_tag, Iterator &itr, T* ptr)
    {
        return itr;
    }

}} //namespace bolt::cl

#endif
