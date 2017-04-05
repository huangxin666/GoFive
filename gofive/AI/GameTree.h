#ifndef GAMETREE_H
#define GAMETREE_H

#include "ChessBoard.h"
#include "utils.h"
#include <memory>
#include <unordered_map>
#define MAX_CHILD_NUM 225


struct transTableData
{
    uint64_t checksum;
    RatingInfo black;
    RatingInfo white;
    int depth;
};

class GameTreeNode
{
public:
    //shared_ptr<GameTreeNode>;
    friend class ThreadPool;
    GameTreeNode();
    GameTreeNode(ChessBoard* chessBoard, int high, int temphigh);
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
    void addChild(int &row, int &col, int depth_d, int extra_d);
    void deleteChild();
    void deleteChessBoard();

    void debug(ChildInfo* threatInfo);
    int searchBest(ChildInfo *threatInfo, SortInfo *sortList);
    int searchBest2(ChildInfo *threatInfo, SortInfo *sortList);
    RatingInfo getBestRating();
    void buildAtackSearchTree();
    void buildFirstChilds();
    void buildPlayer(bool recursive = true);//死四活三继续
    void buildAI(bool recursive = true);//死四活三继续
    void buildSortListInfo(int, ChildInfo*, SortInfo *sortList);
    void buildNodeInfo(int, int*);
    int findBestNode(int*);
    int getAtackChild(ChildInfo *childsInfo);
    int getDefendChild();
    int getSpecialAtack(ChildInfo *childsInfo);
    int findWorstNode();
    void printTree();
    void printTree(stringstream &f, string);
    //static void initTranspositionTable();
    static void buildTreeThreadFunc(int n, ChildInfo* threatInfo, GameTreeNode* child);
public:
    static int8_t playerColor;
    static bool multiThread;
    static size_t maxTaskNum;
    static unordered_map<uint32_t, transTableData> transpositionTable;
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
    int8_t depth, extraDepth;
};

struct Task
{
    //bool *hasSearch; //用于记录是否完成一条线路
    //ChildInfo* bestChild; //需要加锁？
    //int currentScore; //节点对应的最开始节点的分数，用来计算bestChild
    ChildInfo *threatInfo;
    int index;//节点对应的最开始节点的索引
    GameTreeNode *node;//任务需要计算的节点
};

#endif