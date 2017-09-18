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

enum VCXRESULT :uint8_t
{
    VCXRESULT_NOSEARCH,
    VCXRESULT_UNSURE,   // both unsure
    VCXRESULT_FAIL, // VCF和VCT都fail
    VCXRESULT_SUCCESS,  // 100
};

struct TransTableVCXData
{
    uint32_t checkHash = 0;
    //Position bestStep;
    uint8_t VCTmaxdepth = 0;
    uint8_t VCFmaxdepth = 0;
    union
    {
        struct
        {   
            VCXRESULT VCTflag : 2;
            uint8_t VCTdepth :6;//real depth
            VCXRESULT VCFflag : 2;
            uint8_t VCFdepth : 6;
        };
        uint16_t bitset = 0;
    };
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
    MovePath(uint16_t start) :startStep(start)
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

struct StepCandidateItem
{
    Position pos;
    int priority;
    StepCandidateItem(Position i, int p) :pos(i), priority(p)
    {};
    bool operator<(const StepCandidateItem& other)  const
    {
        return pos < other.pos;
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

    static size_t getNormalCandidates(ChessBoard* board, vector<StepCandidateItem>& moves, set<Position>* reletedset, bool full_search);

    static void getALLFourkillDefendSteps(ChessBoard* board, vector<StepCandidateItem>& moves, bool is33);

    static void getFourkillDefendCandidates(ChessBoard* board, Position pos, vector<StepCandidateItem>& moves);

    static void getVCTAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, set<Position>* reletedset);

    static void getVCFAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, set<Position>* reletedset);

private:
    void allocatedTime(uint32_t& max_time, uint32_t&suggest_time);

    void getNormalRelatedSet(ChessBoard* board, set<Position>& reletedset, MovePath& optimalPath);

    MovePath solveBoard(ChessBoard* board, StepCandidateItem& bestStep);

    static void solveBoardForEachThread(PVSearchData data);

    void doAlphaBetaSearch(ChessBoard* board, int depth, int alpha, int beta, MovePath& optimalPath, bool useTransTable, bool deepSearch = true);

    VCXRESULT doVCTSearch(ChessBoard* board, int depth, MovePath& optimalPath, set<Position>* reletedset, bool useTransTable);

    VCXRESULT doVCTSearchWrapper(ChessBoard* board, int depth, MovePath& optimalPath, set<Position>* reletedset, bool useTransTable);

    VCXRESULT doVCFSearch(ChessBoard* board, int depth, MovePath& optimalPath, set<Position>* reletedset, bool useTransTable);

    VCXRESULT doVCFSearchWrapper(ChessBoard* board, int depth, MovePath& optimalPath, set<Position>* reletedset, bool useTransTable);

    bool doVCTStruggleSearch(ChessBoard* board, int depth, set<Position>& reletedset, bool useTransTable);

    void getPathFromTransTable(ChessBoard* board, MovePath& path);

    inline uint8_t getPlayerSide()
    {
        return startStep.getState();
    }
    inline uint8_t getAISide()
    {
        return Util::otherside(startStep.getState());
    }

    inline bool isPlayerSide(uint8_t side)
    {
        return startStep.getState() == side;
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
    TransTable<TransTableVCXData> transTableVCX;

private://搜索过程中的全局变量
    int currentAlphaBetaDepth;//迭代加深，当前最大层数
    time_point<system_clock> startSearchTime;
    bool global_isOverTime = false;
public://statistic
    int VCXSuccessCount[20] = { 0 };
    int errorVCFSuccessInVCTCount = 0;
    HashStat transTableStat;
    static mutex message_queue_lock;
    static queue<string> message_queue;
    static bool getDebugMessage(string &debugstr);
    void sendMessage(string &debugstr);
private://settings
    MessageCallBack msgCallBack;
    uint32_t maxStepTimeMs = 10000;
    uint32_t restMatchTimeMs = UINT32_MAX;
    uint32_t maxMemoryBytes = 350000000;
    bool useMultiThread = false;
    bool fullSearch = false;
    bool useTransTable = false;
    bool enableDebug = true;
    int maxAlphaBetaDepth = 20;
    int minAlphaBetaDepth = 2;
    int VCFExpandDepth = 15;//冲四
    int VCTExpandDepth = 6;//追三
};



#endif // !__GOSEARCH_H__
