#pragma once
#include "ChessBoard.h"
#include "defines.h"
#include <sstream>
#include <string>
#include <fstream>
#define MAX_CHILD_NUM 225
class TreeNode
{
public:
	TreeNode(ChessBoard chessBoard,int high,bool isAI,int =0);
	~TreeNode();
	void buildChildren();
	int getChildNum();
	void addChild(TreeNode *child);
	ChessBoard getChessBoard();
	int getHighest(int side);
	int getTotal(int side);
	AISTEP searchBest();
	THREATINFO getBestThreat();
	void buildPlayer();//死四活三继续
	void buildPlayer(int);//死四活三继续
	void buildAI();//死四活三继续
	bool isAI;
	int getCurrentScore();
	void printTree();
	void printTree(std::stringstream &f);
private:
	int child_num;
	ChessBoard currentBoard;
	int side;
	TreeNode *childs[MAX_CHILD_NUM];
	int blackThreat, whiteThreat, blackHighest, whiteHighest, currentScore;
	int high;
	int *childrenInfo;
	bool *hasSearch;
	int findWorstChild();
	int findBestChild();
	void buildChildrenInfo(int);
	void buildNodeInfo(int);
	int findBestNode();
	void deleteChild();
};

