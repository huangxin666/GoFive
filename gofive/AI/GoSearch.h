#ifndef __GOSEARCH_H__
#define __GOSEARCH_H__
#include "ChessBoard.h"
#include "defines.h"

#define MAX_CHILD_NUM 10

struct TransTableData
{
    uint64_t checkHash;

};

struct OptimalPath
{
    vector<ChessStep> path;
    int boardScore;
    uint8_t startStep;
};

struct GoTreeNode
{
    uint8_t index;
    uint8_t side;
    uint8_t chessType;
    int stepScore;
    int globalRating;
};

class GoSearchEngine
{
public:
    GoSearchEngine();
    ~GoSearchEngine();
    void initSearchEngine(ChessBoard * board, ChessStep lastStep);
    uint8_t getBestStep();

private:
    void doAlphaBeta(ChessBoard* board, int alpha, int beta, OptimalPath& optimalPath);

    void getNextSteps(ChessBoard* board, uint8_t side, vector<GoTreeNode>& childs);

    void doKillCalculate();

    inline bool getTransTable(uint32_t key, TransTableData& data)
    {
        transTableLock.lock_shared();
        if (transTable.find(key) != transTable.end())
        {
            data = transTable[key];
            transTableLock.unlock_shared();
            transTableStat.hit++;
            return true;
        }
        else
        {
            transTableLock.unlock_shared();
            transTableStat.miss++;
            return false;
        }
    }
    inline void putTransTable(uint32_t key, const TransTableData& data)
    {
        transTableLock.lock();
        transTable[key] = data;
        transTableLock.unlock();
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

    int global_AlphaBetaDepth;//迭代加深，当前最大层数，偶数
    time_t global_StartSearchTime;
    bool global_OverTime;
private://statistic
    HashStat transTableStat;

private://settings
    time_t maxSearchTime = 120;
    int maxAlphaBetaDepth = 0;
    int minAlphaBetaDepth = 0;
    int maxKillToEndDepth = 0;
};



#endif // !__GOSEARCH_H__
