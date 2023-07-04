#include <deque>
#include <future>
#include <memory>
#include <functional>
#include <iostream>
#include <iostream>

class function_wrapper //包装成了任务类，可以拷贝也可以移动。
{
    struct impl_base {
        virtual void call()=0;
        virtual ~impl_base() {}
    };
    std::unique_ptr<impl_base> impl;
    template<typename F>
    struct impl_type: impl_base
    {
        F f;
        impl_type(F&& f_): f(std::move(f_)) {}
        void call() { f(); }
    };
public:
    template<typename F>
    function_wrapper(F&& f):
        impl(new impl_type<F>(std::move(f)))
    {}

    void call() { impl->call(); }

    function_wrapper(function_wrapper&& other):
        impl(std::move(other.impl))
    {}

    function_wrapper& operator=(function_wrapper&& other)
    {
        impl=std::move(other.impl);
        return *this;
    }

    function_wrapper(const function_wrapper&)=delete;
    function_wrapper(function_wrapper&)=delete;
    function_wrapper& operator=(const function_wrapper&)=delete;
};

class thread_pool
{

public:
    std::deque<function_wrapper> work_queue; //这里使用wrapper
    /*
    void worker_thread() {
        function_wrapper task;
        if (work_queue.try_pop(task)) {
            task();
        } else {
            std::this_thread::yield();
        }

    }*/
    template<typename FunctionType>
    std::future<typename std::result_of<FunctionType()>::type>
    submit(FunctionType f) //调用submit后会返回一个future实例。std::result_of<FunctionType()>::type可以返回FunctionType函数的返回型别
    {
        typedef typename std::result_of<FunctionType()>::type result_type;
        
        std::packaged_task<result_type()> task(std::move(f));
        std::future<result_type> res(task.get_future());
        work_queue.push_back(std::move(task));
        return res; //将future句柄返回
    }
    // rest as before
};
