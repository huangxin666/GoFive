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
    bool enableDebug;//����������������������Ϣ
    bool useTransTable;
    bool useDBSearch;//��������alphabeta����ʱ������ȫ���ڵ㣬��������һЩ���۲��õĽڵ㣨���ܻᵼ�¹ؼ��ڵ㶪ʧ��

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