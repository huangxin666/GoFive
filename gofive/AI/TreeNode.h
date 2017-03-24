#ifndef TREENODE_H
#define TREENODE_H

#include "ChessBoard.h"
#include "utils.h"
#define MAX_CHILD_NUM 225

class TreeNode
{
public:
    friend class ThreadPool;
    TreeNode();
    TreeNode(ChessBoard* chessBoard, int high, int temphigh, int = 0);
    ~TreeNode();
    const TreeNode& operator=(const TreeNode&);
    //AIStepResult searchBest();
    Position searchBest();
    void setPlayerColor(int);
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
    inline void addChild(TreeNode* child)
    {
        childs.push_back(child);
    }
    void deleteChild();
    void printTree();
    void printTree(stringstream &f, string);
    void debug(ThreatInfo* threatInfo);

    ThreatInfo getBestThreat();
    void buildPlayer();//死四活三继续
    void buildAtackSearchTree();
    void buildChildren();
    void buildAI();//死四活三继续	
    int findBestChild(int* childrenInfo);
    void buildChildrenInfo(int* childrenInfo, int);
    void buildSortListInfo(int, ThreatInfo*, bool*);
    void buildNodeInfo(int, int*);
    int findBestNode(int*);
    int getAtack();
    int getDefense();
    int getSpecialAtack();
    int findWorstChild();
    static void buildTreeThreadFunc(int n, ThreatInfo* threatInfo, TreeNode* child);
public:
    static int8_t playerColor;
private:
    vector<TreeNode*>childs;
    ChessStep lastStep;
    int blackThreat, whiteThreat, blackHighest, whiteHighest, currentScore;
    ChessBoard *currentBoard;
    int8_t depth, tempdepth;
};

#endif