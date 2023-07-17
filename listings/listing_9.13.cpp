#include <vector>
#include <mutex>
#include <thread>
#include <future>

class interrupt_flag
{
public:
    void set();
    bool is_set() const;
};
thread_local interrupt_flag this_thread_interrupt_flag;

class interruptible_thread
{
    std::thread internal_thread;
    interrupt_flag* flag;
public:
    template<typename FunctionType>
    interruptible_thread(FunctionType f)
    {
        std::promise<interrupt_flag*> p;
        internal_thread=std::thread([f,&p]{
                p.set_value(&this_thread_interrupt_flag);
                f(); //这里应该捕获一下
            });
        flag=p.get_future().get();
    }
    void interrupt()
    {
        if(flag)
        {
            flag->set();
        }
    }
};

std::mutex config_mutex;
std::vector<interruptible_thread> background_threads;

void background_thread(int disk_id)
{
    while(true)
    {
        interruption_point(); //先检查中断，如果中断则抛出异常
        fs_change fsc=get_fs_changes(disk_id); //获取文件变化
        if(fsc.has_changes()) //如果文件状态有改变，则更新索引
        {
            update_index(fsc); 
        }
    }
}

void start_background_processing()
{
    background_threads.push_back(
        interruptible_thread(background_thread,disk_1));
    background_threads.push_back(
        interruptible_thread(background_thread,disk_2));
}

int main()
{
    start_background_processing();
    process_gui_until_exit(); // ui程序
    std::unique_lock<std::mutex> lk(config_mutex);
    for(unsigned i=0;i<background_threads.size();++i)
    {
        //在所有后台更新状态的线程上执行中断，background_thread中的while循环就可以结束了
        //因为线程只会运行到下一个中断点才会结束运行，为提高性能，将所有线程都调用中断函数后才调用join等待所有线程结束
        background_threads[i].interrupt(); 
    }
    for(unsigned i=0;i<background_threads.size();++i)
    {
        background_threads[i].join(); //汇合所有线程
    }
}
