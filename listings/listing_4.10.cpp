/*** 
 * @Author: baisichen
 * @Date: 2023-04-26 16:40:32
 * @LastEditTime: 2023-06-04 21:12:01
 * @LastEditors: baisichen
 * @Description: 
 */
#include <future>
	
class connection_set  {
};

void process_connections(connection_set& connections)
{
    //这个例子感觉不太好。应该是一个epoll内部的简易实现，接受数据时，找到对应的promise，设置数据唤醒本机等待的线程；发送数据时，同样设置发送成功标志告诉等待发送完成的本机线程
    while(!done(connections))
    {
        for(connection_iterator
                connection=connections.begin(),end=connections.end();
            connection!=end;
            ++connection)
        {
            if(connection->has_incoming_data())
            {
                data_packet data=connection->incoming();
                std::promise<payload_type>& p=
                    connection->get_promise(data.id);
                p.set_value(data.payload);
            }
            if(connection->has_outgoing_data())
            {
                outgoing_packet data=
                    connection->top_of_outgoing_queue();
                connection->send(data.payload);
                data.promise.set_value(true);
            }
        }
    }
}
