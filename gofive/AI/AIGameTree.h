#ifndef AIGAMETREE_H
#define AIGAMETREE_H
#include "ChessAI.h"
class AIGameTree :
    public ChessAI
{
public:
    AIGameTree();
    virtual ~AIGameTree();
    virtual Position getNextStep(ChessBoard *cb, AIParam param);

    static void setThreadPoolSize(int num);
    static AIRESULTFLAG getResultFlag();
    static HashStat getTransTableHashStat();
};

#endif