#include "stdafx.h"
#include "TreeNode.h"
#include "hxtools.h"
#include <fstream>
using namespace std;

static int countTreeNum = 0;
static bool ban = false;


//void TreeNode::debug(THREATINFO *threatInfo)
//{
//	stringstream ss;
//	fstream of("debug.txt", ios::out);	
//	for (int n = 0; n < child_num; n++)
//		ss <<n<<":"<< threatInfo[n].HighestScore << "|" << threatInfo[n].totalScore << "\n";
//	of << ss.str().c_str();
//	of.close();
//}


TreeNode::TreeNode()
{

}

TreeNode::TreeNode(ChessBoard chessBoard, int high, bool isAI, int current)
{
	this->currentBoard = new ChessBoard;
	*currentBoard=chessBoard;
	lastStep = currentBoard->getLastStep();
	this->child_num = 0;
	this->high = high;
	THREATINFO black, white;
	black = currentBoard->getThreatInfo(1);
	white = currentBoard->getThreatInfo(-1);
	this->blackThreat = black.totalScore;
	this->whiteThreat = white.totalScore;
	this->blackHighest = black.HighestScore;
	this->whiteHighest = white.HighestScore;
	this->side = currentBoard->getPiece().getState();
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
		delete []childrenInfo;
	if (hasSearch)
		delete []hasSearch;
	if (currentBoard)
		delete currentBoard;
}

const TreeNode& TreeNode::operator=(const TreeNode& other)
{
	isAI=other.isAI;

	child_num=other.child_num;
	currentBoard = new ChessBoard;
	*currentBoard = *other.currentBoard;
	lastStep = other.lastStep;
	side=other.side;
	blackThreat = other.blackThreat;
	whiteThreat = other.whiteThreat;
	blackHighest = other.blackHighest;
	whiteHighest = other.whiteHighest;
	currentScore=other.currentScore;
	high=other.high;
	childrenInfo=0;
	hasSearch=0;
	return *this;
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

void TreeNode::setBan(bool b)
{
	ban = b;
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
	countTreeNum = 0;
	buildChildren();
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
		else if (childs[bestPos]->currentScore < 0)
			childrenInfo[bestPos] -= 100000;//保证禁手不走
		/*if (childrenInfo[bestPos] < -20000)
			break;*/

		if (hasSearch[bestPos])
			break;
		else
		{
			int t = childrenInfo[bestPos];
			childs[bestPos]->buildPlayer();
			buildChildrenInfo(bestPos);
			t = childrenInfo[bestPos];
			hasSearch[bestPos] = true;
			childs[bestPos]->deleteChild();
			count++;
		}

	}
	return AISTEP(childs[bestPos]->lastStep.uRow, childs[bestPos]->lastStep.uCol, 0);
}

int TreeNode::findBestChild()
{
	int bestScore = -500000;
	int randomStep[100];
	int randomCount = 0;
	for (int i = 0; i < child_num; i++)
	{
		if (childs[i]->currentScore >= 100000)
			return i;
		else if (childs[i]->currentScore >= 10000 && childs[i]->getHighest(-childs[i]->side)<100000)
			return i;
		if (childrenInfo[i]>bestScore)
		{
			randomCount = 0;
			bestScore = childrenInfo[i];
			randomStep[randomCount] = i;
			randomCount++;
		}
		else if (childrenInfo[i] == bestScore)
		{
			randomStep[randomCount] = i;
			randomCount++;
		}
	}
	return randomStep[rand() % randomCount];
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
	THREATINFO tempThreat(0, 0), best(0, 0);
	if (!isAI)
	{
		best.HighestScore = 500000;
		best.totalScore = 500000;
	}
	if (isAI&&child_num == 0)
		return THREATINFO(getTotal(-side), getHighest(-side));
	else if (!isAI&&child_num == 0)
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
				if (best.HighestScore - tempThreat.HighestScore < best.HighestScore / 2)
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
				if (tempThreat.HighestScore - best.HighestScore < tempThreat.HighestScore / 2)
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
	int score, tempscore;
	for (int i = 0; i < BOARD_ROW_MAX; ++i)
	{
		for (int j = 0; j < BOARD_COL_MAX; ++j)
		{
			if (currentBoard->getPiece(i, j).isHot() && currentBoard->getPiece(i, j).getState() == 0)
			{
				tempBoard = *currentBoard;
				score = currentBoard->getPiece(i, j).getThreat(-side);
				tempscore = score + currentBoard->getPiece(i, j).getThreat(side);
				tempBoard.doNextStep(i, j, -side);
				tempBoard.updateThreat(ban);
				tempNode = new TreeNode(tempBoard, high - 1, !isAI, score);
				addChild(tempNode);
			}
		}
	}
}

//多线程
struct treeInfo
{
	TreeNode child;
	THREATINFO *threatInfo;
	int n;
};

static bool *isout;
static CHILDINFO *sortList;

static bool isAllOut(int begin,int end)
{
	for (int i = begin; i < end ; i++)
	{
		if (!isout[i])
			return false;
	}
	return true;
}

static UINT buildTreeThreadFunc(LPVOID lpParam)
{
	treeInfo *info = (treeInfo*)lpParam;
	info->child.buildPlayer();
	info->threatInfo[sortList[info->n].key] = info->child.getBestThreat();
	isout[info->n] = true;
	delete info;
	return 0;
}

AISTEP TreeNode::searchBest2()
{
	bool needSearch = true;
	int bestPos;
	int searchNum = 10;
	buildChildren();
	hasSearch = new bool[child_num];
	isout = new bool[child_num];
	sortList = new CHILDINFO[child_num];
	THREATINFO *threatInfo = new THREATINFO[child_num];
	HANDLE buildTreeThread[MultipleThread_MAXIMUM];
	for (int i = 0; i < child_num; ++i)
	{
		hasSearch[i]=false;
		sortList[i].key = i;
		buildSortListInfo(i, threatInfo);
		if (childs[i]->currentScore >= 100000)
		{
			bestPos = i;
			needSearch = false;
			break;
		}
		else if (childs[i]->currentScore >= 10000 && childs[i]->getHighest(-childs[i]->side) < 100000)
		{
			bestPos = i;
			needSearch = false;
			break;
		}
		else if (childs[i]->currentScore < 0)
			sortList[i].value -= 100000;//保证禁手不走
	}
	if (needSearch)
	{
		hxtools::quicksort(sortList, 0, child_num - 1);
		while (true)
		{
			if (hasSearch[sortList[child_num - 1].key])
				break;
		

			if (searchNum>child_num)
				searchNum = child_num;
		
			for (int i = child_num - searchNum, j = 0; i < child_num; i++)
			{
				if (hasSearch[sortList[i].key])
					isout[i] = true;
				else
				{
					isout[i] = false;
					treeInfo *info = new treeInfo;
					info->child = *childs[sortList[i].key];
					info->n = i;
					info->threatInfo = threatInfo;
					buildTreeThread[j] = AfxBeginThread(buildTreeThreadFunc, info);
					j++;
					hasSearch[sortList[i].key] = true;
				}
			}
			while (!isAllOut(child_num - searchNum, child_num))
			{
				Sleep(100);
			}
			for (int i = child_num - searchNum; i < child_num; i++)
			{
				buildSortListInfo(i, threatInfo);
			}
			hxtools::quicksort(sortList, 0, child_num - 1);
			searchNum += 10;
		}

		//随机化
		int i = child_num - 1;
		while (i>0&&sortList[i-1].value == sortList[i].value)
			i--;
		int random=i+rand()%(child_num - i);
		bestPos = sortList[random].key;
	}
	delete[]isout;
	delete[]sortList;
	return AISTEP(childs[bestPos]->lastStep.uRow, childs[bestPos]->lastStep.uCol, 0);
}

void TreeNode::buildSortListInfo(int n, THREATINFO *threatInfo)
{
	int i = sortList[n].key;
	if (!hasSearch[i])
	{
		sortList[n].value = childs[i]->currentScore - childs[i]->getTotal(-childs[i]->side);
		if (sortList[n].value <= -100000)//增加堵冲四的优先级
			sortList[n].value -= 100000;
	}
	else
	{
		THREATINFO temp = threatInfo[i];
		if (temp.totalScore > childs[i]->getTotal(-childs[i]->side) && childs[i]->getHighest(-childs[i]->side) - temp.HighestScore < childs[i]->getHighest(-childs[i]->side) / 2)
		{
			sortList[n].value = childs[i]->currentScore - temp.totalScore;
		}
	}
}

//多线程end

void TreeNode::buildPlayer()
{
	if (getHighest(-side) >= 100000)
		return;
	if (high > -1 && getHighest(side) >= 100000)
	{
		ChessBoard tempBoard;
		TreeNode *tempNode;
		int score;
		for (int i = 0; i < BOARD_ROW_MAX; ++i)
		{
			for (int j = 0; j < BOARD_COL_MAX; ++j)
			{
				if (currentBoard->getPiece(i, j).isHot() && currentBoard->getPiece(i, j).getState() == 0)
				{
					if (currentBoard->getPiece(i, j).getThreat(side) >= 100000)
					{
						score = currentBoard->getPiece(i, j).getThreat(-side);
						tempBoard = *currentBoard;
						tempBoard.doNextStep(i, j, -side);
						tempBoard.updateThreat(ban);
						tempNode = new TreeNode(tempBoard, high, !isAI, score);//AI才减1
						addChild(tempNode);
					}
				}
			}
		}
	}
	else if (high > 0 && getHighest(-side) > 99 && getHighest(-side) < 1300)
	{
		ChessBoard tempBoard;
		TreeNode *tempNode;
		int score, highTemp;
		THREATINFO tempInfo;
		int worst;
		for (int i = 0; i < BOARD_ROW_MAX; ++i)
		{
			for (int j = 0; j < BOARD_COL_MAX; ++j)
			{
				if (currentBoard->getPiece(i, j).isHot() && currentBoard->getPiece(i, j).getState() == 0)
				{
					score = currentBoard->getPiece(i, j).getThreat(-side);
					if (score>99 && score < 1300)
					{
						tempBoard = *currentBoard;
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
				if (currentBoard->getPiece(i, j).isHot() && currentBoard->getPiece(i, j).getState() == 0)
				{
					score = currentBoard->getPiece(i, j).getThreat(-side);
					if (score >2000)
					{
						tempBoard = *currentBoard;
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
	if (child_num > 0)
	{
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
			bestPos = findBestNode();
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
	delete currentBoard;
	currentBoard = 0;
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
			if (currentBoard->getPiece(m, n).isHot() && currentBoard->getPiece(m, n).getState() == 0)
			{
				if (currentBoard->getPiece(m, n).getThreat(side) >= 100000)
				{
					tempBoard = *currentBoard;
					score = currentBoard->getPiece(m, n).getThreat(-side);
					tempBoard.doNextStep(m, n, -side);
					tempBoard.updateThreat(ban);
					tempNode = new TreeNode(tempBoard, high - 1, !isAI, score);
					addChild(tempNode);
				}
				else if (currentBoard->getPiece(m, n).getThreat(side) >= 10000 &&
					highest < 100000)
				{
					tempBoard = *currentBoard;
					score = currentBoard->getPiece(m, n).getThreat(-side);
					tempBoard.doNextStep(m, n, -side);
					tempBoard.updateThreat(ban);
					tempNode = new TreeNode(tempBoard, high - 1, !isAI, score);
					addChild(tempNode);
				}
				else if (getHighest(side) < 100000)//进攻就是防守
				{
					if (currentBoard->getPiece(m, n).getThreat(-side)>998 && currentBoard->getPiece(m, n).getThreat(-side) < 1100)
					{
						tempBoard = *currentBoard;
						score = currentBoard->getPiece(m, n).getThreat(-side);
						tempBoard.doNextStep(m, n, -side);
						tempBoard.updateThreat(ban);
						tempNode = new TreeNode(tempBoard, high - 1, !isAI, score);
						addChild(tempNode);
					}
				}
				else if (getHighest(side) < 10000)
				{
					if (currentBoard->getPiece(m, n).getThreat(-side)>1199 && currentBoard->getPiece(m, n).getThreat(-side) < 1300)//跳三没考虑
					{
						tempBoard = *currentBoard;
						score = currentBoard->getPiece(m, n).getThreat(-side);
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
	delete currentBoard;
	currentBoard = 0;
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
		ss << flag << "(" << getHighest(playerside) << "|" << getTotal(playerside) << ")" << "  ";

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
		ss << high << ":" << flag << "(" << getHighest(playerside) << "|" << getTotal(playerside) << ")" << "  ";

	}
	else
	{
		ss << high << ":" << flag << "{";
		for (int i = 0; i < child_num; ++i)
		{
			childs[i]->printTree(ss);
		}
		ss << "}" << "*";
	}
}