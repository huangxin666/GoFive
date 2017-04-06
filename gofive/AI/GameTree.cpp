#include "GameTree.h"
#include "utils.h"
#include "ThreadPool.h"
#include <thread>

int8_t GameTreeNode::playerColor = 1;
bool GameTreeNode::multiThread = true;
uint8_t GameTreeNode::startStep = 0;
uint8_t GameTreeNode::maxSearchDepth = 0;
uint8_t GameTreeNode::transTableMaxDepth = 0;
size_t GameTreeNode::maxTaskNum = 0;
int GameTreeNode::bestRating = 0;
unordered_map<uint32_t, transTableData> GameTreeNode::transpositionTable;
shared_mutex GameTreeNode::mut_transTable;
uint64_t GameTreeNode::hash_hit = 0;
uint64_t GameTreeNode::hash_clash = 0;
uint64_t GameTreeNode::hash_miss = 0;

void GameTreeNode::debug(ChildInfo *threatInfo)
{
    stringstream ss;
    fstream of("debug.txt", ios::out);
    for (size_t n = 0; n < childs.size(); n++)
        ss << n << ":" << threatInfo[n].rating.highestScore << "|" << threatInfo[n].rating.totalScore << "\n";
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
    ChessBoard::setLevel(param.level);
    ChessBoard::setBan(param.ban);
    playerColor = playercolor;
    multiThread = param.multithread;
    maxSearchDepth = param.caculateSteps * 2;
    transTableMaxDepth = maxSearchDepth > 1 ? maxSearchDepth - 1 : 0;
    startStep = lastStep.step;
    hash = chessBoard->toHash();
    hash_hit = 0;
    hash_miss = 0;
    hash_clash = 0;
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

int GameTreeNode::searchBest2(ChildInfo *threatInfo, SortInfo *sortList)
{

    size_t searchNum = 10;
    ThreadPool pool;
    pool.start();
    sort(sortList, 0, childs.size() - 1);
    while (true)
    {
        if (threatInfo[sortList[childs.size() - 1].key].hasSearch)//深度搜索过的得分不会比搜索之前高，如果目前得分最高的已经是搜索过的了，再进行搜索也不会找到得分比它更高的了
        {
            //防止随机化算法导致
            //最后一个搜索过，但倒数第二个未搜索过并且得分等于最后一个，有概率选取到倒数第二个
            int i = childs.size() - 1;
            while (i > 0 && sortList[i - 1].value == sortList[i].value)
            {
                i--;
                if (!threatInfo[sortList[i].key].hasSearch)//相等得分中有没搜索过的
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
            if (!threatInfo[sortList[i].key].hasSearch)
            {
                Task t;
                t.node = childs[sortList[i].key];
                t.index = sortList[i].key;
                t.threatInfo = threatInfo;
                pool.run(t, false);
            }
        }

        //等待线程
        while (true)
        {
            if (!pool.is_still_working())
            {
                break;
            }
            this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        for (size_t i = childs.size() - searchNum; i < childs.size(); i++)
        {
            if (!threatInfo[sortList[i].key].hasSearch)
            {
                threatInfo[sortList[i].key].hasSearch = true;//hasSearch值对buildSortListInfo有影响
                //threatInfo[sortList[i].key] = childs[sortList[i].key]->getBestThreat();
                //childs[sortList[i].key]->deleteChild();
                buildSortListInfo(i, threatInfo, sortList);
            }
        }
        sort(sortList, 0, childs.size() - 1);
        searchNum += 10;
    }
    pool.stop();
    //随机化
    int i = childs.size() - 1;
    while (i > 0 && sortList[i - 1].value == sortList[i].value) i--;
    return i + rand() % (childs.size() - i);
}

//多线程

void GameTreeNode::buildTreeThreadFunc(int n, ChildInfo *threatInfo, GameTreeNode *child)
{
    child->buildPlayer();
    threatInfo[n].rating = child->getBestRating();
    delete child;
}

int GameTreeNode::searchBest(ChildInfo *threatInfo, SortInfo *sortList)
{
    size_t searchNum = 10;
    thread buildTreeThread[MAXTHREAD];
    sort(sortList, 0, childs.size() - 1);
    while (true)
    {
        if (threatInfo[sortList[childs.size() - 1].key].hasSearch)//深度搜索过的得分不会比搜索之前高，如果目前得分最高的已经是搜索过的了，再进行搜索也不会找到得分比它更高的了
        {
            //防止随机化算法导致
            //最后一个搜索过，但倒数第二个未搜索过并且得分等于最后一个，有概率选取到倒数第二个
            int i = childs.size() - 1;
            while (i > 0 && sortList[i - 1].value == sortList[i].value)
            {
                i--;
                if (!threatInfo[sortList[i].key].hasSearch)//相等得分中有没搜索过的
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
            if (!threatInfo[sortList[i].key].hasSearch)
            {
                GameTreeNode *node = new GameTreeNode;
                *node = *childs[sortList[i].key];
                buildTreeThread[j] = thread(buildTreeThreadFunc, sortList[i].key, threatInfo, node);
                j++;
                threatInfo[sortList[i].key].hasSearch = true;
            }
        }

        for (int i = 0; i < j; i++)
        {
            buildTreeThread[i].join();
        }

        for (size_t i = childs.size() - searchNum; i < childs.size(); i++)
        {
            buildSortListInfo(i, threatInfo, sortList);
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
    bestRating = INT32_MIN;
    Position result;
    int bestSearchPos;
    buildFirstChilds();
    SortInfo *sortList = new SortInfo[childs.size()];
    ChildInfo *childsInfo = new ChildInfo[childs.size()];
    int score;

    for (size_t i = 0; i < childs.size(); ++i)//初始化，顺便找出特殊情况
    {
        score = childs[i]->chessBoard->getLastStepScores(false);
        if (score >= SCORE_5_CONTINUE)
        {
            result = Position{ childs[i]->lastStep.row, childs[i]->lastStep.col };
            goto endsearch;
        }
        else if (score < 0)
        {
            if (childs.size() == 1)//只有这一个,只能走禁手了
            {
                result = Position{ childs[i]->lastStep.row, childs[i]->lastStep.col };
                goto endsearch;
            }
            childs.erase(childs.begin() + i);//保证禁手不走
            i--;
            continue;
        }
        childsInfo[i].hasSearch = false;
        childsInfo[i].lastStepScore = score;
        sortList[i].key = i;
        buildSortListInfo(i, childsInfo, sortList);
    }


    int specialAtackStep = getSpecialAtack(childsInfo);//此时还未排序，sortList[specialAtackStep].key = specialAtackStep
    if (specialAtackStep >= 0)
    {
        childs[specialAtackStep]->buildPlayer();
        childsInfo[specialAtackStep].hasSearch = true;
        childsInfo[specialAtackStep].rating = childs[specialAtackStep]->getBestRating();
        buildSortListInfo(specialAtackStep, childsInfo, sortList);
        childs[specialAtackStep]->deleteChilds();
    }

    int atackChildIndex = getAtackChild(childsInfo);
    if (!childsInfo[atackChildIndex].hasSearch)
    {
        childs[atackChildIndex]->buildPlayer();
        childsInfo[atackChildIndex].hasSearch = true;
        childsInfo[atackChildIndex].rating = childs[atackChildIndex]->getBestRating();
        buildSortListInfo(atackChildIndex, childsInfo, sortList);
        childs[atackChildIndex]->deleteChilds();
    }

    //开始深度搜索
    bestSearchPos = multiThread ? searchBest2(childsInfo, sortList) : searchBest(childsInfo, sortList);

    //childs[39]->buildPlayer();
    //hasSearch[39] = true;
    //threatInfo[39] = childs[39]->getBestThreat();
    //childs[39]->printTree();
    //childs[39]->deleteChild();

    if (childsInfo[atackChildIndex].lastStepScore >= 10000 &&
        childs[atackChildIndex]->getHighest(lastStep.getColor()) < SCORE_5_CONTINUE &&
        childsInfo[atackChildIndex].rating.highestScore < SCORE_5_CONTINUE)
    {
        result = Position{ childs[atackChildIndex]->lastStep.row, childs[atackChildIndex]->lastStep.col };
    }
    else if (playerColor == STATE_CHESS_BLACK && lastStep.step < 20)//防止开局被布阵
    {
        atackChildIndex = getDefendChild();
        result = Position{ childs[atackChildIndex]->lastStep.row, childs[atackChildIndex]->lastStep.col };
    }
    else if (childsInfo[sortList[bestSearchPos].key].rating.highestScore > 80000 ||
        (childsInfo[sortList[bestSearchPos].key].rating.highestScore >= 10000 &&
        (childsInfo[sortList[bestSearchPos].key].lastStepScore <= 1200 ||
            (childsInfo[sortList[bestSearchPos].key].lastStepScore >= 8000 &&
                childsInfo[sortList[bestSearchPos].key].lastStepScore < 10000))))
    {
        if (specialAtackStep > -1 && childsInfo[specialAtackStep].lastStepScore > 1200 &&
            childs[specialAtackStep]->getHighest(lastStep.getColor()) < SCORE_5_CONTINUE)
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
    else if (specialAtackStep > -1 && childsInfo[specialAtackStep].rating.highestScore < SCORE_3_DOUBLE)//add at 17.3.23 防止走向失败
    {
        result = Position{ childs[specialAtackStep]->lastStep.row, childs[specialAtackStep]->lastStep.col };
    }
    else if (childsInfo[atackChildIndex].lastStepScore > 1000 && childsInfo[atackChildIndex].rating.highestScore <= SCORE_3_DOUBLE)
    {
        result = Position{ childs[atackChildIndex]->lastStep.row, childs[atackChildIndex]->lastStep.col };
    }
    else
    {
        result = Position{ childs[sortList[bestSearchPos].key]->lastStep.row, childs[sortList[bestSearchPos].key]->lastStep.col };
    }
endsearch:
    delete[] sortList;
    delete[] childsInfo;
    transpositionTable.clear();
    return result;
}

RatingInfo GameTreeNode::getBestRating()
{
    if (childs.size() == 0)
    {
        if (lastStep.getColor() != playerColor)//叶子节点是AI
        {
            return RatingInfo(getTotal(playerColor), getHighest(playerColor));//取得player分数
        }
        else//叶子节点是player,表示提前结束,AI取胜,否则一定会是AI
        {
            return RatingInfo(getTotal(playerColor), getHighest(playerColor));//取得player分数
        }
    }

    RatingInfo tempThreat;
    RatingInfo best = (lastStep.getColor() == playerColor) ? RatingInfo{ 500000, 500000 } : RatingInfo{ 0,0 };//初始化best

    if (lastStep.getColor() != playerColor)//AI节点
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

void GameTreeNode::buildSortListInfo(int n, ChildInfo *childsInfo, SortInfo *sortList)
{
    int i = sortList[n].key;
    if (!childsInfo[i].hasSearch)
    {
        sortList[n].value = childsInfo[i].lastStepScore - childs[i]->getHighest(lastStep.getColor()) - childs[i]->getTotal(lastStep.getColor()) / 10;
        if (sortList[n].value <= -SCORE_5_CONTINUE)//增加堵冲四的优先级
        {
            sortList[n].value -= SCORE_5_CONTINUE;
        }
    }
    else
    {
        ChildInfo temp = childsInfo[i];
        int tempScore = childsInfo[i].lastStepScore - temp.rating.highestScore - temp.rating.totalScore / 10;
        if (tempScore < sortList[n].value)
        {
            //sortList[n].value = childs[i]->currentScore - temp.totalScore;
            sortList[n].value = tempScore;
        }
    }
}

int GameTreeNode::getSpecialAtack(ChildInfo *childsInfo)
{
    if (getHighest(lastStep.getColor()) >= SCORE_5_CONTINUE)
        return -1;
    int max = 3000, flag = -1, temp;
    for (size_t i = 0; i < childs.size(); ++i)
    {
        if (childsInfo[i].lastStepScore >= 1210 && childsInfo[i].lastStepScore < 2000 /*||
                                                                                (getHighest(side) < 10000 && childs[i]->currentScore < 1210 && childs[i]->currentScore>1000)*/)
        {
            ChessBoard tempboard = *childs[i]->chessBoard;
            tempboard.setGlobalThreat(false);//进攻权重
            temp = tempboard.getAtackScore(childsInfo[i].lastStepScore, getTotal(lastStep.getColor()));
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
                    childsInfo[i].lastStepScore = 123; //modify at 17.3.23
                }
            }//可能出现权重BUG(已经出现，3.23修复)
        }
    }
    return flag;
}

int GameTreeNode::getAtackChild(ChildInfo *childsInfo)
{
    int max = INT_MIN, flag = 0, temp;
    for (size_t i = 0; i < childs.size(); ++i)
    {
        temp = chessBoard->getPiece(childs[i]->lastStep.row, childs[i]->lastStep.col).getThreat(lastStep.getColor()) / 50;
        if (childsInfo[i].lastStepScore + temp > max)
        {
            max = childsInfo[i].lastStepScore + temp;
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

void GameTreeNode::buildPlayer(bool recursive)
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

    if (getHighest(-playerColor) >= SCORE_5_CONTINUE)
    {
        //int score;
        for (int i = 0; i < BOARD_ROW_MAX; ++i)
        {
            for (int j = 0; j < BOARD_COL_MAX; ++j)
            {
                if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                {
                    if (chessBoard->getPiece(i, j).getThreat(-playerColor) >= SCORE_5_CONTINUE)
                    {
                        //score = chessBoard->getPiece(i, j).getThreat(playerColor);
                        createChildNode(i, j);//AI才减1
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
        RatingInfo tempInfo = { 0,0 };
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
                        highTemp = tempBoard.getRatingInfo(-playerColor).highestScore;//AI

                        if (highTemp >= SCORE_5_CONTINUE) continue;
                        else if (highTemp >= 8000 && (score > 1100 || score < 900)) continue;

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
                    if (getDepth() < transTableMaxDepth)
                    {
                        mut_transTable.lock_shared();
                        if (transpositionTable.find(childs[bestPos]->hash.z32key) != transpositionTable.end())//命中
                        {
                            transTableData data = transpositionTable[childs[bestPos]->hash.z32key];
                            mut_transTable.unlock_shared();
                            if (data.checksum == childs[bestPos]->hash.z64key)//校验成功
                            {
                                hash_hit++;
                                //不用build了，直接用现成的
                                if (playerColor == STATE_CHESS_BLACK)
                                {
                                    childs[bestPos]->black = data.black;
                                }
                                else
                                {
                                    childs[bestPos]->white = data.white;
                                }
                                goto complete;
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
                        transTableData data;
                        data.checksum = childs[bestPos]->hash.z64key;
                        childs[bestPos]->buildAI();
                        if (playerColor == STATE_CHESS_BLACK)
                        {
                            data.black = childs[bestPos]->getBestRating();
                        }
                        else
                        {
                            data.white = childs[bestPos]->getBestRating();
                        }
                        mut_transTable.lock();
                        transpositionTable[childs[bestPos]->hash.z32key] = data;
                        mut_transTable.unlock();
                    }
                    else
                    {
                        childs[bestPos]->buildAI();
                    }

                complete:
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

void GameTreeNode::buildAI(bool recursive)
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
    else if (getHighest(-playerColor) >= SCORE_4_DOUBLE && getHighest(playerColor) < SCORE_5_CONTINUE)// playerColor 没有即将形成的五连，可以去绝杀
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
    if (recursive)//需要递归
    {
        for (size_t i = 0; i < childs.size(); i++)
        {
            if (getDepth() < transTableMaxDepth)
            {
                mut_transTable.lock_shared();
                if (transpositionTable.find(childs[i]->hash.z32key) != transpositionTable.end())//命中
                {
                    transTableData data = transpositionTable[childs[i]->hash.z32key];
                    mut_transTable.unlock_shared();
                    if (data.checksum == childs[i]->hash.z64key)//校验成功
                    {
                        hash_hit++;
                        //不用build了，直接用现成的
                        if (playerColor == STATE_CHESS_BLACK)
                        {
                            childs[i]->black = data.black;
                        }
                        else
                        {
                            childs[i]->white = data.white;
                        }
                        continue;
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
                transTableData data;
                data.checksum = childs[i]->hash.z64key;
                childs[i]->buildPlayer();
                if (playerColor == STATE_CHESS_BLACK)
                {
                    data.black = childs[i]->getBestRating();
                }
                else
                {
                    data.white = childs[i]->getBestRating();
                }
                mut_transTable.lock();
                transpositionTable[childs[i]->hash.z32key] = data;
                mut_transTable.unlock();
            }
            else
            {
                childs[i]->buildPlayer();
            }
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