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

struct AISettings
{
    //common
    bool ban;
    uint32_t maxSearchTimeMs;
    time_t startTimeMs;
    //

    //GameTree
    uint8_t maxSearchDepth;
    bool enableAtack;
    bool extraSearch;
    //

    //GoSearch
    int minAlphaBetaDepth;
    int maxAlphaBetaDepth;
    int VCFExpandDepth;
    int VCTExpandDepth;
    bool enableDebug;//����������������������Ϣ
    bool fullUseTime;//��������AI���þ�ʱ�䣬����ᾡ����ʡʱ��
    bool useTranTable;
    bool fullSearch;//��������alphabeta����ʱ������ȫ���ڵ㣬��������һЩ���۲��õĽڵ㣨���ܻᵼ�¹ؼ��ڵ㶪ʧ��
    //
    void defaultBase()
    {
        ban = false;
        maxSearchTimeMs = 10000;
    }

    void defaultGameTree(AILEVEL level);

    void defaultGoSearch(AILEVEL level);

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
    static map<uint64_t, OpenInfo> openMap;
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
    virtual Position getNextStep(ChessBoard *cb, time_t start_time) = 0;
    virtual void applyAISettings(AISettings setting) = 0;
    virtual void updateTextOut() = 0;
    static string textOut;
};

class AIWalker :
    public AIEngine
{
public:
    AIWalker(int type);
    virtual ~AIWalker();
    virtual Position getNextStep(ChessBoard *cb, time_t start_time);
    virtual void applyAISettings(AISettings setting);
    virtual void updateTextOut();
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
    virtual void updateTextOut();
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
    virtual void updateTextOut();

    static void getMoveList(ChessBoard* board, vector<pair<uint8_t, int>>& moves, int type, bool global);
};




#endif