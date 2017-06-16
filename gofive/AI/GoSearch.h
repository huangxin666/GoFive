#ifndef __GOSEARCH_H__
#define __GOSEARCH_H__
#include "ChessBoard.h"
#include "defines.h"

#define MAX_CHILD_NUM 10

#define TRANSTYPE_EXACT    0
#define TRANSTYPE_LOWER    1
#define TRANSTYPE_HIGHER   2

#define LOCAL_SEARCH_RANGE 4

struct TransTableData
{
    uint64_t checkHash;
    uint8_t type;
    int situationRating;
    uint8_t endStep;
    bool isEnd()
    {
        if (situationRating == chesstype2rating[CHESSTYPE_5] || situationRating == -chesstype2rating[CHESSTYPE_5])
        {
            return true;
        }
        return false;
    }
};

struct OptimalPath
{
    vector<ChessStep> path;
    int situationRating;
    uint8_t startStep;
    uint8_t endStep;
};

struct GoTreeNode
{
    int stepScore;
    uint8_t index;
    uint8_t side;
    uint8_t chessType;
};

struct StepCandidateItem
{
    uint8_t index;
    int8_t priority;
    StepCandidateItem(uint8_t i, int8_t p) :index(i), priority(p)
    {};
};

class GoSearchEngine
{
public:
    GoSearchEngine();
    ~GoSearchEngine();
    void initSearchEngine(ChessBoard * board, ChessStep lastStep);
    uint8_t getBestStep();

private:
    void solve(ChessBoard* board);

    void doAlphaBetaSearch(ChessBoard* board, int alpha, int beta, OptimalPath& optimalPath);

    //Wrapper with transTable
    void doAlphaBetaSearchWrapper(ChessBoard* board, int alpha, int beta, OptimalPath& optimalPath);

    void getNormalSteps(ChessBoard* board, vector<StepCandidateItem>& moves);

    void getFourkillDefendSteps(ChessBoard* board, uint8_t index, vector<StepCandidateItem>& moves);

    bool doVCTSearch(ChessBoard* board, uint8_t side);

    void getVCTAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, bool global = true);

    bool doVCFSearch(ChessBoard* board, uint8_t side, bool global = true);

    void getVCFAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, bool global = true);

    void textOutSearchInfo(OptimalPath& optimalPath);

    void textOutPathInfo(OptimalPath& optimalPath);

    inline bool getTransTable(uint32_t key, TransTableData& data)
    {
        //transTableLock.lock_shared();
        map<uint32_t, TransTableData>::iterator it = transTable.find(key);
        if (it != transTable.end())
        {
            data = it->second;
            //transTableLock.unlock_shared();
            return true;
        }
        else
        {
            //transTableLock.unlock_shared();
            return false;
        }
    }
    inline void putTransTable(uint32_t key, const TransTableData& data)
    {
        //transTableLock.lock();
        transTable[key] = data;
        //transTableLock.unlock();
    }

    inline uint8_t getPlayerSide()
    {
        return startStep.getColor();
    }
    inline uint8_t getAISide()
    {
        return util::otherside(startStep.getColor());
    }
private:
    ChessBoard* board;
    ChessStep startStep;
    map<uint32_t, TransTableData> transTable;
    shared_mutex transTableLock;

private://搜索过程中的全局变量

    int global_currentMaxDepth;//迭代加深，当前最大层数，偶数
    time_t global_startSearchTime;
    bool global_isOverTime;
public://statistic
    static HashStat transTableStat;
    static string textout;
    static int maxKillSearchDepth;
private://settings
    time_t maxSearchTime = 120;
    int maxAlphaBetaDepth = 10;
    int minAlphaBetaDepth = 5;
    int maxVCFDepth = 30;
};



#endif // !__GOSEARCH_H__
