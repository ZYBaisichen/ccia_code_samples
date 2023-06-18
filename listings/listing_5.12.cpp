#include <atomic>
#include <thread>
#include <assert.h>

std::atomic<bool> x,y;
std::atomic<int> z;

void write_x_then_y()
{
    x.store(true,std::memory_order_relaxed); //1
    //此释放栅栏和下面的获取栅栏形成同步关系，构成1->3->4->6的同步序列，后面操作一定可以看到前面的操作的结果
    std::atomic_thread_fence(std::memory_order_release);//2
    y.store(true,std::memory_order_relaxed);//3
}

void read_y_then_x()
{
    while(!y.load(std::memory_order_relaxed));//4
    std::atomic_thread_fence(std::memory_order_acquire);//5
    if(x.load(std::memory_order_relaxed))//6
        ++z;
}

int main()
{
    x=false;
    y=false;
    z=0;
    std::thread a(write_x_then_y);
    std::thread b(read_y_then_x);
    a.join();
    b.join();
    assert(z.load()!=0);
}
