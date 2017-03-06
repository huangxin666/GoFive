#ifndef TREENODE_H
#define TREENODE_H

#include "ChessBoard.h"
#include "utils.h"
#define MAX_CHILD_NUM 225

class TreeNode
{
public:
	TreeNode();
	TreeNode(ChessBoard chessBoard,int high,int temphigh,int=0);
	~TreeNode();
	const TreeNode& operator=(const TreeNode&);
	AIStep searchBest();
	Position searchBest2();
	ThreatInfo getBestThreat();
	void buildPlayer();//死四活三继续
	void setBan(bool);
	void setPlayerColor(int);
	void buildAtackSearchTree();
private:
	void buildChildren();	
	int getChildNum();
	void addChild(TreeNode *child);	
	void buildAI();//死四活三继续	
	int getHighest(int side);
	int getTotal(int side);
	int findBestChild(int *childrenInfo);
	void buildChildrenInfo(int *childrenInfo,int);
	void buildSortListInfo(int, ThreatInfo *, bool *);
	void buildNodeInfo(int, int *);
	int findBestNode(int *);
	void deleteChild();
	void printTree();
	void printTree(stringstream &f,string);
	void debug(ThreatInfo *threatInfo);
	int getAtack();
	int getDefense();
	int getSpecialAtack();
	int findWorstChild();
public:
	static bool ban;
	static int playerColor;
private:
	vector<TreeNode *>childs;
	STEP lastStep;
	int blackThreat, whiteThreat, blackHighest, whiteHighest, currentScore;
	int depth, tempdepth;
	ChessBoard *currentBoard;
};

#endif