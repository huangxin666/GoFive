#pragma once
#include "stdafx.h"
#include "GameTreeNode.h"
#include "ChessBoard.h"
class GameTreeHead
{
public:
	GameTreeHead(ChessBoard board);
	~GameTreeHead();
	bool addChild(GameTreeNode child);
	int getBestRoute();//获取最佳路径
private:
	int childCount;
	GameTreeNode* children[GAMETREE_CHILD_MAX];
	ChessBoard chessBoard;
};

