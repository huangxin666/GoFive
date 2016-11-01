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
	void buildTree();//����������
	
	std::vector<GameTreeNode> getLeaves();
	int getThreat();
	int getScore();
	int getHighestChildScore();
	ChessBoard getChessBoard();
protected:
	bool buildChildren();//��������
	void pruneChildren();//��֦
	int threat;
	int highestChildScore;
	int score;
	int depth;
	std::vector<GameTreeNode> children;
	ChessBoard chessBoard;
};

