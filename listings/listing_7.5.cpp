#include <atomic>

template<typename T>
//在高负荷场景中，静态工作点也始终无法出现，原因是线程不可能主动等待，他们不会等到别的线程全部执行完pop(),离开这个函数之后，才发起pop调用进入。
//这样一来，to_be_delete将会无休止的增加，导致实际上的内存泄露。万一不存在任何静态工作点，我们就需要寻求不同的方法来回收节点。一种机制就是通过后面的风险指针实现。
class lock_free_stack
{
private:
    std::atomic<node*> to_be_deleted;
    static void delete_nodes(node* nodes)
    {
        while(nodes)
        {
            node* next=nodes->next;
            delete nodes;
            nodes=next;
        }
    }
    void try_reclaim(node* old_head)
    {
        if(threads_in_pop==1) //1
        {   
            //将候选删除链表收归己有，to_be_deleted在2处被改为null，其他线程拿到的候选删除链表是空的，相当于执行完下面的exchange之后，候选删除链表就被当前线程独占了
            node* nodes_to_delete=to_be_deleted.exchange(nullptr); //2 
            //如果不再判断一次，让1和2之间可能会有其他线程进入，并插入新的节点Y，如果还有其他指向Y的线程。导致本线程删除的之后，那个用Y的线程就会操作空指针，所以要做这一次双重验证，和锁的双重检查机制一样
            if(!--threads_in_pop)
            {
                delete_nodes(nodes_to_delete); //只有当前线程在使用stack的pop，直接删除候选链表中所有的节点
            }
            else if(nodes_to_delete)
            {
                chain_pending_nodes(nodes_to_delete); //如果有其他线程重入，则将delete链表重新放回to_be_deleted
            }
            //在1处判断了当前计数器的值为1，就表明仅有当前一个线程正在调用pop()， 那么可以安全删除刚刚弹出的节点。注意这里对old_head的处理手法。先释放候选链表，再删除old_head
            //如果threads_in_pop等于1，候选链表就不加入old_head节点了，而是直接删除。这样便跳过了添加动作，从而提高了效率。
            delete old_head; //7 
        }
        else
        {
            chain_pending_node(old_head); //12 向链表中添加单个节点，old_node的next会指向候选链表中的头(to_be_deleted)， 链表中的头会再等于last->next的时候改变为old_head，是典型的头插法，在chain_pending_nodes函数中也成立
            --threads_in_pop;
        }
    }
    void chain_pending_nodes(node* nodes)
    {
        node* last=nodes;
        while(node* const next=last->next)
        {
            last=next;
        }
        chain_pending_nodes(nodes,last); //将要删除的节点，放在最后
    }
    void chain_pending_nodes(node* first,node* last)
    {
        last->next=to_be_deleted; //先给一个虚位节点，将deleted赋值为first
        //下面这段代码也很巧妙，last为候选链表的最后，如果有其他线程想候删链表添加了元素，则last->next的值会变化，会复制给to_be_deleted，最终to_be_deleted和last->next相等，并将其赋值给第一个节点
        while(!to_be_deleted.compare_exchange_weak(
                  last->next,first));
    }
    void chain_pending_node(node* n)
    { 
        chain_pending_nodes(n,n);
    }
};
