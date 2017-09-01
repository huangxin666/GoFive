#ifndef __AIENGINE_H__
#define __AIENGINE_H__

#include "defines.h"
#include "ChessBoard.h"
#include <chrono>
using namespace std::chrono;

enum AIENGINE
{
    AIWALKER_ATACK,
    AIWALKER_DEFEND,
    AIGAMETREE,
    AIGOSEARCH
};

enum AILEVEL
{
    AILEVEL_PRIMARY = 1,
    AILEVEL_INTERMEDIATE,
    AILEVEL_HIGH,
    AILEVEL_MASTER,
    AILEVEL_UNLIMITED
};

class OpenEngine
{
public:
    struct OpenInfo
    {

    };
    static Position getOpen1(ChessBoard *cb);
    static bool checkOpen2(ChessBoard *cb);
    static Position getOpen2(ChessBoard *cb);
    static bool checkOpen3(ChessBoard *cb);
    static Position getOpen3(ChessBoard *cb);
private:
    //static map<uint64_t, OpenInfo> openMap;
};

class AIEngine
{
public:
    AIEngine()
    {

    }
    virtual ~AIEngine()
    {

    }
    virtual Position getNextStep(ChessBoard *cb, time_t start_time) = 0;
    virtual void applyAISettings(AISettings setting) = 0;
    virtual bool getMessage(string &msg) = 0;
};

class AIWalker :
    public AIEngine
{
public:
    AIWalker(int type);
    virtual ~AIWalker();
    virtual Position getNextStep(ChessBoard *cb, time_t start_time);
    virtual void applyAISettings(AISettings setting);
    virtual bool getMessage(string &msg);
    Position level1(ChessBoard *cb);
    Position level2(ChessBoard *cb);

private:
    int AIType;
};

class AIGameTree :
    public AIEngine
{
public:
    AIGameTree();
    virtual ~AIGameTree();
    virtual Position getNextStep(ChessBoard *cb, time_t start_time);
    virtual void applyAISettings(AISettings setting);
    virtual bool getMessage(string &msg);
private:
    int level;
};



class AIGoSearch :
    public AIEngine
{
public:
    AIGoSearch();
    virtual ~AIGoSearch();
    virtual Position getNextStep(ChessBoard *cb, time_t start_time);
    virtual void applyAISettings(AISettings setting);
    virtual bool getMessage(string &msg);

    static void getMoveList(ChessBoard* board, vector<pair<Position, int>>& moves, int type, bool global);

private:
    AISettings setting;
};




#endif