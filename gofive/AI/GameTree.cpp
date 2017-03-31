#include "GameTree.h"
#include "utils.h"
#include "ThreadPool.h"
#include <thread>

static int countTreeNum = 0;
int8_t GameTreeNode::playerColor = 1;
bool GameTreeNode::multiThread = true;
size_t GameTreeNode::maxTaskNum = 0;
vector<map<string, GameTreeNode*>> GameTreeNode::historymaps(0);

void GameTreeNode::debug(ThreatInfo *threatInfo)
{
    stringstream ss;
    fstream of("debug.txt", ios::out);
    for (size_t n = 0; n < childs.size(); n++)
        ss << n << ":" << threatInfo[n].HighestScore << "|" << threatInfo[n].totalScore << "\n";
    of << ss.str().c_str();
    of.close();
}


GameTreeNode::GameTreeNode()
{

}

GameTreeNode::GameTreeNode(ChessBoard *board, int depth, int tempdepth, int score) :
    depth(depth), extraDepth(tempdepth), lastStepScore(score)
{
    this->chessBoard = new ChessBoard;
    *(this->chessBoard) = *board;
    lastStep = chessBoard->lastStep;
    ThreatInfo black = chessBoard->getThreatInfo(1);
    ThreatInfo white = chessBoard->getThreatInfo(-1);
    this->blackThreat = black.totalScore;
    this->whiteThreat = white.totalScore;
    this->blackHighest = black.HighestScore;
    this->whiteHighest = white.HighestScore;
    countTreeNum++;
}

GameTreeNode::~GameTreeNode()
{
    deleteChild();
    if (chessBoard) delete chessBoard;
}

const GameTreeNode& GameTreeNode::operator=(const GameTreeNode& other)
{
    if (other.chessBoard)
    {
        chessBoard = new ChessBoard;
        *chessBoard = *other.chessBoard;
    }
    else
    {
        chessBoard = NULL;
    }
    lastStep = other.lastStep;
    blackThreat = other.blackThreat;
    whiteThreat = other.whiteThreat;
    blackHighest = other.blackHighest;
    whiteHighest = other.whiteHighest;
    lastStepScore = other.lastStepScore;
    depth = other.depth;
    extraDepth = other.extraDepth;
    return *this;
}

void GameTreeNode::deleteChild()
{
    for (size_t i = 0; i < childs.size(); i++)
    {
        if (!childs_isref[i])
        {
            delete childs[i];
        }
    }
    childs.clear();
    childs_isref.clear();
}

void GameTreeNode::deleteChessBoard()
{
    delete chessBoard;
    chessBoard = 0;
}

void GameTreeNode::buildAllChild()
{
    ChessBoard tempBoard;
    GameTreeNode *tempNode;
    int score;
    for (int i = 0; i < BOARD_ROW_MAX; ++i)
    {
        for (int j = 0; j < BOARD_COL_MAX; ++j)
        {
            if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
            {
                tempBoard = *chessBoard;
                tempBoard.doNextStep(i, j, -lastStep.getColor());
                tempBoard.updateThreat();
                score = tempBoard.getLastStepScores(false);
                tempNode = new GameTreeNode(&tempBoard, depth - 1, extraDepth, score);
                addChild(tempNode);
            }
        }
    }
}

int GameTreeNode::searchBest2(bool *hasSearch, ThreatInfo *threatInfo, ChildInfo *sortList)
{
    if (depth - MAP_IGNORE_DEPTH > 0)
    {
        historymaps.resize(depth - MAP_IGNORE_DEPTH);
    }
    size_t searchNum = 10;
    ThreadPool pool;
    pool.start();
    sort(sortList, 0, childs.size() - 1);
    while (true)
    {
        if (hasSearch[sortList[childs.size() - 1].key])//深度搜索过的得分不会比搜索之前高，如果目前得分最高的已经是搜索过的了，再进行搜索也不会找到得分比它更高的了
        {
            //防止随机化算法导致
            //最后一个搜索过，但倒数第二个未搜索过并且得分等于最后一个，有概率选取到倒数第二个
            int i = childs.size() - 1;
            while (i > 0 && sortList[i - 1].value == sortList[i].value)
            {
                i--;
                if (!hasSearch[sortList[i].key])//相等得分中有没搜索过的
                {
                    //searchNum = 10;
                    goto continueSearch;
                }
            }
            break;
        }
    continueSearch:
        if (searchNum > childs.size())
        {
            searchNum = childs.size();
        }

        for (size_t i = childs.size() - searchNum; i < childs.size(); i++)
        {
            if (!hasSearch[sortList[i].key])
            {
                Task t;
                t.node = childs[sortList[i].key];
                pool.run(t, false);
            }
        }

        //等待线程
        while (true)
        {
            if (pool.getTaskNum() == 0 && pool.getWorkNum() == 0)
            {
                break;
            }
            this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        for (size_t i = childs.size() - searchNum; i < childs.size(); i++)
        {
            if (!hasSearch[sortList[i].key])
            {
                hasSearch[sortList[i].key] = true;//hasSearch值对buildSortListInfo有影响
                threatInfo[sortList[i].key] = childs[sortList[i].key]->getBestThreat();
                //childs[sortList[i].key]->deleteChild();
                buildSortListInfo(i, threatInfo, sortList, hasSearch);
            }
        }
        sort(sortList, 0, childs.size() - 1);
        searchNum += 10;
    }
    pool.stop();
    historymaps.clear();
    //随机化
    int i = childs.size() - 1;
    while (i > 0 && sortList[i - 1].value == sortList[i].value) i--;
    return i + rand() % (childs.size() - i);
}

//多线程

void GameTreeNode::buildTreeThreadFunc(int n, ThreatInfo *threatInfo, GameTreeNode *child)
{
    child->buildPlayer();
    threatInfo[n] = child->getBestThreat();
    delete child;
}

int GameTreeNode::searchBest(bool *hasSearch, ThreatInfo *threatInfo, ChildInfo *sortList)
{
    size_t searchNum = 10;
    thread buildTreeThread[MAXTHREAD];
    sort(sortList, 0, childs.size() - 1);
    while (true)
    {
        if (hasSearch[sortList[childs.size() - 1].key])//深度搜索过的得分不会比搜索之前高，如果目前得分最高的已经是搜索过的了，再进行搜索也不会找到得分比它更高的了
        {
            //防止随机化算法导致
            //最后一个搜索过，但倒数第二个未搜索过并且得分等于最后一个，有概率选取到倒数第二个
            int i = childs.size() - 1;
            while (i > 0 && sortList[i - 1].value == sortList[i].value)
            {
                i--;
                if (!hasSearch[sortList[i].key])//相等得分中有没搜索过的
                {
                    //searchNum = 10;
                    goto continueSearch;
                }
            }
            break;
        }
    continueSearch:
        if (searchNum > childs.size())
        {
            searchNum = childs.size();
        }

        int j = 0;
        for (size_t i = childs.size() - searchNum; i < childs.size(); i++)
        {
            if (!hasSearch[sortList[i].key])
            {
                GameTreeNode *node = new GameTreeNode;
                *node = *childs[sortList[i].key];
                buildTreeThread[j] = thread(buildTreeThreadFunc, sortList[i].key, threatInfo, node);
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
            buildSortListInfo(i, threatInfo, sortList, hasSearch);
        }
        sort(sortList, 0, childs.size() - 1);
        searchNum += 10;
    }

    //随机化
    int i = childs.size() - 1;
    while (i > 0 && sortList[i - 1].value == sortList[i].value) i--;
    return i + rand() % (childs.size() - i);
}

Position GameTreeNode::getBestStep()
{
    bool needSearch = true;
    int bestSearchPos;
    buildAllChild();
    bool *hasSearch = new bool[childs.size()];
    ChildInfo *sortList = new ChildInfo[childs.size()];
    ThreatInfo *threatInfo = new ThreatInfo[childs.size()];

    for (size_t i = 0; i < childs.size(); ++i)
    {
        hasSearch[i] = false;
        sortList[i].key = i;
        buildSortListInfo(i, threatInfo, sortList, hasSearch);
        if (childs[i]->lastStepScore >= SCORE_5_CONTINUE)
        {
            bestSearchPos = i;
            needSearch = false;
        }
        else if (childs[i]->lastStepScore >= SCORE_4_DOUBLE && childs[i]->getHighest(-childs[i]->lastStep.getColor()) < SCORE_5_CONTINUE)
        {
            bestSearchPos = i;
            needSearch = false;
        }
        else if (childs[i]->lastStepScore < 0)
        {
            sortList[i].value -= SCORE_5_CONTINUE;//保证禁手不走
        }
    }


    int specialAtackStep = getSpecialAtack();//此时还未排序，sortList[specialAtackStep].key = specialAtackStep
    if (specialAtackStep >= 0)
    {
        childs[specialAtackStep]->buildPlayer();
        hasSearch[specialAtackStep] = true;
        threatInfo[specialAtackStep] = childs[specialAtackStep]->getBestThreat();
        buildSortListInfo(specialAtackStep, threatInfo, sortList, hasSearch);
        childs[specialAtackStep]->deleteChild();
    }

    int atackChildIndex = getAtackChild();
    if (!hasSearch[atackChildIndex])
    {
        childs[atackChildIndex]->buildPlayer();
        hasSearch[atackChildIndex] = true;
        threatInfo[atackChildIndex] = childs[atackChildIndex]->getBestThreat();
        buildSortListInfo(atackChildIndex, threatInfo, sortList, hasSearch);
        childs[atackChildIndex]->deleteChild();
    }

    if (needSearch)
    {
        bestSearchPos = multiThread ? searchBest2(hasSearch, threatInfo, sortList) : searchBest(hasSearch, threatInfo, sortList);
    }

    Position result;



    //childs[39]->buildPlayer();
    //hasSearch[39] = true;
    //threatInfo[39] = childs[39]->getBestThreat();
    //childs[39]->printTree();
    //childs[39]->deleteChild();

    if (childs[atackChildIndex]->lastStepScore >= SCORE_5_CONTINUE ||
        (childs[atackChildIndex]->lastStepScore >= 10000 && childs[atackChildIndex]->getHighest(lastStep.getColor()) < SCORE_5_CONTINUE && threatInfo[atackChildIndex].HighestScore < SCORE_5_CONTINUE))
    {
        result = Position{ childs[atackChildIndex]->lastStep.row, childs[atackChildIndex]->lastStep.col };
    }
    else if (playerColor == STATE_CHESS_BLACK && lastStep.step < 20)//防止开局被布阵
    {
        atackChildIndex = getDefendChild();
        result = Position{ childs[atackChildIndex]->lastStep.row, childs[atackChildIndex]->lastStep.col };
    }
    else if (threatInfo[sortList[bestSearchPos].key].HighestScore > 80000 ||
        (threatInfo[sortList[bestSearchPos].key].HighestScore >= 10000 && (childs[sortList[bestSearchPos].key]->lastStepScore <= 1200 || (childs[sortList[bestSearchPos].key]->lastStepScore >= 8000 && childs[sortList[bestSearchPos].key]->lastStepScore < 10000))))
    {
        if (specialAtackStep > -1 && childs[specialAtackStep]->lastStepScore > 1200 && childs[specialAtackStep]->getHighest(lastStep.getColor()) < SCORE_5_CONTINUE)
        {
            result = Position{ childs[specialAtackStep]->lastStep.row, childs[specialAtackStep]->lastStep.col };
        }
        else
        {
            atackChildIndex = getDefendChild();//必输局面跟随玩家的落子去堵
            if (chessBoard->getPiece(childs[atackChildIndex]->lastStep.row, childs[atackChildIndex]->lastStep.col).getThreat(lastStep.getColor()) > 2000)
                result = Position{ childs[atackChildIndex]->lastStep.row, childs[atackChildIndex]->lastStep.col };
            else
                result = Position{ childs[sortList[bestSearchPos].key]->lastStep.row, childs[sortList[bestSearchPos].key]->lastStep.col };
        }
    }
    else if (specialAtackStep > -1 && threatInfo[specialAtackStep].HighestScore < SCORE_3_DOUBLE)//add at 17.3.23 防止走向失败
    {
        result = Position{ childs[specialAtackStep]->lastStep.row, childs[specialAtackStep]->lastStep.col };
    }
    else if (childs[atackChildIndex]->lastStepScore > 1000 && threatInfo[atackChildIndex].HighestScore <= SCORE_3_DOUBLE)
    {
        result = Position{ childs[atackChildIndex]->lastStep.row, childs[atackChildIndex]->lastStep.col };
    }
    else
    {
        result = Position{ childs[sortList[bestSearchPos].key]->lastStep.row, childs[sortList[bestSearchPos].key]->lastStep.col };
    }

    delete[] sortList;
    delete[] threatInfo;
    delete[] hasSearch;
    //historymap->clear();
    //delete historymap;
    //historymap = NULL;
    return result;
}

ThreatInfo GameTreeNode::getBestThreat()
{
    ThreatInfo tempThreat(0, 0), best(0, 0);
    if (lastStep.getColor() == playerColor)//初始化best
    {
        best.HighestScore = 500000;
        best.totalScore = 500000;
    }

    if (lastStep.getColor() != playerColor && childs.size() == 0)//叶子节点是AI
    {
        return ThreatInfo(getTotal(-lastStep.getColor()), getHighest(-lastStep.getColor()));//取得player分数
    }

    if (lastStep.getColor() == playerColor && childs.size() == 0)//叶子节点是player,表示提前结束,玩家取胜,否则一定会是AI
    {
        return ThreatInfo(getTotal(lastStep.getColor()), getHighest(lastStep.getColor()));//取得player分数
    }

    for (size_t i = 0; i < childs.size(); ++i)
    {
        if ((lastStep.getColor() != playerColor))//AI节点
        {
            tempThreat = childs[i]->getBestThreat();//递归
            if (tempThreat.totalScore > best.totalScore)//best原则:player下过的节点player得分越大越好(默认player走最优点)
            {
                if (tempThreat.HighestScore > best.HighestScore / 2)
                {
                    best = tempThreat;
                }
            }
        }
        else//player节点
        {
            tempThreat = childs[i]->getBestThreat();//child是AI节点
            if (tempThreat.totalScore < best.totalScore)//best原则:AI下过的节点player得分越小越好
            {
                if (tempThreat.HighestScore / 2 < best.HighestScore)
                {
                    best = tempThreat;
                }
            }
        }
    }
    return best;
}

void GameTreeNode::buildSortListInfo(int n, ThreatInfo *threatInfo, ChildInfo *sortList, bool *hasSearch)
{
    int i = sortList[n].key;
    if (!hasSearch[i])
    {
        sortList[n].value = childs[i]->lastStepScore - childs[i]->getHighest(lastStep.getColor()) - childs[i]->getTotal(lastStep.getColor()) / 10;
        if (sortList[n].value <= -SCORE_5_CONTINUE)//增加堵冲四的优先级
        {
            sortList[n].value -= SCORE_5_CONTINUE;
        }
    }
    else
    {
        ThreatInfo temp = threatInfo[i];
        int tempScore = childs[i]->lastStepScore - temp.HighestScore - temp.totalScore / 10;
        if (tempScore < sortList[n].value)
        {
            //sortList[n].value = childs[i]->currentScore - temp.totalScore;
            sortList[n].value = tempScore;
        }
    }
}

int GameTreeNode::getSpecialAtack()
{
    if (getHighest(lastStep.getColor()) >= SCORE_5_CONTINUE)
        return -1;
    int max = 3000, flag = -1, temp;
    for (size_t i = 0; i < childs.size(); ++i)
    {
        if (childs[i]->lastStepScore >= 1210 && childs[i]->lastStepScore < 2000 /*||
                                                                                (getHighest(side) < 10000 && childs[i]->currentScore < 1210 && childs[i]->currentScore>1000)*/)
        {
            ChessBoard tempboard = *childs[i]->chessBoard;
            tempboard.setGlobalThreat(false);//进攻权重
            temp = tempboard.getAtackScore(childs[i]->lastStepScore, getTotal(lastStep.getColor()));
            if (temp > max)
            {
                max = temp;
                flag = i;
            }
            if (temp < 3000)//减小无意义冲四的优先级
            {
                if (chessBoard->getPiece(childs[i]->lastStep.row, childs[i]->lastStep.col).getThreat(lastStep.getColor()) < 8000)
                {
                    //childs[i]->currentScore -= 10000;
                    childs[i]->lastStepScore = 123; //modify at 17.3.23
                }
            }//可能出现权重BUG(已经出现，3.23修复)
        }
    }
    return flag;
}

int GameTreeNode::getAtackChild()
{
    int max = INT_MIN, flag = 0, temp;
    for (size_t i = 0; i < childs.size(); ++i)
    {
        temp = chessBoard->getPiece(childs[i]->lastStep.row, childs[i]->lastStep.col).getThreat(lastStep.getColor()) / 50;
        if (childs[i]->lastStepScore + temp > max)
        {
            max = childs[i]->lastStepScore + temp;
            flag = i;
        }
    }
    return flag;
}

int GameTreeNode::getDefendChild()
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
        temp = chessBoard->getPiece(childs[i]->lastStep.row, childs[i]->lastStep.col).getThreat(-lastStep.getColor()) / 50;
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

void GameTreeNode::buildPlayer(bool recursive)//好好改改
{
    //lastStep.getColor()是AI下的
    if (getHighest(playerColor) >= SCORE_5_CONTINUE)
    {
        delete chessBoard;
        chessBoard = 0;
        return;
    }
    else if (getHighest(playerColor) >= 10000 && getHighest(-playerColor) < SCORE_5_CONTINUE)
    {
        if (getHighest(playerColor) < 12000)
        {
            ChessBoard tempBoard;
            GameTreeNode *tempNode;
            int score;
            for (int i = 0; i < BOARD_ROW_MAX; ++i)
            {
                for (int j = 0; j < BOARD_COL_MAX; ++j)
                {
                    if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                    {
                        if (chessBoard->getPiece(i, j).getThreat(playerColor) >= 10000)
                        {
                            score = chessBoard->getPiece(i, j).getThreat(playerColor);
                            tempBoard = *chessBoard;
                            tempBoard.doNextStep(i, j, playerColor);
                            tempBoard.updateThreat();
                            tempNode = new GameTreeNode(&tempBoard, depth, extraDepth, score);//AI才减1
                            addChild(tempNode);
                        }
                    }
                }
            }
        }
        else//  >=12000
        {
            delete chessBoard;
            chessBoard = 0;
            return;
        }
    }

    if (depth > 0)
    {
        if (getHighest(-playerColor) >= SCORE_5_CONTINUE)
        {
            ChessBoard tempBoard;
            GameTreeNode *tempNode;
            int score;
            for (int i = 0; i < BOARD_ROW_MAX; ++i)
            {
                for (int j = 0; j < BOARD_COL_MAX; ++j)
                {
                    if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                    {
                        if (chessBoard->getPiece(i, j).getThreat(-playerColor) >= SCORE_5_CONTINUE)
                        {
                            score = chessBoard->getPiece(i, j).getThreat(playerColor);
                            tempBoard = *chessBoard;
                            tempBoard.doNextStep(i, j, playerColor);
                            tempBoard.updateThreat();
                            tempNode = new GameTreeNode(&tempBoard, depth, extraDepth, score);//AI才减1
                            addChild(tempNode);
                        }
                    }
                }
            }
        }
        else if (getHighest(playerColor) > 99 && getHighest(playerColor) < 10000)
        {
            ChessBoard tempBoard;
            GameTreeNode *tempNode;
            int score, highTemp;
            ThreatInfo tempInfo = { 0,0 };
            int worst;
            for (int i = 0; i < BOARD_ROW_MAX; ++i)
            {
                for (int j = 0; j < BOARD_COL_MAX; ++j)
                {
                    if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                    {
                        score = chessBoard->getPiece(i, j).getThreat(playerColor);//player
                        if (score > 99 && score < 10000)
                        {
                            tempBoard = *chessBoard;
                            tempBoard.doNextStep(i, j, playerColor);
                            tempBoard.updateThreat();
                            highTemp = tempBoard.getThreatInfo(-playerColor).HighestScore;//AI

                            if (highTemp >= SCORE_5_CONTINUE) continue;
                            else if (highTemp >= 8000 && (score > 1100 || score < 900)) continue;

                            if (childs.size() > 4)
                            {
                                tempInfo = tempBoard.getThreatInfo(playerColor);
                                worst = findWorstNode();
                                if (childs[worst]->getTotal(playerColor) < tempInfo.totalScore)
                                {
                                    delete childs[worst];
                                    childs[worst] = new GameTreeNode(&tempBoard, depth, extraDepth, score);
                                }
                            }
                            else
                            {
                                tempNode = new GameTreeNode(&tempBoard, depth, extraDepth, score);//AI才减1
                                addChild(tempNode);
                            }
                        }
                    }
                }
            }
        }
        else if (getHighest(playerColor) >= 10000)
        {
            ChessBoard tempBoard;
            GameTreeNode *tempNode;
            int score;
            for (int i = 0; i < BOARD_ROW_MAX; ++i)
            {
                for (int j = 0; j < BOARD_COL_MAX; ++j)
                {
                    if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                    {
                        score = chessBoard->getPiece(i, j).getThreat(playerColor);
                        if (score >= 10000)
                        {
                            tempBoard = *chessBoard;
                            tempBoard.doNextStep(i, j, playerColor);
                            tempBoard.updateThreat();
                            tempNode = new GameTreeNode(&tempBoard, depth, extraDepth, score);//AI才减1
                            addChild(tempNode);
                        }
                    }
                }
            }
        }
    }

    if (recursive)//需要递归
    {
        /*for (int i = 0; i < childs.size(); i++)
        {
            childs[i]->buildAI();
        }*/
        if (childs.size() > 0)//效果很好
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
    }
    delete chessBoard;
    chessBoard = 0;
}

void GameTreeNode::buildNodeInfo(int i, int *childrenInfo)
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

int GameTreeNode::findBestNode(int *childrenInfo)
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

int GameTreeNode::findWorstNode()
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

void GameTreeNode::buildAI(bool recursive)
{
    ChessBoard tempBoard;
    GameTreeNode *tempNode;
    int score;
    int highest = getHighest(-playerColor);
    //lastStep.getColor()是player下的
    if (getHighest(-playerColor) >= SCORE_5_CONTINUE)
    {

        for (int m = 0; m < BOARD_ROW_MAX; ++m)
        {
            for (int n = 0; n < BOARD_COL_MAX; ++n)
            {
                if (chessBoard->getPiece(m, n).hot && chessBoard->getPiece(m, n).state == 0)
                {
                    if (chessBoard->getPiece(m, n).getThreat(-playerColor) >= SCORE_5_CONTINUE)
                    {
                        tempBoard = *chessBoard;
                        score = chessBoard->getPiece(m, n).getThreat(-playerColor);
                        tempBoard.doNextStep(m, n, -playerColor);
                        tempBoard.updateThreat();
                        tempNode = new GameTreeNode(&tempBoard, 0, 0, score);
                        if (playerColor == 1)
                        {
                            tempNode->blackHighest -= SCORE_5_CONTINUE;
                            tempNode->blackThreat -= SCORE_5_CONTINUE;
                        }
                        else
                        {
                            tempNode->whiteHighest -= SCORE_5_CONTINUE;
                            tempNode->whiteThreat -= SCORE_5_CONTINUE;
                        }
                        addChild(tempNode);
                        goto end;
                    }
                }
            }
        }
    }
    else if (getHighest(-playerColor) >= 10000 && getHighest(playerColor) < SCORE_5_CONTINUE)// >=10000 已确认有问题-特殊情况
    {
        for (int m = 0; m < BOARD_ROW_MAX; ++m)
        {
            for (int n = 0; n < BOARD_COL_MAX; ++n)
            {
                if (chessBoard->getPiece(m, n).hot && chessBoard->getPiece(m, n).state == 0)
                {
                    if (chessBoard->getPiece(m, n).getThreat(-playerColor) >= 10000)
                    {
                        tempBoard = *chessBoard;
                        score = chessBoard->getPiece(m, n).getThreat(-playerColor);
                        tempBoard.doNextStep(m, n, -playerColor);
                        tempBoard.updateThreat();
                        tempNode = new GameTreeNode(&tempBoard, 0, 0, score);
                        if (playerColor == 1)
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
                        goto end;
                    }
                }
            }
        }
    }


    for (int m = 0; m < BOARD_ROW_MAX; ++m)
    {
        for (int n = 0; n < BOARD_COL_MAX; ++n)
        {
            if (chessBoard->getPiece(m, n).hot && chessBoard->getPiece(m, n).state == 0)
            {
                if (chessBoard->getPiece(m, n).getThreat(playerColor) >= SCORE_5_CONTINUE)
                {
                    tempBoard = *chessBoard;
                    score = chessBoard->getPiece(m, n).getThreat(-playerColor);
                    if (score < 0)//被禁手了
                    {
                        continue;
                    }
                    tempBoard.doNextStep(m, n, -playerColor);
                    tempBoard.updateThreat();
                    tempNode = new GameTreeNode(&tempBoard, extraDepth > 0 ? depth : depth - 1, extraDepth > 0 ? extraDepth - 1 : extraDepth, score);//flag high-1
                    addChild(tempNode);
                    goto end;
                }
                else if (chessBoard->getPiece(m, n).getThreat(playerColor) >= 10000 &&
                    highest < SCORE_5_CONTINUE)
                {
                    tempBoard = *chessBoard;
                    score = chessBoard->getPiece(m, n).getThreat(-playerColor);
                    if (score < 0)//被禁手了
                    {
                        continue;
                    }
                    tempBoard.doNextStep(m, n, -playerColor);
                    tempBoard.updateThreat();
                    tempNode = new GameTreeNode(&tempBoard, depth - 1, extraDepth, score);
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
    //if (0 == childs.size())//有可能有未知的问题
    //{
    //    int best = 0;
    //    int i, j;
    //    for (int m = 0; m < BOARD_ROW_MAX; ++m)
    //    {
    //        for (int n = 0; n < BOARD_COL_MAX; ++n)
    //        {
    //            if (chessBoard->getPiece(m, n).hot && chessBoard->getPiece(m, n).state == 0)
    //            {
    //                if (chessBoard->getPiece(m, n).getThreat(playerColor) > best)
    //                {
    //                    best = chessBoard->getPiece(m, n).getThreat(playerColor);
    //                    i = m, j = n;
    //                }
    //            }
    //        }
    //    }
    //    tempBoard = *chessBoard;
    //    score = chessBoard->getPiece(i, j).getThreat(-playerColor);
    //    tempBoard.doNextStep(i, j, -playerColor);
    //    tempBoard.updateThreat();
    //    tempNode = new GameTreeNode(&tempBoard, depth - 1, extraDepth, score);
    //    addChild(tempNode);
    //}
end:
    if (recursive)//需要递归
    {
        for (size_t i = 0; i < childs.size(); i++)
        {
            childs[i]->buildPlayer();
        }
    }
    delete chessBoard;
    chessBoard = 0;
}

void GameTreeNode::buildAtackSearchTree()
{

}

void GameTreeNode::printTree()
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

void GameTreeNode::printTree(stringstream &ss, string pre)
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