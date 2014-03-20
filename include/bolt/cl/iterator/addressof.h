#ifndef BOLT_ADDRESSOF_H
#define BOLT_ADDRESSOF_H
#include <bolt/cl/device_vector.h>

namespace bolt{
namespace cl{

    template <typename Iterator>
    typename Iterator::value_type * addressof(typename Iterator itr)
    {
        return std::addressof(*itr);
    }

    template <typename UnaryFunction, typename Iterator>
    typename bolt::cl::transform_iterator<typename UnaryFunction, typename Iterator>::pointer 
        addressof(typename bolt::cl::transform_iterator<typename UnaryFunction, typename Iterator> itr)
    {
        typedef typename bolt::cl::transform_iterator<typename UnaryFunction, typename Iterator>::pointer pointer;
        pointer ptr = itr;
        return ptr;
    }


    template <typename Iterator, typename DeviceIterator>
    transform_iterator<typename Iterator::unary_func, typename DeviceIterator> 
    create_device_itr(bolt::cl::transform_iterator_tag, Iterator &itr, DeviceIterator &dev_itr)
    {
        typedef typename Iterator::unary_func unary_func;    
        return transform_iterator<unary_func, typename DeviceIterator> (dev_itr);
    }   

    template <typename Iterator, typename DeviceIterator>
    typename DeviceIterator 
    create_device_itr(std::random_access_iterator_tag, Iterator &itr, DeviceIterator &dev_itr)
    {
        return dev_itr;
    }   


}} //namespace bolt::cl

#endif