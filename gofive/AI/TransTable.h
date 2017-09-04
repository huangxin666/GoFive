#ifndef __TRANSTABLE_H__
#define __TRANSTABLE_H__
#include <map>
#include <unordered_map>
using namespace std;

template<class data_type>
class TransTable
{
public:
    typedef uint32_t key_type;
    typedef map<key_type, data_type>  NMap;

    struct TransTableMap
    {
        NMap map;
        shared_mutex lock;
    };

    void setMaxMemory(uint32_t maxmem)
    {
        maxTableSize = maxmem / 3 / (sizeof(NMap::value_type) + sizeof(void*));
    }

    bool memoryValid()
    {
        return transTable.map.size() < maxTableSize;
    }

    inline bool getTransTable(key_type key, data_type& data)
    {
        transTable.lock.lock_shared();
        NMap::iterator it = transTable.map.find(key);
        if (it != transTable.map.end())
        {
            data = it->second;
            transTable.lock.unlock_shared();
            return true;
        }
        else
        {
            transTable.lock.unlock_shared();
            return false;
        }
    }
    inline void putTransTable(key_type key, const data_type& data)
    {
        transTable.lock.lock();
        NMap::iterator it = transTable.map.find(key);
        if (it != transTable.map.end())
        {
            it->second = data;
        }
        else
        {
            transTable.map.insert(make_pair(key, data));
        }
        transTable.lock.unlock();
    }

    void clearTransTable()
    {
        transTable.map.clear();
    }

    size_t getTransTableSize()
    {
        return transTable.map.size();
    }

private:
    size_t maxTableSize;
    TransTableMap transTable;
};


#endif
