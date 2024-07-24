#include <memory>
#include <atomic>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <mutex>
#include <condition_variable>
#include <vector>

#include <future>
using namespace std;
// g++ -std=c++0x -o a main.cpp   -pthread -l:libatomic.so.1

template <typename T>
class lock_free_stack
{
private:
    struct node;
    struct counted_node_ptr //尺寸是8字节，64位。若硬件平台支持双字节比较-交换操作(即64位数据的比较-交换操作)，则可以搭配atomic使用；否则需要使用shared_ptr了，因为如果不支持的话atomic会自动转为使用互斥来保证原子性
    {
        int external_count; // 使用外部引用，是为了防止在外面自增期间，内部ptr指针无效
        node *ptr;
    };
    struct node
    {
        std::shared_ptr<T> data;
        std::atomic<int> internal_count;
        counted_node_ptr next;
        node(T const& data_):
            data(std::make_shared<T>(data_)),
            internal_count(0)
        {}
    };
    std::atomic<counted_node_ptr> head;

    void increase_head_count(counted_node_ptr &old_counter)
    {
        counted_node_ptr new_counter;
        do
        {
            new_counter = old_counter;
            ++new_counter.external_count;
        } while (!head.compare_exchange_strong(old_counter, new_counter)); // 防止更新前被别的线程先更新
        old_counter.external_count = new_counter.external_count;
    }

public:
    ~lock_free_stack()
    {
        while (pop())
            ;
    }
    void push(T const &data)
    {
        counted_node_ptr new_node;
        new_node.ptr = new node(data);
        new_node.external_count = 1; // 初始外部计数器为1，表示有一个指向它的指针
        new_node.ptr->next = head.load();
        while (!head.compare_exchange_weak(new_node.ptr->next, new_node))
            ;
    }
    std::shared_ptr<T> pop()
    {
        counted_node_ptr old_head = head.load(); // 自增之前就读取了。这期间有可能有别的线程删除它
        for (;;)
        {
            increase_head_count(old_head); // 如果使用了head而不自增，则有可能导致其他线程将外部引用减少至0，从而导致悬空指针。使用外部引用的话，可以保证其他线程不释放，从而当前线程下面的操作都保持了head的可见。
            node *const ptr = old_head.ptr; // Once the count has been increased, you can safely dereference the ptr field of the value loaded from head in order to access the pointed-to node .
            if (!ptr)
            {
                return std::shared_ptr<T>();
            }
            //如果ptr不是空，则不是空栈，尝试弹出head。如果成功，则old_head指向的原头结点就不能被其他线程访问了，即形成了独占。
            if (head.compare_exchange_strong(old_head, ptr->next)) //1
            {
                std::shared_ptr<T> res;
                res.swap(ptr->data);                                    // 一旦compare_exchange_strong成功，就表明node被当前线程独占了，则将ptr的数据交换出来
                int const count_increase = old_head.external_count - 2; // 链表里的元素不在指涉-1，当前线程退出时还要再减1（即ptr不再指涉）。
                cout << "count_increase:" << count_increase << "external:" <<  old_head.external_count << endl;
                //内外部计数器相等时删除。这里应该少了一步，如果不相等，应该放入侯删队列。
                /*
                    1. 假设有5个线程共同pop，外部计数器变成了6；假设被A线程占上了head的所有权，则在下面这句话结束之后内部计数器add成4，外部计数器也变成了4，本线程不会释放该节点。
                然后其他4个线程执行1位置的compare_exchange_strong失败，会执行2位置的将内部计数器减一，当最后一个线程持有该指针时，内部计数器的值将会是1，其退出后，将会没有指针指向改节点，所以直接删除。
                    2. 还有一种情况是：其他线程执行2操作要先于下面的internal_count.fetch_add。则其他四个线程有可能将internal_count减为-4，这时说明当前线程是最后一个线程，即内外计数器的和将是0，所以可以直接删除ptr
                从这里也可以看出，内部计数器的含义其实就是有多少指针再指向自己，之所以在pop前先增加外部计数器，是因为有的线程可能加了之后，获取头结点所有权失败后还需要再减计数器的值。如果只有一个计数器，就会让竞争变的很激烈，两个计数器可以很大程度的提高并发效率。
                */
                if (ptr->internal_count.fetch_add(count_increase) ==
                    -count_increase) // fetch_add返回的是加之前的值。两个行为，1.将外部计数器加到内部计数器；2.因为要离开当前node，所以减个head和当前线程的退出。 
                {
                    delete ptr;
                }
                return res; //无论删不删节点，都可以将数据返回了
            }
            else if (ptr->internal_count.fetch_sub(1) == 1)  //2
            {
                // fetch_sub返回的是减之前的值。 如果变成了1，说明只有ptr在指向该节点。则当前线程结束后，ptr是局部变量会被销毁，则肯定不会有其他指针再指向该节点了
                //如果本次compare_exchange_strong失败了，则下次循环不会再访问该old_head了，所以本线程的外部指涉减1，如果是最后一个线程持有它。
                cout << "delete ptr" << endl;
                delete ptr;
            }
        }
    }
};

lock_free_stack<int> lfs;
int MAX_NUM = 100000;
void thread1(int threadid, std::shared_future<int> sf)
{
    cout << "t" << threadid << ", sf:" << sf.get() << endl;
    for (int i=0;i<MAX_NUM;i++) {
        lfs.push(i);
    }
}

void thread_test(int threadid, std::shared_future<int> sf)
{
    cout << "t" << threadid << ", sf:" << sf.get() << endl;
    for (int i = 0; i < MAX_NUM; i++)
    {
        std::shared_ptr<int> aa = lfs.pop();
        if (!aa) {
            cout << "t" << threadid << "stack empty " << endl;
            break;
        }
        // sleep(1);
        cout << "t" << threadid << " pop : " << *aa << endl;
    }
}

int main() {
    const int thread_num = 100;
    std::vector<std::thread> threads(thread_num);

    std::promise<int> pro;
    std::shared_future<int> sf1(pro.get_future());
    std::thread t1(thread1, -1, sf1);
    for (int i = 0; i <thread_num;i++) {
        threads[i] = std::thread(thread_test, i, sf1);
    }
    pro.set_value(110);
    for (int i = 0; i < thread_num; i++){
        threads[i].join();
    }
    return 0;
}
