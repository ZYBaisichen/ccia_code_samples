/*** 
 * @Author: baisichen
 * @Date: 2023-04-26 16:40:32
 * @LastEditTime: 2023-07-03 11:47:04
 * @LastEditors: baisichen
 * @Description: 
 */
#include <vector>
#include <thread>
#include <algorithm>
template<typename Iterator,typename T>
struct accumulate_block
{
    void operator()(Iterator first,Iterator last,T& result)
    {
        result=std::accumulate(first,last,result);//这里也有可能发生异常
    }
};

template<typename Iterator,typename T>
T parallel_accumulate(Iterator first,Iterator last,T init)
{
    unsigned long const length=std::distance(first,last); //在多线程开始前，抛异常也没事

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

    std::vector<T> results(num_threads);
    std::vector<std::thread> threads(num_threads-1); //在单线程中分配内存，发生异常后，析构时自动放回内存

    Iterator block_start=first;
    for(unsigned long i=0;i<(num_threads-1);++i)
    {
        Iterator block_end=block_start;
        std::advance(block_end,block_size);
        threads[i]=std::thread( //如果这里发生了异常thread对象就会调用析构函数，进而调用std::terminate()函数终结整个程序
            accumulate_block<Iterator,T>(),
            block_start,block_end,std::ref(results[i]));
        block_start=block_end;
    }
    accumulate_block<Iterator,T>()(block_start,last,results[num_threads-1]);

    std::for_each(threads.begin(),threads.end(),
                  std::mem_fn(&std::thread::join));

    return std::accumulate(results.begin(),results.end(),init); //最后这里不会有问题
}
