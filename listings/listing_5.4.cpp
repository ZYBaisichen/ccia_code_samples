/*** 
 * @Author: baisichen
 * @Date: 2023-04-26 16:40:32
 * @LastEditTime: 2023-06-16 23:05:50
 * @LastEditors: baisichen
 * @Description: 
 */
#include <atomic>
#include <thread>
#include <assert.h>

std::atomic<bool> x,y;
std::atomic<int> z;

void write_x()
{
    x.store(true,std::memory_order_seq_cst);
}

void write_y()
{
    y.store(true,std::memory_order_seq_cst);
}

void read_x_then_y()
{
    while(!x.load(std::memory_order_seq_cst)); 
    if(y.load(std::memory_order_seq_cst))
        ++z;
}

void read_y_then_x()
{
    while(!y.load(std::memory_order_seq_cst));
    if(x.load(std::memory_order_seq_cst))
        ++z;
}
//三种情况
1. read_x_then_y先见到了x==true，但见到的是y==false；此时read_y_then_x中会一直等待y==true，当见到y为true时，x已经为ture了，z=1
2. read_y_then_x先见到了y==true，但见到的是x==false；此时read_x_then_y中会一直等待x==true，当见到x为true时，y已经为ture了，z=1
3. x、y同时为true，z为2
int main()
{
    x=false;
    y=false;
    z=0;
    std::thread a(write_x);
    std::thread b(write_y);
    std::thread c(read_x_then_y);
    std::thread d(read_y_then_x);
    a.join();
    b.join();
    c.join();
    d.join();
    assert(z.load()!=0);
}
