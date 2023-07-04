/*** 
 * @Author: baisichen
 * @Date: 2023-04-26 16:40:32
 * @LastEditTime: 2023-07-03 15:46:29
 * @LastEditors: baisichen
 * @Description: 
 */
#include <future>
#include <algorithm>
struct join_threads
{
    join_threads(std::vector<std::thread>&)
    {}
};

//这里找到的元素并不一定是出现在数组中的第一个，所以和find函数还是有一定的差异
template<typename Iterator,typename MatchType>
Iterator parallel_find(Iterator first,Iterator last,MatchType match) 
{
    struct find_element
    {
        void operator()(Iterator begin,Iterator end,
                        MatchType match,
                        std::promise<Iterator>* result,
                        std::atomic<bool>* done_flag)
        {
            try
            {
                for(;(begin!=end) && !done_flag->load();++begin) //每次itor迭代都会判断一次done_flag标记，一定程度上会减缓当前线程的执行速度。
                {
                    if(*begin==match)
                    {
                        result->set_value(begin);
                        done_flag->store(true); //找到设置标记，中断其他线程
                        return;
                    }
                }
            }
            catch(...)
            {
                try
                {
                    result->set_exception(std::current_exception());
                    done_flag->store(true); //抛出异常的话也做中断。
                }
                catch(...) //如果设置原子变量的过程中已经被其他线程设置，则会抛出异常；此时则不处理直接跳过
                {}
            }
        }
    };

    unsigned long const length=std::distance(first,last);

    if(!length)
        return last;

    unsigned long const min_per_thread=25;
    unsigned long const max_threads=
        (length+min_per_thread-1)/min_per_thread;

    unsigned long const hardware_threads=
        std::thread::hardware_concurrency();

    unsigned long const num_threads=
        std::min(hardware_threads!=0?hardware_threads:2,max_threads);

    unsigned long const block_size=length/num_threads;

    std::promise<Iterator> result;
    std::atomic<bool> done_flag(false);
    std::vector<std::thread> threads(num_threads-1);
    {
        join_threads joiner(threads); //离开当前作用域后就会在析构中调用join

        Iterator block_start=first;
        for(unsigned long i=0;i<(num_threads-1);++i)
        {
            Iterator block_end=block_start;
            std::advance(block_end,block_size);
            threads[i]=std::thread(find_element(),
                                   block_start,block_end,match,
                                   &result,&done_flag);
            block_start=block_end;
        }
        find_element()(block_start,last,match,&result,&done_flag);
    }
    if(!done_flag.load())
    {
        return last;
    }
    return result.get_future().get(); //如果找不到却没有join，也没有判断done_flag的话，这里就会一直等待；
}

