/*** 
 * @Author: baisichen
 * @Date: 2023-04-26 16:40:32
 * @LastEditTime: 2023-06-18 16:07:47
 * @LastEditors: baisichen
 * @Description: 
 */
#include <exception>
#include <stack>
#include <mutex>
#include <memory>


// 根据设计指引分析本例子安全性
/*
1. 每个函数都在内部互斥m上加锁，因此保障了基本的线程安全。没有线程可以见到不变量被破坏
2. empty和pop合到了一起，避免了函数接口的固有竞争(分开的话，锁粒度太细了)
3. 产生异常：给锁加锁产生异常比较罕见，因为每个函数都是先加锁，后面数据还没变，所以异常也没事；push的时候可能产生异常，主要是复制/移动数据的过程抛出异常。pop可能会抛出empty_stack异常，但还没改动数据，所以异常安全, 同样如果调用了value=std::move(xxx), 可能会在赋值操作上报异常，同样也在pop之前，所以没关系；empty不改动任何数据所以异常安全
4. 死锁：自定义对象上如果构造函数中进一步调用了栈容器的函数，形成了循环调用，这时需要锁住互斥，但在外层互斥已经被锁住了，所以就造成了死锁。另外栈容器自身初始化过程中，其他线程不得访问数据，数据初始化过程中其他线程也不能访问，这都需要对象自身保证
*/
// 效率
/*
因为每个函数都是用一个互斥排他的访问，所以所有线程均串行化的访问数据结构，效率上不太好。另外当栈容器为空时，再想取数据需要不停的调用empty判断是否为空
*/
struct empty_stack: std::exception
{
    const char* what() const throw()
    {
        return "empty stack";
    }
};

template<typename T>
class threadsafe_stack
{
private:
    std::stack<T> data;
    mutable std::mutex m;
public:
    threadsafe_stack(){}
    threadsafe_stack(const threadsafe_stack& other)
    {
        std::lock_guard<std::mutex> lock(other.m);
        data=other.data;
    }
    threadsafe_stack& operator=(const threadsafe_stack&) = delete;

    void push(T new_value)
    {
        std::lock_guard<std::mutex> lock(m);
        data.push(std::move(new_value));
    }
    std::shared_ptr<T> pop()
    {
        std::lock_guard<std::mutex> lock(m);
        if(data.empty()) throw empty_stack();
        std::shared_ptr<T> const res(
            std::make_shared<T>(std::move(data.top()))); 
        data.pop();
        return res;
    }
    void pop(T& value)
    {
        std::lock_guard<std::mutex> lock(m);
        if(data.empty()) throw empty_stack();
        value=std::move(data.top());
        data.pop();
    }
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m);
        return data.empty();
    }
};
