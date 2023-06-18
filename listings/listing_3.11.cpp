/*** 
 * @Author: baisichen
 * @Date: 2023-04-26 16:40:32
 * @LastEditTime: 2023-05-31 21:17:55
 * @LastEditors: baisichen
 * @Description: 
 */
#include <memory>
#include <mutex>

struct some_resource
{
    void do_something()
    {}
    
};


std::shared_ptr<some_resource> resource_ptr;
std::mutex resource_mutex;
void foo()
{
    std::unique_lock<std::mutex> lk(resource_mutex); //迫使每个线程都要加一次锁才能看resource_ptr，有人改进成双重检验锁定模式，但一定要配合内存顺序来使用
    if(!resource_ptr)
    {
        resource_ptr.reset(new some_resource);
    }
    lk.unlock();
    resource_ptr->do_something();
}

int main()
{
    foo();
}
