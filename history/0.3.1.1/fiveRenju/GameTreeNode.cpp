#include "stdafx.h"
#include "GameTreeNode.h"


GameTreeNode::GameTreeNode(ChessBoard board,int dep)
{
	chessBoard = board;
	depth = dep;
	score = 0;
	highestChildScore = 0;
	threat = 0;
}

GameTreeNode::GameTreeNode()
{
	children.clear();
}

GameTreeNode::~GameTreeNode()
{
}

void GameTreeNode::setScore(int s)
{
	score = s;
}

int GameTreeNode::getScore()
{
	return score;
}

int GameTreeNode::getThreat()
{
	return threat;
}

int GameTreeNode::getHighestChildScore()
{
	return highestChildScore;
}

ChessBoard GameTreeNode::getChessBoard()
{
	return chessBoard;
}

void GameTreeNode::buildTree(){
	int flag = 0;
	if (score>998 && score<1201)
	{
		if (depth == 0)
			flag = 1;
		else if (depth == -2)
		{
			flag = 1;
			depth = 0;
		}
	}
	if (depth == -1)
	{
		flag = 1;
	}
	if (depth != 0 && score<100000 || flag == 1)
	{
		if (buildChildren())//如果出现5连珠，结束子树构建，没出现才继续构建
		{
			pruneChildren();
			for (int i = 0, size = children.size(); i < size; i++)
			{
				children[i].buildTree();//继续构建子树
			}
		}
	}
}

bool GameTreeNode::buildChildren()
{
	if (children.size() == 0)
	{
		ChessBoard tempBoard = chessBoard;
		int state = chessBoard.getPiece().getState();
		int scoreTemp = 0;
		for (int i = 0; i < BOARD_ROW_MAX; ++i)
		{
			for (int j = 0; j < BOARD_COL_MAX; ++j)
			{
				if (!chessBoard.getPiece(i, j).isHot())
					continue;
				if (chessBoard.getPiece(i, j).getState() == 0)
				{
					tempBoard.doNextStep(i, j, -state);
					scoreTemp = tempBoard.getStepScores();
					if (scoreTemp>highestChildScore)
					{
						highestChildScore = scoreTemp;
					}
					threat += scoreTemp;
					children.push_back(GameTreeNode(tempBoard, depth - 1));
					children.back().setScore(scoreTemp);
					if (scoreTemp >= 100000)
						return false;//如果出现5连珠，结束子树构建
					tempBoard = chessBoard;
				}
			}
		}	
	}
	return true;
}

void GameTreeNode::pruneChildren()
{
	//if (depth % 2 == 0)
	//{
	//	if (score == 1000 || score == 999 || score == 1030)
	//	{
	//		std::vector<GameTreeNode> children_temp;
	//		for (int i = 0,size=children.size(); i < size; i++)
	//		{
	//			
	//			if (children[i].chessBoard.getThreatInfo(-children[i].chessBoard.getPiece().getState(), 1)
	//				.HighestScore < 100000)
	//			{
	//				children_temp.push_back(children[i]);
	//			}
	//		}
	//		children = children_temp;
	//	}
	//}
	if (depth == 0)
	{
		std::vector<GameTreeNode> children_temp;
		for (int i = 0, size = children.size(); i < size; i++)
		{
			if (children[i].children.size() == 0)
			{
				children[i].buildChildren();
			}
			if (children[i].getHighestChildScore()<7999)
			{
				children_temp.push_back(children[i]);
			}
		}
		children = children_temp;
	}
	if (depth % 2 != 0)
	{
		GameTreeNode children_best;
		int high = -1000000;
		for (int i = 0, size = children.size(); i < size; i++)
		{
			if (children[i].children.size() == 0)
			{
				children[i].buildChildren();
			}
			if (children[i].getScore() - children[i].getThreat()>high)
			{
				high = children[i].getScore() - children[i].getThreat();
				children_best = children[i];
			}
		}
		children.clear();
		children_best.children.clear();
		children.push_back(children_best);
	}
}

std::vector<GameTreeNode> GameTreeNode::getLeaves()
{
	std::vector<GameTreeNode> nodes;
	std::vector<GameTreeNode> temp;
	int size = children.size();
	if (size > 0){
		if (children[0].children.size() == 0)
		{
			if (depth % 2 != 0)
			{
				for (int i = 0; i < size; i++)
				{
					children[i].children.clear();
					nodes.push_back(children[i]);
				}
			}
			
			
		}
		else
		{
			for (int i = 0; i < size; i++)
			{
				temp=children[i].getLeaves();
				for (int j = 0, s = temp.size(); j < s; j++){
					nodes.push_back(temp[j]);
				}
			}
		}
	}
	return nodes;
}
