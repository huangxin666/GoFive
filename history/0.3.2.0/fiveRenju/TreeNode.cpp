#include "stdafx.h"
#include "TreeNode.h"
#include <sstream>
#include <string>
#include <fstream>
using namespace std;

static int countTreeNum = 0;
CString debug(int mode,ChessBoard cs)
{
	CString info;
	string s;
	stringstream ss(s);
	fstream of("debug.txt", ios::out);
	ChessBoard temp = cs;
	if (mode == 1)
	{
		//temp.setGlobalThreat(1);
		ss << " " << "\t";
		for (int j = 0; j < BOARD_COL_MAX; ++j)
			ss << j << "\t";
		ss << "\n";
		for (int i = 0; i < BOARD_ROW_MAX; ++i)
		{
			ss << i << "\t";
			for (int j = 0; j < BOARD_COL_MAX; ++j)
			{
				ss << temp.getPiece(i, j).getThreat(1) << "|"
					<< temp.getPiece(i, j).getThreat(-1) << "\t";
			}
			ss << "\n\n\n";
		}
		ss << temp.getThreatInfo(1).HighestScore << "," << temp.getThreatInfo(1).totalScore
			<< "|" << temp.getThreatInfo(-1).HighestScore << "," << temp.getThreatInfo(-1).totalScore;


	}
	of << ss.str().c_str();
	of.close();
	return CString(ss.str().c_str());
}

TreeNode::TreeNode(ChessBoard chessBoard,int high,bool isAI,int current)
{
	this->currentBoard = chessBoard;
	this->child_num = 0;
	this->high = high;
	THREATINFO black, white;
	black = currentBoard.getThreatInfo(1);
	white = currentBoard.getThreatInfo(-1);
	this->blackThreat = black.totalScore;
	this->whiteThreat = white.totalScore;
	this->blackHighest = black.HighestScore;
	this->whiteHighest = white.HighestScore;
	this->side = currentBoard.getPiece().getState();
	this->isAI = isAI;
	this->currentScore = current;
	countTreeNum++;
}


TreeNode::~TreeNode()
{
	for (int i = 0; i < child_num; i++)
	{
		delete childs[i];
	}
	countTreeNum--;
}

int TreeNode::getChildNum()
{
	return child_num;
}

ChessBoard TreeNode::getChessBoard()
{
	return currentBoard;
}

int TreeNode::getCurrentScore()
{
	return currentScore;
}

int TreeNode::getHighest(int side)
{
	if (side == 1)
		return blackHighest;
	else
		return whiteHighest;
}

int TreeNode::getTotal(int side)
{
	if (side == 1)
		return blackThreat;
	else
		return whiteThreat;
}

void TreeNode::addChild(TreeNode *child)
{
	childs[child_num] = child;
	child_num++;
}

int TreeNode::findWorstChild()
{
	int min = 0;
	if (child_num > 0)
	{	
		int  score = childs[0]->getTotal(-side);
		for (int i = 1; i < child_num; i++)
		{
			if (childs[i]->getTotal(-side) < score)
			{
				score = childs[i]->getTotal(-side);
				min = i;
			}
		}
	}
	return min;
}

AISTEP TreeNode::searchBest()
{
	AISTEP best(0,0,0);
	int bestPos=0,bestScore=-100000;
	THREATINFO tempThreat(0, 0);
	int tempScore = 0;
	for (int i = 0; i < child_num; i++)
	{
		if (childs[i]->currentScore>=100000)
			return AISTEP(childs[i]->currentBoard.getPiece().getRow(), childs[i]->currentBoard.getPiece().getCol(), 0);
		else if (childs[i]->currentScore >= 10000 && childs[i]->getHighest(-childs[i]->side)<100000)
			return AISTEP(childs[i]->currentBoard.getPiece().getRow(), childs[i]->currentBoard.getPiece().getCol(), 0);
		if (childs[i]->getChildNum() == 0)
		{
			tempScore = childs[i]->currentScore - childs[i]->getTotal(-childs[i]->side);	
		}
		else
		{
			if (childs[i]->getBestThreat().HighestScore>childs[i]->getHighest(-childs[i]->side))
				tempScore = childs[i]->currentScore - childs[i]->getBestThreat().totalScore;
			else
				tempScore = childs[i]->currentScore - childs[i]->getTotal(-childs[i]->side);
		}
		if (tempScore>bestScore)
		{
			bestPos = i;
			bestScore = tempScore;
		}
	}
	return AISTEP(childs[bestPos]->currentBoard.getPiece().getRow(), childs[bestPos]->currentBoard.getPiece().getCol(),0);
}

THREATINFO TreeNode::getBestThreat()
{
	THREATINFO tempThreat(0, 0),best(0,0);
	if (!isAI)
		best.HighestScore = 200000;
	if (child_num == 0)
		return THREATINFO(getTotal(-side),getHighest(-side));
	for (int i = 0; i < child_num; i++)
	{
		if (isAI)
		{
			tempThreat = childs[i]->getBestThreat();
			if (tempThreat.HighestScore>best.HighestScore)
			{
				best = tempThreat;
			}
			else if (tempThreat.HighestScore == best.HighestScore&&tempThreat.totalScore > best.totalScore)
			{
				best = tempThreat;
			}
		}
		else
		{
			tempThreat = childs[i]->getBestThreat();
			if (tempThreat.HighestScore<best.HighestScore)
			{
				best = tempThreat;
			}
			else if (tempThreat.HighestScore == best.HighestScore&&tempThreat.totalScore < best.totalScore)
			{
				best = tempThreat;
			}
		}
	}
	return best;
}

void TreeNode::buildChildren()
{
	countTreeNum = 0;
	ChessBoard tempBoard;
	TreeNode *tempNode;
	int score,highest=-500000,lowest=500000;
	for (int i = 0; i < BOARD_ROW_MAX; ++i)
	{
		for (int j = 0; j < BOARD_COL_MAX; ++j)
		{
			if (currentBoard.getPiece(i, j).isHot() && currentBoard.getPiece(i, j).getState() == 0)
			{
				tempBoard = currentBoard;
				score = currentBoard.getPiece(i, j).getThreat(-side);
				tempBoard.doNextStep(i, j, -side);
				tempBoard.updateThreat(true);
				//score = tempBoard.getThreatInfo(-side).totalScore - currentBoard.getThreatInfo(-side).totalScore;
				tempNode = new TreeNode(tempBoard, high - 1, !isAI, score);
				addChild(tempNode);
				if (childs[child_num - 1]->getCurrentScore() - childs[child_num - 1]->getTotal(side)>highest)
					highest = childs[child_num - 1]->getCurrentScore() - childs[child_num - 1]->getTotal(side);
				if (childs[child_num - 1]->getCurrentScore() - childs[child_num - 1]->getTotal(side) < lowest)
					lowest = childs[child_num - 1]->getCurrentScore() - childs[child_num - 1]->getTotal(side);
			}
		}
	}
	highest =( highest + lowest)/2;
	for (int i = 0; i < child_num; i++)
	{
		if (childs[i]->getCurrentScore() - childs[i]->getTotal(side)>highest)
			childs[i]->buildPlayer();
	}
}

void TreeNode::buildPlayer()
{	
	if (high>0&&getHighest(-side) >99 && getHighest(-side) < 1300)
	{
		ChessBoard tempBoard;
		TreeNode *tempNode;
		int score,highTemp;
		THREATINFO tempInfo;
		int worst;
		for (int i = 0; i < BOARD_ROW_MAX; ++i)
		{
			for (int j = 0; j < BOARD_COL_MAX; ++j)
			{
				if (currentBoard.getPiece(i, j).isHot() && currentBoard.getPiece(i, j).getState() == 0)
				{
					score = currentBoard.getPiece(i, j).getThreat(-side);
					if (score>99 && score < 1300)
					{
						tempBoard = currentBoard;
						tempBoard.doNextStep(i, j, -side);
						tempBoard.updateThreat(true);
						highTemp = tempBoard.getThreatInfo(side).HighestScore;
						if (highTemp >= 100000)
							continue;
						else if (highTemp >= 10000 && (score>1100 || score < 900))
							continue;
						if (child_num>5)
						{
							tempInfo = tempBoard.getThreatInfo(-side);
							worst = findWorstChild();
							if (childs[worst]->getTotal(-side) < tempInfo.totalScore)
							{
								delete childs[worst];
								childs[worst] = new TreeNode(tempBoard, high, !isAI, score);
							}
						}
						else
						{
							tempNode = new TreeNode(tempBoard, high, !isAI, score);//AI²Å¼õ1
							addChild(tempNode);
						}	
					}
				}
			}
		}
	}
	for (int i = 0; i < child_num; i++)
	{
		childs[i]->buildAI();
	}
}

void TreeNode::buildAI()
{
	ChessBoard tempBoard;
	TreeNode *tempNode;
	int score;
	int highest = getHighest(-side);
	for (int m = 0; m < BOARD_ROW_MAX; ++m)
	{
		for (int n = 0; n < BOARD_COL_MAX; ++n)
		{
			if (currentBoard.getPiece(m, n).isHot() && currentBoard.getPiece(m, n).getState() == 0)
			{
				if (currentBoard.getPiece(m, n).getThreat(side) >= 100000)
				{
					tempBoard = currentBoard;
					score = currentBoard.getPiece(m, n).getThreat(-side);
					tempBoard.doNextStep(m, n, -side);
					tempBoard.updateThreat(true);
					tempNode = new TreeNode(tempBoard, high - 1, !isAI, score);
					addChild(tempNode);
				}
				else if (currentBoard.getPiece(m, n).getThreat(side) >= 10000 &&
					highest< 100000)
				{
					tempBoard = currentBoard;
					score = currentBoard.getPiece(m, n).getThreat(-side);
					tempBoard.doNextStep(m, n, -side);
					tempBoard.updateThreat(true);
					tempNode = new TreeNode(tempBoard, high - 1, !isAI, score);
					addChild(tempNode);
				}
			}
		}
	}
	for (int i = 0; i < child_num; i++)
	{
		childs[i]->buildPlayer();
	}
}