#include "stdafx.h"
#include "TreeNode.h"

using namespace std;

static int countTreeNum = 0;
static bool ban = true;
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
	childrenInfo = 0;
	hasSearch = 0;
	countTreeNum++;
}


TreeNode::~TreeNode()
{
	deleteChild();
	if (childrenInfo)
		delete childrenInfo;
	if (hasSearch)
		delete hasSearch;
}

void TreeNode::deleteChild()
{
	for (int i = 0; i < child_num; i++)
	{
		delete childs[i];
	}
	child_num = 0;
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

//AISTEP TreeNode::searchBest()
//{
//	int bestPos=0,bestScore=-500000;
//	THREATINFO tempThreat(0, 0);
//	int tempScore = 0;
//	for (int i = 0; i < child_num; i++)
//	{
//		if (childs[i]->currentScore>=100000)
//			return AISTEP(childs[i]->currentBoard.getPiece().getRow(), childs[i]->currentBoard.getPiece().getCol(), 0);
//		else if (childs[i]->currentScore >= 10000 && childs[i]->getHighest(-childs[i]->side)<100000)
//			return AISTEP(childs[i]->currentBoard.getPiece().getRow(), childs[i]->currentBoard.getPiece().getCol(), 0);
//		if (childs[i]->getChildNum() == 0)
//		{
//			tempScore = childs[i]->currentScore - childs[i]->getTotal(-childs[i]->side);	
//		}
//		else
//		{
//			if (childs[i]->getBestThreat().HighestScore>childs[i]->getHighest(-childs[i]->side))
//				tempScore = childs[i]->currentScore - childs[i]->getBestThreat().totalScore;
//			else
//				tempScore = childs[i]->currentScore - childs[i]->getTotal(-childs[i]->side);
//		}
//		if (tempScore>bestScore)
//		{
//			/*if (childs[i]->getChildNum() == 0)
//			{
//				childs[i]->buildPlayer();
//				if (childs[i]->getChildNum() > 0)
//				{
//					if (childs[i]->getBestThreat().HighestScore>childs[i]->getHighest(-childs[i]->side))
//						tempScore = childs[i]->currentScore - childs[i]->getBestThreat().totalScore;
//					else
//						tempScore = childs[i]->currentScore - childs[i]->getTotal(-childs[i]->side);
//				}
//			}
//			else
//			{*/
//				bestPos = i;
//				bestScore = tempScore;
//			/*}	*/
//		}
//	}
//	return AISTEP(childs[bestPos]->currentBoard.getPiece().getRow(), childs[bestPos]->currentBoard.getPiece().getCol(),0);
//}

AISTEP TreeNode::searchBest()
{
	countTreeNum = 0;
	int count = 0;
	childrenInfo = new int[child_num];
	hasSearch = new bool[child_num];
	for (int i = 0; i < child_num; ++i)
	{
		buildChildrenInfo(i);
		hasSearch[i] = false;
	}
	int bestPos;
	while (1)
	{
		bestPos = findBestChild();
		if (childs[bestPos]->currentScore >= 100000)
			break;
		else if (childs[bestPos]->currentScore >= 10000 && childs[bestPos]->getHighest(-childs[bestPos]->side) < 100000)
			break;

		/*if (childrenInfo[bestPos] < -20000)
			break;*/

		if (hasSearch[bestPos])
			break;
		else
		{
			childs[bestPos]->buildPlayer();
			buildChildrenInfo(bestPos);
			hasSearch[bestPos] = true;
			childs[bestPos]->deleteChild();
			count++;
		}
		
	}
	return AISTEP(childs[bestPos]->currentBoard.getPiece().getRow(), childs[bestPos]->currentBoard.getPiece().getCol(), 0);
}

int TreeNode::findBestChild()
{
	int bestPos = -1;
	int bestScore=-500000;
	for (int i = 0; i < child_num; i++)
	{
		if (childs[i]->currentScore >= 100000)
			return i;
		else if (childs[i]->currentScore >= 10000 && childs[i]->getHighest(-childs[i]->side)<100000)
			return i;
		if (childrenInfo[i]>bestScore)
		{
			bestPos = i;
			bestScore = childrenInfo[i];
		}
	}
	return bestPos;
}


void TreeNode::buildChildrenInfo(int i)
{
	if (childs[i]->getChildNum() == 0)
	{
		childrenInfo[i] = childs[i]->currentScore - childs[i]->getTotal(-childs[i]->side);
		if (childrenInfo[i] <= -100000)//增加堵冲四的优先级
			childrenInfo[i] -= 100000;
	}
	else
	{
		THREATINFO temp = childs[i]->getBestThreat();
		if (temp.totalScore > childs[i]->getTotal(-childs[i]->side) && childs[i]->getHighest(-childs[i]->side) - temp.HighestScore < childs[i]->getHighest(-childs[i]->side) / 2)
		{
			childrenInfo[i] = childs[i]->currentScore - temp.totalScore;
		}	
		else
			childrenInfo[i] = childs[i]->currentScore - childs[i]->getTotal(-childs[i]->side);
	}
}

THREATINFO TreeNode::getBestThreat()
{
	THREATINFO tempThreat(0, 0),best(0,0);
	if (!isAI)
	{
		best.HighestScore = 500000;
		best.totalScore = 500000;
	}	
	if (isAI&&child_num == 0)
		return THREATINFO(getTotal(-side),getHighest(-side));
	else if(!isAI&&child_num == 0)
		return THREATINFO(getTotal(side), getHighest(side));
	for (int i = 0; i < child_num; i++)
	{
		if (isAI)
		{
			tempThreat = childs[i]->getBestThreat();
			/*if (tempThreat.HighestScore>best.HighestScore)
			{
				best = tempThreat;
			}
			else if (tempThreat.HighestScore == best.HighestScore&&tempThreat.totalScore > best.totalScore)
			{
				best = tempThreat;
			}*/
			if (tempThreat.totalScore > best.totalScore)
			{
				if (best.HighestScore - tempThreat.HighestScore <best.HighestScore / 2)
					best = tempThreat;
			}
		}
		else
		{
			tempThreat = childs[i]->getBestThreat();
			/*if (tempThreat.HighestScore<best.HighestScore)
			{
				best = tempThreat;
			}
			else if (tempThreat.HighestScore == best.HighestScore&&tempThreat.totalScore < best.totalScore)
			{
				best = tempThreat;
			}*/
			if (tempThreat.totalScore < best.totalScore)
			{
				if (tempThreat.HighestScore - best.HighestScore <tempThreat.HighestScore / 2)
					best = tempThreat;
			}
		}
	}
	return best;
}

void TreeNode::buildChildren()
{
	ChessBoard tempBoard;
	TreeNode *tempNode;
	int score,tempscore;
	//AISTEP nextChild[GAMETREE_CHILD_SEARCH];
	for (int i = 0; i < BOARD_ROW_MAX; ++i)
	{
		for (int j = 0; j < BOARD_COL_MAX; ++j)
		{
			if (currentBoard.getPiece(i, j).isHot() && currentBoard.getPiece(i, j).getState() == 0)
			{
				tempBoard = currentBoard;
				score = currentBoard.getPiece(i, j).getThreat(-side);
				tempscore = score + currentBoard.getPiece(i, j).getThreat(side);
				tempBoard.doNextStep(i, j, -side);
				tempBoard.updateThreat(ban);
				//score = tempBoard.getThreatInfo(-side).totalScore - currentBoard.getThreatInfo(-side).totalScore;
				tempNode = new TreeNode(tempBoard, high - 1, !isAI, score);
				addChild(tempNode);
				//for (int n = 0; n < GAMETREE_CHILD_SEARCH; n++)
				//{
				//	if (tempscore>nextChild[n].score)
				//	{
				//		for (int m = GAMETREE_CHILD_SEARCH - 1; m >n; m--)//移位
				//		{
				//			nextChild[m] = nextChild[m - 1];
				//		}
				//		nextChild[n] = AISTEP(child_num, 0, tempscore);
				//		break;
				//	}
				//}
			}
		}
	}
	/*for (int n = 0; n < GAMETREE_CHILD_SEARCH; n++)
	{
		if (nextChild[n].x>0)
			childs[nextChild[n].x-1]->buildPlayer();
	}*/
}

void TreeNode::buildPlayer(int i)
{
	if (getHighest(-side) >= 100000)
		return;
	if (high > 0)
	{
		buildChildren();
		childrenInfo = new int[child_num];
		for (int i = 0; i < child_num; ++i)
		{
			buildChildrenInfo(i);
		}
		int bestPos, flagPos = -1;
		while (1)
		{
			bestPos = findBestChild();
			if (childs[bestPos]->currentScore >= 100000)
				break;
			else if (childs[bestPos]->currentScore >= 10000 && childs[bestPos]->getHighest(-childs[bestPos]->side) < 100000)
				break;

			if (childs[bestPos]->child_num == 0)
			{
				if (bestPos == flagPos)
					break;
				else
				{
					childs[bestPos]->buildPlayer();
					buildChildrenInfo(bestPos);
					flagPos = bestPos;
				}
			}
			else if (childs[bestPos]->child_num>0)
			{
				break;
			}
		}
	}
}

void TreeNode::buildPlayer()
{	
	if (getHighest(-side) >= 100000)
		return;
	if (high>-1&&getHighest(side) >= 100000)
	{
		ChessBoard tempBoard;
		TreeNode *tempNode;
		int score;
		for (int i = 0; i < BOARD_ROW_MAX; ++i)
		{
			for (int j = 0; j < BOARD_COL_MAX; ++j)
			{
				if (currentBoard.getPiece(i, j).isHot() && currentBoard.getPiece(i, j).getState() == 0)
				{
					if (currentBoard.getPiece(i, j).getThreat(side) >= 100000)
					{
						score = currentBoard.getPiece(i, j).getThreat(-side);
						tempBoard = currentBoard;
						tempBoard.doNextStep(i, j, -side);
						tempBoard.updateThreat(ban);
						tempNode = new TreeNode(tempBoard, high, !isAI, score);//AI才减1
						addChild(tempNode);
					}
				}
			}
		}
	}
	else if (high>0&&getHighest(-side) >99 && getHighest(-side) < 1300)
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
						tempBoard.updateThreat(ban);
						highTemp = tempBoard.getThreatInfo(side).HighestScore;
						if (highTemp >= 100000)
							continue;
						else if (highTemp >= 10000 && (score>1100 || score < 900))
							continue;
						if (child_num>4)
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
							tempNode = new TreeNode(tempBoard, high, !isAI, score);//AI才减1
							addChild(tempNode);
						}	
					}
				}
			}
		}
	}
	else if (high > 0 && getHighest(-side) > 2000)
	{
		ChessBoard tempBoard;
		TreeNode *tempNode;
		int score;
		for (int i = 0; i < BOARD_ROW_MAX; ++i)
		{
			for (int j = 0; j < BOARD_COL_MAX; ++j)
			{
				if (currentBoard.getPiece(i, j).isHot() && currentBoard.getPiece(i, j).getState() == 0)
				{
					score = currentBoard.getPiece(i, j).getThreat(-side);
					if (score >2000)
					{
						tempBoard = currentBoard;
						tempBoard.doNextStep(i, j, -side);
						tempBoard.updateThreat(ban);
						tempNode = new TreeNode(tempBoard, high, !isAI, score);//AI才减1
						addChild(tempNode);
					}
				}
			}
		}
	}
	/*for (int i = 0; i < child_num; i++)
	{
		childs[i]->buildAI();
	}*/
	childrenInfo = new int[child_num];
	hasSearch = new bool[child_num];
	for (int i = 0; i < child_num; ++i)
	{
		buildNodeInfo(i);
		hasSearch[i] = false;
	}
	int bestPos;
	while (child_num>0)
	{
		bestPos=findBestNode();
		if (hasSearch[bestPos])
			break;
		else
		{
			childs[bestPos]->buildAI();
			buildNodeInfo(bestPos);
			hasSearch[bestPos] = true;
		}
	}
}

void TreeNode::buildNodeInfo(int i)
{
	int playerside = (isAI) ? (-side) : side;

	if (childs[i]->getChildNum() == 0)
	{
		childrenInfo[i] = childs[i]->getTotal(playerside);
	}
	else
	{
		THREATINFO temp = childs[i]->getBestThreat();
		childrenInfo[i] = temp.totalScore;
	}
}

int TreeNode::findBestNode()
{
	int bestPos = -1;
	int bestScore = -500000;
	for (int i = 0; i < child_num; i++)
	{
		if (childrenInfo[i]>bestScore)
		{
			bestPos = i;
			bestScore = childrenInfo[i];
		}
	}
	return bestPos;
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
					tempBoard.updateThreat(ban);
					tempNode = new TreeNode(tempBoard, high - 1, !isAI, score);
					addChild(tempNode);
				}
				else if (currentBoard.getPiece(m, n).getThreat(side) >= 10000 &&
					highest< 100000)
				{
					tempBoard = currentBoard;
					score = currentBoard.getPiece(m, n).getThreat(-side);
					tempBoard.doNextStep(m, n, -side);
					tempBoard.updateThreat(ban);
					tempNode = new TreeNode(tempBoard, high - 1, !isAI, score);
					addChild(tempNode);
				}
				else if (getHighest(side) < 100000)//进攻就是防守
				{
					if (currentBoard.getPiece(m, n).getThreat(-side)>998 && currentBoard.getPiece(m, n).getThreat(-side) < 1100)
					{
						tempBoard = currentBoard;
						score = currentBoard.getPiece(m, n).getThreat(-side);
						tempBoard.doNextStep(m, n, -side);
						tempBoard.updateThreat(ban);
						tempNode = new TreeNode(tempBoard, high - 1, !isAI, score);
						addChild(tempNode);
					}
				}
				else if (getHighest(side) < 10000)
				{
					if (currentBoard.getPiece(m, n).getThreat(-side)>1199 && currentBoard.getPiece(m, n).getThreat(-side) < 1300)//跳三没考虑
					{
						tempBoard = currentBoard;
						score = currentBoard.getPiece(m, n).getThreat(-side);
						tempBoard.doNextStep(m, n, -side);
						tempBoard.updateThreat(ban);
						tempNode = new TreeNode(tempBoard, high - 1, !isAI, score);
						addChild(tempNode);
					}
				}
			}
		}
	}
	for (int i = 0; i < child_num; i++)
	{
		childs[i]->buildPlayer();
	}
}

void TreeNode::printTree()
{
	string s;
	stringstream ss(s);
	fstream of("tree.txt", ios::out);
	int playerside = (isAI) ? (-side) : side;
	string flag = (isAI) ? "AI" : "人";

	if (child_num == 0)
	{
		ss << flag << "(" << getHighest(playerside)<< "|" << getTotal(playerside) << ")" << "  ";
		
	}
	else
	{
		for (int i = 0; i < child_num; ++i)
		{
			childs[i]->printTree(ss); ss << "\n";
		}
	}
	of << ss.str().c_str();
	of.close();
}

void TreeNode::printTree(stringstream &ss)
{
	int playerside = (isAI) ? (-side) : side;
	string flag = (isAI) ? "AI" : "人";

	if (child_num == 0)
	{
		ss << high << ":"<<flag << "(" << getHighest(playerside) << "|" << getTotal(playerside) << ")" << "  ";

	}
	else
	{
		ss << high<<":"<<flag << "{";
		for (int i = 0; i < child_num; ++i)
		{
			childs[i]->printTree(ss);
		}
		ss << "}" << "*";
	}
}