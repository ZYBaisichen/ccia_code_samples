#include <thread>
#include <atomic>
class barrier
{
    unsigned const count; //座位数目 
    std::atomic<unsigned> spaces; //空闲座位数，起初为count，即所有座位都是空的
    std::atomic<unsigned> generation;
public:
    explicit barrier(unsigned count_):
        count(count_),spaces(count),generation(0)
    {}
    void wait() //调用数量超过了count，就会报错。最好还是采用std::barrier
    {
        unsigned const my_generation=generation;//隐式调用了load
        if(!--spaces)
        {
            spaces=count;//如果空闲作为数变为了0，则重置为0
            ++generation; //只有当全部线程都调用wait运行到线程卡处，才更新generation。
        }
        else
        {
            while(generation==my_generation) //如果还有线程没到，则以自旋锁的方式等待
                std::this_thread::yield(); //主动让出cpu占用
        }
    }
};

