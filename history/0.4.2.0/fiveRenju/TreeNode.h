#pragma once
#include "ChessBoard.h"
#include "defines.h"
#include <sstream>

#define MAX_CHILD_NUM 225
class TreeNode
{
public:
	TreeNode();
	TreeNode(ChessBoard chessBoard,int high,bool isAI,int =0);
	~TreeNode();
	const TreeNode& operator=(const TreeNode&);
	void buildChildren();
	int getChildNum();
	void addChild(TreeNode *child);
	int getHighest(int side);
	int getTotal(int side);
	AISTEP searchBest();
	AISTEP searchBest2();
	THREATINFO getBestThreat();
	void buildPlayer();//死四活三继续
	void buildAI();//死四活三继续	
	int getCurrentScore();
	void setBan(bool);
private:
	int findWorstChild();
	int findBestChild();
	void buildChildrenInfo(int);
	void buildSortListInfo(int, THREATINFO *);
	void buildNodeInfo(int);
	int findBestNode();
	void deleteChild();
	void printTree();
	void printTree(std::stringstream &f);
	void debug(THREATINFO *threatInfo);
	int getAtack();
	int getDefense();
public:
	bool isAI;
private:
	byte child_num;
	STEP lastStep;
	int side;
	TreeNode *childs[MAX_CHILD_NUM];
	int blackThreat, whiteThreat, blackHighest, whiteHighest, currentScore;
	int high;
	ChessBoard *currentBoard;
	int *childrenInfo;
	bool *hasSearch;
};

