void test_concurrent_push_and_pop_on_empty_queue()
{
    threadsafe_queue<int> q;

    std::promise<void> go,push_ready,pop_ready;
    std::shared_future<void> ready(go.get_future());

    std::future<void> push_done;
    std::future<int> pop_done;

    try
    {
        push_done=std::async(std::launch::async,
                             [&q,ready,&push_ready]()
                             {
                                 push_ready.set_value();
                                 ready.wait(); //同时在go上等待
                                 q.push(42);
                             }
            );
        pop_done=std::async(std::launch::async,
                            [&q,ready,&pop_ready]()
                            {
                                pop_ready.set_value();
                                ready.wait();
                                return q.pop();
                            }
            );
        push_ready.get_future().wait(); //等待push线程就绪
        pop_ready.get_future().wait(); //等待pop线程就绪
        go.set_value(); //两个线程就绪后才发送运行信号

        push_done.get();
        assert(pop_done.get()==42);
        assert(q.empty());
    }
    catch(...)
    {
        go.set_value();
        throw;
    }
}
