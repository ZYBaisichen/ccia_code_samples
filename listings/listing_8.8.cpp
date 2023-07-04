/*** 
 * @Author: baisichen
 * @Date: 2023-04-26 16:40:32
 * @LastEditTime: 2023-07-03 15:38:36
 * @LastEditors: baisichen
 * @Description: 
 */
#include <future>
#include <algorithm>
template<typename Iterator,typename Func>
void parallel_for_each(Iterator first,Iterator last,Func f)
{
    unsigned long const length=std::distance(first,last);

    if(!length)
        return;

    unsigned long const min_per_thread=25;

    if(length<(2*min_per_thread))
    {
        std::for_each(first,last,f); //当最后不可以再分的时候就直接调用for_each
    }
    else
    {
        Iterator const mid_point=first+length/2; //这种划分方式是不均匀的, 越往后越少, 数据会倾斜
        std::future<void> first_half=
            std::async(&parallel_for_each<Iterator,Func>,
                       first,mid_point,f); //这里可以由系统决定是否创建新的线程
        parallel_for_each(mid_point,last,f);
        first_half.get();
    }
}
