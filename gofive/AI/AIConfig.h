#ifndef __AICONFIG_H__
#define __AICONFIG_H__

#include "defines.h"

struct AIConfig
{
    //common
    GAME_RULE rule;
    bool multithread;
    uint32_t maxStepTimeMs;
    uint32_t restMatchTimeMs;
    uint32_t maxMemoryBytes;
    time_t startTimeMs;
    MessageCallBack msgfunc;
    //

    //GameTree
    uint8_t pnMaxDepth;
    //

    //GoSearch
    int minAlphaBetaDepth;
    int maxAlphaBetaDepth;
    int VCFExpandDepth;
    int VCTExpandDepth;
    bool enableDebug;//若开启，会输出更多调试信息
    bool useTransTable;
    bool useDBSearch;//若开启，alphabeta搜索时会搜索全部节点，否则会放弃一些评价不好的节点（可能会导致关键节点丢失）

    void init(uint8_t level);

    void changeLevel(uint8_t level);

    static AIConfig* getInstance()
    {
        static AIConfig* instance = NULL;
        if (instance == NULL)
        {
            instance = new AIConfig();
        }
        return instance;
    }
};

#endif