#ifndef __AIENGINE_H__
#define __AIENGINE_H__

#include "defines.h"
#include "ChessBoard.h"

struct AISettings
{
    uint8_t maxSearchDepth;
    uint64_t maxSearchTime;
    bool multiThread;
};

class OpenEngine
{
public:
    static Position getOpen1(ChessBoard *cb);
    static bool checkOpen2(ChessBoard *cb);
    static Position getOpen2(ChessBoard *cb);
    static bool checkOpen3(ChessBoard *cb);
    static Position getOpen3(ChessBoard *cb);
};

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

    static void getMoveList(ChessBoard* board, vector<pair<uint8_t,int>>& moves, int type, bool global);
};




#endif