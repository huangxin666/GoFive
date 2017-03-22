#include "TreeNode.h"
#include "utils.h"
#include <thread>

static int countTreeNum = 0;
bool TreeNode::ban = false;
int8_t TreeNode::playerColor = 1;
int8_t TreeNode::level = 0;

void TreeNode::debug(ThreatInfo *threatInfo)
{
    stringstream ss;
    fstream of("debug.txt", ios::out);
    for (size_t n = 0; n < childs.size(); n++)
        ss << n << ":" << threatInfo[n].HighestScore << "|" << threatInfo[n].totalScore << "\n";
    of << ss.str().c_str();
    of.close();
}


TreeNode::TreeNode()
{

}

TreeNode::TreeNode(ChessBoard *chessBoard, int depth, int tempdepth, int score) :
    depth(depth), tempdepth(tempdepth), currentScore(score)
{
    this->currentBoard = new ChessBoard;
    *currentBoard = *chessBoard;
    lastStep = currentBoard->lastStep;
    ThreatInfo black = currentBoard->getThreatInfo(1);
    ThreatInfo white = currentBoard->getThreatInfo(-1);
    this->blackThreat = black.totalScore;
    this->whiteThreat = white.totalScore;
    this->blackHighest = black.HighestScore;
    this->whiteHighest = white.HighestScore;
    countTreeNum++;
}


TreeNode::~TreeNode()
{
    deleteChild();
    if (currentBoard) delete currentBoard;
}

const TreeNode& TreeNode::operator=(const TreeNode& other)
{
    if (other.currentBoard)
    {
        currentBoard = new ChessBoard;
        *currentBoard = *other.currentBoard;
    }
    else
    {
        currentBoard = NULL;
    }
    lastStep = other.lastStep;
    blackThreat = other.blackThreat;
    whiteThreat = other.whiteThreat;
    blackHighest = other.blackHighest;
    whiteHighest = other.whiteHighest;
    currentScore = other.currentScore;
    depth = other.depth;
    tempdepth = other.tempdepth;
    return *this;
}

void TreeNode::deleteChild()
{
    for (size_t i = 0; i < childs.size(); i++)
    {
        delete childs[i];
    }
    childs.clear();
}

void TreeNode::setBan(bool b)
{
    ban = b;
}

void TreeNode::setLevel(int l)
{
    level = l;
}

void TreeNode::setPlayerColor(int color)
{
    playerColor = color;
}

//AIStepResult TreeNode::searchBest()
//{
//	countTreeNum = 0;
//	buildChildren();
//	int count = 0;
//	int *childrenInfo = new int[childs.size()];
//	bool *hasSearch = new bool[childs.size()];
//	for (size_t i = 0; i < childs.size(); ++i)
//	{
//		buildChildrenInfo(childrenInfo, i);
//		hasSearch[i] = false;
//	}
//	int bestPos;
//	while (1)
//	{
//		bestPos = findBestChild(childrenInfo);
//		if (childs[bestPos]->currentScore >= 100000)
//			break;
//		else if (childs[bestPos]->currentScore >= 10000 && childs[bestPos]->getHighest(lastStep.getColor()) < 100000)
//			break;
//		else if (childs[bestPos]->currentScore < 0)
//			childrenInfo[bestPos] -= 100000;//保证禁手不走
//		/*if (childrenInfo[bestPos] < -20000)
//			break;*/
//
//		if (hasSearch[bestPos])
//			break;
//		else
//		{
//			childs[bestPos]->buildPlayer();
//			buildChildrenInfo(childrenInfo, bestPos);
//			hasSearch[bestPos] = true;
//			childs[bestPos]->deleteChild();
//			count++;
//		}
//
//	}
//	delete[]childrenInfo;
//	delete[]hasSearch;
//	return AIStepResult(childs[bestPos]->lastStep.uRow, childs[bestPos]->lastStep.uCol, 0);
//}

int TreeNode::findBestChild(int *childrenInfo)
{
    int bestScore = -500000;
    int randomStep[100];
    int randomCount = 0;
    for (size_t i = 0; i < childs.size(); i++)
    {
        if (childs[i]->currentScore >= 100000)
        {
            return i;
        }
        else if (childs[i]->currentScore >= 10000 && childs[i]->getHighest(lastStep.getColor()) < 100000)
        {
            return i;
        }
        if (childrenInfo[i] > bestScore)
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

void TreeNode::buildChildrenInfo(int *childrenInfo, int i)
{
    if (childs[i]->getChildNum() == 0)
    {
        childrenInfo[i] = childs[i]->currentScore - childs[i]->getHighest(lastStep.getColor()) - childs[i]->getTotal(lastStep.getColor()) / 10;
        if (childrenInfo[i] <= -100000)//增加堵冲四的优先级
        {
            childrenInfo[i] -= 100000;
        }
    }
    else
    {
        ThreatInfo temp = childs[i]->getBestThreat();
        int tempscore = childs[i]->currentScore - temp.totalScore - temp.totalScore / 10;
        if (tempscore < childrenInfo[i])
        {
            childrenInfo[i] = tempscore;
        }
    }
}

ThreatInfo TreeNode::getBestThreat()
{
    ThreatInfo tempThreat(0, 0), best(0, 0);
    if (!(lastStep.getColor() != playerColor))
    {
        best.HighestScore = 500000;
        best.totalScore = 500000;
    }
    if (lastStep.getColor() != playerColor&&childs.size() == 0)
    {
        return ThreatInfo(getTotal(-lastStep.getColor()), getHighest(-lastStep.getColor()));
    }
    else if (!(lastStep.getColor() != playerColor) && childs.size() == 0)
    {
        return ThreatInfo(getTotal(lastStep.getColor()), getHighest(lastStep.getColor()));
    }
    for (size_t i = 0; i < childs.size(); i++)
    {
        if ((lastStep.getColor() != playerColor))
        {
            tempThreat = childs[i]->getBestThreat();
            if (tempThreat.totalScore > best.totalScore)
            {
                if (best.HighestScore - tempThreat.HighestScore < best.HighestScore / 2)
                {
                    best = tempThreat;
                }
            }
        }
        else
        {
            tempThreat = childs[i]->getBestThreat();
            if (tempThreat.totalScore < best.totalScore)
            {
                if (tempThreat.HighestScore - best.HighestScore < tempThreat.HighestScore / 2)
                {
                    best = tempThreat;
                }
            }
        }
    }
    return best;
}

void TreeNode::buildChildren()
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
                tempBoard = *currentBoard;
                //score = currentBoard->getPiece(i, j).getThreat(-side);
                tempBoard.doNextStep(i, j, -lastStep.getColor());
                tempBoard.updateThreat(ban);
                score = tempBoard.getLastStepScores(ban, false);
                tempNode = new TreeNode(&tempBoard, depth - 1, tempdepth, score);
                addChild(tempNode);
            }
        }
    }
}

//多线程
static ChildInfo *sortList;

void TreeNode::buildTreeThreadFunc(int n, ThreatInfo *threatInfo, TreeNode *child)
{
    child->buildPlayer();
    threatInfo[sortList[n].key] = child->getBestThreat();
    delete child;
}

Position TreeNode::searchBest()
{
    bool needSearch = true;
    int bestPos;
    size_t searchNum = 10;
    buildChildren();
    bool *hasSearch = new bool[childs.size()];
    ThreatInfo *threatInfo = new ThreatInfo[childs.size()];
    sortList = new ChildInfo[childs.size()];
    thread buildTreeThread[MAXTHREAD];
    int tempi = getSpecialAtack();
    for (size_t i = 0; i < childs.size(); ++i)
    {
        hasSearch[i] = false;
        sortList[i].key = i;
        buildSortListInfo(i, threatInfo, hasSearch);
        if (childs[i]->currentScore >= 100000)
        {
            bestPos = i;
            needSearch = false;
        }
        else if (childs[i]->currentScore > 10000 && childs[i]->getHighest(-childs[i]->lastStep.getColor()) < 100000)
        {
            bestPos = i;
            needSearch = false;
        }
        else if (childs[i]->currentScore < 0)
        {
            sortList[i].value -= 100000;//保证禁手不走
        }
    }

    if (needSearch)
    {
        sort(sortList, 0, childs.size() - 1);
        while (true)
        {
            if (hasSearch[sortList[childs.size() - 1].key]) break;

            if (searchNum > childs.size()) 
            {
                searchNum = childs.size();
            }

            int j = 0;
            for (size_t i = childs.size() - searchNum; i < childs.size(); i++)
            {
                if (!hasSearch[sortList[i].key])
                {
                    TreeNode *node = new TreeNode;
                    *node = *childs[sortList[i].key];
                    buildTreeThread[j] = thread(buildTreeThreadFunc, i, threatInfo, node);
                    j++;
                    hasSearch[sortList[i].key] = true;
                }
            }

            for (int i = 0; i < j; i++)
            {
                buildTreeThread[i].join();
            }

            for (size_t i = childs.size() - searchNum; i < childs.size(); i++)
            {
                buildSortListInfo(i, threatInfo, hasSearch);
            }
            sort(sortList, 0, childs.size() - 1);
            searchNum += 10;
        }

        //随机化
        int i = childs.size() - 1;
        while (i > 0 && sortList[i - 1].value == sortList[i].value) i--;
        bestPos = i + rand() % (childs.size() - i);
    }

    Position result;

    int planB = getAtack();
    for (size_t i = 0; i < childs.size(); ++i)
    {
        if (planB == sortList[i].key)
        {
            planB = i; break;
        }
    }

    if (!hasSearch[sortList[planB].key])//如果没深度搜索过，则深度搜索，防止走向失败
    {
        childs[sortList[planB].key]->buildPlayer();
        hasSearch[sortList[planB].key] = true;
        threatInfo[sortList[planB].key] = childs[sortList[planB].key]->getBestThreat();
        //childs[sortList[planB].key]->printTree();
        buildSortListInfo(planB, threatInfo, hasSearch);
        childs[sortList[planB].key]->deleteChild();
    }

    /*childs[sortList[bestPos].key]->buildPlayer();
    hasSearch[sortList[bestPos].key] = true;
    threatInfo[sortList[bestPos].key] = childs[sortList[bestPos].key]->getBestThreat();
    childs[sortList[bestPos].key]->printTree();
    childs[sortList[bestPos].key]->deleteChild();*/

    if (childs[sortList[planB].key]->currentScore >= 100000 ||
        (childs[sortList[planB].key]->currentScore >= 10000 && childs[sortList[planB].key]->getHighest(lastStep.getColor()) < 100000 && threatInfo[sortList[planB].key].HighestScore < 100000))
    {
        result = Position{ childs[sortList[planB].key]->lastStep.uRow, childs[sortList[planB].key]->lastStep.uCol };
    }
    else if (playerColor == 1 && lastStep.step < 10)//防止开局被布阵
    {
        planB = getDefense();
        result = Position{ childs[planB]->lastStep.uRow, childs[planB]->lastStep.uCol };
    }
    else if (threatInfo[sortList[bestPos].key].HighestScore > 80000 ||
        (threatInfo[sortList[bestPos].key].HighestScore >= 10000 && (childs[sortList[bestPos].key]->currentScore <= 1200 || (childs[sortList[bestPos].key]->currentScore >= 8000 && childs[sortList[bestPos].key]->currentScore < 10000))))
    {
        if (tempi > -1 && childs[tempi]->currentScore > 1200 && childs[tempi]->getHighest(lastStep.getColor()) < 100000)
        {
            result = Position{ childs[tempi]->lastStep.uRow, childs[tempi]->lastStep.uCol };
        }
        else
        {
            planB = getDefense();
            if (currentBoard->getPiece(childs[planB]->lastStep.uRow, childs[planB]->lastStep.uCol).getThreat(lastStep.getColor()) > 2000)
                result = Position{ childs[planB]->lastStep.uRow, childs[planB]->lastStep.uCol };
            else
                result = Position{ childs[sortList[bestPos].key]->lastStep.uRow, childs[sortList[bestPos].key]->lastStep.uCol };
        }
    }
    else if (tempi > -1)
    {
        result = Position{ childs[tempi]->lastStep.uRow, childs[tempi]->lastStep.uCol };
    }
    else if (threatInfo[sortList[planB].key].HighestScore <= 8000 && childs[sortList[planB].key]->currentScore > 1000)
    {
        result = Position{ childs[sortList[planB].key]->lastStep.uRow, childs[sortList[planB].key]->lastStep.uCol };
    }
    else
    {
        result = Position{ childs[sortList[bestPos].key]->lastStep.uRow, childs[sortList[bestPos].key]->lastStep.uCol };
    }
        
    delete[] sortList;
    sortList = 0;
    delete[] threatInfo;
    delete[] hasSearch;
    return result;
}

int TreeNode::getAtack()
{
    int max = INT_MIN, flag = 0, temp;
    for (size_t i = 0; i < childs.size(); ++i)
    {
        temp = currentBoard->getPiece(childs[i]->lastStep).getThreat(lastStep.getColor()) / 50;
        if (childs[i]->currentScore + temp > max)
        {
            max = childs[i]->currentScore + temp;
            flag = i;
        }
    }
    return flag;
}

int TreeNode::getSpecialAtack()
{
    if (getHighest(lastStep.getColor()) >= 100000)
        return -1;
    int max = 3000, flag = -1, temp;
    for (size_t i = 0; i < childs.size(); ++i)
    {
        if (childs[i]->currentScore >= 1210 && childs[i]->currentScore < 2000 /*||
            (getHighest(side) < 10000 && childs[i]->currentScore < 1210 && childs[i]->currentScore>1000)*/)
        {
            ChessBoard tempboard = *childs[i]->currentBoard;
            temp = tempboard.getAtackScore(childs[i]->currentScore, getTotal(lastStep.getColor()), ban);
            if (temp > max)
            {
                max = temp;
                flag = i;
            }
            if (temp < 3000)//减小无意义冲四的优先级
            {
                if (currentBoard->getPiece(childs[i]->lastStep.uRow, childs[i]->lastStep.uCol).getThreat(lastStep.getColor()) < 8000)
                {
                    childs[i]->currentScore -= 10000;
                }
            }//可能出现权重BUG
        }
    }
    return flag;
}

int TreeNode::getDefense()
{
    int min = INT_MAX, temp;
    vector<int> results;
    for (size_t i = 0; i < childs.size(); ++i)
    {
        /*if (currentBoard->getPiece(childs[i]->lastStep).getThreat(-lastStep.getColor()) < 1200 && currentBoard->getPiece(childs[i]->lastStep).getThreat(-lastStep.getColor()) > 900)
        {
            if (-currentBoard->getPiece(childs[i]->lastStep).getThreat(lastStep.getColor()) < min)
            {
                min = -currentBoard->getPiece(childs[i]->lastStep).getThreat(lastStep.getColor());
                flag = i;
            }
        }
        else
        {*/
        temp = currentBoard->getPiece(childs[i]->lastStep).getThreat(-lastStep.getColor()) / 50;
        if (childs[i]->getTotal(lastStep.getColor()) - temp < min)
        {
            results.clear();
            min = childs[i]->getTotal(lastStep.getColor()) - temp;
            results.push_back(i);
        }
        else if (childs[i]->getTotal(lastStep.getColor()) - temp == min)
        {
            results.push_back(i);
        }
        /*}*/
    }
    return results[rand() % results.size()];
}

void TreeNode::buildSortListInfo(int n, ThreatInfo *threatInfo, bool *hasSearch)
{
    int i = sortList[n].key;
    if (!hasSearch[i])
    {
        sortList[n].value = childs[i]->currentScore - childs[i]->getHighest(lastStep.getColor()) - childs[i]->getTotal(lastStep.getColor()) / 10;
        if (sortList[n].value <= -100000)//增加堵冲四的优先级
        {
            sortList[n].value -= 100000;
        }
    }
    else
    {
        ThreatInfo temp = threatInfo[i];
        int tempScore = childs[i]->currentScore - temp.HighestScore - temp.totalScore / 10;
        if (tempScore < sortList[n].value)
        {
            //sortList[n].value = childs[i]->currentScore - temp.totalScore;
            sortList[n].value = tempScore;
        }
    }
}

int TreeNode::findWorstChild()
{
    int min = 0;
    if (childs.size() > 0)
    {
        int  score = childs[0]->getTotal(-lastStep.getColor());
        for (size_t i = 1; i < childs.size(); i++)
        {
            if (childs[i]->getTotal(-lastStep.getColor()) < score)
            {
                score = childs[i]->getTotal(-lastStep.getColor());
                min = i;
            }
        }
    }
    return min;
}

void TreeNode::buildPlayer()//好好改改
{
    if (getHighest(-lastStep.getColor()) >= 100000)
    {
        delete currentBoard;
        currentBoard = 0;
        return;
    }
    else if (getHighest(-lastStep.getColor()) >= 10000 && getHighest(lastStep.getColor()) < 100000)
    {
        if (getHighest(-lastStep.getColor()) < 12000)
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
                        if (currentBoard->getPiece(i, j).getThreat(-lastStep.getColor()) >= 10000)
                        {
                            score = currentBoard->getPiece(i, j).getThreat(-lastStep.getColor());
                            tempBoard = *currentBoard;
                            tempBoard.doNextStep(i, j, -lastStep.getColor());
                            tempBoard.updateThreat(ban);
                            tempNode = new TreeNode(&tempBoard, depth, tempdepth, score);//AI才减1
                            addChild(tempNode);
                        }
                    }
                }
            }
        }
        else//  >=12000
        {
            delete currentBoard;
            currentBoard = 0;
            return;
        }
    }

    if (depth > 0)
    {
        if (getHighest(lastStep.getColor()) >= 100000)
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
                        if (currentBoard->getPiece(i, j).getThreat(lastStep.getColor()) >= 100000)
                        {
                            score = currentBoard->getPiece(i, j).getThreat(-lastStep.getColor());
                            tempBoard = *currentBoard;
                            tempBoard.doNextStep(i, j, -lastStep.getColor());
                            tempBoard.updateThreat(ban);
                            tempNode = new TreeNode(&tempBoard, depth, tempdepth, score);//AI才减1
                            addChild(tempNode);
                        }
                    }
                }
            }
        }
        else if (getHighest(-lastStep.getColor()) > 99 && getHighest(-lastStep.getColor()) < 10000)
        {
            ChessBoard tempBoard;
            TreeNode *tempNode;
            int score, highTemp;
            ThreatInfo tempInfo = { 0,0 };
            int worst;
            for (int i = 0; i < BOARD_ROW_MAX; ++i)
            {
                for (int j = 0; j < BOARD_COL_MAX; ++j)
                {
                    if (currentBoard->getPiece(i, j).isHot() && currentBoard->getPiece(i, j).getState() == 0)
                    {
                        score = currentBoard->getPiece(i, j).getThreat(-lastStep.getColor());//player
                        if (score > 99 && score < 10000)
                        {
                            tempBoard = *currentBoard;
                            tempBoard.doNextStep(i, j, -lastStep.getColor());
                            tempBoard.updateThreat(ban);
                            highTemp = tempBoard.getThreatInfo(lastStep.getColor()).HighestScore;//AI

                            if (highTemp >= 100000) continue;
                            else if (highTemp >= 8000 && (score > 1100 || score < 900)) continue;

                            if (childs.size() > 4)
                            {
                                tempInfo = tempBoard.getThreatInfo(-lastStep.getColor());
                                worst = findWorstChild();
                                if (childs[worst]->getTotal(-lastStep.getColor()) < tempInfo.totalScore)
                                {
                                    delete childs[worst];
                                    childs[worst] = new TreeNode(&tempBoard, depth, tempdepth, score);
                                }
                            }
                            else
                            {
                                tempNode = new TreeNode(&tempBoard, depth, tempdepth, score);//AI才减1
                                addChild(tempNode);
                            }
                        }
                    }
                }
            }
        }
        else if (getHighest(-lastStep.getColor()) >= 10000)
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
                        score = currentBoard->getPiece(i, j).getThreat(-lastStep.getColor());
                        if (score >= 10000)
                        {
                            tempBoard = *currentBoard;
                            tempBoard.doNextStep(i, j, -lastStep.getColor());
                            tempBoard.updateThreat(ban);
                            tempNode = new TreeNode(&tempBoard, depth, tempdepth, score);//AI才减1
                            addChild(tempNode);
                        }
                    }
                }
            }
        }
    }
    //for (int i = 0; i < childs.size(); i++)
    //{
    //	childs[i]->buildAI();
    //}
    if (childs.size() > 0)
    {
        int *childrenInfo = new int[childs.size()];
        bool *hasSearch = new bool[childs.size()];
        for (size_t i = 0; i < childs.size(); ++i)
        {
            buildNodeInfo(i, childrenInfo);
            hasSearch[i] = false;
        }
        int bestPos;
        while (childs.size() > 0)
        {
            bestPos = findBestNode(childrenInfo);
            if (hasSearch[bestPos]) break;
            else
            {
                childs[bestPos]->buildAI();
                buildNodeInfo(bestPos, childrenInfo);
                hasSearch[bestPos] = true;
            }
        }
        delete[] hasSearch;
        delete[] childrenInfo;
    }
    delete currentBoard;
    currentBoard = 0;
}

void TreeNode::buildNodeInfo(int i, int *childrenInfo)
{
    int playerside = ((lastStep.getColor() != playerColor)) ? (-lastStep.getColor()) : lastStep.getColor();

    if (childs[i]->getChildNum() == 0)
    {
        childrenInfo[i] = childs[i]->getTotal(playerside);
    }
    else
    {
        ThreatInfo temp = childs[i]->getBestThreat();
        childrenInfo[i] = temp.totalScore;
    }
}

int TreeNode::findBestNode(int *childrenInfo)
{
    int bestPos = -1;
    int bestScore = -500000;
    for (size_t i = 0; i < childs.size(); i++)
    {
        if (childrenInfo[i] > bestScore)
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
    int highest = getHighest(-lastStep.getColor());

    if (getHighest(-lastStep.getColor()) >= 100000)
    {

        for (int m = 0; m < BOARD_ROW_MAX; ++m)
        {
            for (int n = 0; n < BOARD_COL_MAX; ++n)
            {
                if (currentBoard->getPiece(m, n).isHot() && currentBoard->getPiece(m, n).getState() == 0)
                {
                    if (currentBoard->getPiece(m, n).getThreat(-lastStep.getColor()) >= 100000)
                    {
                        tempBoard = *currentBoard;
                        score = currentBoard->getPiece(m, n).getThreat(-lastStep.getColor());
                        tempBoard.doNextStep(m, n, -lastStep.getColor());
                        tempBoard.updateThreat(ban);
                        tempNode = new TreeNode(&tempBoard, 0, 0, score);
                        if (lastStep.getColor() == 1)
                        {
                            tempNode->blackHighest -= 100000;
                            tempNode->blackThreat -= 100000;
                        }
                        else
                        {
                            tempNode->whiteHighest -= 100000;
                            tempNode->whiteThreat -= 100000;
                        }
                        addChild(tempNode);
                        goto flag;
                    }
                }
            }
        }
    }
    else if (getHighest(-lastStep.getColor()) >= 10000 && getHighest(lastStep.getColor()) < 100000)// >=10000 已确认有问题-特殊情况
    {
        for (int m = 0; m < BOARD_ROW_MAX; ++m)
        {
            for (int n = 0; n < BOARD_COL_MAX; ++n)
            {
                if (currentBoard->getPiece(m, n).isHot() && currentBoard->getPiece(m, n).getState() == 0)
                {
                    if (currentBoard->getPiece(m, n).getThreat(-lastStep.getColor()) >= 10000)
                    {
                        tempBoard = *currentBoard;
                        score = currentBoard->getPiece(m, n).getThreat(-lastStep.getColor());
                        tempBoard.doNextStep(m, n, -lastStep.getColor());
                        tempBoard.updateThreat(ban);
                        tempNode = new TreeNode(&tempBoard, 0, 0, score);
                        if (lastStep.getColor() == 1)
                        {
                            tempNode->blackHighest -= 10000;
                            tempNode->blackThreat -= 10000;
                        }
                        else
                        {
                            tempNode->whiteHighest -= 10000;
                            tempNode->whiteThreat -= 10000;
                        }
                        addChild(tempNode);
                        goto flag;
                    }
                }
            }
        }
    }


    for (int m = 0; m < BOARD_ROW_MAX; ++m)
    {
        for (int n = 0; n < BOARD_COL_MAX; ++n)
        {
            if (currentBoard->getPiece(m, n).isHot() && currentBoard->getPiece(m, n).getState() == 0)
            {
                if (currentBoard->getPiece(m, n).getThreat(lastStep.getColor()) >= 100000)
                {
                    tempBoard = *currentBoard;
                    score = currentBoard->getPiece(m, n).getThreat(-lastStep.getColor());
                    tempBoard.doNextStep(m, n, -lastStep.getColor());
                    tempBoard.updateThreat(ban);
                    tempNode = new TreeNode(&tempBoard, tempdepth > 0 ? depth : depth - 1, tempdepth > 0 ? tempdepth - 1 : tempdepth, score);//flag high-1
                    addChild(tempNode);
                    goto flag;
                }
                else if (currentBoard->getPiece(m, n).getThreat(lastStep.getColor()) >= 10000 &&
                    highest < 100000)
                {
                    tempBoard = *currentBoard;
                    score = currentBoard->getPiece(m, n).getThreat(-lastStep.getColor());
                    tempBoard.doNextStep(m, n, -lastStep.getColor());
                    tempBoard.updateThreat(ban);
                    tempNode = new TreeNode(&tempBoard, depth - 1, tempdepth, score);
                    addChild(tempNode);
                }
                //else if (getHighest(side) < 100000 && getHighest(side)>=10000)//进攻就是防守
                //{
                //	if (currentBoard->getPiece(m, n).getThreat(-side)>900 && currentBoard->getPiece(m, n).getThreat(-side) < 1100)
                //	{
                //		tempBoard = *currentBoard;
                //		score = currentBoard->getPiece(m, n).getThreat(-side);
                //		tempBoard.doNextStep(m, n, -side);
                //		tempBoard.updateThreat(ban);
                //		tempNode = new TreeNode(tempBoard, temphigh>0 ? high : high - 1, temphigh - 1, score);//存疑
                //		addChild(tempNode);
                //	}
                //}
                //else if (getHighest(side) < 10000)
                //{
                //	if (currentBoard->getPiece(m, n).getThreat(-side)>1199 && currentBoard->getPiece(m, n).getThreat(-side) < 1300)//跳三没考虑
                //	{
                //		tempBoard = *currentBoard;
                //		score = currentBoard->getPiece(m, n).getThreat(-side);
                //		tempBoard.doNextStep(m, n, -side);
                //		tempBoard.updateThreat(ban);
                //		tempNode = new TreeNode(tempBoard, high, score);
                //		addChild(tempNode);
                //	}
                //}
            }
        }
    }
    if (0 == childs.size())//有可能有未知的问题
    {
        int best = 0;
        int i, j;
        for (int m = 0; m < BOARD_ROW_MAX; ++m)
        {
            for (int n = 0; n < BOARD_COL_MAX; ++n)
            {
                if (currentBoard->getPiece(m, n).isHot() && currentBoard->getPiece(m, n).getState() == 0)
                {
                    if (currentBoard->getPiece(m, n).getThreat(lastStep.getColor()) > best)
                    {
                        best = currentBoard->getPiece(m, n).getThreat(lastStep.getColor());
                        i = m, j = n;
                    }
                }
            }
        }
        tempBoard = *currentBoard;
        score = currentBoard->getPiece(i, j).getThreat(-lastStep.getColor());
        tempBoard.doNextStep(i, j, -lastStep.getColor());
        tempBoard.updateThreat(ban);
        tempNode = new TreeNode(&tempBoard, depth - 1, tempdepth, score);
        addChild(tempNode);
    }
flag:
    for (size_t i = 0; i < childs.size(); i++)
    {
        childs[i]->buildPlayer();
    }
    delete currentBoard;
    currentBoard = 0;
}

void TreeNode::buildAtackSearchTree()
{

}

void TreeNode::printTree()
{
    string s;
    stringstream ss(s);
    fstream of("tree.txt", ios::out);

    if (childs.size() == 0)
    {
        ss << "(" << getHighest(playerColor) << "|" << getTotal(playerColor) << ")" << "  ";

    }
    else
    {

        for (size_t i = 0; i < childs.size(); ++i)
        {
            stringstream temps;
            temps << i << "-";
            childs[i]->printTree(ss, temps.str()); ss << "\n";
        }
    }
    of << ss.str().c_str();
    of.close();
}

void TreeNode::printTree(stringstream &ss, string pre)
{
    if (childs.size() == 0)
    {
        ss << pre << ":" << "(" << getHighest(playerColor) << "|" << getTotal(playerColor) << ")" << "  ";

    }
    else
    {
        ss << depth << ":" << "{";
        for (size_t i = 0; i < childs.size(); ++i)
        {
            stringstream temps;
            temps << i << "-";
            childs[i]->printTree(ss, pre + temps.str());
        }
        ss << "}" << "*";
    }
}