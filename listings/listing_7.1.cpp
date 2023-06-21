#include <atomic>

//没有任何阻塞型函数，lock里不断循环，知道test_and_set返回false停止。
class spinlock_mutex
{
    std::atomic_flag flag;
public:
    spinlock_mutex():
        flag(ATOMIC_FLAG_INIT)
    {}
    void lock()
    {
        while(flag.test_and_set(std::memory_order_acquire)); //自旋锁
    }
    void unlock()
    {
        flag.clear(std::memory_order_release);
    }
};
