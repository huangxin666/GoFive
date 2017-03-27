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
    void deleteChild();
    void deleteChessBoard();
    void printTree();
    void printTree(stringstream &f, string);
    void debug(ThreatInfo* threatInfo);
    int searchBest(bool *hasSearch, ThreatInfo *threatInfo);
    int searchBest2(bool *hasSearch, ThreatInfo *threatInfo);
    ThreatInfo getBestThreat();
    void buildAtackSearchTree();
    void buildAllChild();
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
    void buildPlayer(bool recursive = true);//���Ļ�������
    void buildAI(bool recursive = true);//���Ļ�������
    int findBestChild(int* childrenInfo);
    void buildChildrenInfo(int* childrenInfo, int);
    void buildSortListInfo(int, ThreatInfo*, bool*);
    void buildNodeInfo(int, int*);
    int findBestNode(int*);
    int getAtack();
    int getDefense();
    int getSpecialAtack();
    int findWorstChild();
    static void buildTreeThreadFunc(int n, ThreatInfo* threatInfo, GameTreeNode* child);
public:
    static int8_t playerColor;
    static bool multiThread;
    static int maxTaskNum;
    static unordered_map<string, GameTreeNode*>* historymap;
private:
    vector<GameTreeNode*>childs;
    ChessStep lastStep;
    int blackThreat, whiteThreat, blackHighest, whiteHighest, currentScore;
    ChessBoard *currentBoard;
    int8_t depth, tempdepth;
};

struct Task
{
    //bool *hasSearch; //���ڼ�¼�Ƿ����һ����·
    //ChildInfo* bestChild; //��Ҫ������
    GameTreeNode *node;//������Ҫ����Ľڵ�
                   //int currentScore; //�ڵ��Ӧ���ʼ�ڵ�ķ�������������bestChild
    int index;//�ڵ��Ӧ���ʼ�ڵ������
};

#endif