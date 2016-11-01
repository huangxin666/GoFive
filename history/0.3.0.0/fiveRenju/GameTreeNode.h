#pragma once
#include "stdafx.h"
#include "ChessBoard.h"
class GameTreeNode
{
public:
	GameTreeNode(ChessBoard board);
	~GameTreeNode();
	bool addChild(GameTreeNode child);
private:
	int childCount;
	GameTreeNode* children[GAMETREE_CHILD_MAX];
	ChessBoard chessBoard;
};

