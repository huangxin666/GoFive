#ifndef __TRANSTABLE_H__
#define __TRANSTABLE_H__
#include "defines.h"



enum TRANSTYPE :uint8_t
{
    TRANSTYPE_UNSURE,
    TRANSTYPE_EXACT,
    TRANSTYPE_LOWER,//还可能有比value小的
    TRANSTYPE_UPPER //还可能有比value大的
};

enum VCXRESULT :uint8_t
{
    VCXRESULT_NOSEARCH,
    VCXRESULT_FALSE,
    VCXRESULT_TRUE,
    VCXRESULT_UNSURE
};

struct TransTableVCXData
{
    uint32_t checkHash = 0;
    uint8_t VCFEndStep = 0;
    uint8_t VCTEndStep = 0;
    union
    {
        struct
        {
            uint8_t VCFDepth : 6;
            VCXRESULT VCFflag : 2;
            uint8_t VCTDepth : 6;
            VCXRESULT VCTflag : 2;
        };
        uint16_t bitset = 0;
    };
};

struct TransTableData
{
    uint32_t checkHash = 0;
    int16_t value = 0;
    uint8_t continue_index = 0;
    uint8_t depth = 0;
    uint8_t type = TRANSTYPE_UNSURE;
    uint8_t endStep = 0;
    Position bestStep;
};

struct TransTableMap
{
    unordered_map<uint64_t, TransTableData> map;
    shared_mutex lock;
};

struct TransTableVCXMap
{
    unordered_map<uint64_t, TransTableVCXData> map;
    shared_mutex lock;
};

class TransTable
{
public:

    inline bool getTransTable(uint64_t key, TransTableData& data)
    {
        transTable.lock.lock_shared();
        unordered_map<uint64_t, TransTableData>::iterator it = transTable.map.find(key);
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
    inline void putTransTable(uint64_t key, const TransTableData& data)
    {
        transTable.lock.lock();
        transTable.map[key] = data;
        transTable.lock.unlock();
    }

    inline bool getTransTableVCX(uint64_t key, TransTableVCXData& data)
    {
        transTableVCX.lock.lock_shared();
        unordered_map<uint64_t, TransTableVCXData>::iterator it = transTableVCX.map.find(key);
        if (it != transTableVCX.map.end())
        {
            data = it->second;
            transTableVCX.lock.unlock_shared();
            return true;
        }
        else
        {
            transTableVCX.lock.unlock_shared();
            return false;
        }
    }
    inline void putTransTableVCX(uint64_t key, const TransTableVCXData& data)
    {
        transTableVCX.lock.lock();
        transTableVCX.map[key] = data;
        transTableVCX.lock.unlock();
    }

    void clearTransTable()
    {
        transTable.map.clear();
    }

    void clearTransTableVCX()
    {
        transTableVCX.map.clear();
    }

    size_t getTransTableSize()
    {
        return transTable.map.size();
    }

    size_t getTransTableVCXSize()
    {
        return transTableVCX.map.size();
    }

private:
    TransTableMap transTable;
    TransTableVCXMap transTableVCX;
};


#endif
