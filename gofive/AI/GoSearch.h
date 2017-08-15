#ifndef __GOSEARCH_H__
#define __GOSEARCH_H__
#include "ChessBoard.h"
#include "defines.h"
#include "TransTable.h"
#include <chrono>
#include <unordered_map>
using namespace std::chrono;
#define MAX_CHILD_NUM 10



#define LOCAL_SEARCH_RANGE 4

struct OptimalPath
{
    vector<Position> path;
    int rating; //对于 VCF\VCT 10000 代表成功
    uint8_t startStep;
    uint8_t endStep;
    OptimalPath(uint8_t start) :startStep(start)
    {

    }
    void cat(OptimalPath& other)
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
};

class GoSearchEngine
{
    friend class AIGoSearch;
public:
    GoSearchEngine();
    ~GoSearchEngine();

    void initSearchEngine(ChessBoard * board);

    Position getBestStep(uint64_t startSearchTime);

    static void applySettings(uint32_t max_searchtime_ms, int min_depth, int max_depth, int vcf_expand, int vct_expand, bool enable_debug, bool useTansTable, bool full_search);

    static void getNormalSteps(ChessBoard* board, vector<StepCandidateItem>& moves, set<Position>* reletedset);

    static void getFourkillDefendSteps(ChessBoard* board, Position pos, vector<StepCandidateItem>& moves);

    static void getVCTAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, set<Position>* reletedset);

    static void getVCFAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, set<Position>* reletedset);

private:
    void getNormalDefendSteps(ChessBoard* board, vector<StepCandidateItem>& moves, set<Position>* reletedset);

    void getNormalRelatedSet(ChessBoard* board, set<Position>& reletedset, OptimalPath& optimalPath);

    OptimalPath solveBoard(ChessBoard* board, vector<StepCandidateItem>& solveList);

    OptimalPath makeSolveList(ChessBoard* board, vector<StepCandidateItem>& solveList);

    void doAlphaBetaSearch(ChessBoard* board, int depth, int alpha, int beta, OptimalPath& optimalPath, bool useTransTable);

    VCXRESULT doVCTSearch(ChessBoard* board, int depth, OptimalPath& optimalPath, set<Position>* reletedset, bool useTransTable);

    VCXRESULT doVCTSearchWrapper(ChessBoard* board, int depth, OptimalPath& optimalPath, set<Position>* reletedset, bool useTransTable);

    VCXRESULT doVCFSearch(ChessBoard* board, int depth, OptimalPath& optimalPath, set<Position>* reletedset, bool useTransTable);

    VCXRESULT doVCFSearchWrapper(ChessBoard* board, int depth, OptimalPath& optimalPath, set<Position>* reletedset, bool useTransTable);

    bool doNormalStruggleSearch(ChessBoard* board, int depth, int alpha, int beta, set<Position>& reletedset, OptimalPath& optimalPath, vector<StepCandidateItem>* solveList, bool useTransTable);

    bool doVCTStruggleSearch(ChessBoard* board, int depth, Position &nextstep, bool useTransTable);

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
        return VCFExpandDepth + currentAlphaBetaDepth + startStep.step - cstep;
    }

    inline int getVCTDepth(uint16_t cstep)
    {
        return VCTExpandDepth + currentAlphaBetaDepth + startStep.step - cstep;
    }

    void textOutSearchInfo(OptimalPath& optimalPath);
    void textOutPathInfo(OptimalPath& optimalPath);
    void textSearchList(vector<StepCandidateItem>& moves, Position current, Position best);
    void textForTest(OptimalPath& optimalPath, int priority);
private:
    ChessBoard* board;
    ChessStep startStep;
    TransTable transTable;

private://搜索过程中的全局变量
    int currentAlphaBetaDepth;//迭代加深，当前最大层数
    time_point<system_clock> startSearchTime;
    bool global_isOverTime = false;
public://statistic
    static HashStat transTableStat;
    static string textout;
    string textold;
    string texttemp;
private://settings
    static uint32_t maxSearchTimeMs;
    static bool fullUseOfTime;
    static bool fullSearch;
    static bool enableDebug;
    static bool useTransTable;
    static int maxAlphaBetaDepth;
    static int minAlphaBetaDepth;
    static int VCFExpandDepth;//冲四
    static int VCTExpandDepth;//追三
};



#endif // !__GOSEARCH_H__
