template<typename T>
class lock_free_stack
{
private:
    struct node
    {
        std::shared_ptr<T> data; //节点内数据引用计数变成0后会自动释放，倒是省去了自己搞delete
        std::experimental::atomic_shared_ptr<node> next;
        node(T const& data_):
            data(std::make_shared<T>(data_))
        {}
    };
    std::experimental::atomic_shared_ptr<node> head; //在C++20中正式引入了偏特化的std::atomic<std::shared_ptr<T>>，取代std::experimental::atomic_shared_ptr<std::shared_ptr<T>>
public:
    void push(T const& data)
    {
        std::shared_ptr<node> const new_node=std::make_shared<node>(data);
        new_node->next=head.load();
        while(!head.compare_exchange_weak(new_node->next,new_node));
    }
    std::shared_ptr<T> pop()
    {
        std::shared_ptr<node> old_head=head.load();
        while(old_head && !head.compare_exchange_weak(
                  old_head,old_head->next.load()));
        if(old_head) {
            old_head->next=std::shared_ptr<node>();
            return old_head->data;
        }
        
        return std::shared_ptr<T>();
    }
    ~lock_free_stack(){
        while(pop());
    }
};
