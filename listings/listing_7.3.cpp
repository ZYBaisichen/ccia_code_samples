/*** 
 * @Author: baisichen
 * @Date: 2023-04-26 16:40:32
 * @LastEditTime: 2023-06-20 11:14:37
 * @LastEditors: baisichen
 * @Description: 
 */
#include <atomic>
#include <memory>

template<typename T>
class lock_free_stack
{
private:
    struct node
    {
        std::shared_ptr<T> data;
        node* next;
        node(T const& data_):
            data(std::make_shared<T>(data_))
        {}
    };
    std::atomic<node*> head;
public:
    void push(T const& data)
    {
        node* const new_node=new node(data);
        new_node->next=head.load();
        while(!head.compare_exchange_weak(new_node->next,new_node));
    }

    // void pop(T& result) {
    //     /*
    //     问题：
    //         1. 没有判断head是否为空而直接使用了->next，容易报出异常
    //         2. 次pop的data是基于的对象来的，所以传递进来的是个对象，所以会触发复制构造。需要确保栈容器上只有当前线程时，做赋值构造才是安全的(这句话没理解)
    //             T data;
    //             pop(data);
    //             process(data);
    //     */
    //     node* old_head = head.load(); 
    //     while(!head.compare_exchange_weak(old_head, old_head->next));
    //     result=old_head->data; 
    // }
    

    std::shared_ptr<T> pop()
    {
        node* old_head=head.load();//这里存在内存泄露，如果当前线程执行到最下面，并释放了head。其他线程才执行到load这句话
        //判断了old_head是否为空
        while(old_head &&
              !head.compare_exchange_weak(old_head,old_head->next));  //并非免等实现，如果这里的weak一直返回false，就会一直等待
        //返回智能指针，避免了赋值构造 
        return old_head ? old_head->data : std::shared_ptr<T>(); 

    }
};
