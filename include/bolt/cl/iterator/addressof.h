#ifndef BOLT_ADDRESSOF_H
#define BOLT_ADDRESSOF_H

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
}} //namespace bolt::cl

#endif