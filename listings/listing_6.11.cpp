#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <list>
#include <utility>
#include <shared_mutex>

template<typename Key,typename Value,typename Hash=std::hash<Key> >
class threadsafe_lookup_table
{
private:
    class bucket_type
    {
    private:
        typedef std::pair<Key,Value> bucket_value;
        typedef std::list<bucket_value> bucket_data;
        typedef typename bucket_data::iterator bucket_iterator;

        bucket_data data;
        mutable std::shared_mutex mutex;

        bucket_iterator find_entry_for(Key const& key) const
        {
            return std::find_if(data.begin(),data.end(),
                [&](bucket_value const& item)
                {return item.first==key;});
        }
    public:
        Value value_for(Key const& key,Value const& default_value) const
        {
            std::shared_lock<std::shared_mutex> lock(mutex);
            bucket_iterator const found_entry=find_entry_for(key);
            return (found_entry==data.end())?
                default_value : found_entry->second;
        }

        void add_or_update_mapping(Key const& key,Value const& value)
        {
            std::unique_lock<std::shared_mutex> lock(mutex);
            bucket_iterator const found_entry=find_entry_for(key);
            if(found_entry==data.end())
            {
                //异常安全：即便抛出异常也能保持数据不变(这也是异常安全的一个最重要的理解)
                data.push_back(bucket_value(key,value));
            }
            else
            {
                //这里赋值构造可能会产生异常，如果产生异常有可能原值未动或改变了一部分，但对整体数据结构没有影响，可以留待使用者处理异常情况
                found_entry->second=value;
            }
        }
    
        void remove_mapping(Key const& key)
        {
            std::unique_lock<std::shared_mutex> lock(mutex);
            bucket_iterator const found_entry=find_entry_for(key);
            if(found_entry!=data.end())
            {
                data.erase(found_entry);
            }
        }
    };
    
    std::vector<std::unique_ptr<bucket_type> > buckets;
    Hash hasher;

    bucket_type& get_bucket(Key const& key) const
    {
        std::size_t const bucket_index=hasher(key)%buckets.size();
        return *buckets[bucket_index];
    }

public:
    typedef Key key_type;
    typedef Value mapped_type;
    typedef Hash hash_type;
    
    threadsafe_lookup_table(
        unsigned num_buckets=19, Hash const& hasher_=Hash()):
        buckets(num_buckets),hasher(hasher_)
    {
        for(unsigned i=0;i<num_buckets;++i)
        {
            buckets[i].reset(new bucket_type);
        }
    }

    threadsafe_lookup_table(threadsafe_lookup_table const& other)=delete;
    threadsafe_lookup_table& operator=(
        threadsafe_lookup_table const& other)=delete;
    
    Value value_for(Key const& key,
        Value const& default_value=Value()) const
    {
        //以桶为单位并行 
        return get_bucket(key).value_for(key,default_value);
    }
    
    void add_or_update_mapping(Key const& key,Value const& value)
    {
        get_bucket(key).add_or_update_mapping(key,value);
    }
    
    void remove_mapping(Key const& key)
    {
        get_bucket(key).remove_mapping(key);
    }
};

