#include <atomic>
#include <thread>
#include <assert.h>

std::atomic<bool> x,y;
std::atomic<int> z;

void write_x_then_y()
{
    x.store(true,std::memory_order_relaxed);
    y.store(true,std::memory_order_release); //这里相当于有一个写内存屏障，将前面的写操作从store buffer写回到主存中，保证后面的可见
    //y.store(true,std::memory_order_relaxed); //如果这里换成了relaxed，那就不保证前面的写操作一定可以写回主存了
}

void read_y_then_x()
{
    while(!y.load(std::memory_order_acquire));//这里相当于有一个读内存屏障, 将invalidate que都处理完，保证可以读取到最新的值
    //while(!y.load(std::memory_order_relaxed)); //如果这里换成了relaxed，就不保证invalidate一定可以处理完，即后面的load操作不一定可以读到最新的值
    if(x.load(std::memory_order_relaxed))
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
