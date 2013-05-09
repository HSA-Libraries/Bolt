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

#if !defined( BTBB_REDUCE_INL)
#define BTBB_REDUCE_INL
#pragma once


namespace bolt{
    namespace btbb {

             /*For documentation on the reduce object see below link
             *http://threadingbuildingblocks.org/docs/help/reference/algorithms/parallel_reduce_func.htm
             *The imperative form of parallel_reduce is used.
             *
            */
            template <typename T, typename BinaryFunction>
            struct Reduce {
                T value;
                BinaryFunction op;
                bool flag;

                //TODO - Decide on how many threads to spawn? Usually it should be equal to th enumber of cores
                //You might need to look at the tbb::split and there there cousin's
                //
                Reduce(const T &init) : value(init) {}
                Reduce(const BinaryFunction &_op, const T &init) : op(_op), value(init), flag(FALSE) {}
                Reduce() : value(0) {}
                Reduce( Reduce& s, tbb::split ) : flag(TRUE), op(s.op) {}
                void operator()( const tbb::blocked_range<T*>& r ) {
                    T temp = value;
                    for( T* a=r.begin(); a!=r.end(); ++a ) {
                      if(flag){
                        temp = *a;
                        flag = FALSE;
                      }
                      else
                        temp = op(temp,*a);
                    }
                    value = temp;
                }
                //Join is called by the parent thread after the child finishes to execute.
                void join( Reduce& rhs )
                {
                    value = op(value,rhs.value);
                }
            };

        template<typename InputIterator>
        typename std::iterator_traits<InputIterator>::value_type
            reduce(InputIterator first,
            InputIterator last)
        {

            typedef typename std::iterator_traits<InputIterator>::value_type iType;
            tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
            Reduce<iType, BinaryFunction> reduce_op();
            tbb::parallel_reduce( tbb::blocked_range<iType*>( &*first, (iType*)&*(last-1) + 1), reduce_op );
            return reduce_op.value;

        }

        template<typename InputIterator, typename T>
        T   reduce(InputIterator first,
            InputIterator last,
            T init)
        {

            typedef typename std::iterator_traits<InputIterator>::value_type iType;
            tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
            Reduce<iType, BinaryFunction> reduce_op(init);
            tbb::parallel_reduce( tbb::blocked_range<iType*>( &*first, (iType*)&*(last-1) + 1), reduce_op );
            return reduce_op.value;

        }


        template<typename InputIterator, typename T, typename BinaryFunction>
        T reduce(InputIterator first,
            InputIterator last,
            T init,
            BinaryFunction binary_op)
        {
            typedef typename std::iterator_traits<InputIterator>::value_type iType;
            tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
            Reduce<iType, BinaryFunction> reduce_op(binary_op, init);
            tbb::parallel_reduce( tbb::blocked_range<iType*>( &*first, (iType*)&*(last-1) + 1), reduce_op );
            return reduce_op.value;
        }


    } //tbb
} // bolt

#endif //BTBB_REDUCE_INL