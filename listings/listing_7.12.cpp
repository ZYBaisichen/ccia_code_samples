/*** 
 * @Author: baisichen
 * @Date: 2023-04-26 16:40:32
 * @LastEditTime: 2023-06-21 11:11:50
 * @LastEditors: baisichen
 * @Description: 
 */
#include <memory>
#include <atomic>

template<typename T>
class lock_free_stack
{
private:
    struct counted_node_ptr
    {
        int external_count; //使用外部引用，是为了防止在外面自增期间，内部ptr指针无效
        node* ptr;
    };
    std::atomic<counted_node_ptr> head;

    void increase_head_count(counted_node_ptr& old_counter)
    {
        counted_node_ptr new_counter;
        do
        {
            new_counter=old_counter;
            ++new_counter.external_count;
        }
        while(!head.compare_exchange_strong(old_counter,new_counter)); //防止更新前被别的线程先更新
        old_counter.external_count=new_counter.external_count;
    }
public:
    std::shared_ptr<T> pop()
    {
        counted_node_ptr old_head=head.load();//自增之前就读取了。这期间有可能有别的线程删除它
        for(;;)
        {
            increase_head_count(old_head); //递增外部引用，表示当前head正在被指涉
            node* const ptr=old_head.ptr;
            if(!ptr)
            {
                return std::shared_ptr<T>();
            }
            if(head.compare_exchange_strong(old_head,ptr->next))
            {
                std::shared_ptr<T> res;
                res.swap(ptr->data);
                int const count_increase=old_head.external_count-2; //链表里的元素不在指涉-1，当前线程退出时还要再减1
                if(ptr->internal_count.fetch_add(count_increase)==
                   -count_increase) //fetch_add返回的是加之前的值
                {
                    delete ptr;
                }
                return res;
            }
            else if(ptr->internal_count.fetch_sub(1)==1) //fetch_sub返回的是减之前的值
            {
                delete ptr;
            }
        }
    }
};
