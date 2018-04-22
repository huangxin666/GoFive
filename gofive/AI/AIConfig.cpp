#include "AIConfig.h"
#include "AIEngine.h"

void AIConfig::init(uint8_t level)
{
    enableDebug = false;
    maxAlphaBetaDepth = 20;
    minAlphaBetaDepth = 2;
    VCFExpandDepth = 10;//³åËÄ
    VCTExpandDepth = 0;//×·Èý
    useTransTable = true;
    useDBSearch = true;
    multithread = false;
}

void AIConfig::changeLevel(uint8_t level)
{
    switch (level)
    {
    case AILEVEL_PRIMARY:
        rule = FREESTYLE;
        maxStepTimeMs = 1000;
        break;
    case AILEVEL_INTERMEDIATE:
        rule = FREESTYLE;
        maxStepTimeMs = 4000;
        break;
    case AILEVEL_HIGH:
        rule = RENJU;
        maxStepTimeMs = 10000;
        break;
    case AILEVEL_MASTER:
        rule = RENJU;
        maxStepTimeMs = 30000;
        break;
    default:
        break;
    }
}