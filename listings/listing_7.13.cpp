#include <atomic>
#include <memory>

template<typename T>
class lock_free_stack
{
private:
    struct node;
    struct counted_node_ptr
    {
        int external_count;
        node* ptr;
    };
    struct node
    {
        std::shared_ptr<T> data;
        std::atomic<int> internal_count;
        counted_node_ptr next;
        node(T const& data_):
            data(std::make_shared<T>(data_)),
            internal_count(0)
        {}
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
        while(!head.compare_exchange_strong(
                  old_counter,new_counter,
                  std::memory_order_acquire,
                  std::memory_order_relaxed));
        old_counter.external_count=new_counter.external_count;
    }
public:
    ~lock_free_stack()
    {
        while(pop());
    }
    void push(T const& data)
    {
        counted_node_ptr new_node;
        new_node.ptr=new node(data);//1
        new_node.external_count=1;//2
        //因为这里还没加入到栈中，节点为当前线程独占，并且下面的head更改操作会原子的更新ptr->next，所以使用relaxed就好。
        new_node.ptr->next=head.load(std::memory_order_relaxed); //3
            while(!head.compare_exchange_weak( //4
                      new_node.ptr->next,new_node,
                      std::memory_order_release,
                      std::memory_order_relaxed));
    }
    std::shared_ptr<T> pop()
    {
        counted_node_ptr old_head=
            head.load(std::memory_order_relaxed);
        for(;;)
        {
            //访问下面的ptr和ptr->next之前需要增加acquire次序，所以在这个函数里面加了acquire次序，下面的compare_exchange_strong调用就不用加了
            increase_head_count(old_head); //5
            node* const ptr=old_head.ptr;
            if(!ptr)
            {
                return std::shared_ptr<T>();
            }
            //如果失败，下一次接触到head，也是在下个循环，下个循环同样有increase_head_count函数里的acquire次序在前面保证
            if(head.compare_exchange_strong(
                   old_head,ptr->next,std::memory_order_relaxed)) //6
            {
                std::shared_ptr<T> res;
                //这里访问了ptr->data，需要在访问它前使用push准备好；因为7、6在5后面，5跨线程的在4后面，1、2、3在4前面，所以构成了完整的依赖次序1,2,3,4,5,6,7，从而保证了push中data的可见性
                res.swap(ptr->data); //7
                int const count_increase=old_head.external_count-2;
                /*还是根据笔记中的时序图，跨线程的依赖是7和11, 只要保证在删除之前swap出了节点即可，形成了跨线程的依赖。
                  按理说只需8处使用release，9处使用acquire即可，但因为只会有一个线程能够进入到internal为0的地方执行删除ptr语句，
                  所以这样的次序太过严格，根据第5章的知识(笔记4.3.x或书5.3.4)，读改写操作构成的释放序列中，只需要最后一个load和第一个store构成同步关系即可，所以10处增加了acquire，9处就可以使用relaxed了。
                  这样就构成7,8,10,11的内存次序，保证了在删除ptr之前，swap除了它的data；同时也增加了效率，只有一个线程可以进入11处删除节点
                */
                if(ptr->internal_count.fetch_add(
                       count_increase,std::memory_order_release)==-count_increase) //8
                {
                    delete ptr; //这里的删除只和本线程有关，和4、5、6、7构成了释放序列
                }
                return res;
            }
            else if(ptr->internal_count.fetch_add(
                        -1,std::memory_order_relaxed)==1) //9
            {
                ptr->internal_count.load(std::memory_order_acquire); //10
                delete ptr; //11
            }
        }
    }
};
