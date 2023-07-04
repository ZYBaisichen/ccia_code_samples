/*** 
 * @Author: baisichen
 * @Date: 2023-04-26 16:40:32
 * @LastEditTime: 2023-07-04 11:36:03
 * @LastEditors: baisichen
 * @Description: 
 */
class thread_pool
{
    thread_safe_queue<function_wrapper> pool_work_queue;

    typedef std::queue<function_wrapper> local_queue_type;
    static thread_local std::unique_ptr<local_queue_type>
        local_work_queue; //每个池内线程都会有一个unique_ptr指针指向其专属队列。这里线程中只使用了unique_ptr，而没有直接使用std::queue<function_wrapper>对象，可以节省较大空间
   
    void worker_thread() //此函数是线程池内每个线程循环执行的，池外的线程不会执行，也就不会new local_queue_type
    {
        local_work_queue.reset(new local_queue_type); //只有在每个线程内部才会new出来local_queue
        
        while(!done)
        {
            run_pending_task();
        }
    }

public:
    template<typename FunctionType>
    std::future<std::result_of<FunctionType()>::type>
        submit(FunctionType f)
    {
        typedef std::result_of<FunctionType()>::type result_type;
        
        std::packaged_task<result_type()> task(f); //传递给线程池子队列的为一个对象，即一个任务
        std::future<result_type> res(task.get_future());
        if(local_work_queue) //如果是池内线程，则提交到自己的队列中
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
        function_wrapper task;
        if(local_work_queue && !local_work_queue->empty()) 
        {
            task=std::move(local_work_queue->front());
            local_work_queue->pop();
            task();
        }
        else if(pool_work_queue.try_pop(task)) //池内线程只有在自己的队列中没有任务时才会从全局队列中取任务
        {
            task();
        }
        else
        {
            std::this_thread::yield();
        }
    }
    // rest as before
};
