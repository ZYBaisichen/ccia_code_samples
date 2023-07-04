#include <memory>
#include <atomic>

template<typename T>
class lock_free_queue
{
private:
    struct node
    {
        std::shared_ptr<T> data;
        node* next;
        node():
            next(nullptr)
        {}
    };
    std::atomic<node*> head;
    std::atomic<node*> tail;
    node* pop_head()
    {
        node* const old_head=head.load();
        if(old_head==tail.load())
        {
            return nullptr;
        }
        head.store(old_head->next);
        return old_head;
    }
public:
    lock_free_queue():
        head(new node),tail(head.load())
    {}
    lock_free_queue(const lock_free_queue& other)=delete;
    lock_free_queue& operator=(const lock_free_queue& other)=delete;
    ~lock_free_queue()
    {
        while(node* const old_head=head.load())
        {
            head.store(old_head->next);
            delete old_head;
        }
    }
    std::shared_ptr<T> pop()
    {
        node* old_head=pop_head();
        if(!old_head)
        {
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> const res(old_head->data);
        delete old_head;
        return res;
    }
    //相较于无锁栈，push和pop是只对head做操作，所以只有一个引用计数即可解决所有的内存回收问题
    //而无锁队列有两个指针head和tail，这样就会复杂了，要有一个引用计数表示head出队，还要有一个引用计数作用于tail上
    void push(T new_value)
    {
        std::shared_ptr<T> new_data(std::make_shared<T>(new_value));
        node* p=new node;
        node* const old_tail=tail.load();
        old_tail->data.swap(new_data);
        old_tail->next=p; 
        tail.store(p); //直接store的话其他线程可能会抢先执行，造成失败。
    }
};
