/*** 
 * @Author: baisichen
 * @Date: 2023-04-26 16:40:32
 * @LastEditTime: 2023-06-28 21:47:03
 * @LastEditors: baisichen
 * @Description: 
 */
template<typename T>
class lock_free_queue
{
private:
    void set_new_tail(counted_node_ptr &old_tail,
                      counted_node_ptr const &new_tail)
    {
        node* const current_tail_ptr=old_tail.ptr;
        while(!tail.compare_exchange_weak(old_tail,new_tail) &&
              old_tail.ptr==current_tail_ptr);
        if(old_tail.ptr==current_tail_ptr)
            free_external_counter(old_tail); //释放一次tail指针指向的它
        else
            current_tail_ptr->release_ref();
    }
public:
    void push(T new_value)
    {
        std::unique_ptr<T> new_data(new T(new_value));
        counted_node_ptr new_next;
        new_next.ptr=new node;
        new_next.external_count=1;
        counted_node_ptr old_tail=tail.load();
        for(;;)
        {
            increase_external_count(tail,old_tail);
            T* old_data=nullptr;
            //当有一个线程成功执行了push，这里的cas操作就会失败，会进行下一次等待，这样很容易陷入忙等待，浪费cpu时间
            //所以才会有下一个分支，帮其他的线程创建一个新的node，这样其他也在等待的线程push的时候就可以省去new新的node的时间
            //通过这里可以体会到书中关于盲等的定义，cpu一直在做无意义的运行等待即为盲等；如果cpu在等待期间可以做一些别的事情就不是盲等;
            //本质上解决这种盲等是为了提高并发度，和缩小锁的粒度有异曲同工之妙
            if(old_tail.ptr->data.compare_exchange_strong(
                   old_data,new_data.get())) 
            {
                counted_node_ptr old_next={0};
                if(!old_tail.ptr->next.compare_exchange_strong(
                       old_next,new_next))
                {
                    delete new_next.ptr;
                    new_next=old_next;
                }
                set_new_tail(old_tail, new_next);
                new_data.release();
                break;
            }
            else
            {
                counted_node_ptr old_next={0}; //当push data失败后，说明其他线程抢占了tail.data，则尝试为其他等待的线程创建空的node，以提高效率
                if(old_tail.ptr->next.compare_exchange_strong(
                       old_next,new_next))
                {
                    old_next=new_next;
                    new_next.ptr=new node;
                }
                set_new_tail(old_tail, old_next);
            }
        }
    }
};
