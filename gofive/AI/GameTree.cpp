#include "GameTree.h"
#include "utils.h"
#include "ThreadPool.h"
#include <thread>
#include <assert.h>


SortInfo *GameTreeNode::sortList = NULL;
ChildInfo *GameTreeNode::childsInfo = NULL;

int8_t GameTreeNode::playerColor = 1;
bool GameTreeNode::multiThread = true;
int GameTreeNode::resultFlag = AIRESULTFLAG_NORMAL;
uint8_t GameTreeNode::startStep = 0;
uint8_t GameTreeNode::maxSearchDepth = 0;
uint8_t GameTreeNode::transTableMaxDepth = 0;
size_t GameTreeNode::maxTaskNum = 0;
int GameTreeNode::bestRating = 0;
int GameTreeNode::bestIndex = -1;
unordered_map<uint32_t, TransTableData> GameTreeNode::transpositionTable;
shared_mutex GameTreeNode::mut_transTable;
uint64_t GameTreeNode::hash_hit = 0;
uint64_t GameTreeNode::hash_clash = 0;
uint64_t GameTreeNode::hash_miss = 0;

void GameTreeNode::debug()
{
    stringstream ss;
    fstream of("debug.txt", ios::out);
    for (size_t n = 0; n < childs.size(); n++)
        ss << n << ":" << childsInfo[n].rating.highestScore << "|" << childsInfo[n].rating.totalScore << "\n";
    of << ss.str().c_str();
    of.close();
}


GameTreeNode::GameTreeNode()
{

}

GameTreeNode::GameTreeNode(ChessBoard *board)
{
    this->chessBoard = new ChessBoard;
    *(this->chessBoard) = *board;
    lastStep = chessBoard->lastStep;
    black = chessBoard->getRatingInfo(1);
    white = chessBoard->getRatingInfo(-1);
}

GameTreeNode::~GameTreeNode()
{
    deleteChilds();
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
    hash = other.hash;
    black = other.black;
    white = other.white;
    return *this;
}

void GameTreeNode::initTree(AIParam param, int8_t playercolor)
{
    //init static param
    playerColor = playercolor;
    multiThread = param.multithread;
    maxSearchDepth = param.caculateSteps * 2;
    transTableMaxDepth = maxSearchDepth > 1 ? maxSearchDepth - 1 : 0;
    startStep = lastStep.step;
    hash_hit = 0;
    hash_miss = 0;
    hash_clash = 0;
    hash = chessBoard->toHash();
}

void GameTreeNode::deleteChilds()
{
    for (size_t i = 0; i < childs.size(); i++)
    {
        delete childs[i];
    }
    childs.clear();
}

void GameTreeNode::deleteChessBoard()
{
    delete chessBoard;
    chessBoard = 0;
}

void GameTreeNode::createChildNode(int &row, int &col)
{
    ChessBoard tempBoard = *chessBoard;
    tempBoard.doNextStep(row, col, -lastStep.getColor());
    tempBoard.updateThreat();
    GameTreeNode *tempNode = new GameTreeNode(&tempBoard);
    tempNode->hash = hash;
    tempBoard.updateHashPair(tempNode->hash, row, col, -lastStep.getColor());
    childs.push_back(tempNode);
}

void GameTreeNode::buildFirstChilds()
{
    //build AI step
    if (getHighest(-playerColor) >= SCORE_5_CONTINUE)
    {
        for (int i = 0; i < BOARD_ROW_MAX; ++i)
        {
            for (int j = 0; j < BOARD_COL_MAX; ++j)
            {
                if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                {
                    if (chessBoard->getPiece(i, j).getThreat(-playerColor) >= SCORE_5_CONTINUE)
                    {
                        createChildNode(i, j);
                    }
                }
            }
        }
    }
    else if (getHighest(playerColor) >= SCORE_5_CONTINUE)
    {
        for (int i = 0; i < BOARD_ROW_MAX; ++i)
        {
            for (int j = 0; j < BOARD_COL_MAX; ++j)
            {
                if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                {
                    if (chessBoard->getPiece(i, j).getThreat(playerColor) >= SCORE_5_CONTINUE)//堵player即将形成的五连
                    {
                        createChildNode(i, j);
                    }
                }
            }
        }
    }
    else
    {
        //int score;
        for (int i = 0; i < BOARD_ROW_MAX; ++i)
        {
            for (int j = 0; j < BOARD_COL_MAX; ++j)
            {
                if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                {
                    createChildNode(i, j);
                }
            }
        }
    }

}

int GameTreeNode::searchBest2(ThreadPool &pool)
{
    size_t searchNum = 10;
    sort(sortList, 0, childs.size() - 1);
    while (true)
    {
        if (childsInfo[sortList[childs.size() - 1].key].hasSearch)//深度搜索过的得分不会比搜索之前高，如果目前得分最高的已经是搜索过的了，再进行搜索也不会找到得分比它更高的了
        {
            //防止随机化算法导致
            //最后一个搜索过，但倒数第二个未搜索过并且得分等于最后一个，有概率选取到倒数第二个
            int i = childs.size() - 1;
            while (i > 0 && sortList[i - 1].value == sortList[i].value)
            {
                i--;
                if (!childsInfo[sortList[i].key].hasSearch)//相等得分中有没搜索过的
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
            if (!childsInfo[sortList[i].key].hasSearch)
            {
                Piece p = chessBoard->getPiece(childs[sortList[i].key]->lastStep.row, childs[sortList[i].key]->lastStep.col);
                if (childsInfo[sortList[i].key].lastStepScore > 998 && p.getThreat(playerColor) < 8000)//无意义的冲四
                {
                    childsInfo[sortList[i].key].hasSearch = true;
                    sortList[i].value = -1000000;
                    continue;
                }
                Task t;
                t.node = childs[sortList[i].key];
                t.index = sortList[i].key;
                //t.threatInfo = childsInfo;
                t.type = TASKTYPE_DEFEND;
                pool.run(t, false);
            }
        }

        //等待线程
        pool.wait();
        for (size_t i = childs.size() - searchNum; i < childs.size(); i++)
        {
            if (!childsInfo[sortList[i].key].hasSearch)
            {
                childsInfo[sortList[i].key].hasSearch = true;//hasSearch值对buildSortListInfo有影响
                buildSortListInfo(i);
                int temp = childsInfo[sortList[i].key].lastStepScore - childsInfo[sortList[i].key].rating.totalScore;
                if (temp > bestRating)
                {
                    bestRating = temp;
                }
            }
        }
        sort(sortList, 0, childs.size() - 1);
        searchNum += 10;
    }
    //随机化
    int i = childs.size() - 1;
    while (i > 0 && sortList[i - 1].value == sortList[i].value) i--;
    return i + rand() % (childs.size() - i);
}

//多线程

void GameTreeNode::buildTreeThreadFunc(int n, GameTreeNode *child)
{
    child->buildDefendPlayerNode();
    childsInfo[n].rating = child->getBestRating();
    delete child;
}

int GameTreeNode::searchBest()
{
    size_t searchNum = 10;
    thread buildTreeThread[MAXTHREAD];
    sort(sortList, 0, childs.size() - 1);
    while (true)
    {
        if (childsInfo[sortList[childs.size() - 1].key].hasSearch)//深度搜索过的得分不会比搜索之前高，如果目前得分最高的已经是搜索过的了，再进行搜索也不会找到得分比它更高的了
        {
            //防止随机化算法导致
            //最后一个搜索过，但倒数第二个未搜索过并且得分等于最后一个，有概率选取到倒数第二个
            int i = childs.size() - 1;
            while (i > 0 && sortList[i - 1].value == sortList[i].value)
            {
                i--;
                if (!childsInfo[sortList[i].key].hasSearch)//相等得分中有没搜索过的
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
            if (!childsInfo[sortList[i].key].hasSearch)
            {
                GameTreeNode *node = new GameTreeNode;
                *node = *childs[sortList[i].key];
                buildTreeThread[j] = thread(buildTreeThreadFunc, sortList[i].key, node);
                j++;
                childsInfo[sortList[i].key].hasSearch = true;
            }
        }

        for (int i = 0; i < j; i++)
        {
            buildTreeThread[i].join();
        }

        for (size_t i = childs.size() - searchNum; i < childs.size(); i++)
        {
            buildSortListInfo(i);
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
    ThreadPool pool;
    Position result;
    int bestSearchPos;
    buildFirstChilds();
    sortList = new SortInfo[childs.size()];
    childsInfo = new ChildInfo[childs.size()];
    int score;

    if (childs.size() == 1)//只有这一个
    {
        if ((resultFlag == AIRESULTFLAG_NEARWIN || resultFlag == AIRESULTFLAG_TAUNT) &&
            getHighest(-playerColor) < SCORE_5_CONTINUE && getHighest(playerColor) >= SCORE_5_CONTINUE)//垂死冲四
        {
            resultFlag = AIRESULTFLAG_TAUNT;//嘲讽
        }
        else
        {
            resultFlag = AIRESULTFLAG_NORMAL;
        }

        result = Position{ childs[0]->lastStep.row, childs[0]->lastStep.col };
        goto endsearch;
    }

    for (size_t i = 0; i < childs.size(); ++i)//初始化，顺便找出特殊情况
    {
        score = childs[i]->chessBoard->getLastStepScores(false);//进攻权重
        if (score >= SCORE_5_CONTINUE)
        {
            resultFlag = AIRESULTFLAG_WIN;
            result = Position{ childs[i]->lastStep.row, childs[i]->lastStep.col };
            goto endsearch;
        }
        else if (score < 0)
        {
            if (childs.size() == 1)//只有这一个,只能走禁手了
            {
                resultFlag = AIRESULTFLAG_FAIL;
                result = Position{ childs[i]->lastStep.row, childs[i]->lastStep.col };
                goto endsearch;
            }
            delete childs[i];
            childs.erase(childs.begin() + i);//保证禁手不走
            i--;
            continue;
        }
        childsInfo[i].hasSearch = false;
        childsInfo[i].lastStepScore = score;
        sortList[i].key = i;
        buildSortListInfo(i);
    }

    pool.start();

    if (ChessBoard::level >= AILEVEL_MASTER)
    {
        bestRating = 100;//代表步数
        int atackSearchTreeResult = buildAtackSearchTree(pool);
        if (atackSearchTreeResult > -1)
        {
            resultFlag = AIRESULTFLAG_NEARWIN;
            result = Position{ childs[atackSearchTreeResult]->lastStep.row, childs[atackSearchTreeResult]->lastStep.col };
            goto endsearch;
        }
    }
    transpositionTable.clear();

    bestRating = INT32_MIN;
    int activeChildIndex = getActiveChild();
    childs[activeChildIndex]->buildDefendPlayerNode();
    //childs[activeChildIndex]->printTree();
    childsInfo[activeChildIndex].hasSearch = true;
    childsInfo[activeChildIndex].rating = childs[activeChildIndex]->getBestRating();
    buildSortListInfo(activeChildIndex);
    childs[activeChildIndex]->deleteChilds();
    bestRating = childsInfo[activeChildIndex].lastStepScore - childsInfo[activeChildIndex].rating.totalScore;

    //int humancontrol = 41;
    //childs[humancontrol]->buildPlayer();
    //childsInfo[humancontrol].hasSearch = true;
    //childsInfo[humancontrol].rating = childs[humancontrol]->getBestRating();
    //buildSortListInfo(humancontrol, childsInfo, sortList);
    //childs[humancontrol]->printTree();
    //childs[humancontrol]->deleteChilds();

    //开始深度搜索
    bestSearchPos = multiThread ? searchBest2(pool) : searchBest();

    transpositionTable.clear();

    resultFlag = AIRESULTFLAG_NORMAL;
    //if (playerColor == STATE_CHESS_BLACK && lastStep.step < 10)//防止开局被布阵
    //{
    //    activeChildIndex = getDefendChild();
    //    result = Position{ childs[activeChildIndex]->lastStep.row, childs[activeChildIndex]->lastStep.col };
    //}
    //else 
    if (childsInfo[sortList[bestSearchPos].key].rating.highestScore >= SCORE_5_CONTINUE)
    {
        resultFlag = AIRESULTFLAG_FAIL;
        activeChildIndex = getDefendChild();//必输局面跟随玩家的落子去堵
        if (chessBoard->getPiece(childs[activeChildIndex]->lastStep.row, childs[activeChildIndex]->lastStep.col).getThreat(lastStep.getColor()) > 2000)
            result = Position{ childs[activeChildIndex]->lastStep.row, childs[activeChildIndex]->lastStep.col };
        else
            result = Position{ childs[sortList[bestSearchPos].key]->lastStep.row, childs[sortList[bestSearchPos].key]->lastStep.col };
    }
    else if (lastStep.step > 10 && childsInfo[activeChildIndex].rating.highestScore < SCORE_3_DOUBLE)//如果主动出击不会导致走向失败，则优先主动出击
    {
        result = Position{ childs[activeChildIndex]->lastStep.row, childs[activeChildIndex]->lastStep.col };
    }
    else
    {
        result = Position{ childs[sortList[bestSearchPos].key]->lastStep.row, childs[sortList[bestSearchPos].key]->lastStep.col };
    }
endsearch:
    delete[] sortList;
    sortList = NULL;
    delete[] childsInfo;
    childsInfo = NULL;
    pool.stop();
    return result;
}

RatingInfo GameTreeNode::getBestRating()
{
    if (childs.size() == 0)
    {
        if (lastStep.getColor() == -playerColor)//叶子节点是AI
        {
            return RatingInfo(getTotal(playerColor), getHighest(playerColor));//取得player分数
        }
        else//叶子节点是player,表示提前结束,AI取胜,否则叶子节点一定会是AI
        {
            return RatingInfo(getTotal(playerColor), getHighest(playerColor));//取得player分数，getHighest必定是-100000
        }
    }

    RatingInfo tempThreat;
    RatingInfo best = (lastStep.getColor() == playerColor) ? RatingInfo{ 500000, 500000 } : RatingInfo{ 0,0 };//初始化best

    if (lastStep.getColor() == -playerColor)//AI节点
    {
        for (size_t i = 0; i < childs.size(); ++i)
        {
            tempThreat = childs[i]->getBestRating();//递归
            if (tempThreat.totalScore > best.totalScore)//best原则:player下过的节点player得分越大越好(默认player走最优点)
            {
                if (tempThreat.highestScore > best.highestScore / 2)
                {
                    best = tempThreat;
                }
            }
        }
    }
    else//player节点
    {
        for (size_t i = 0; i < childs.size(); ++i)
        {
            tempThreat = childs[i]->getBestRating();//child是AI节点
            if (tempThreat.totalScore < best.totalScore)//best原则:AI下过的节点player得分越小越好
            {
                if (tempThreat.highestScore / 2 < best.highestScore)
                {
                    best = tempThreat;
                }
            }
        }
    }

    return best;
}

void GameTreeNode::buildSortListInfo(int n)
{
    int i = sortList[n].key;
    if (!childsInfo[i].hasSearch)
    {
        // sortList[n].value = childsInfo[i].lastStepScore - childs[i]->getHighest(playerColor) - childs[i]->getTotal(playerColor) / 10;
        sortList[n].value = childsInfo[i].lastStepScore - childs[i]->getTotal(playerColor);
    }
    else
    {
        ChildInfo temp = childsInfo[i];
        //int tempScore = childsInfo[i].lastStepScore - temp.rating.highestScore - temp.rating.totalScore / 10;
        int tempScore = childsInfo[i].lastStepScore - temp.rating.totalScore;
        if (tempScore < sortList[n].value)
        {
            //sortList[n].value = childs[i]->currentScore - temp.totalScore;
            sortList[n].value = tempScore;
        }
    }
}

int GameTreeNode::getActiveChild()
{
    int max = INT_MIN, flag = 0, temp;
    for (size_t i = 0; i < childs.size(); ++i)
    {

        temp = childs[i]->getTotal(-playerColor);
        if (temp >= SCORE_5_CONTINUE)
        {
            temp -= SCORE_5_CONTINUE;//降低无意义冲四的优先级
        }
        if (temp > max)
        {
            max = temp;
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
        temp = childs[i]->getTotal(playerColor) - chessBoard->getPiece(childs[i]->lastStep.row, childs[i]->lastStep.col).getThreat(-playerColor);
        if (temp < min)
        {
            results.clear();
            min = temp;
            results.push_back(i);
        }
        else if (temp == min)
        {
            results.push_back(i);
        }
    }
    return results[rand() % results.size()];
}

void GameTreeNode::buildDefendPlayerNode(bool recursive)
{
    //lastStep.getColor()是AI下的
    if (getHighest(playerColor) >= SCORE_5_CONTINUE)
    {
        goto end;
    }

    if (getDepth() >= maxSearchDepth)//除非特殊情况，保证最后一步是AI下的，故而=maxSearchDepth时就直接结束
    {
        goto end;
    }

    if (getHighest(-playerColor) >= SCORE_5_CONTINUE)//防冲四
    {
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
                        if (score < 0)
                        {
                            goto end;
                        }
                        createChildNode(i, j);
                    }
                }
            }
        }
    }
    else if (getHighest(playerColor) > 99 && getHighest(playerColor) < SCORE_4_DOUBLE)//进攻
    {
        ChessBoard tempBoard;
        GameTreeNode *tempNode;
        int score;
        RatingInfo tempInfo = { 0,0 };
        int worst;
        for (int i = 0; i < BOARD_ROW_MAX; ++i)
        {
            for (int j = 0; j < BOARD_COL_MAX; ++j)
            {
                if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                {
                    score = chessBoard->getPiece(i, j).getThreat(playerColor);//player
                    if (ChessBoard::level >= AILEVEL_MASTER && score > 0 && score < 100)//特殊情况，会形成三四
                    {
                        tempBoard = *chessBoard;
                        tempBoard.doNextStep(i, j, playerColor);
                        tempBoard.updateThreat();
                        tempInfo = tempBoard.getRatingInfo(playerColor);
                        if (tempInfo.highestScore >= SCORE_4_DOUBLE)
                        {

                            if (childs.size() > 4)
                            {

                                worst = findWorstNode();
                                if (childs[worst]->getTotal(playerColor) < tempInfo.totalScore)
                                {
                                    delete childs[worst];
                                    childs.erase(childs.begin() + worst);
                                }
                            }

                            tempNode = new GameTreeNode(&tempBoard);
                            tempNode->hash = hash;
                            tempBoard.updateHashPair(tempNode->hash, i, j, -lastStep.getColor());
                            childs.push_back(tempNode);
                        }
                    }
                    else if (score > 99 && score < 10000)
                    {
                        tempBoard = *chessBoard;
                        tempBoard.doNextStep(i, j, playerColor);
                        tempBoard.updateThreat();

                        if (childs.size() > 4)
                        {
                            tempInfo = tempBoard.getRatingInfo(playerColor);
                            worst = findWorstNode();
                            if (childs[worst]->getTotal(playerColor) < tempInfo.totalScore)
                            {
                                delete childs[worst];
                                childs.erase(childs.begin() + worst);
                            }
                        }

                        tempNode = new GameTreeNode(&tempBoard);
                        tempNode->hash = hash;
                        tempBoard.updateHashPair(tempNode->hash, i, j, -lastStep.getColor());
                        childs.push_back(tempNode);
                    }
                }
            }
        }
    }
    else if (getHighest(playerColor) >= SCORE_4_DOUBLE)
    {
        int score;
        for (int i = 0; i < BOARD_ROW_MAX; ++i)
        {
            for (int j = 0; j < BOARD_COL_MAX; ++j)
            {
                if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                {
                    score = chessBoard->getPiece(i, j).getThreat(playerColor);
                    if (score >= SCORE_4_DOUBLE)
                    {
                        createChildNode(i, j);//AI才减1
                    }
                }
            }
        }
    }

end:
    delete chessBoard;
    chessBoard = 0;
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
                if (hasSearch[bestPos])
                {
                    break;
                }
                else
                {
                    TransTableData data = buildDefendChildWithTransTable(childs[bestPos]);
                    //if (getDepth() < transTableMaxDepth)
                    //{
                    //    mut_transTable.lock_shared();
                    //    if (transpositionTable.find(childs[bestPos]->hash.z32key) != transpositionTable.end())//命中
                    //    {
                    //        data = transpositionTable[childs[bestPos]->hash.z32key];
                    //        mut_transTable.unlock_shared();
                    //        if (data.checksum == childs[bestPos]->hash.z64key)//校验成功
                    //        {
                    //            hash_hit++;
                    //            //不用build了，直接用现成的
                    //            if (playerColor == STATE_CHESS_BLACK)
                    //            {
                    //                childs[bestPos]->black = data.black;
                    //            }
                    //            else
                    //            {
                    //                childs[bestPos]->white = data.white;
                    //            }
                    //            goto complete;
                    //        }
                    //        else//冲突，覆盖
                    //        {
                    //            hash_clash++;
                    //        }
                    //    }
                    //    else//未命中
                    //    {
                    //        mut_transTable.unlock_shared();
                    //        hash_miss++;
                    //    }

                    //    data.checksum = childs[bestPos]->hash.z64key;
                    //    childs[bestPos]->buildAI();
                    //    if (playerColor == STATE_CHESS_BLACK)
                    //    {
                    //        data.black = childs[bestPos]->getBestRating();
                    //    }
                    //    else
                    //    {
                    //        data.white = childs[bestPos]->getBestRating();
                    //    }
                    //    mut_transTable.lock();
                    //    transpositionTable[childs[bestPos]->hash.z32key] = data;
                    //    mut_transTable.unlock();
                    //}
                    //else
                    //{
                    //    childs[bestPos]->buildAI();
                    //}

                complete:
                    if (bestRating > (playerColor == STATE_CHESS_BLACK ? -data.black.totalScore : -data.white.totalScore))
                    {
                        break;//alpha剪枝
                    }
                    buildNodeInfo(bestPos, childrenInfo);
                    hasSearch[bestPos] = true;
                }
            }
            delete[] hasSearch;
            delete[] childrenInfo;
        }
    }
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
        RatingInfo temp = childs[i]->getBestRating();
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

void GameTreeNode::buildDefendAINode(bool recursive)
{
    //player节点
    int score;
    int highest = getHighest(-playerColor);
    //进攻
    if (getHighest(-playerColor) >= SCORE_5_CONTINUE)
    {
        if (playerColor == 1)
        {
            black = { -SCORE_5_CONTINUE , -SCORE_5_CONTINUE };
        }
        else
        {
            white = { -SCORE_5_CONTINUE , -SCORE_5_CONTINUE };
        }
        goto end;
    }
    else if (getHighest(-playerColor) >= SCORE_4_DOUBLE && getHighest(playerColor) < SCORE_5_CONTINUE)// 没有即将形成的五连，可以去绝杀
    {
        for (int i = 0; i < BOARD_ROW_MAX; ++i)
        {
            for (int j = 0; j < BOARD_COL_MAX; ++j)
            {
                if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                {
                    if (chessBoard->getPiece(i, j).getThreat(-playerColor) >= SCORE_4_DOUBLE && chessBoard->getPiece(i, j).getThreat(playerColor) < SCORE_4_DOUBLE)//防止和后面重复
                    {
                        createChildNode(i, j);
                    }
                }
            }
        }
    }

    //防守
    if (getHighest(playerColor) >= SCORE_5_CONTINUE)//堵playerd的冲四(即将形成的五连)
    {
        for (int i = 0; i < BOARD_ROW_MAX; ++i)
        {
            for (int j = 0; j < BOARD_COL_MAX; ++j)
            {
                if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                {
                    if (chessBoard->getPiece(i, j).getThreat(playerColor) >= SCORE_5_CONTINUE)//堵player即将形成的五连
                    {
                        score = chessBoard->getPiece(i, j).getThreat(-playerColor);
                        if (score < 0)//被禁手了
                        {
                            goto end;//被禁手，必输无疑
                        }
                        createChildNode(i, j);
                        goto end;//必堵，堵一个就行了，如果还有一个就直接输了
                    }
                }
            }
        }
    }
    else if (getHighest(playerColor) >= SCORE_4_DOUBLE)//堵player的活三(即将形成的三四、活四)
    {
        for (int i = 0; i < BOARD_ROW_MAX; ++i)
        {
            for (int j = 0; j < BOARD_COL_MAX; ++j)
            {
                if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                {
                    if (chessBoard->getPiece(i, j).getThreat(playerColor) >= SCORE_4_DOUBLE)//堵player的活三、即将形成的三四
                    {
                        score = chessBoard->getPiece(i, j).getThreat(-playerColor);
                        if (score < 0)//被禁手了
                        {
                            continue;
                        }
                        createChildNode(i, j);
                    }
                }
            }
        }
    }

end:
    delete chessBoard;
    chessBoard = 0;
    if (recursive)//需要递归
    {
        for (size_t i = 0; i < childs.size(); i++)
        {
            //if (getDepth() < transTableMaxDepth)
            //{
            //    mut_transTable.lock_shared();
            //    if (transpositionTable.find(childs[i]->hash.z32key) != transpositionTable.end())//命中
            //    {
            //        TransTableData data = transpositionTable[childs[i]->hash.z32key];
            //        mut_transTable.unlock_shared();
            //        if (data.checksum == childs[i]->hash.z64key)//校验成功
            //        {
            //            hash_hit++;
            //            //不用build了，直接用现成的
            //            if (playerColor == STATE_CHESS_BLACK)
            //            {
            //                childs[i]->black = data.black;
            //            }
            //            else
            //            {
            //                childs[i]->white = data.white;
            //            }
            //            continue;
            //        }
            //        else//冲突，覆盖
            //        {
            //            hash_clash++;
            //        }
            //    }
            //    else//未命中
            //    {
            //        mut_transTable.unlock_shared();
            //        hash_miss++;
            //    }
            //    TransTableData data;
            //    data.checksum = childs[i]->hash.z64key;
            //    childs[i]->buildPlayer();
            //    if (playerColor == STATE_CHESS_BLACK)
            //    {
            //        data.black = childs[i]->getBestRating();
            //    }
            //    else
            //    {
            //        data.white = childs[i]->getBestRating();
            //    }
            //    mut_transTable.lock();
            //    transpositionTable[childs[i]->hash.z32key] = data;
            //    mut_transTable.unlock();
            //}
            //else
            //{
            //    childs[i]->buildPlayer();
            //}
            buildDefendChildWithTransTable(childs[i]);
        }
    }
}

TransTableData GameTreeNode::buildDefendChildWithTransTable(GameTreeNode* child)
{
    TransTableData data;
    if (getDepth() < transTableMaxDepth)
    {
        mut_transTable.lock_shared();
        if (transpositionTable.find(child->hash.z32key) != transpositionTable.end())//命中
        {
            data = transpositionTable[child->hash.z32key];
            mut_transTable.unlock_shared();
            if (data.checksum == child->hash.z64key)//校验成功
            {
                hash_hit++;
                //不用build了，直接用现成的
                if (playerColor == STATE_CHESS_BLACK)
                {
                    child->black = data.black;
                }
                else
                {
                    child->white = data.white;
                }
                return data;
            }
            else//冲突，覆盖
            {
                hash_clash++;
            }
        }
        else//未命中
        {
            mut_transTable.unlock_shared();
            hash_miss++;
        }

        data.checksum = child->hash.z64key;
        child->buildChild(true);
        if (playerColor == STATE_CHESS_BLACK)
        {
            data.black = child->getBestRating();
        }
        else
        {
            data.white = child->getBestRating();
        }
        mut_transTable.lock();
        transpositionTable[child->hash.z32key] = data;
        mut_transTable.unlock();
    }
    else
    {
        child->buildChild(true);
        if (playerColor == STATE_CHESS_BLACK)
        {
            data.black = child->getBestRating();
        }
        else
        {
            data.white = child->getBestRating();
        }
    }
    return data;
}

int GameTreeNode::buildAtackSearchTree(ThreadPool &pool)
{
    if (getHighest(-playerColor) >= SCORE_5_CONTINUE)//已有5连，不用搜索了
    {
        assert(0);//不会来这里
    }
    //vector<int> index;
    bestIndex = -1;
    for (size_t i = 0; i < childs.size(); ++i)
    {
        //lastStepScore是进攻权重
        if (childsInfo[i].lastStepScore < 1000 && childsInfo[i].lastStepScore > 0 && getHighest(-playerColor) < SCORE_4_DOUBLE)//特殊情况
        {
            if (childs[i]->getHighest(-playerColor) >= SCORE_4_DOUBLE)
            {
                Task t;
                t.node = new GameTreeNode();
                *t.node = *childs[i];
                //t.node = childs[i];
                t.index = i;
                //t.threatInfo = childsInfo;
                t.type = TASKTYPE_ATACK;
                pool.run(t, false);
                //index.push_back(i);
            }
        }
        else if (childsInfo[i].lastStepScore > 1000 && childs[i]->getHighest(playerColor) < SCORE_5_CONTINUE)
        {
            Task t;
            t.node = new GameTreeNode();
            *t.node = *childs[i];
            //t.node = childs[i];
            t.index = i;
            //t.threatInfo = childsInfo;
            t.type = TASKTYPE_ATACK;
            pool.run(t, false);
            //index.push_back(i);
        }
    }
    pool.wait();
    //int best = -1;
    //for (auto i : index)
    //{
    //    //childs[i]->printTree();
    //    if (childsInfo[i].depth < 0)
    //    {
    //        continue;
    //    }
    //    if (childsInfo[i].rating.highestScore >= SCORE_5_CONTINUE)
    //    {
    //        if (best < 0)
    //        {
    //            best = i;
    //        }
    //        else
    //        {
    //            if (childsInfo[best].depth > childsInfo[i].depth)
    //            {
    //                best = i;
    //            }
    //        }
    //    }
    //    //childs[i]->deleteChilds();
    //}
    return bestIndex;
}

void GameTreeNode::buildAtackTreeNode()
{
    if (lastStep.getColor() == playerColor)//build AI
    {

        if (getHighest(-playerColor) >= SCORE_5_CONTINUE)//成功
        {
            goto end;
        }

        if (getHighest(playerColor) >= SCORE_5_CONTINUE)//防冲四
        {
            int score;
            for (int i = 0; i < BOARD_ROW_MAX; ++i)
            {
                for (int j = 0; j < BOARD_COL_MAX; ++j)
                {
                    if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                    {
                        if (chessBoard->getPiece(i, j).getThreat(playerColor) >= SCORE_5_CONTINUE)
                        {
                            score = chessBoard->getPiece(i, j).getThreat(-playerColor);
                            if (score < 0)
                            {
                                goto end;
                            }
                            createChildNode(i, j);
                            buildAtackChildWithTransTable(childs.back());
                        }
                    }
                }
            }
        }
        else if (getHighest(-playerColor) > 99 && getHighest(-playerColor) < SCORE_4_DOUBLE)//进攻
        {
            ChessBoard tempBoard;
            GameTreeNode *tempNode;
            int score;
            RatingInfo tempInfo = { 0,0 };
            //int worst;
            for (int i = 0; i < BOARD_ROW_MAX; ++i)
            {
                for (int j = 0; j < BOARD_COL_MAX; ++j)
                {
                    if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                    {
                        score = chessBoard->getPiece(i, j).getThreat(-playerColor);
                        if (score > 0 && score < 100)
                        {
                            tempBoard = *chessBoard;
                            tempBoard.doNextStep(i, j, -playerColor);
                            tempBoard.updateThreat();
                            if (tempBoard.getRatingInfo(-playerColor).highestScore >= SCORE_4_DOUBLE)
                            {
                                tempNode = new GameTreeNode(&tempBoard);
                                tempNode->hash = hash;
                                tempBoard.updateHashPair(tempNode->hash, i, j, -lastStep.getColor());
                                childs.push_back(tempNode);
                                buildAtackChildWithTransTable(childs.back());
                            }
                        }
                        else if (score > 99 && score < 10000)
                        {
                            tempBoard = *chessBoard;
                            tempBoard.doNextStep(i, j, -playerColor);
                            tempBoard.updateThreat();

                            /*if (childs.size() > 4)
                            {
                                tempInfo = tempBoard.getRatingInfo(playerColor);
                                worst = findWorstNode();
                                if (childs[worst]->getTotal(playerColor) < tempInfo.totalScore)
                                {
                                    delete childs[worst];
                                    childs.erase(childs.begin() + worst);
                                }
                            }*/

                            tempNode = new GameTreeNode(&tempBoard);
                            tempNode->hash = hash;
                            tempBoard.updateHashPair(tempNode->hash, i, j, -lastStep.getColor());
                            childs.push_back(tempNode);
                            buildAtackChildWithTransTable(childs.back());
                        }
                    }
                }
            }
        }
        else if (getHighest(-playerColor) >= SCORE_4_DOUBLE)
        {
            int score;
            for (int i = 0; i < BOARD_ROW_MAX; ++i)
            {
                for (int j = 0; j < BOARD_COL_MAX; ++j)
                {
                    if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                    {
                        score = chessBoard->getPiece(i, j).getThreat(-playerColor);
                        if (score >= SCORE_4_DOUBLE)
                        {
                            createChildNode(i, j);
                            buildAtackChildWithTransTable(childs.back());
                        }
                    }
                }
            }
        }
    }
    else//buildplayer
    {
        //五连
        if (getDepth() >= maxSearchDepth)//除非特殊情况，保证最后一步是AI下的，故而=maxSearchDepth时就直接结束
        {
            goto end;
        }
        if (GameTreeNode::bestRating > -1 && getDepth() > GameTreeNode::bestRating)
        {
            goto end;
        }
        if (getHighest(playerColor) >= SCORE_5_CONTINUE)
        {
            goto end;
        }
        else if (getHighest(playerColor) >= SCORE_4_DOUBLE && getHighest(-playerColor) < SCORE_5_CONTINUE)// 没有即将形成的五连，可以去冲四
        {
            for (int i = 0; i < BOARD_ROW_MAX; ++i)
            {
                for (int j = 0; j < BOARD_COL_MAX; ++j)
                {
                    if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                    {
                        if (chessBoard->getPiece(i, j).getThreat(playerColor) >= SCORE_4_DOUBLE)
                        {
                            createChildNode(i, j);
                            RatingInfo2 info = buildAtackChildWithTransTable(childs.back());
                            if (info.depth < 0 || info.depth >= GameTreeNode::bestRating)
                            {
                                goto end;
                            }
                        }
                    }
                }
            }
        }
        //防守
        if (getHighest(-playerColor) >= SCORE_5_CONTINUE)//堵playerd的冲四(即将形成的五连)
        {
            for (int i = 0; i < BOARD_ROW_MAX; ++i)
            {
                for (int j = 0; j < BOARD_COL_MAX; ++j)
                {
                    if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                    {
                        if (chessBoard->getPiece(i, j).getThreat(-playerColor) >= SCORE_5_CONTINUE)//堵player即将形成的五连
                        {
                            int score = chessBoard->getPiece(i, j).getThreat(playerColor);
                            if (score < 0)//被禁手了
                            {
                                goto end;//被禁手，必输无疑
                            }
                            createChildNode(i, j);
                            RatingInfo2 info = buildAtackChildWithTransTable(childs.back());
                            goto end;//必堵，堵一个就行了，如果还有一个就直接输了
                        }
                    }
                }
            }
        }
        else if (getHighest(-playerColor) >= SCORE_3_DOUBLE)//堵player的活三(即将形成的三四、活四、三三)
        {
            ChessBoard tempBoard;
            GameTreeNode *tempNode;
            for (int i = 0; i < BOARD_ROW_MAX; ++i)
            {
                for (int j = 0; j < BOARD_COL_MAX; ++j)
                {
                    if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0 && chessBoard->getPiece(i, j).getThreat(playerColor) < SCORE_4_DOUBLE)//防止和前面重复
                    {
                        if (chessBoard->getPiece(i, j).getThreat(-playerColor) >= SCORE_3_DOUBLE)//堵player的活三、即将形成的三四
                        {
                            int score = chessBoard->getPiece(i, j).getThreat(playerColor);
                            if (score < 0)//被禁手了
                            {
                                continue;
                            }
                            createChildNode(i, j);
                            RatingInfo2 info = buildAtackChildWithTransTable(childs.back());
                            if (info.depth < 0 || info.depth >= GameTreeNode::bestRating)
                            {
                                goto end;
                            }
                        }
                        //else if (chessBoard->getPiece(i, j).getThreat(-playerColor) >= 100)//间接堵三三、三四
                        //{
                        //    int score = chessBoard->getPiece(i, j).getThreat(playerColor);
                        //    if (score < 0)//被禁手了
                        //    {
                        //        continue;
                        //    }
                        //    //createChildNode(i, j);
                        //    //RatingInfo2 info = buildAtackChildWithTransTable(childs.back());
                        //    //if (info.depth < 0 || info.depth >= GameTreeNode::bestRating)
                        //    //{
                        //    //    goto end;
                        //    //}
                        //    tempBoard = *chessBoard;
                        //    tempBoard.doNextStep(i, j, playerColor);
                        //    tempBoard.updateThreat();
                        //    if (tempBoard.getRatingInfo(-playerColor).highestScore < SCORE_3_DOUBLE)
                        //    {
                        //        tempNode = new GameTreeNode(&tempBoard);
                        //        tempNode->hash = hash;
                        //        tempBoard.updateHashPair(tempNode->hash, i, j, -lastStep.getColor());
                        //        childs.push_back(tempNode);
                        //        RatingInfo2 info = buildAtackChildWithTransTable(childs.back());
                        //        if (info.depth < 0 || info.depth >= GameTreeNode::bestRating)
                        //        {
                        //            goto end;
                        //        }
                        //    }
                        //}
                    }
                }
            }
        }
    }
end:
    delete chessBoard;
    chessBoard = 0;
    //RatingInfo2 info;
    //for (size_t i = 0; i < childs.size(); i++)
    //{
    //    //if (getDepth() < transTableMaxDepth)
    //    //{
    //    //    mut_transTable.lock_shared();
    //    //    if (transpositionTable.find(childs[i]->hash.z32key) != transpositionTable.end())//命中
    //    //    {
    //    //        TransTableData data = transpositionTable[childs[i]->hash.z32key];
    //    //        mut_transTable.unlock_shared();
    //    //        if (data.checksum == childs[i]->hash.z64key)//校验成功
    //    //        {
    //    //            hash_hit++;
    //    //            //不用build了，直接用现成的
    //    //            childs[i]->black = data.black;
    //    //            childs[i]->white = data.white;
    //    //            childs[i]->lastStep.step = data.steps;
    //    //            info = childs[i]->getBestAtackRating();
    //    //            goto endtans;
    //    //        }
    //    //        else//冲突，覆盖
    //    //        {
    //    //            hash_clash++;
    //    //        }
    //    //    }
    //    //    else//未命中
    //    //    {
    //    //        mut_transTable.unlock_shared();
    //    //        hash_miss++;
    //    //    }
    //    //    TransTableData data;
    //    //    data.checksum = childs[i]->hash.z64key;
    //    //    childs[i]->buildAtackTreeNode();
    //    //    info = childs[i]->getBestAtackRating();
    //    //    data.black = info.black;
    //    //    data.white = info.white;
    //    //    data.steps = info.depth + startStep;

    //    //    mut_transTable.lock();
    //    //    transpositionTable[childs[i]->hash.z32key] = data;
    //    //    mut_transTable.unlock();
    //    //}
    //    //else
    //    //{
    //    //    childs[i]->buildAtackTreeNode();
    //    //    info = childs[i]->getBestAtackRating();
    //    //}
    //    info = atackSearchBuildToLeaf(childs[i]);
    ////endtans:
    ////    if (lastStep.getColor() == playerColor)//build AI
    ////    {
    ////        //if ((playerColor == STATE_CHESS_BLACK ? info.white.highestScore : info.black.highestScore) >= SCORE_5_CONTINUE)
    ////        //{
    ////        //    /*if (info.depth > GameTreeNode::bestRating)
    ////        //    {
    ////        //        break;
    ////        //    }*/
    ////        //    /*if (info.depth > -1 && info.depth < GameTreeNode::bestRating)
    ////        //    {
    ////        //        break;
    ////        //    }*/
    ////        //}
    ////    }
    ////    else
    ////    {
    ////        //如果有一个不能绝杀，那么都不能绝杀，因为这里是player落子，默认选最优
    ////        if (info.depth < 0 || info.depth > GameTreeNode::bestRating)
    ////        {
    ////            break;
    ////        }
    ////    }
    //}
}

RatingInfo2 GameTreeNode::buildAtackChildWithTransTable(GameTreeNode* child)
{
    RatingInfo2 info;
    if (getDepth() < transTableMaxDepth)
    {
        mut_transTable.lock_shared();
        if (transpositionTable.find(child->hash.z32key) != transpositionTable.end())//命中
        {
            TransTableData data = transpositionTable[child->hash.z32key];
            mut_transTable.unlock_shared();
            if (data.checksum == child->hash.z64key)//校验成功
            {
                hash_hit++;
                //不用build了，直接用现成的
                child->black = data.black;
                child->white = data.white;
                child->lastStep.step = data.steps;
                info = child->getBestAtackRating();
                return info;
            }
            else//冲突，覆盖
            {
                hash_clash++;
            }
        }
        else//未命中
        {
            mut_transTable.unlock_shared();
            hash_miss++;
        }
        TransTableData data;
        data.checksum = child->hash.z64key;
        child->buildAtackTreeNode();
        info = child->getBestAtackRating();
        data.black = info.black;
        data.white = info.white;
        data.steps = info.depth + startStep;

        mut_transTable.lock();
        transpositionTable[child->hash.z32key] = data;
        mut_transTable.unlock();
    }
    else
    {
        child->buildAtackTreeNode();
        info = child->getBestAtackRating();
    }
    return info;
}

RatingInfo2 GameTreeNode::getBestAtackRating()
{
    RatingInfo2 result, temp;
    if (childs.size() == 0)
    {
        if (playerColor == STATE_CHESS_BLACK)
        {
            result.black = RatingInfo{ getTotal(playerColor), getHighest(playerColor) };
            result.white = RatingInfo{ getTotal(-playerColor) , getHighest(-playerColor) };
        }
        else
        {
            result.white = RatingInfo{ getTotal(playerColor), getHighest(playerColor) };
            result.black = RatingInfo{ getTotal(-playerColor), getHighest(-playerColor) };
        }
        if (lastStep.getColor() != playerColor)//叶子节点是AI,表示未结束
        {
            result.depth = -1;
        }
        else//叶子节点是player,表示提前结束,AI取胜,否则一定会是AI
        {
            result.depth = lastStep.step - startStep;
        }
        return result;
    }
    result = childs[0]->getBestAtackRating();
    if (lastStep.getColor() != playerColor)//节点是AI
    {
        //player落子
        for (size_t i = 1; i < childs.size(); ++i)
        {
            temp = childs[i]->getBestAtackRating();
            if (result.depth < 0)
            {
                break;
            }
            else if (temp.depth < 0)//depth =-1表示不成立
            {
                result = temp;
            }
            else if (playerColor == STATE_CHESS_BLACK)
            {
                if (temp.white.highestScore < result.white.highestScore)
                {
                    result = temp;
                }
                else if (temp.white.highestScore == result.white.highestScore)
                {
                    if (result.depth < temp.depth)
                    {
                        result = temp;
                    }
                }
            }
            else
            {
                if (temp.black.highestScore < result.black.highestScore)
                {
                    result = temp;
                }
                else if (temp.black.highestScore == result.black.highestScore)
                {
                    if (result.depth < temp.depth)
                    {
                        result = temp;
                    }
                }
            }

        }
    }
    else
    {
        //AI落子
        for (size_t i = 1; i < childs.size(); ++i)
        {
            temp = childs[i]->getBestAtackRating();
            if (temp.depth < 0)
            {
                continue;
            }
            else if (result.depth < 0)
            {
                result = temp;
            }
            else
            {
                if (playerColor == STATE_CHESS_BLACK)
                {
                    if (temp.white.highestScore > result.white.highestScore)
                    {
                        result = temp;
                    }
                    else if (temp.white.highestScore == result.white.highestScore)
                    {
                        if (result.depth > temp.depth)
                        {
                            result = temp;
                        }
                    }
                }
                else
                {
                    if (temp.black.highestScore > result.black.highestScore)
                    {
                        result = temp;
                    }
                    else if (temp.black.highestScore == result.black.highestScore)
                    {
                        if (result.depth > temp.depth)
                        {
                            result = temp;
                        }
                    }
                }
            }
        }
    }
    return result;
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
        ss << getDepth() << ":" << "{";
        for (size_t i = 0; i < childs.size(); ++i)
        {
            stringstream temps;
            temps << i << "-";
            childs[i]->printTree(ss, pre + temps.str());
        }
        ss << "}" << "*";
    }
}