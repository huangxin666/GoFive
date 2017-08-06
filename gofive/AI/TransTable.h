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
    VCXRESULT_FALSE,
    VCXRESULT_TRUE,
    VCXRESULT_UNSURE,
    VCXRESULT_NOSEARCH
};

struct TransTableVCXData
{
    TransTableVCXData() :checkHash(0), VCFEndStep(0), VCTEndStep(0), VCFflag(VCXRESULT_NOSEARCH), VCTflag(VCXRESULT_NOSEARCH)
    {

    }
    uint32_t checkHash;
    uint8_t VCFEndStep;
    uint8_t VCFDepth;
    VCXRESULT VCFflag;
    uint8_t VCTEndStep;
    uint8_t VCTDepth;
    VCXRESULT VCTflag;
};

struct TransTableData
{
    uint32_t checkHash;
    int16_t value;
    uint8_t depth;
    uint8_t type;
    uint8_t endStep;
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
        //if (step < startstep) return false;

        unordered_map<uint64_t, TransTableData>::iterator it = transTable.map.find(key);
        if (it != transTable.map.end())
        {
            data = it->second;
            //transTableLock.unlock_shared();
            return true;
        }
        else
        {
            //transTableLock.unlock_shared();
            return false;
        }
    }
    inline void putTransTable(uint64_t key, const TransTableData& data)
    {
        //if (step < startstep) return;
        //transTableLock.lock();
        transTable.map[key] = data;
        //transTableLock.unlock();
    }

    inline bool getTransTableVCX(uint64_t key, TransTableVCXData& data)
    {
        //transTableLock.lock_shared();
        unordered_map<uint64_t, TransTableVCXData>::iterator it = transTableVCX.map.find(key);
        if (it != transTableVCX.map.end())
        {
            data = it->second;
            //transTableLock.unlock_shared();
            return true;
        }
        else
        {
            //transTableLock.unlock_shared();
            return false;
        }
    }
    inline void putTransTableVCX(uint64_t key, const TransTableVCXData& data)
    {
        //transTableLock.lock();
        transTableVCX.map[key] = data;
        //transTableLock.unlock();
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
    //uint16_t startstep;
    TransTableMap transTable;
    TransTableVCXMap transTableVCX;
    //map<int,TransTableMap*> transTable;
    //map<int,TransTableMapSpecial*> transTableSpecial;
};


#endif
