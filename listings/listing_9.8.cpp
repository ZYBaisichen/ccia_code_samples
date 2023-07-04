#include <vector>
#include <atomic>
#include <thread>
class thread_pool
{
    typedef function_wrapper task_type;
    
    std::atomic_bool done;
    thread_safe_queue<task_type> pool_work_queue;
    std::vector<std::unique_ptr<work_stealing_queue> > queues;
    std::vector<std::thread> threads;
    join_threads joiner;

    static thread_local work_stealing_queue* local_work_queue;
    static thread_local unsigned my_index;
   
    void worker_thread(unsigned my_index_)
    {
        my_index=my_index_;
        local_work_queue=queues[my_index].get();
        while(!done)
        {
            run_pending_task();
        }
    }

    bool pop_task_from_local_queue(task_type& task)
    {
        return local_work_queue && local_work_queue->try_pop(task);
    }

    bool pop_task_from_pool_queue(task_type& task)
    {
        return pool_work_queue.try_pop(task);
    }

    bool pop_task_from_other_thread_queue(task_type& task)
    {
        //逐个遍历其他线程局部队列，看是否能拿到任务
        for(unsigned i=0;i<queues.size();++i)
        {
            //每次都是从当前线程id的下一个线程开始检查窃取，避免所有的都从第一个线程开始检查
            unsigned const index=(my_index+i+1)%queues.size();
            if(queues[index]->try_steal(task))
            {
                return true;
            }
        }
        
        return false;
    }

public:
    thread_pool():
        joiner(threads),done(false)
    {
        unsigned const thread_count=std::thread::hardware_concurrency();

        try
        {
            for(unsigned i=0;i<thread_count;++i)
            {
                //将所有本地队列维护在一个数组中。线程池代为分配局部任务队列
                queues.push_back(std::unique_ptr<work_stealing_queue>(
                                     new work_stealing_queue)); 
                threads.push_back(
                    std::thread(&thread_pool::worker_thread,this,i));
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

    template<typename ResultType>
    using task_handle=std::unique_future<ResultType>;

    template<typename FunctionType>
    task_handle<std::result_of<FunctionType()>::type> submit(
        FunctionType f)
    {
        typedef std::result_of<FunctionType()>::type result_type;
        
        std::packaged_task<result_type()> task(f);
        task_handle<result_type> res(task.get_future());
        if(local_work_queue)
        {
            local_work_queue->push(std::move(task));
        }
        else
        {
            pool_work_queue.push(std::move(task));
        }
        return res;
    }

    void run_pending_task()
    {
        task_type task;
        if(pop_task_from_local_queue(task) ||
           pop_task_from_pool_queue(task) ||
           pop_task_from_other_thread_queue(task))
        {
            task();
        }
        else
        {
            std::this_thread::yield();
        }
    }
};
