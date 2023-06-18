/*** 
 * @Author: baisichen
 * @Date: 2023-04-26 16:40:32
 * @LastEditTime: 2023-06-16 12:18:40
 * @LastEditors: baisichen
 * @Description: 
 */
#include <iostream>

void foo(int a,int b)
{
    std::cout<<a<<","<<b<<std::endl;
}

int get_num()
{
    static int i=0;
    return ++i;
}

int main()
{
    foo(get_num(),get_num()); //同一个语句中的参数调用，先调用第一个get_num还是第二个是不明确的。有可能输出2,1
}
