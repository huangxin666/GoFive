#ifndef GAMETREE_H
#define GAMETREE_H

#include "ChessBoard.h"
#include "utils.h"
#include <memory>
#include <unordered_map>
#include <shared_mutex>
#define MAX_CHILD_NUM 225


struct TransTableData
{
    uint64_t checksum;
    RatingInfo black;
    RatingInfo white;
    int steps;
};

struct ChildInfo
{
    RatingInfo rating;
    int lastStepScore;
    int8_t depth;
    bool hasSearch;
};

class GameTreeNode
{
public:
    //shared_ptr<GameTreeNode>;
    friend class ThreadPool;
    GameTreeNode();
    GameTreeNode(ChessBoard* chessBoard);
    ~GameTreeNode();
    const GameTreeNode& operator=(const GameTreeNode&);
    Position getBestStep();
    void initTree(AIParam param,int8_t playercolor);
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
    inline void buildChild(bool recursive)
    {
        if (lastStep.getColor() == playerColor)
        {
            buildAI(recursive);
        }
        else
        {
            buildPlayer(recursive);
        }
    }
    inline int getDepth() 
    {
        return lastStep.step - startStep;
    }
    void createChildNode(int &row, int &col);
    void deleteChilds();
    void deleteChessBoard();

    void debug();
    int searchBest();
    int searchBest2(ThreadPool &pool);
    RatingInfo getBestRating();
    RatingInfo2 getBestAtackRating();
    int buildAtackSearchTree(ThreadPool &pool);
    TransTableData defendSearchBuildToLeaf(GameTreeNode* child);
    RatingInfo2 atackSearchBuildToLeaf(GameTreeNode* child);
    void buildAtackTreeNode();
    void buildFirstChilds();
    void buildPlayer(bool recursive = true);//死四活三继续
    void buildAI(bool recursive = true);//死四活三继续
    void buildSortListInfo(int);
    void buildNodeInfo(int, int*);
    int findBestNode(int*);
    int getActiveChild();
    int getDefendChild();
    int findWorstNode();
    void printTree();
    void printTree(stringstream &f, string);
    static void buildTreeThreadFunc(int n, GameTreeNode* child);
private:
    static SortInfo *sortList;
    static ChildInfo *childsInfo;
public:
    static int resultFlag;
    static int8_t playerColor;
    static uint8_t maxSearchDepth;
    static uint8_t startStep;
    static uint8_t transTableMaxDepth;//太深的节点没必要加入置换表
    static bool multiThread;
    static size_t maxTaskNum;
    static unordered_map<uint32_t, TransTableData> transpositionTable;
    static shared_mutex mut_transTable;
    static int bestRating;
    static uint64_t hash_hit;
    static uint64_t hash_clash;
    static uint64_t hash_miss;
private:
    vector<GameTreeNode*>childs;
    ChessStep lastStep;
    HashPair hash;
    RatingInfo black, white;
    ChessBoard *chessBoard;
};

struct Task
{
    //ChildInfo* bestChild; //需要加锁？
    //int currentScore; //节点对应的最开始节点的分数，用来计算bestChild
    int index;//节点对应的最开始节点的索引
    GameTreeNode *node;//任务需要计算的节点
    int type;//计算类型
};

#define TASKTYPE_DEFEND 1
#define TASKTYPE_ATACK  2

#endif