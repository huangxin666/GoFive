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
    uint32_t checkHash;
    int16_t value;
    uint8_t type;
    uint8_t endStep;
    uint8_t depth;
};

enum VCXRESULT :uint8_t
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
    uint32_t checkHash;
    uint8_t VCFEndStep;
    uint8_t VCFDepth;
    VCXRESULT VCFflag;
    uint8_t VCTEndStep;
    uint8_t VCTDepth;
    VCXRESULT VCTflag;
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

typedef unordered_map<uint64_t, TransTableData> TransTableMap;
typedef unordered_map<uint64_t, TransTableDataSpecial> TransTableMapSpecial;

class GoSearchEngine
{
    friend class AIGoSearch;
public:
    GoSearchEngine();
    ~GoSearchEngine();

    void initSearchEngine(ChessBoard * board);

    uint8_t getBestStep(uint64_t startSearchTime);

    static void applySettings(uint32_t max_searchtime_ms, int min_depth, int max_depth, int vcf_expand, int vct_expand, int vcx_extra, bool enable_debug);

    static void getNormalSteps(ChessBoard* board, vector<StepCandidateItem>& moves, set<uint8_t>* reletedset);

    static void getFourkillDefendSteps(ChessBoard* board, uint8_t index, vector<StepCandidateItem>& moves);

    static void getVCTAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, set<uint8_t>* reletedset);

    static void getVCFAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, set<uint8_t>* reletedset);

private:
    void getNormalDefendSteps(ChessBoard* board, vector<StepCandidateItem>& moves, set<uint8_t>* reletedset);

    void getNormalRelatedSet(ChessBoard* board, set<uint8_t>& reletedset, OptimalPath& optimalPath);

    OptimalPath solveBoard(ChessBoard* board, vector<StepCandidateItem>& solveList);

    OptimalPath makeSolveList(ChessBoard* board, vector<StepCandidateItem>& solveList);

    void doAlphaBetaSearch(ChessBoard* board, csidx index, int alpha, int beta, OptimalPath& optimalPath);

    VCXRESULT doVCTSearch(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, set<uint8_t>* reletedset);

    VCXRESULT doVCTSearchWrapper(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, set<uint8_t>* reletedset);

    VCXRESULT doVCFSearch(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, set<uint8_t>* reletedset);

    VCXRESULT doVCFSearchWrapper(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, set<uint8_t>* reletedset);

    bool doNormalStruggleSearch(ChessBoard* board, int alpha, int beta, set<uint8_t>& reletedset, OptimalPath& optimalPath);

    bool doVCTStruggleSearch(ChessBoard* board, uint8_t &nextstep);

    inline bool getTransTable(uint64_t key, TransTableData& data)
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
    inline void putTransTable(uint64_t key, const TransTableData& data)
    {
        //transTableLock.lock();
        transTable[key] = data;
        //transTableLock.unlock();
    }

    inline bool getTransTableSpecial(uint64_t key, TransTableDataSpecial& data)
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
    inline void putTransTableSpecial(uint64_t key, const TransTableDataSpecial& data)
    {
        //transTableLock.lock();
        transTableSpecial[key] = data;
        //transTableLock.unlock();
    }

    inline uint8_t getPlayerSide()
    {
        return startStep.getSide();
    }
    inline uint8_t getAISide()
    {
        return Util::otherside(startStep.getSide());
    }

    inline bool isPlayerSide(uint8_t side)
    {
        return startStep.getSide() == side;
    }

    void textOutSearchInfo(OptimalPath& optimalPath);
    void textOutPathInfo(OptimalPath& optimalPath);
    void textSearchList(vector<StepCandidateItem>& moves, uint8_t currentindex, uint8_t best);
    void textForTest(uint8_t currentindex, int rating, int priority);
private:

    ChessBoard* board;
    ChessStep startStep;
    static TransTableMap transTable;
    static shared_mutex transTableLock;
    static TransTableMapSpecial transTableSpecial;

private://搜索过程中的全局变量
    int global_currentMaxDepth;//迭代加深，当前最大层数
    time_point<system_clock> global_startSearchTime;
    bool global_isOverTime = false;
    int struggleFlag = 0;
public://statistic
    static HashStat transTableStat;
    static string textout;
    string textold;
    string texttemp;
private://settings
    static uint32_t maxSearchTimeMs;
    static bool fullUseOfTime;
    static bool enableDebug;
    static int maxAlphaBetaDepth;
    static int minAlphaBetaDepth;
    static int maxVCFDepth;//冲四
    static int maxVCTDepth;//追三
    static int extraVCXDepth;
};



#endif // !__GOSEARCH_H__
