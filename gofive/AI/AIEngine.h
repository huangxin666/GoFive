#ifndef __AIENGINE_H__
#define __AIENGINE_H__

#include "defines.h"
#include "ChessBoard.h"
#include <chrono>
using namespace std::chrono;

enum AIENGINE
{
    AISIMPLE,
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
    virtual Position getNextStep(ChessBoard *cb, time_t start_time, AISettings setting) = 0;
};

class AISimple :
    public AIEngine
{
public:
    AISimple();
    virtual ~AISimple();
    virtual Position getNextStep(ChessBoard *cb, time_t start_time, AISettings setting);
};

class AIGameTree :
    public AIEngine
{
public:
    AIGameTree();
    virtual ~AIGameTree();
    virtual Position getNextStep(ChessBoard *cb, time_t start_time, AISettings setting);
};



class AIGoSearch :
    public AIEngine
{
public:
    AIGoSearch();
    virtual ~AIGoSearch();
    virtual Position getNextStep(ChessBoard *cb, time_t start_time, AISettings setting);

    static void getMoveList(ChessBoard* board, vector<pair<Position, int>>& moves, int type, bool global);
};




#endif