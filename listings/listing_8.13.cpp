#include <atomic>
#include <thread>
#include <vector>

struct join_threads
{
    join_threads(std::vector<std::thread>&)
    {}
};
    

struct barrier
{
    std::atomic<unsigned> count;
    std::atomic<unsigned> spaces;
    std::atomic<unsigned> generation;
    barrier(unsigned count_):
        count(count_),spaces(count_),generation(0)
    {}
    void wait()
    {
        unsigned const gen=generation.load(); 
        if(!--spaces)
        {
            spaces=count.load();
            ++generation;
        }
        else
        {
            while(generation.load()==gen) //循环在这里等待generation的改变，简易自旋锁
            {
                std::this_thread::yield();
            }
        }
    }

    //调用该函数表示有个线程结束了任务
    void done_waiting()
    {
        --count;
        if(!--spaces)
        {
            spaces=count.load();
            ++generation;
        }
    }
};

template<typename Iterator>
void parallel_partial_sum(Iterator first,Iterator last)
{
    typedef typename Iterator::value_type value_type;

    struct process_element
    {
        void operator()(Iterator first,Iterator last,
                        std::vector<value_type>& buffer,
                        unsigned i,barrier& b)
        //这个函数内如果有异常会终止整个程序。可以再加一些promis和future传播异常
        {
            value_type& ith_element=*(first+i);
            bool update_source=false;
            //这里每个线程传递过来一个线程，这个线程负责累加前方所有元素到自己身上。但每一批次计算需要等所有线程计算完之后才能开始下一批次计算，因为每次计算出的数列之间有依赖，如下计算过程：
            // {1,2,3,4,5,6,7,8,9} -> {1,3,5,7,9,11,13,15,17} -> {1,3,6,10,14,18,22,26,30}
            // -> {1,3,5,10,15,21,28,36,44} -> {1,3,6,10,15,21,28,36,45}
            for(unsigned step=0,stride=1;stride<=i;++step,stride*=2)
            {
                value_type const& source=(step%2)?
                    buffer[i]:ith_element; //上次读了buffer，这次就读原数组的元素
                value_type& dest=(step%2)?
                    ith_element:buffer[i]; //上次写了原数组的元素，这次就写buffer AB缓存
                value_type const& addend=(step%2)?
                    buffer[i-stride]:*(first+i-stride);
                dest=source+addend;
                update_source=!(step%2);
                b.wait(); //每次加完之后都要等待其他线程也处理完，才能保证下一个步骤stride*=2能取到最新的值。
            }
            if(update_source)
            {
                ith_element=buffer[i];
            }
            b.done_waiting();
        }
    };

    unsigned long const length=std::distance(first,last);

    if(length<=1)
        return;

    std::vector<value_type> buffer(length);
    barrier b(length);
    std::vector<std::thread> threads(length-1); //元素数量有多少就开多少线程
    join_threads joiner(threads);

    Iterator block_start=first;
    for(unsigned long i=0;i<(length-1);++i)
    {
        threads[i]=std::thread(process_element(),first,last,
                               std::ref(buffer),i,std::ref(b));
    }
    process_element()(first,last,buffer,length-1,b);
}
