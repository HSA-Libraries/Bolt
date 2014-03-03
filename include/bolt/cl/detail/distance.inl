
namespace bolt
{
namespace cl
{
namespace detail
{


    template<typename InputIterator>
    inline typename bolt::cl::iterator_traits<InputIterator>::difference_type
    distance(InputIterator first, InputIterator last, bolt::cl::incrementable_traversal_tag)
    {
        typename bolt::cl::iterator_traits<InputIterator>::difference_type result(0);

        while(first != last)
        {
        ++first;
        ++result;
        } 
        return result;
    } 


    template<typename InputIterator>
    inline typename bolt::cl::iterator_traits<InputIterator>::difference_type
    distance(InputIterator first, InputIterator last, bolt::cl::random_access_traversal_tag)
    {
        return last - first;
    } 


} // namespace detail


    template<typename InputIterator>
    inline typename bolt::cl::iterator_traits<InputIterator>::difference_type
        distance(InputIterator first, InputIterator last)
    {
      // dispatch on iterator traversal
      return bolt::cl::detail::distance(first, last, typename bolt::cl::iterator_traversal<InputIterator>::type());
    } 


} //  namespace cl
} //  namespace bolt

