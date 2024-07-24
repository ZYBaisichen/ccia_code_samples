#include <atomic>
#include <memory>
//这种方式的确可以安全的回收节点，但其处理过程带有不烧额外的开销。当pop时，每次都要调用outstanding_hazard_pointers_for扫描整个侯删链表，查验原子变量max_hazard_pointers次
std::shared_ptr<T> pop()
{
    std::atomic<void*>& hp=get_hazard_pointer_for_current_thread(); //为各线程的风险指针实例分配内存。这里是线程id与其专属风险指针配对
    node* old_head=head.load();
    //外层循环更新head指针，如果失败则重新更新。
    do
    {
        node* temp;
        do
        {
            temp=old_head;
            hp.store(old_head);  //读取head操作和风险指针设立存在间隙，确保风险指针指向的节点不会在间隙中删除
            old_head=head.load(); //再次load一下head指针，看head指针和temp是否相同，直到将hp风险指针设置为正确的head指针
            //因为这里会再取一遍，所以外层循环没有必要使用compare_exchange_weak，因为发生佯败，old_head会被更新成最新的head，从而让内存循环的while条件不成立
        } while(old_head!=temp);
    }
    while(old_head &&
          !head.compare_exchange_strong(old_head,old_head->next)); //如果改成weak发生了佯败，则可能毫无必要的设置风险指针
    //即head被更新成head->next，访问原来的头结点只能通过old_head之访问，形成了当前线程独占头的情况
    hp.store(nullptr);  //一旦更新了head指针（head被当前线程收归了己有，只能通过old_head访问它），便将风险指针清。
    std::shared_ptr<T> res;
    if(old_head)
    {
        res.swap(old_head->data);
        if(outstanding_hazard_pointers_for(old_head)) //节点实质数据被取出，就将old_head与隶属于其他线程的风险指针逐一比对，判定它是不是被其他的线程锁指涉，如果被指涉就不能马上删除，必须放入候选删除队列中
        {
            reclaim_later(old_head);
        }
        else
        {
            delete old_head; //没有被其他线程所指涉，则直接安全删除
        }
        delete_nodes_with_no_hazards(); //扫描整个待删除链表，以删除那些未被风险指针所指涉的节点
    }
    return res;
}
