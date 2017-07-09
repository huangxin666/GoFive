#ifndef __GOSEARCH_H__
#define __GOSEARCH_H__
#include "ChessBoard.h"
#include "defines.h"
#include <chrono>
#include <unordered_map>
using namespace std::chrono;
#define MAX_CHILD_NUM 10

#define TRANSTYPE_UNSURE    0
#define TRANSTYPE_EXACT     1

#define LOCAL_SEARCH_RANGE 4

struct TransTableData
{
    uint64_t checkHash;
    int16_t value;
    uint8_t type;
    uint8_t endStep;
    uint8_t depth;
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
    TransTableDataSpecial() :checkHash(0), VCFEndStep(0), VCTEndStep(0), VCFflag(VCXRESULT_NOSEARCH), VCTflag(VCXRESULT_NOSEARCH)
    {

    }
    uint64_t checkHash;
    uint8_t VCFEndStep;
    uint8_t VCFDepth;
    uint8_t VCFflag;
    uint8_t VCTEndStep;
    uint8_t VCTDepth;
    uint8_t VCTflag;
};

struct OptimalPath
{
    vector<uint8_t> path;
    int rating; //对于 VCF\VCT 10000 代表成功
    uint8_t startStep;
    uint8_t endStep;
    OptimalPath(uint8_t start) :startStep(start)
    {

    }
    void cat(OptimalPath& other)
    {
        for (auto step : other.path)
        {
            path.push_back(step);
        }
        endStep = other.endStep;
    }
    void push(uint8_t index)
    {
        path.push_back(index);
        endStep = startStep + (uint8_t)path.size();
    }
};

struct StepCandidateItem
{
    uint8_t index;
    int priority;
    StepCandidateItem(uint8_t i, int p) :index(i), priority(p)
    {};
};

class GoSearchEngine
{
public:
    GoSearchEngine();
    ~GoSearchEngine();

    void initSearchEngine(ChessBoard * board, ChessStep lastStep, uint64_t startSearchTime, uint64_t maxSearchTime);

    uint8_t getBestStep();

    static void getNormalSteps(ChessBoard* board, vector<StepCandidateItem>& moves);

    static void getFourkillDefendSteps(ChessBoard* board, uint8_t index, vector<StepCandidateItem>& moves);

    static void getVCTAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, uint8_t atackcenter, bool global);

    static void getVCFAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, uint8_t atackcenter, bool global);

private:
    OptimalPath solveBoard(ChessBoard* board, vector<StepCandidateItem>& solveList);

    OptimalPath makeSolveList(ChessBoard* board, vector<StepCandidateItem>& solveList);

    void doAlphaBetaSearch(ChessBoard* board, int alpha, int beta, OptimalPath& optimalPath);

    //wrapper with transTable
    void doAlphaBetaSearchWrapper(ChessBoard* board, int alpha, int beta, OptimalPath& optimalPath);

    uint8_t doVCTSearch(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, uint8_t atackcenter, bool global);

    uint8_t doVCTSearchWrapper(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, uint8_t atackcenter, bool global);

    uint8_t doVCFSearch(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, uint8_t atackcenter, bool global);

    uint8_t doVCFSearchWrapper(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, uint8_t atackcenter, bool global);

    bool doStruggleSearch(ChessBoard* board, uint8_t side, uint8_t &nextstep);

    void textOutSearchInfo(OptimalPath& optimalPath);

    void textOutPathInfo(OptimalPath& optimalPath);

    void textSearchList(vector<StepCandidateItem>& moves, uint8_t currentindex, uint8_t best);

    void textForTest(uint8_t currentindex, int rating, int priority);

    inline bool getTransTable(uint32_t key, TransTableData& data)
    {
        //transTableLock.lock_shared();
        TransTableMap::iterator it = transTable.find(key);
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
        TransTableMapSpecial::iterator it = transTableSpecial.find(key);
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
    typedef unordered_map<uint32_t, TransTableData> TransTableMap;
    typedef unordered_map<uint32_t, TransTableDataSpecial> TransTableMapSpecial;
    ChessBoard* board;
    ChessStep startStep;
    TransTableMap transTable;
    shared_mutex transTableLock;
    TransTableMapSpecial transTableSpecial;

private://搜索过程中的全局变量
    int global_currentMaxDepth;//迭代加深，当前最大层数
    time_point<system_clock> global_startSearchTime;
    bool global_isOverTime;
    int struggleFlag = 0;
public://statistic
    static HashStat transTableStat;
    static string textout;
    string textold;
    string texttemp;
private://settings
    uint32_t maxSearchTimeMs;
    bool enable_debug = true;
    int maxAlphaBetaDepth = 12;
    int minAlphaBetaDepth = 5;
    int maxVCFDepth = 20;//冲四
    int maxVCTDepth = 10;//追三
    int extraVCXDepth = 4;
};



#endif // !__GOSEARCH_H__
