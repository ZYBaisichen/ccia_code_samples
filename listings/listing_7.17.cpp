/*** 
 * @Author: baisichen
 * @Date: 2023-04-26 16:40:32
 * @LastEditTime: 2023-06-25 15:02:02
 * @LastEditors: baisichen
 * @Description: 
 */
#include <memory>
#include <atomic>
template<typename T>
class lock_free_queue
{
private:
#include "node.hpp"
    struct counted_node_ptr
    {
        int external_count;
        node* ptr;
    };
    
    std::atomic<counted_node_ptr> head;
    std::atomic<counted_node_ptr> tail;
public:
    std::unique_ptr<T> pop()
    {
        counted_node_ptr old_head=head.load(std::memory_order_relaxed);
        for(;;)
        {
            increase_external_count(head,old_head);
            node* const ptr=old_head.ptr;
            if(ptr==tail.load().ptr)
            {
                ptr->release_ref();
                return std::unique_ptr<T>();
            }
            if(head.compare_exchange_strong(old_head,ptr->next)) //这句话更多的作用感觉是将head原子的收为己有
            {
                T* const res=ptr->data.exchange(nullptr);
                free_external_counter(old_head);
                return std::unique_ptr<T>(res);
            }
            ptr->release_ref();
        }
    }
};
