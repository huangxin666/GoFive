#ifndef __GOSEARCH_H__
#define __GOSEARCH_H__
#include "ChessBoard.h"
#include "defines.h"
#include "TransTable.h"
#include <chrono>
#include <unordered_map>
#include <queue>
using namespace std::chrono;
#define MAX_CHILD_NUM 10

enum ABNODETYPE :uint8_t
{
    UNSURE,
    PV_NODE, // exact value
    CUT_NODE,// lower bound (might be greater)
    ALL_NODE // upper bound (the exact score might be less)
};

struct TransTableData
{
    uint32_t checkHash = 0;
    int16_t value = 0;
    Position bestStep;
    uint8_t depth = 0;//real depth
    union
    {
        struct
        {
            uint8_t age : 6;
            uint8_t type : 2;
        };
        uint8_t bitset = 0;
    };
};

#define LOCAL_SEARCH_RANGE 4

struct MovePath
{
    vector<Position> path;
    int rating; //对于 VCF\VCT 10000 代表成功
    uint16_t startStep;
    uint16_t endStep;
    MovePath(uint16_t start) :startStep(start), endStep(start)
    {

    }
    void cat(MovePath& other)
    {
        path.insert(path.end(), other.path.begin(), other.path.end());
        endStep = other.endStep;
    }
    void push(Position pos)
    {
        path.push_back(pos);
        endStep = startStep + (uint8_t)path.size();
    }
};

class GoSearchEngine;
struct PVSearchData
{
    GoSearchEngine *engine;
    vector<StepCandidateItem>::iterator it;
    bool struggle;

    PVSearchData(GoSearchEngine* e, vector<StepCandidateItem>::iterator it) :engine(e), it(it), struggle(false)
    {
    }
};

class GoSearchEngine
{
    friend class AIGoSearch;
public:
    GoSearchEngine();
    ~GoSearchEngine();

    void initSearchEngine(ChessBoard * board);

    Position getBestStep(uint64_t startSearchTime);

    void applySettings(AISettings setting);

private:

    void allocatedTime(uint32_t& max_time, uint32_t&suggest_time);

    void analysePosition(ChessBoard* board, vector<StepCandidateItem>& moves, MovePath& path);

    void selectBestMove(ChessBoard* board, vector<StepCandidateItem>& moves, MovePath& path);

    void doABSearch(ChessBoard* board, MovePath& optimalPath, int depth, int depth_extend, int alpha, int beta, bool enableVCT, bool useTransTable);

    void doPVSearch(ChessBoard* board, MovePath& optimalPath, int depth, int depth_extend, int alpha, int beta, uint8_t type, bool enableVCT, bool useTransTable);

    int doQuiescentSearch(ChessBoard* board, int depth, int alpha, int beta, bool enableVCT, bool check);

    bool doVCXExpand(ChessBoard* board, MovePath& optimalPath, bool useTransTable, bool onlyVCF);

    inline uint8_t getPlayerSide()
    {
        return startStep.state;
    }
    inline uint8_t getAISide()
    {
        return Util::otherside(startStep.state);
    }

    inline bool isPlayerSide(uint8_t side)
    {
        return startStep.state == side;
    }

    inline int getVCFDepth(uint16_t cstep)
    {
        return VCFExpandDepth + currentAlphaBetaDepth * 4 + startStep.step - cstep;
    }

    inline int getVCTDepth(uint16_t cstep)
    {
        //return (VCTExpandDepth + currentAlphaBetaDepth + 4 + startStep.step - cstep);
        return VCTExpandDepth + currentAlphaBetaDepth * 2 + startStep.step - cstep;
    }

    void textOutIterativeInfo(MovePath& optimalPath);
    void textOutResult(MovePath& optimalPath);
    void textForTest(MovePath& optimalPath, int priority);

    void textOutAllocateTime(uint32_t max_time, uint32_t suggest_time);
private:
    ChessBoard* board;
    ChessStep startStep;
    TransTableArray<TransTableData> transTable;

private://搜索过程中的全局变量
    int currentAlphaBetaDepth;//迭代加深，当前最大层数
    time_point<system_clock> startSearchTime;
    bool find_winning_move = false;
public://statistic
    int complexity = 0;
    int DBSearchNodeCount = 0;
    int MaxDepth = 0;
    int maxDBSearchNodeCount = 0;
    HashStat transTableStat;
    int node_count = 0;
    int node_count_try = 0;
    static mutex message_queue_lock;
    static queue<string> message_queue;
    static bool getDebugMessage(string &debugstr);
    void sendMessage(string &debugstr);
private://settings
    int AIweight = 100;
    MessageCallBack msgCallBack;
    uint32_t maxStepTimeMs = 10000;
    uint32_t restMatchTimeMs = UINT32_MAX;
    uint32_t maxMemoryBytes = 350000000;
    bool useMultiThread = false;
    bool useDBSearch = false;
    bool useTransTable = false;
    bool enableDebug = true;
    GAME_RULE rule = FREESTYLE;
    int maxAlphaBetaDepth = 20;
    int minAlphaBetaDepth = 1;
    int VCFExpandDepth = 10;//冲四
    int VCTExpandDepth = 0;//追三
};



#endif // !__GOSEARCH_H__
