/*** 
 * @Author: baisichen
 * @Date: 2023-04-26 16:40:32
 * @LastEditTime: 2023-07-03 15:54:53
 * @LastEditors: baisichen
 * @Description: 
 */
#include <atomic>
#include <future>
template<typename Iterator,typename MatchType>
Iterator parallel_find_impl(Iterator first,Iterator last,MatchType match,
                            std::atomic<bool>& done) //传入是否完成的引用标志
{
    try
    {
        unsigned long const length=std::distance(first,last);
        unsigned long const min_per_thread=25;
        if(length<(2*min_per_thread)) //单一线程处理的最小数据量
        {
            for(;(first!=last) && !done.load();++first)
            {
                if(*first==match)
                {
                    done=true;
                    return first;
                }
            }
            return last;
        }
        else
        {
            Iterator const mid_point=first+(length/2);
            //async能自行决定是否新建线程做处理还是使用当前线程
            std::future<Iterator> async_result=
                std::async(&parallel_find_impl<Iterator,MatchType>,
                           mid_point,last,match,std::ref(done)); //async_result保证了在析构时，先终结执行异常的线程，然后向外传播异常
            Iterator const direct_result=
                parallel_find_impl(first,mid_point,match,done);
            return (direct_result==mid_point)?
                async_result.get():direct_result; //如果递归调用的结果刚好等于mid_point节点，说明没找到。否则返回找到的res
        }
    }
    catch(...)
    {
        done=true;
        throw;
    }
}

template<typename Iterator,typename MatchType>
Iterator parallel_find(Iterator first,Iterator last,MatchType match)
{
    std::atomic<bool> done(false);
    return parallel_find_impl(first,last,match,done);
}
