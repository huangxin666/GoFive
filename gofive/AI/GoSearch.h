#ifndef __GOSEARCH_H__
#define __GOSEARCH_H__
#include "ChessBoard.h"
#include "defines.h"

#define MAX_CHILD_NUM 10

struct TransTableData
{
    uint64_t checkHash;
    int situationRating;
    uint8_t endStep;
    uint8_t endSide;
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

class GoSearchEngine
{
public:
    GoSearchEngine();
    ~GoSearchEngine();
    void initSearchEngine(ChessBoard * board, ChessStep lastStep);
    uint8_t getBestStep();

private:
    void doAlphaBetaSearch(ChessBoard* board, int alpha, int beta, OptimalPath& optimalPath, int extraDepth);

    void getNextSteps(ChessBoard* board, uint8_t side, vector<GoTreeNode>& childs);

    void doKillSearch(ChessBoard* board, OptimalPath& optimalPath, int bestRating, uint8_t atackSide);

    void textOutSearchInfo(OptimalPath& optimalPath);

    void textOutPathInfo(OptimalPath& optimalPath);

    inline bool getTransTable(uint32_t key, TransTableData& data)
    {
        //transTableLock.lock_shared();
        if (transTable.find(key) != transTable.end())
        {
            data = transTable[key];
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
    int maxKillToEndDepth = 30;
};



#endif // !__GOSEARCH_H__
