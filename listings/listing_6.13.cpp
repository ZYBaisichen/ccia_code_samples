#include <memory>
#include <mutex>

template<typename T>
class threadsafe_list
{
    struct node
    {
        std::mutex m;
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;

        node():
            next()
        {}
        
        node(T const& value):
            data(std::make_shared<T>(value))
        {}
    };
    
    node head;

public:
    threadsafe_list()
    {}

    ~threadsafe_list()
    {
        remove_if([](T const&){return true;});
    }
    
    threadsafe_list(threadsafe_list const& other)=delete;
    threadsafe_list& operator=(threadsafe_list const& other)=delete;

    //不同的线程可以在不同的节点上工作，无论是使用for_each、还是find_first_if进行查找或通过remove_if删除节点，互斥的加锁顺序必须链表中的前后顺序进行，否则可能会出现死锁。
    
    void push_front(T const& value)
    {
        std::unique_ptr<node> new_node(new node(value));
        std::lock_guard<std::mutex> lk(head.m);
        new_node->next=std::move(head.next);
        head.next=std::move(new_node);
    }

    template<typename Function>
    void for_each(Function f)
    {
        node* current=&head;
        std::unique_lock<std::mutex> lk(head.m); //交替加锁
        while(node* const next=current->next.get())
        {
            std::unique_lock<std::mutex> next_lk(next->m);
            lk.unlock(); //一旦锁住了下一个节点的锁，就释放上一个节点的锁
            f(*next->data);
            current=next;
            lk=std::move(next_lk); //锁转移，避免重复获取锁
        }
    }

    template<typename Predicate>
    std::shared_ptr<T> find_first_if(Predicate p)
    {
        node* current=&head;
        std::unique_lock<std::mutex> lk(head.m);
        while(node* const next=current->next.get())
        {
            std::unique_lock<std::mutex> next_lk(next->m);
            lk.unlock();
            if(p(*next->data))
            {
                return next->data;
            }
            current=next;
            lk=std::move(next_lk);
        }
        return std::shared_ptr<T>();
    }

    template<typename Predicate>
    void remove_if(Predicate p)
    {
        node* current=&head;
        std::unique_lock<std::mutex> lk(head.m);
        while(node* const next=current->next.get())
        {
            std::unique_lock<std::mutex> next_lk(next->m);
            if(p(*next->data))
            {
                std::unique_ptr<node> old_next=std::move(current->next); //移出来自动销毁。但是在解锁之后销毁，可能存在竞争。但在释放锁之后立刻销毁，中间间隙很短，其他线程很难插入
                current->next=std::move(next->next);
                next_lk.unlock();
            }
            else
            {
                lk.unlock();
                current=next;
                lk=std::move(next_lk);
            }
        }
    }
};
