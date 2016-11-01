#pragma once
#include "ChessBoard.h"
#include "defines.h"
#define MAX_CHILD_NUM 225

class TreeNode
{
public:
	TreeNode();
	TreeNode(ChessBoard chessBoard,int high,int temphigh,int =0);
	~TreeNode();
	const TreeNode& operator=(const TreeNode&);
	AISTEP searchBest();
	AISTEP searchBest2();
	THREATINFO getBestThreat();
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
	void buildSortListInfo(int, THREATINFO *, bool *);
	void buildNodeInfo(int, int *);
	int findBestNode(int *);
	void deleteChild();
	void printTree();
	void printTree(stringstream &f,string);
	void debug(THREATINFO *threatInfo);
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
	short high, temphigh;
	ChessBoard *currentBoard;
};