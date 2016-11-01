#pragma once
#include "ChessBoard.h"
#include "defines.h"
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
	void buildAI();//死四活三继续
	bool isAI;
	int getCurrentScore();
private:
	TreeNode *childs[MAX_CHILD_NUM];
	int child_num;
	ChessBoard currentBoard;
	int high;
	int blackThreat, whiteThreat,blackHighest,whiteHighest,currentScore;
	int side;
	int findWorstChild();
	
};

