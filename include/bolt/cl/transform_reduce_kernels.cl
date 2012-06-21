template <typename T,  typename unary_function, typename binary_function>
kernel
void transform_reduceTemplate(
    global T* input, 
    const int length,

    global unary_function *transformFunctor,

	const T init,
    global binary_function *reduceFunctor,

    global T* result,
    local T *scratch
)
{
    int gx = get_global_id (0);

    T accumulator = init;

    // Loop sequentially over chunks of input vector
    while (gx < length) {
        T element = (input[gx]);
        element = (*transformFunctor)(element);

        accumulator = (*reduceFunctor)(accumulator, element);;
        gx += get_global_size(0);
    }

    // Perform parallel reduction through shared memory:
    int local_index = get_local_id(0);
    scratch[local_index] = accumulator;
    barrier(CLK_LOCAL_MEM_FENCE);
    

    for(int offset = get_local_size(0) / 2;
        offset > 0;
        offset = offset / 2) {
        bool test = local_index < offset;
        if (test) {
            int other = scratch[local_index + offset];
            int mine  = scratch[local_index];
            scratch[local_index] = (*reduceFunctor)(mine, other);;
      
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if (local_index == 0) {
        result[get_group_id(0)] = scratch[0];
    }
};
