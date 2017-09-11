#ifndef __GAMETREE_H__
#define __GAMETREE_H__

#include "ChessBoard.h"
#include "defines.h"
#include <map>
#define MAX_CHILD_NUM 225

//#define GAMETREE_DEBUG


enum AIRESULTFLAG :uint8_t
{
    AIRESULTFLAG_NORMAL,
    AIRESULTFLAG_WIN,
    AIRESULTFLAG_FAIL,
    AIRESULTFLAG_NEARWIN,
    AIRESULTFLAG_TAUNT,
    AIRESULTFLAG_COMPLAIN
};

struct RatingInfo
{
    int totalScore;     //分数
    int highestScore;	//最高分
    RatingInfo() :totalScore(0), highestScore(0) {};
    RatingInfo(int total, int high) :totalScore(total), highestScore(high) {};
};

struct RatingInfoAtack
{
#ifdef GAMETREE_DEBUG
    vector<ChessStep> moveList;//for debug
#endif // GAMETREE_DEBUG
    RatingInfo black;
    RatingInfo white;
    ChessStep lastStep;
    int8_t depth;
};

struct RatingInfoDenfend
{
#ifdef GAMETREE_DEBUG
    vector<ChessStep> moveList;//for debug
#endif
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
struct TaskItems
{
    GameTreeNode *node;//任务需要计算的节点
    int index;//节点对应的最开始节点的索引
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
    Position getBestStep(uint8_t playercolor, uint16_t startstep);
    static void initTree(uint8_t maxDepth, bool multiThread, bool extra);

private:

    static void threadPoolWorkFunc(TaskItems t);

    inline int getChildNum()
    {
        return (int)childs.size();
    }
    inline int getHighest(uint8_t side)
    {
        return (side == PIECE_BLACK) ? black.highestScore : white.highestScore;
    }
    inline int getTotal(uint8_t side)
    {
        return (side == PIECE_BLACK) ? black.totalScore : white.totalScore;
    }
    inline int getDepth()
    {
        return lastStep.step - startStep;
    }
    void createChildNode(int row, int col);
    void deleteChilds();
    void deleteChessBoard();

    void buildAllChilds();
    int getActiveChild();
    int getDefendChild();

    int buildAtackSearchTree();
    RatingInfoAtack getBestAtackRating();
    RatingInfoAtack buildAtackChildWithTransTable(GameTreeNode* child, int deepen);
    bool buildAtackChildsAndPrune(int deepen);
    void buildAtackTreeNode(int deepen);

    int buildDefendSearchTree();
    RatingInfoDenfend getBestDefendRating(int basescore);
    RatingInfoDenfend buildDefendChildWithTransTable(GameTreeNode* child, int basescore);
    bool buildDefendChildsAndPrune(int basescore);
    void buildDefendTreeNode(int basescore);

    void buildDefendTreeNodeSimple(int basescore);

    static void clearTransTable();
    static void popHeadTransTable();
public:
    static ChildInfo *childsInfo;
    static AIRESULTFLAG resultFlag;
    static uint8_t playerColor;
    static uint8_t maxSearchDepth;
    static uint16_t startStep;
    static uint8_t transTableMaxDepth;//太深的节点没必要加入置换表
    static bool enableAtack;
    static size_t maxTaskNum;
    static trans_table transTable_atack;
    static int bestRating;//根节点alpha值，会动态更新
    static int bestIndex;
    static HashStat transTableHashStat;
    static bool extraSearch;
private:
    vector<GameTreeNode*>childs;
    ChessStep lastStep;
    RatingInfo black, white;
    int alpha, beta;
    ChessBoard *chessBoard;
};



#define TASKTYPE_DEFEND 1
#define TASKTYPE_ATACK  2

#endif