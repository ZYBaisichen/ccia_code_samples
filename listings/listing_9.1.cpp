#include <thread>
#include <vector>
#include <atomic>
struct join_threads
{
    join_threads(std::vector<std::thread>&)
    {}
};

//此线程池子可以完成基本的功能，适合的场景：彼此完全独立，也没有任何返回值、且不执行阻塞操作。
class thread_pool
{
    std::atomic_bool done; 
    thread_safe_queue<std::function<void()> > work_queue;
    std::vector<std::thread> threads;
    join_threads joiner; //这里的声明顺序需要确定，done最前面，joiner在最后，确保销毁done前joiner析构，从而确保所有线程可以正确结束

    void worker_thread()
    {
        while(!done)
        {
            std::function<void()> task;
            if(work_queue.try_pop(task))
            {
                task();
            }
            else
            {
                std::this_thread::yield();
            }
        }
    }
public:
    thread_pool():
        done(false),joiner(threads)
    {
        unsigned const thread_count=std::thread::hardware_concurrency();
        try
        {
            for(unsigned i=0;i<thread_count;++i)
            {
                threads.push_back(
                    std::thread(&thread_pool::worker_thread,this));
            }
        }
        catch(...)
        {
            done=true;
            throw;
        }
    }

    ~thread_pool()
    {
        done=true;
    }

    template<typename FunctionType>
    void submit(FunctionType f)
    {
        work_queue.push(std::function<void()>(f));
    }
};
