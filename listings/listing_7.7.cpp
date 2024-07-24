#include <atomic>
#include <thread>

unsigned const max_hazard_pointers=100;
struct hazard_pointer
{
    std::atomic<std::thread::id> id;
    std::atomic<void*> pointer;
};
hazard_pointer hazard_pointers[max_hazard_pointers];
class hp_owner
{
    hazard_pointer* hp;
public:
    hp_owner(hp_owner const&)=delete;
    hp_owner operator=(hp_owner const&)=delete;
    hp_owner():
        hp(nullptr)
    {
        for(unsigned i=0;i<max_hazard_pointers;++i)
        {
            std::thread::id old_id;
            //看是否有空的id类。
            if(hazard_pointers[i].id.compare_exchange_strong(
                   old_id,std::this_thread::get_id())) //看下有没有空出来的位置可以注册当前线程的风险指针
            {
                hp=&hazard_pointers[i];
                break;
            }
        }
        if(!hp) //没有找到空闲的位置，则说明有太多的现成使用风险指针
        {
            throw std::runtime_error("No hazard pointers available");
        }
    }
    std::atomic<void*>& get_pointer()
    {
        return hp->pointer;
    }
    ~hp_owner()
    {
        hp->pointer.store(nullptr);
        hp->id.store(std::thread::id());
    }
};
std::atomic<void*>& get_hazard_pointer_for_current_thread()
{
    thread_local static hp_owner hazard; //每次线程启动都会有一个这样的变量，用于管控当前线程独有的风险指针，在初始化阶段确定风险指针
    return hazard.get_pointer();
}

bool outstanding_hazard_pointers_for(void* p) {
    for (unsigned i =0; i<max_hazard_pointers;++i) {
        if (hazard_pointers[i].pointer.load() == p) {
            return true;
        }
    }
    return false;
}