#include <vector>
#include <atomic>
#include <iostream>
#include <chrono>
#include <thread>

std::vector<int> data;
std::atomic_bool data_ready(false);

void reader_thread()
{
    //1会一直循环等待4执行完毕，这样4就在1之前发生，1在2之前发生，3在4之前发生，因为先行关系的传递性，预订次序为3->4->1->2
    while(!data_ready.load())  // 1 
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::cout<<"The answer="<<data[0]<<"\n";//2
}
void writer_thread()
{
    data.push_back(42);//3
    data_ready=true; //4
}
