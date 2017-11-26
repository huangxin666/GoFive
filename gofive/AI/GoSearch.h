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

enum TRANSTYPE :uint8_t
{
    TRANSTYPE_UNSURE,
    TRANSTYPE_EXACT,
    TRANSTYPE_LOWER,//还可能有比value小的
    TRANSTYPE_UPPER //还可能有比value大的
};

struct TransTableData
{
    uint32_t checkHash = 0;
    int16_t value = 0;
    uint8_t depth = 0;//real depth
    Position bestStep;
    uint8_t continue_index = 0;
    union
    {
        struct
        {
            uint8_t maxdepth : 6;
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

    MovePath selectBestMove(ChessBoard* board, StepCandidateItem& bestStep);

    static void solveBoardForEachThread(PVSearchData data);

    void doAlphaBetaSearch(ChessBoard* board, int depth, int alpha, int beta, MovePath& optimalPath, Position lastlastPos, bool useTransTable);

    bool doVCXExpand(ChessBoard* board, MovePath& optimalPath, Position* center, bool useTransTable, bool firstExpand);

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
    TransTable<TransTableData> transTable;

private://搜索过程中的全局变量
    int currentAlphaBetaDepth;//迭代加深，当前最大层数
    time_point<system_clock> startSearchTime;
    bool global_isOverTime = false;
    bool find_winning_move = false;
public://statistic
    int DBSearchNodeCount = 0;
    int MaxDepth = 0;
    int maxDBSearchNodeCount = 0;
    HashStat transTableStat;
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
    int minAlphaBetaDepth = 2;
    int VCFExpandDepth = 10;//冲四
    int VCTExpandDepth = 0;//追三
};



#endif // !__GOSEARCH_H__
