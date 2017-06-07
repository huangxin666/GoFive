#ifndef __AIENGINE_H__
#define __AIENGINE_H__

#include "ChessBoard.h"
#include "defines.h"

class AIEngine
{
public:
    AIEngine()
    {
        textOut.clear();
    }
    virtual ~AIEngine()
    {

    }
    virtual Position getNextStep(ChessBoard *cb, AISettings setting, ChessStep lastStep, uint8_t side, uint8_t level, bool ban) = 0;
    virtual void applyAISettings(AISettings setting) = 0;
    virtual void updateTextOut() = 0;
    static string textOut;
};

class AIWalker :
    public AIEngine
{
public:
    AIWalker();
    virtual ~AIWalker();
    virtual Position getNextStep(ChessBoard *cb, AISettings setting, ChessStep lastStep, uint8_t side, uint8_t level, bool ban);
    virtual void applyAISettings(AISettings setting);
    virtual void updateTextOut();
    Position level1(ChessBoard *cb, uint8_t side);
    Position level2(ChessBoard *cb, uint8_t side);

};

class AIGameTree :
    public AIEngine
{
public:
    AIGameTree();
    virtual ~AIGameTree();
    virtual Position getNextStep(ChessBoard *cb, AISettings setting, ChessStep lastStep, uint8_t side, uint8_t level, bool ban);
    virtual void applyAISettings(AISettings setting);
    virtual void updateTextOut();

    static void setThreadPoolSize(int num);

};

class AIGoSearch :
    public AIEngine
{
public:
    AIGoSearch();
    virtual ~AIGoSearch();
    virtual Position getNextStep(ChessBoard *cb, AISettings setting, ChessStep lastStep, uint8_t side, uint8_t level, bool ban);
    virtual void applyAISettings(AISettings setting);
    virtual void updateTextOut();

};

#endif