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
	int getBestRoute();//��ȡ���·��
private:
	int childCount;
	GameTreeNode* children[GAMETREE_CHILD_MAX];
	ChessBoard chessBoard;
};

