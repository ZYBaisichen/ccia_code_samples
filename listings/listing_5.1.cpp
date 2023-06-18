#include <atomic>
class spinlock_mutex
{
    std::atomic_flag flag;
public:
    spinlock_mutex():
        flag(ATOMIC_FLAG_INIT)
    {}
    void lock()
    {
        while(flag.test_and_set(std::memory_order_acquire)); //为忙等待。会占用大量cpu时间，压制多线程的空间
    }
    void unlock()
    {
        flag.clear(std::memory_order_release);
    }
};
