#include <atomic>

template<typename T>
class lock_free_stack
{
private:
    struct node
    {
        T data;
        node* next;
        node(T const& data_):
            data(data_)
        {}
    };
    std::atomic<node*> head;
public:
    void push(T const& data)
    {
        node* const new_node=new node(data);
        new_node->next=head.load();
        //为保证head一旦更新，其他线程能够立刻访问；所以预选准备好新节点的new操作（耗时）
        /*
            异常安全性：唯一的可以抛出异常的就是new node处，但如果失败此时还没加入链表不会影响链表结构
            恶性竞争：compare_exchange_weak是原子的，并且会将其他线程更改的head体现在当前修改中
            死锁：不含锁，所以不会造成死锁
            优点是compare_exchange_weak不会阻塞掉其他线程更新head。当前线程也可以在很大概率上原子赋值成功
        */
        while(!head.compare_exchange_weak(new_node->next,new_node)); //修改head，当前拿到的head值是new_node->next，如果其他线程改了head，则compare_exchange_weak返回false，同时将new_node->next赋值为最新的head，继续尝试修改head
    }
};
