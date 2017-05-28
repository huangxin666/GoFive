#ifndef CHESSAI_H
#define CHESSAI_H

#include "ChessBoard.h"
#include "defines.h"

class ChessAI
{
public:
    ChessAI()
    {

    }
    virtual ~ChessAI()
    {

    }
    virtual Position getNextStep(ChessBoard *cb, AISettings setting, ChessStep lastStep, uint8_t side, uint8_t level, bool ban) = 0;
    virtual void applyAISettings(AISettings setting) = 0;
};

class AIWalker :
    public ChessAI
{
public:
    AIWalker();
    virtual ~AIWalker();
    virtual Position getNextStep(ChessBoard *cb, AISettings setting, ChessStep lastStep, uint8_t side, uint8_t level, bool ban);
    virtual void applyAISettings(AISettings setting);
    Position level1(ChessBoard *cb, uint8_t side);
    Position level2(ChessBoard *cb, uint8_t side);
};

class AIGameTree :
    public ChessAI
{
public:
    AIGameTree();
    virtual ~AIGameTree();
    virtual Position getNextStep(ChessBoard *cb, AISettings setting, ChessStep lastStep, uint8_t side, uint8_t level, bool ban);

    virtual void applyAISettings(AISettings setting);
    static void setThreadPoolSize(int num);
    static AIRESULTFLAG getResultFlag();
    static HashStat getTransTableHashStat();
};

#endif