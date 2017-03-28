#ifndef GAMETREE_H
#define GAMETREE_H

#include "ChessBoard.h"
#include "utils.h"
#define MAX_CHILD_NUM 225

class GameTreeNode
{
public:
    friend class ThreadPool;
    GameTreeNode();
    GameTreeNode(ChessBoard* chessBoard, int high, int temphigh, int = 0);
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

    void debug(ThreatInfo* threatInfo);
    int searchBest(bool *hasSearch, ThreatInfo *threatInfo, ChildInfo *sortList);
    int searchBest2(bool *hasSearch, ThreatInfo *threatInfo, ChildInfo *sortList);
    ThreatInfo getBestThreat();
    void buildAtackSearchTree();
    void buildAllChild();
    void buildPlayer(bool recursive = true);//���Ļ�������
    void buildAI(bool recursive = true);//���Ļ�������
    void buildSortListInfo(int, ThreatInfo*, ChildInfo *sortList, bool*);
    void buildNodeInfo(int, int*);
    int findBestNode(int*);
    int getAtack();
    int getDefense();
    int getSpecialAtack();
    int findWorstChild();
    void printTree();
    void printTree(stringstream &f, string);
    static void buildTreeThreadFunc(int n, ThreatInfo* threatInfo, GameTreeNode* child);
public:
    static int8_t playerColor;
    static bool multiThread;
    static size_t maxTaskNum;
    static map<string, GameTreeNode*>* historymap;
private:
    vector<GameTreeNode*>childs;
    ChessStep lastStep;
    int blackThreat, whiteThreat, blackHighest, whiteHighest, lastStepScore;
    ChessBoard *chessBoard;
    int8_t depth, extraDepth;
};

struct Task
{
    //bool *hasSearch; //���ڼ�¼�Ƿ����һ����·
    //ChildInfo* bestChild; //��Ҫ������
    //int currentScore; //�ڵ��Ӧ���ʼ�ڵ�ķ�������������bestChild
    //int index;//�ڵ��Ӧ���ʼ�ڵ������
    GameTreeNode *node;//������Ҫ����Ľڵ�
};

#endif