#include <atomic>
#include <thread>
#include <vector>
std::vector<int> queue_data;
std::atomic<int> count;

void wait_for_more_items() {}
void process(int data){}

void populate_queue()
{
    unsigned const number_of_items=20;
    queue_data.clear();
    for(unsigned i=0;i<number_of_items;++i)
    {
        queue_data.push_back(i);
    }
    
    count.store(number_of_items,std::memory_order_release);
}

void consume_queue_items()
{
    while(true)
    {
        int item_index;
        //当有多个线程时构成串行的原子读改写序列，后一个线程读取的是前一个写出的结果
        /*

            1. 假设这一规则不成立，两个消费者线程不构成先行关系，进而读取共享缓冲的并发读取就不再安全，除非前一个fetch_sub也采用memory_order_release程序，但会在两个线程间引发过分严格的同步，没有必要，且有可能引起串行执行。
            2. 另外要是规则不成立的话或者fetch_sub采用memory_order_release次序，那么queue上存储的值，只能为第一个线程所见，不能被第二个线程所见，这就类似于锁了，引起了多线程竞争这个锁。
            3. 事实是第一个线程的fetch_sub在释放序列中，其store为第二个线程可见，但两个fetch_sub不构成同步关系，他们都是和store同步，但如果并行执行fetch_sub依旧会有次序关系，这是内存模型和cpu为我们实现的特性

        */

        if((item_index=count.fetch_sub(1,std::memory_order_acquire))<=0) 
        {
            wait_for_more_items();
            continue;
        }
        process(queue_data[item_index-1]);
    }
}

int main()
{
    std::thread a(populate_queue);
    std::thread b(consume_queue_items);
    std::thread c(consume_queue_items);
    a.join();
    b.join();
    c.join();
}
