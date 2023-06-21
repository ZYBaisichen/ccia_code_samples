#include <atomic>
#include <functional>
template<typename T>
void do_delete(void* p)
{
    delete static_cast<T*>(p); //先将void指针转化成特定的指针型别，再执行释放操作
}
struct data_to_reclaim
{
    void* data;
    std::function<void(void*)> deleter;
    data_to_reclaim* next;
    template<typename T>
    data_to_reclaim(T* p):
        data(p),
        deleter(&do_delete<T>),
        next(0)
    {}
    ~data_to_reclaim()
    {
        deleter(data);
    }
};
std::atomic<data_to_reclaim*> nodes_to_reclaim;
void add_to_reclaim_list(data_to_reclaim* node)
{
    node->next=nodes_to_reclaim.load();
    while(!nodes_to_reclaim.compare_exchange_weak(node->next,node)); //将节点加入到待删列表的最前面
}
bool outstanding_hazard_pointers_for(void* data){
  return false;
}
template<typename T> //之所以要泛化，是因为要处理所有类型的值
void reclaim_later(T* data)
{
    add_to_reclaim_list(new data_to_reclaim(data)); //将待删节点封装成一个具备删除器的类。析构时调用删除器删除节点
}
void delete_nodes_with_no_hazards()
{
    data_to_reclaim* current=nodes_to_reclaim.exchange(nullptr); //将整个候删除链表收归当前线程所有
    while(current)
    {
        data_to_reclaim* const next=current->next;
        if(!outstanding_hazard_pointers_for(current->data)) //如果没有找到指涉当前节点的风险指针
        {
            delete current; 
        }
        else
        {
            add_to_reclaim_list(current); //如果还有风险指针指涉，则还回去
        }
        current=next;
    }
}
