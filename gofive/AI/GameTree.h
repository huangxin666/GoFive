#ifndef GAMETREE_H
#define GAMETREE_H

#include "ChessBoard.h"
#include "utils.h"
#include <memory>
#define MAX_CHILD_NUM 225

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
private:
    inline int getChildNum()
    {
        return childs.size();
    }
    inline int getHighest(int side)
    {
        return (side == 1) ? blackHighest : whiteHighest;
    }
    inline int getTotal(int side)
    {
        return (side == 1) ? blackThreat : whiteThreat;
    }
    inline void addChild(GameTreeNode* child)
    {
        childs.push_back(child);
        childs_isref.push_back(false);
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
    void deleteChild();
    void deleteChessBoard();

    void debug(ChildInfo* threatInfo);
    int searchBest(ChildInfo *threatInfo, SortInfo *sortList);
    int searchBest2(ChildInfo *threatInfo, SortInfo *sortList);
    RatingInfo getBestRating();
    void buildAtackSearchTree();
    void buildFirstChilds();
    void buildPlayer(bool recursive = true);//���Ļ�������
    void buildAI(bool recursive = true);//���Ļ�������
    void buildSortListInfo(int, ChildInfo*, SortInfo *sortList);
    void buildNodeInfo(int, int*);
    int findBestNode(int*);
    int getAtackChild(ChildInfo *childsInfo);
    int getDefendChild();
    int getSpecialAtack(ChildInfo *childsInfo);
    int findWorstNode();
    void printTree();
    void printTree(stringstream &f, string);
    static void buildTreeThreadFunc(int n, ChildInfo* threatInfo, GameTreeNode* child);
public:
    static int8_t playerColor;
    static bool multiThread;
    static size_t maxTaskNum;
    static vector<map<string, GameTreeNode*>> historymaps;
    static int bestRating;
private:
    vector<GameTreeNode*>childs;
    vector<bool>childs_isref;
    ChessStep lastStep;
    int blackThreat, whiteThreat, blackHighest, whiteHighest;
    ChessBoard *chessBoard;
    int8_t depth, extraDepth;
};

struct Task
{
    //bool *hasSearch; //���ڼ�¼�Ƿ����һ����·
    //ChildInfo* bestChild; //��Ҫ������
    //int currentScore; //�ڵ��Ӧ���ʼ�ڵ�ķ�������������bestChild
    ChildInfo *threatInfo;
    int index;//�ڵ��Ӧ���ʼ�ڵ������
    GameTreeNode *node;//������Ҫ����Ľڵ�
};

#endif