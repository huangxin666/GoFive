#ifndef __GOSEARCH_H__
#define __GOSEARCH_H__
#include "ChessBoard.h"
#include "defines.h"
#include <chrono>
using namespace std::chrono;
#define MAX_CHILD_NUM 10

#define TRANSTYPE_EXACT    0
#define TRANSTYPE_LOWER    1
#define TRANSTYPE_HIGHER   2
#define TRANSTYPE_VCT   3
#define TRANSTYPE_VCF   4

#define LOCAL_SEARCH_RANGE 4

struct TransTableData
{
    uint64_t checkHash;
    uint8_t type;
    int situationRating;
    uint8_t endStep;
    bool isEnd()
    {
        if (situationRating == chesstypes[CHESSTYPE_5].rating || situationRating == -chesstypes[CHESSTYPE_5].rating)
        {
            return true;
        }
        return false;
    }
};

enum VCXRESULT
{
    VCXRESULT_FALSE,
    VCXRESULT_TRUE,
    VCXRESULT_UNSURE,
    VCXRESULT_NOSEARCH
};

struct TransTableDataSpecial
{
    TransTableDataSpecial():checkHash(0), endStep(0), type(0), VCFflag(VCXRESULT_NOSEARCH), VCTflag(VCXRESULT_NOSEARCH)
    {

    }
    uint64_t checkHash;
    uint8_t endStep;
    uint8_t type;
    uint8_t VCFflag;
    uint8_t VCTflag;
};

struct OptimalPath
{
    vector<uint8_t> path;
    int situationRating; //对于 VCF\VCT 10000 代表成功
    uint8_t startStep;
    uint8_t endStep;
    void cat(OptimalPath& other)
    {
        endStep = other.endStep;
        for (auto step : other.path)
        {
            path.push_back(step);
        }
    }
    void push(uint8_t index)
    {
        path.push_back(index);
        endStep++;
    }
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
    void initSearchEngine(ChessBoard * board, ChessStep lastStep, uint64_t maxSearchTime);
    uint8_t getBestStep();

private:
    OptimalPath solveBoard(ChessBoard* board);

    void doAlphaBetaSearch(ChessBoard* board, int alpha, int beta, OptimalPath& optimalPath);

    //Wrapper with transTable
    void doAlphaBetaSearchWrapper(ChessBoard* board, int alpha, int beta, OptimalPath& optimalPath);

    void getNormalSteps(ChessBoard* board, vector<StepCandidateItem>& moves);

    void getFourkillDefendSteps(ChessBoard* board, uint8_t index, vector<StepCandidateItem>& moves);

    void getDeadFourSteps(ChessBoard* board, uint8_t index, vector<StepCandidateItem>& moves, bool global = true);

    bool doVCTSearch(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, bool global = true);

    bool doVCTSearchWrapper(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, bool global = true);

    void getVCTAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, bool global = true);

    bool doVCFSearch(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, bool global = true);

    bool doVCFSearchWrapper(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, bool global = true);

    void getVCFAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, bool global = true);

    void doStruggleSearch();

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

    inline bool getTransTableSpecial(uint32_t key, TransTableDataSpecial& data)
    {
        //transTableLock.lock_shared();
        map<uint32_t, TransTableDataSpecial>::iterator it = transTableSpecial.find(key);
        if (it != transTableSpecial.end())
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
    inline void putTransTableSpecial(uint32_t key, const TransTableDataSpecial& data)
    {
        //transTableLock.lock();
        transTableSpecial[key] = data;
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
    map<uint32_t, TransTableDataSpecial> transTableSpecial;

private://搜索过程中的全局变量

    int global_currentMaxDepth;//迭代加深，当前最大层数，偶数
    time_point<system_clock> global_startSearchTime;
    bool global_isOverTime;
public://statistic
    static HashStat transTableStat;
    static string textout;
private://settings
    int maxSearchTime = 30;
    int maxAlphaBetaDepth = 10;
    int minAlphaBetaDepth = 5;
    int maxVCFDepth = 10;
    int maxVCTDepth = 20;
};



#endif // !__GOSEARCH_H__
