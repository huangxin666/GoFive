#pragma once
#include "stdafx.h"
#include "GameTreeNode.h"
#include "ChessBoard.h"
class GameTreeHead:public GameTreeNode
{
public:
	GameTreeHead(ChessBoard board,int depth);
	~GameTreeHead();

	ChessBoard getBestRoute();//获取最佳路径
};

