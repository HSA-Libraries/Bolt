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


    template <typename Iterator>
    Iterator create_device_itr(Iterator itr)
    {
        return itr;
    }
    //Specialize device_vector iterator
    /*template <typename ValueType>
    bolt::cl::device_vector<ValueType>::iterator & create_device_itr(bolt::cl::device_vector<ValueType>::iterator &itr)
    {
        return itr;
    }*/

    template <typename UnaryFunction, typename Iterator>
    typename bolt::cl::transform_iterator<typename UnaryFunction, typename Iterator>
        create_device_itr(typename bolt::cl::device_vector<typename Iterator::value_type>::iterator itr)
    {
        return transform_iterator<typename UnaryFunction, device_vector<typename Iterator::value_type>::iterator> (itr);
    }
    
}} //namespace bolt::cl

#endif