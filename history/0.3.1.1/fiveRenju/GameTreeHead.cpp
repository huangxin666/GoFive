#include "stdafx.h"
#include "GameTreeHead.h"


GameTreeHead::GameTreeHead(ChessBoard board,int dep)
{
	chessBoard = board;
	depth = dep;
	score = 0;
	highestChildScore = 0;
	threat = 0;
}
//建立子树
//剪枝
//构建子树的子树

GameTreeHead::~GameTreeHead()
{

}

ChessBoard GameTreeHead::getBestRoute()
{
	std::vector<GameTreeNode> nodes = getLeaves();
	int size = nodes.size();
	int best = 0;
	for (int i = 1; i < size; i++)
	{
		if ((nodes[i].getScore() - nodes[i].getThreat())>(nodes[best].getScore() - nodes[best].getThreat()))
		{
			best = i;
		}
	}
	return nodes[best].getChessBoard();
}