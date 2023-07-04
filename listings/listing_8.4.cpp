/*** 
 * @Author: baisichen
 * @Date: 2023-04-26 16:40:32
 * @LastEditTime: 2023-07-03 11:59:40
 * @LastEditors: baisichen
 * @Description: 
 */
#include <iterator>
#include <vector>
#include <thread>
#include <future>
#include <numeric>

struct join_threads
{
    join_threads(std::vector<std::thread>&)
    {}
    ~join_threads() //C++惯用手法，在析构的时候做一些事情，这里是join，确保退出时所有的线程都汇合到主线程
    {
        for (unsigned long int i=0;i<threads.size();++i) {
            if(threads[i].joinable()) {
                threads[i].join();
            }
        }
    }
public:
    std::vector<std::thread> threads;
};
    
template<typename Iterator,typename T>
struct accumulate_block
{
    T operator()(Iterator first,Iterator last)
    {
        return std::accumulate(first,last,T());
    }
};
template<typename Iterator,typename T>
T parallel_accumulate(Iterator first,Iterator last,T init)
{
    unsigned long const length=std::distance(first,last);

    if(!length)
        return init;

    unsigned long const min_per_thread=25;
    unsigned long const max_threads=
        (length+min_per_thread-1)/min_per_thread;

    unsigned long const hardware_threads=
        std::thread::hardware_concurrency();

    unsigned long const num_threads=
        std::min(hardware_threads!=0?hardware_threads:2,max_threads);

    unsigned long const block_size=length/num_threads;

    std::vector<std::future<T> > futures(num_threads-1);
    std::vector<std::thread> threads(num_threads-1);
    join_threads joiner(threads);

    Iterator block_start=first;
    for(unsigned long i=0;i<(num_threads-1);++i)
    {
        Iterator block_end=block_start;
        std::advance(block_end,block_size);
        std::packaged_task<T(Iterator,Iterator)> task(
            accumulate_block<Iterator,T>());
        futures[i]=task.get_future();
        threads[i]=std::thread(std::move(task),block_start,block_end);
        block_start=block_end;
    }
    T last_result=accumulate_block<Iterator,T>(block_start,last);
    T result=init;
    for(unsigned long i=0;i<(num_threads-1);++i)
    {
        result+=futures[i].get();
    }
    result += last_result;
    return result;
}
