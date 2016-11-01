#pragma once
#include "stdafx.h"
#include "ChessBoard.h"
class GameTreeNode
{
public:
	GameTreeNode(ChessBoard board, int depth);
	GameTreeNode();
	~GameTreeNode();
	bool addChild(GameTreeNode child);
	void setScore(int score);
	void buildTree();//构建博弈树
	
	std::vector<GameTreeNode> getLeaves();
	int getThreat();
	int getScore();
	int getHighestChildScore();
	ChessBoard getChessBoard();
protected:
	bool buildChildren();//构建子树
	void pruneChildren();//剪枝
	int threat;
	int highestChildScore;
	int score;
	int depth;
	std::vector<GameTreeNode> children;
	ChessBoard chessBoard;
};

