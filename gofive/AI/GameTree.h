#ifndef GAMETREE_H
#define GAMETREE_H

#include "ChessBoard.h"
#include "utils.h"

#define MAX_CHILD_NUM 225

struct RatingInfoAtack
{
    //vector<ChessStep> moveList;//for debug
    RatingInfo black;
    RatingInfo white;
    ChessStep lastStep;
    int8_t depth;
};

struct RatingInfoDenfend
{
    //vector<ChessStep> moveList;//for debug
    RatingInfo rating;
    RatingInfo black;
    RatingInfo white;
    ChessStep lastStep;
    int8_t depth;
};

struct TransTableNodeData
{
    uint64_t checksum;
    RatingInfo black;
    RatingInfo white;
    ChessStep lastStep;
    // int steps;
};

struct SafeMap
{
    map<uint32_t, TransTableNodeData> m;
    shared_mutex lock;
};

typedef vector<SafeMap*> trans_table;

struct ChildInfo
{
    RatingInfo rating;
    int lastStepScore;
    int8_t depth;
    bool hasSearch;
};

class GameTreeNode;
struct ThreadParam
{
    int index;//节点对应的最开始节点的索引
    GameTreeNode *node;//任务需要计算的节点
    int type;//任务类型
};

class GameTreeNode
{
public:
    friend class ThreadPool;
    GameTreeNode();
    GameTreeNode(ChessBoard* chessBoard);
    ~GameTreeNode();
    const GameTreeNode& operator=(const GameTreeNode&);
    Position getBestStep();
    void initTree(AIParam param, int8_t playercolor);
    static void threadPoolWork(ThreadParam t);
private:
    inline int getChildNum()
    {
        return childs.size();
    }
    inline int getHighest(int side)
    {
        return (side == 1) ? black.highestScore : white.highestScore;
    }
    inline int getTotal(int side)
    {
        return (side == 1) ? black.totalScore : white.totalScore;
    }
    inline int getDepth()
    {
        return lastStep.step - startStep;
    }
    void createChildNode(int row, int col);
    void deleteChilds();
    void deleteChessBoard();

    void debug();

    void buildAllChilds();
    int getActiveChild();
    int getDefendChild();

    int buildAtackSearchTree(ThreadPool &pool);
    RatingInfoAtack getBestAtackRating();
    RatingInfoAtack buildAtackChildWithTransTable(GameTreeNode* child);
    bool buildAtackChildsAndPrune();
    void buildAtackTreeNode();
    
    int buildDefendSearchTree(ThreadPool &pool);
    RatingInfoDenfend getBestDefendRating(int basescore);
    RatingInfoDenfend buildDefendChildWithTransTable(GameTreeNode* child, int basescore);
    bool buildDefendChildsAndPrune(int basescore);
    void buildDefendTreeNode(int basescore);

    void buildDefendTreeNodeSimple(int basescore);

    void setAlpha(int alpha, int type);
    void setBeta(int beta, int type);

    static void clearTransTable();
    static void popHeadTransTable();
public:
    
    static ChildInfo *childsInfo;
    static AIRESULTFLAG resultFlag;
    static int8_t playerColor;
    static uint8_t maxSearchDepth;
    static uint8_t startStep;
    static uint8_t transTableMaxDepth;//太深的节点没必要加入置换表
    static bool enableAtack;
    static size_t maxTaskNum;
    static trans_table transTable_atack;
    static bool longtailmode;
    static atomic<int> longtail_threadcount;
    static int bestRating;//根节点alpha值，会动态更新
    static int bestIndex;
    static HashStat transTableHashStat;
    static bool  iterative_deepening;
private:
    vector<GameTreeNode*>childs;
    ChessStep lastStep;
    HashPair hash;
    RatingInfo black, white;
    int alpha, beta;
    ChessBoard *chessBoard;
    future<void> s;
};



#define TASKTYPE_DEFEND 1
#define TASKTYPE_ATACK  2

#define CUTTYPE_DEFEND 1
#define CUTTYPE_ATACK  2

#endif