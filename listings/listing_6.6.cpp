#include <memory>
#include <mutex>

/*
不变量：
tail->next==nullptr
tail->data==nullptr
head==tail说明队列为空
在单元素队列中，head->next==tail
对于每个节点x，只要x!=tail，则x->data指向一个T类型的实例，且x->next指向后续节点
x->next==tail说明x是最后一个节点。从head指针指向的节点出发，我们沿着next指针反复访问后继节点，最终会到达tail指针指向的节点

可以看出下面所有的函数都没有破坏这些不变量，都对共享数据进行了有效的保护
*/
template<typename T>
class threadsafe_queue
{
private:
    struct node
    {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };
    
    std::mutex head_mutex;
    std::unique_ptr<node> head;
    std::mutex tail_mutex;
    node* tail;
    
    node* get_tail()
    {
        std::lock_guard<std::mutex> tail_lock(tail_mutex); 
        return tail;
    }

    std::unique_ptr<node> pop_head()
    {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        //安全性：get_tail一定要在head_lock保护之内加锁，因为如果先get_tail，其他线程可能会一直pop，将head越过了tail到了虚位节点，导致未定义行为
        //死锁：因为每次都是先获取head_mutex在获取tail_mutex，所以获取锁的顺序一致，不会有死锁风险
        //因为用到tail_mutex锁的地方只有push和try_pop，而try_pop这里对tail的加锁时间很短，几乎是取到指针就立刻释放了锁，所以push和try_pop几乎可以并行执行
        if(head.get()==get_tail()) //get_tail最小粒度的对tail加锁.
        {
            return nullptr;
        }
        std::unique_ptr<node> const old_head=std::move(head);
        head=std::move(old_head->next);
        return old_head;
    }
        

public:
    threadsafe_queue():
        head(new node),tail(head.get())
    {}

    threadsafe_queue(const threadsafe_queue& other)=delete;
    threadsafe_queue& operator=(const threadsafe_queue& other)=delete;

    std::shared_ptr<T> try_pop()
    {
        //异常安全：仅有互斥加锁才会抛出异常，但只有互斥加锁之后才会对数据做操作，所以异常安全
        //这里pop出来的old_head使用的是unique_ptr，node的析构操作耗时很大，所以放在了锁之外，提高了效率
        std::unique_ptr<node> old_head=pop_head(); 
        return old_head?old_head->data:std::shared_ptr<T>(); //将返回挪到锁保护之外
    }
    
    void push(T new_value)
    {   
        //异常安全：在创建new_data和p时可能会抛出异常，但因为使用的是智能指针，有异常会被自动释放。获取锁后，余下的操作都不会抛出异常
        //效率：在加锁之前，为对象完成了内存分配，因为每次push只会涉及到几个指针的操作，所以这部分加锁时间很短。
        std::shared_ptr<T> new_data(
            std::make_shared<T>(std::move(new_value)));
        std::unique_ptr<node> p(new node);
        node* const new_tail=p.get();
        std::lock_guard<std::mutex> tail_lock(tail_mutex); //虚位节点的构造放在加锁之前
        tail->data=new_data;
        tail->next=std::move(p);
        tail=new_tail;
    }
};
