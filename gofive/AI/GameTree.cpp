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
                    if (chessBoard->getPiece(i, j).getThreat(playerColor) >= SCORE_5_CONTINUE)//��player�����γɵ�����
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
        if (childsInfo[sortList[childs.size() - 1].key].hasSearch)//����������ĵ÷ֲ��������֮ǰ�ߣ����Ŀǰ�÷���ߵ��Ѿ������������ˣ��ٽ�������Ҳ�����ҵ��÷ֱ������ߵ���
        {
            //��ֹ������㷨����
            //���һ�����������������ڶ���δ���������ҵ÷ֵ������һ�����и���ѡȡ�������ڶ���
            int i = childs.size() - 1;
            while (i > 0 && sortList[i - 1].value == sortList[i].value)
            {
                i--;
                if (!childsInfo[sortList[i].key].hasSearch)//��ȵ÷�����û��������
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
                if (childsInfo[sortList[i].key].lastStepScore > 998 && p.getThreat(playerColor) < 8000)//������ĳ���
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

        //�ȴ��߳�
        pool.wait();
        for (size_t i = childs.size() - searchNum; i < childs.size(); i++)
        {
            if (!childsInfo[sortList[i].key].hasSearch)
            {
                childsInfo[sortList[i].key].hasSearch = true;//hasSearchֵ��buildSortListInfo��Ӱ��
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
    //�����
    int i = childs.size() - 1;
    while (i > 0 && sortList[i - 1].value == sortList[i].value) i--;
    return i + rand() % (childs.size() - i);
}

//���߳�

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
        if (childsInfo[sortList[childs.size() - 1].key].hasSearch)//����������ĵ÷ֲ��������֮ǰ�ߣ����Ŀǰ�÷���ߵ��Ѿ������������ˣ��ٽ�������Ҳ�����ҵ��÷ֱ������ߵ���
        {
            //��ֹ������㷨����
            //���һ�����������������ڶ���δ���������ҵ÷ֵ������һ�����и���ѡȡ�������ڶ���
            int i = childs.size() - 1;
            while (i > 0 && sortList[i - 1].value == sortList[i].value)
            {
                i--;
                if (!childsInfo[sortList[i].key].hasSearch)//��ȵ÷�����û��������
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

    //�����
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

    if (childs.size() == 1)//ֻ����һ��
    {
        if ((resultFlag == AIRESULTFLAG_NEARWIN || resultFlag == AIRESULTFLAG_TAUNT) &&
            getHighest(-playerColor) < SCORE_5_CONTINUE && getHighest(playerColor) >= SCORE_5_CONTINUE)//��������
        {
            resultFlag = AIRESULTFLAG_TAUNT;//����
        }
        else
        {
            resultFlag = AIRESULTFLAG_NORMAL;
        }

        result = Position{ childs[0]->lastStep.row, childs[0]->lastStep.col };
        goto endsearch;
    }

    for (size_t i = 0; i < childs.size(); ++i)//��ʼ����˳���ҳ��������
    {
        score = childs[i]->chessBoard->getLastStepScores(false);//����Ȩ��
        if (score >= SCORE_5_CONTINUE)
        {
            resultFlag = AIRESULTFLAG_WIN;
            result = Position{ childs[i]->lastStep.row, childs[i]->lastStep.col };
            goto endsearch;
        }
        else if (score < 0)
        {
            if (childs.size() == 1)//ֻ����һ��,ֻ���߽�����
            {
                resultFlag = AIRESULTFLAG_FAIL;
                result = Position{ childs[i]->lastStep.row, childs[i]->lastStep.col };
                goto endsearch;
            }
            delete childs[i];
            childs.erase(childs.begin() + i);//��֤���ֲ���
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
        bestRating = 100;//������
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

    //��ʼ�������
    bestSearchPos = multiThread ? searchBest2(pool) : searchBest();

    transpositionTable.clear();

    resultFlag = AIRESULTFLAG_NORMAL;
    //if (playerColor == STATE_CHESS_BLACK && lastStep.step < 10)//��ֹ���ֱ�����
    //{
    //    activeChildIndex = getDefendChild();
    //    result = Position{ childs[activeChildIndex]->lastStep.row, childs[activeChildIndex]->lastStep.col };
    //}
    //else 
    if (childsInfo[sortList[bestSearchPos].key].rating.highestScore >= SCORE_5_CONTINUE)
    {
        resultFlag = AIRESULTFLAG_FAIL;
        activeChildIndex = getDefendChild();//������������ҵ�����ȥ��
        if (chessBoard->getPiece(childs[activeChildIndex]->lastStep.row, childs[activeChildIndex]->lastStep.col).getThreat(lastStep.getColor()) > 2000)
            result = Position{ childs[activeChildIndex]->lastStep.row, childs[activeChildIndex]->lastStep.col };
        else
            result = Position{ childs[sortList[bestSearchPos].key]->lastStep.row, childs[sortList[bestSearchPos].key]->lastStep.col };
    }
    else if (lastStep.step > 10 && childsInfo[activeChildIndex].rating.highestScore < SCORE_3_DOUBLE)//��������������ᵼ������ʧ�ܣ���������������
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
        if (lastStep.getColor() == -playerColor)//Ҷ�ӽڵ���AI
        {
            return RatingInfo(getTotal(playerColor), getHighest(playerColor));//ȡ��player����
        }
        else//Ҷ�ӽڵ���player,��ʾ��ǰ����,AIȡʤ,����Ҷ�ӽڵ�һ������AI
        {
            return RatingInfo(getTotal(playerColor), getHighest(playerColor));//ȡ��player������getHighest�ض���-100000
        }
    }

    RatingInfo tempThreat;
    RatingInfo best = (lastStep.getColor() == playerColor) ? RatingInfo{ 500000, 500000 } : RatingInfo{ 0,0 };//��ʼ��best

    if (lastStep.getColor() == -playerColor)//AI�ڵ�
    {
        for (size_t i = 0; i < childs.size(); ++i)
        {
            tempThreat = childs[i]->getBestRating();//�ݹ�
            if (tempThreat.totalScore > best.totalScore)//bestԭ��:player�¹��Ľڵ�player�÷�Խ��Խ��(Ĭ��player�����ŵ�)
            {
                if (tempThreat.highestScore > best.highestScore / 2)
                {
                    best = tempThreat;
                }
            }
        }
    }
    else//player�ڵ�
    {
        for (size_t i = 0; i < childs.size(); ++i)
        {
            tempThreat = childs[i]->getBestRating();//child��AI�ڵ�
            if (tempThreat.totalScore < best.totalScore)//bestԭ��:AI�¹��Ľڵ�player�÷�ԽСԽ��
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
            temp -= SCORE_5_CONTINUE;//������������ĵ����ȼ�
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
    //lastStep.getColor()��AI�µ�
    if (getHighest(playerColor) >= SCORE_5_CONTINUE)
    {
        goto end;
    }

    if (getDepth() >= maxSearchDepth)//���������������֤���һ����AI�µģ��ʶ�=maxSearchDepthʱ��ֱ�ӽ���
    {
        goto end;
    }

    if (getHighest(-playerColor) >= SCORE_5_CONTINUE)//������
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
    else if (getHighest(playerColor) > 99 && getHighest(playerColor) < SCORE_4_DOUBLE)//����
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
                    if (ChessBoard::level >= AILEVEL_MASTER && score > 0 && score < 100)//������������γ�����
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
                        createChildNode(i, j);//AI�ż�1
                    }
                }
            }
        }
    }

end:
    delete chessBoard;
    chessBoard = 0;
    if (recursive)//��Ҫ�ݹ�
    {
        /*for (int i = 0; i < childs.size(); i++)
        {
            childs[i]->buildAI();
        }*/
        if (childs.size() > 0)//Ч���ܺ�
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
                    //    if (transpositionTable.find(childs[bestPos]->hash.z32key) != transpositionTable.end())//����
                    //    {
                    //        data = transpositionTable[childs[bestPos]->hash.z32key];
                    //        mut_transTable.unlock_shared();
                    //        if (data.checksum == childs[bestPos]->hash.z64key)//У��ɹ�
                    //        {
                    //            hash_hit++;
                    //            //����build�ˣ�ֱ�����ֳɵ�
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
                    //        else//��ͻ������
                    //        {
                    //            hash_clash++;
                    //        }
                    //    }
                    //    else//δ����
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
                        break;//alpha��֦
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
    //player�ڵ�
    int score;
    int highest = getHighest(-playerColor);
    //����
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
    else if (getHighest(-playerColor) >= SCORE_4_DOUBLE && getHighest(playerColor) < SCORE_5_CONTINUE)// û�м����γɵ�����������ȥ��ɱ
    {
        for (int i = 0; i < BOARD_ROW_MAX; ++i)
        {
            for (int j = 0; j < BOARD_COL_MAX; ++j)
            {
                if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                {
                    if (chessBoard->getPiece(i, j).getThreat(-playerColor) >= SCORE_4_DOUBLE && chessBoard->getPiece(i, j).getThreat(playerColor) < SCORE_4_DOUBLE)//��ֹ�ͺ����ظ�
                    {
                        createChildNode(i, j);
                    }
                }
            }
        }
    }

    //����
    if (getHighest(playerColor) >= SCORE_5_CONTINUE)//��playerd�ĳ���(�����γɵ�����)
    {
        for (int i = 0; i < BOARD_ROW_MAX; ++i)
        {
            for (int j = 0; j < BOARD_COL_MAX; ++j)
            {
                if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                {
                    if (chessBoard->getPiece(i, j).getThreat(playerColor) >= SCORE_5_CONTINUE)//��player�����γɵ�����
                    {
                        score = chessBoard->getPiece(i, j).getThreat(-playerColor);
                        if (score < 0)//��������
                        {
                            goto end;//�����֣���������
                        }
                        createChildNode(i, j);
                        goto end;//�ض£���һ�������ˣ��������һ����ֱ������
                    }
                }
            }
        }
    }
    else if (getHighest(playerColor) >= SCORE_4_DOUBLE)//��player�Ļ���(�����γɵ����ġ�����)
    {
        for (int i = 0; i < BOARD_ROW_MAX; ++i)
        {
            for (int j = 0; j < BOARD_COL_MAX; ++j)
            {
                if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                {
                    if (chessBoard->getPiece(i, j).getThreat(playerColor) >= SCORE_4_DOUBLE)//��player�Ļ����������γɵ�����
                    {
                        score = chessBoard->getPiece(i, j).getThreat(-playerColor);
                        if (score < 0)//��������
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
    if (recursive)//��Ҫ�ݹ�
    {
        for (size_t i = 0; i < childs.size(); i++)
        {
            //if (getDepth() < transTableMaxDepth)
            //{
            //    mut_transTable.lock_shared();
            //    if (transpositionTable.find(childs[i]->hash.z32key) != transpositionTable.end())//����
            //    {
            //        TransTableData data = transpositionTable[childs[i]->hash.z32key];
            //        mut_transTable.unlock_shared();
            //        if (data.checksum == childs[i]->hash.z64key)//У��ɹ�
            //        {
            //            hash_hit++;
            //            //����build�ˣ�ֱ�����ֳɵ�
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
            //        else//��ͻ������
            //        {
            //            hash_clash++;
            //        }
            //    }
            //    else//δ����
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
        if (transpositionTable.find(child->hash.z32key) != transpositionTable.end())//����
        {
            data = transpositionTable[child->hash.z32key];
            mut_transTable.unlock_shared();
            if (data.checksum == child->hash.z64key)//У��ɹ�
            {
                hash_hit++;
                //����build�ˣ�ֱ�����ֳɵ�
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
            else//��ͻ������
            {
                hash_clash++;
            }
        }
        else//δ����
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
    if (getHighest(-playerColor) >= SCORE_5_CONTINUE)//����5��������������
    {
        assert(0);//����������
    }
    //vector<int> index;
    bestIndex = -1;
    for (size_t i = 0; i < childs.size(); ++i)
    {
        //lastStepScore�ǽ���Ȩ��
        if (childsInfo[i].lastStepScore < 1000 && childsInfo[i].lastStepScore > 0 && getHighest(-playerColor) < SCORE_4_DOUBLE)//�������
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

        if (getHighest(-playerColor) >= SCORE_5_CONTINUE)//�ɹ�
        {
            goto end;
        }

        if (getHighest(playerColor) >= SCORE_5_CONTINUE)//������
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
        else if (getHighest(-playerColor) > 99 && getHighest(-playerColor) < SCORE_4_DOUBLE)//����
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
        //����
        if (getDepth() >= maxSearchDepth)//���������������֤���һ����AI�µģ��ʶ�=maxSearchDepthʱ��ֱ�ӽ���
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
        else if (getHighest(playerColor) >= SCORE_4_DOUBLE && getHighest(-playerColor) < SCORE_5_CONTINUE)// û�м����γɵ�����������ȥ����
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
        //����
        if (getHighest(-playerColor) >= SCORE_5_CONTINUE)//��playerd�ĳ���(�����γɵ�����)
        {
            for (int i = 0; i < BOARD_ROW_MAX; ++i)
            {
                for (int j = 0; j < BOARD_COL_MAX; ++j)
                {
                    if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                    {
                        if (chessBoard->getPiece(i, j).getThreat(-playerColor) >= SCORE_5_CONTINUE)//��player�����γɵ�����
                        {
                            int score = chessBoard->getPiece(i, j).getThreat(playerColor);
                            if (score < 0)//��������
                            {
                                goto end;//�����֣���������
                            }
                            createChildNode(i, j);
                            RatingInfo2 info = buildAtackChildWithTransTable(childs.back());
                            goto end;//�ض£���һ�������ˣ��������һ����ֱ������
                        }
                    }
                }
            }
        }
        else if (getHighest(-playerColor) >= SCORE_3_DOUBLE)//��player�Ļ���(�����γɵ����ġ����ġ�����)
        {
            ChessBoard tempBoard;
            GameTreeNode *tempNode;
            for (int i = 0; i < BOARD_ROW_MAX; ++i)
            {
                for (int j = 0; j < BOARD_COL_MAX; ++j)
                {
                    if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0 && chessBoard->getPiece(i, j).getThreat(playerColor) < SCORE_4_DOUBLE)//��ֹ��ǰ���ظ�
                    {
                        if (chessBoard->getPiece(i, j).getThreat(-playerColor) >= SCORE_3_DOUBLE)//��player�Ļ����������γɵ�����
                        {
                            int score = chessBoard->getPiece(i, j).getThreat(playerColor);
                            if (score < 0)//��������
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
                        //else if (chessBoard->getPiece(i, j).getThreat(-playerColor) >= 100)//��Ӷ�����������
                        //{
                        //    int score = chessBoard->getPiece(i, j).getThreat(playerColor);
                        //    if (score < 0)//��������
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
    //    //    if (transpositionTable.find(childs[i]->hash.z32key) != transpositionTable.end())//����
    //    //    {
    //    //        TransTableData data = transpositionTable[childs[i]->hash.z32key];
    //    //        mut_transTable.unlock_shared();
    //    //        if (data.checksum == childs[i]->hash.z64key)//У��ɹ�
    //    //        {
    //    //            hash_hit++;
    //    //            //����build�ˣ�ֱ�����ֳɵ�
    //    //            childs[i]->black = data.black;
    //    //            childs[i]->white = data.white;
    //    //            childs[i]->lastStep.step = data.steps;
    //    //            info = childs[i]->getBestAtackRating();
    //    //            goto endtans;
    //    //        }
    //    //        else//��ͻ������
    //    //        {
    //    //            hash_clash++;
    //    //        }
    //    //    }
    //    //    else//δ����
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
    ////        //�����һ�����ܾ�ɱ����ô�����ܾ�ɱ����Ϊ������player���ӣ�Ĭ��ѡ����
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
        if (transpositionTable.find(child->hash.z32key) != transpositionTable.end())//����
        {
            TransTableData data = transpositionTable[child->hash.z32key];
            mut_transTable.unlock_shared();
            if (data.checksum == child->hash.z64key)//У��ɹ�
            {
                hash_hit++;
                //����build�ˣ�ֱ�����ֳɵ�
                child->black = data.black;
                child->white = data.white;
                child->lastStep.step = data.steps;
                info = child->getBestAtackRating();
                return info;
            }
            else//��ͻ������
            {
                hash_clash++;
            }
        }
        else//δ����
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
        if (lastStep.getColor() != playerColor)//Ҷ�ӽڵ���AI,��ʾδ����
        {
            result.depth = -1;
        }
        else//Ҷ�ӽڵ���player,��ʾ��ǰ����,AIȡʤ,����һ������AI
        {
            result.depth = lastStep.step - startStep;
        }
        return result;
    }
    result = childs[0]->getBestAtackRating();
    if (lastStep.getColor() != playerColor)//�ڵ���AI
    {
        //player����
        for (size_t i = 1; i < childs.size(); ++i)
        {
            temp = childs[i]->getBestAtackRating();
            if (result.depth < 0)
            {
                break;
            }
            else if (temp.depth < 0)//depth =-1��ʾ������
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
        //AI����
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