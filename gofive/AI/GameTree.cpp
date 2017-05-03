#include "GameTree.h"
#include "utils.h"
#include "ThreadPool.h"
#include <thread>
#include <assert.h>

#define LONGTAILMODE_MAX_DEPTH 6
SortInfo *GameTreeNode::sortList = NULL;
ChildInfo *GameTreeNode::childsInfo = NULL;

int8_t GameTreeNode::playerColor = 1;
bool GameTreeNode::enableAtack = true;
int GameTreeNode::resultFlag = AIRESULTFLAG_NORMAL;
uint8_t GameTreeNode::startStep = 0;
uint8_t GameTreeNode::maxSearchDepth = 0;
uint8_t GameTreeNode::transTableMaxDepth = 0;
size_t GameTreeNode::maxTaskNum = 0;
int GameTreeNode::bestRating = 0;
int GameTreeNode::bestIndex = -1;
trans_table GameTreeNode::transpositionTable(0);
uint64_t GameTreeNode::hash_hit = 0;
uint64_t GameTreeNode::hash_clash = 0;
uint64_t GameTreeNode::hash_miss = 0;
bool GameTreeNode::longtailmode = false;
atomic<int> GameTreeNode::longtail_threadcount = 0;

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
    enableAtack = param.multithread;
    maxSearchDepth = param.caculateSteps * 2;
    transTableMaxDepth = maxSearchDepth > 1 ? maxSearchDepth - 1 : 0;
    startStep = lastStep.step;
    hash_hit = 0;
    hash_miss = 0;
    hash_clash = 0;
    hash = chessBoard->toHash();
    if (transpositionTable.size() < maxSearchDepth)
    {
        for (auto t : transpositionTable)
        {
            if (t)
            {
                delete t;
            }
        }
        transpositionTable.resize(maxSearchDepth);
        for (size_t i = 0; i < transpositionTable.size(); i++)
        {
            transpositionTable[i] = new SafeMap;
        }
    }
}

void GameTreeNode::clearTransTable()
{
    for (auto t : transpositionTable)
    {
        if (t)
        {
            t->m.clear();
        }
    }
}

void GameTreeNode::popHeadTransTable()
{
    if (transpositionTable[0])
    {
        delete transpositionTable[0];
    }
    transpositionTable.erase(transpositionTable.begin());
    transpositionTable.push_back(new SafeMap);
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

void GameTreeNode::createChildNode(int row, int col)
{
    ChessBoard tempBoard = *chessBoard;
    tempBoard.doNextStep(row, col, -lastStep.getColor());
    tempBoard.updateThreat();
    GameTreeNode *tempNode = new GameTreeNode(&tempBoard);
    tempNode->hash = hash;
    tempBoard.updateHashPair(tempNode->hash, row, col, -lastStep.getColor());
    tempNode->alpha = alpha;
    tempNode->beta = beta;
    childs.push_back(tempNode);
}

void GameTreeNode::buildAllChilds()
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

int GameTreeNode::searchByMultiThread(ThreadPool &pool)
{
    GameTreeNode::longtailmode = false;
    GameTreeNode::longtail_threadcount = 0;
    sort(sortList, 0, childs.size() - 1);

    for (size_t i = 0; i < childs.size(); i++)
    {
        if (!childsInfo[sortList[i].key].hasSearch)
        {
            Piece p = chessBoard->getPiece(childs[sortList[i].key]->lastStep.row, childs[sortList[i].key]->lastStep.col);
            if (childsInfo[sortList[i].key].lastStepScore > 1000 && p.getThreat(playerColor) < 100)//������ĳ���
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
    for (size_t i = 0; i < childs.size(); i++)
    {
        if (!childsInfo[sortList[i].key].hasSearch)
        {
            childsInfo[sortList[i].key].hasSearch = true;//hasSearchֵ��buildSortListInfo��Ӱ��
            sortList[i].value = -childsInfo[sortList[i].key].rating.totalScore;
        }
    }
    sort(sortList, 0, childs.size() - 1);

    //�����
    int i = childs.size() - 1;
    while (i > 0 && (sortList[i - 1].value == sortList[i].value)) i--;
    return i + rand() % (childs.size() - i);
}

Position GameTreeNode::getBestStep()
{
    ThreadPool pool;
    Position result;
    int bestDefendPos;
    buildAllChilds();
    sortList = new SortInfo[childs.size()];
    childsInfo = new ChildInfo[childs.size()];
    int score;
    RatingInfoDenfend tempinfo;

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
        sortList[i].value = childsInfo[i].lastStepScore - childs[i]->getTotal(playerColor);
    }

    pool.start();

    if (ChessBoard::level >= AILEVEL_MASTER && enableAtack)
    {
        GameTreeNode::bestRating = 100;//������
        int atackSearchTreeResult = buildAtackSearchTree(pool);
        if (atackSearchTreeResult > -1)
        {
            resultFlag = AIRESULTFLAG_NEARWIN;
            result = Position{ childs[atackSearchTreeResult]->lastStep.row, childs[atackSearchTreeResult]->lastStep.col };
            popHeadTransTable();
            popHeadTransTable();
            goto endsearch;
        }
    }
    //transpositionTable.clear();
    clearTransTable();

    GameTreeNode::bestRating = INT32_MIN;
    int activeChildIndex = getActiveChild();
    //���ⱻ��֦���ȵ�����
    childs[activeChildIndex]->alpha = GameTreeNode::bestRating;
    childs[activeChildIndex]->beta = INT32_MAX;
    childs[activeChildIndex]->buildDefendTreeNode(childsInfo[activeChildIndex].lastStepScore);
    childsInfo[activeChildIndex].hasSearch = true;
    tempinfo = childs[activeChildIndex]->getBestDefendRating(childsInfo[activeChildIndex].lastStepScore);
    childsInfo[activeChildIndex].rating = tempinfo.info;
    childsInfo[activeChildIndex].depth = tempinfo.lastStep.step - GameTreeNode::startStep;
    sortList[activeChildIndex].value =  -childsInfo[activeChildIndex].rating.totalScore;
    childs[activeChildIndex]->deleteChilds();

    GameTreeNode::bestRating = -childsInfo[activeChildIndex].rating.totalScore;



    //int humancontrol = 41;
    //childs[humancontrol]->buildPlayer();
    //childsInfo[humancontrol].hasSearch = true;
    //childsInfo[humancontrol].rating = childs[humancontrol]->getBestRating();
    //buildSortListInfo(humancontrol, childsInfo, sortList);
    //childs[humancontrol]->printTree();
    //childs[humancontrol]->deleteChilds();

    //��ʼ�������
    bestDefendPos = searchByMultiThread(pool);

    //transpositionTable.clear();
    clearTransTable();

    resultFlag = AIRESULTFLAG_NORMAL;
    if (playerColor == STATE_CHESS_BLACK && lastStep.step < 20)//��ֹ���ֱ�����
    {
        activeChildIndex = getDefendChild();
        result = Position{ childs[activeChildIndex]->lastStep.row, childs[activeChildIndex]->lastStep.col };
    }
    else
        if (childsInfo[sortList[bestDefendPos].key].rating.highestScore >= SCORE_5_CONTINUE)
        {
            resultFlag = AIRESULTFLAG_FAIL;
            //activeChildIndex = getDefendChild();//������������ҵ�����ȥ��
            //result = Position{ childs[activeChildIndex]->lastStep.row, childs[activeChildIndex]->lastStep.col };
            result = Position{ childs[sortList[bestDefendPos].key]->lastStep.row, childs[sortList[bestDefendPos].key]->lastStep.col };
        }
        else
            if (lastStep.step > 10 && childsInfo[activeChildIndex].rating.highestScore < SCORE_3_DOUBLE)
            {
                //��������������ᵼ������ʧ�ܣ���������������������10�����Ȳ�����
                result = Position{ childs[activeChildIndex]->lastStep.row, childs[activeChildIndex]->lastStep.col };
            }
            else
            {
                result = Position{ childs[sortList[bestDefendPos].key]->lastStep.row, childs[sortList[bestDefendPos].key]->lastStep.col };
            }
endsearch:
    delete[] sortList;
    sortList = NULL;
    delete[] childsInfo;
    childsInfo = NULL;
    pool.stop();
    return result;
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

RatingInfoDenfend GameTreeNode::getBestDefendRating(int basescore)
{
    RatingInfoDenfend result;
    if (childs.size() == 0)
    {
        //if (lastStep.getColor() == -playerColor)//Ҷ�ӽڵ���AI
        //{
        //    return RatingInfo(getTotal(playerColor), getHighest(playerColor));//ȡ��player����
        //}
        //else//Ҷ�ӽڵ���player,��ʾ��ǰ����,AIȡʤ,����Ҷ�ӽڵ�һ������AI
        //{
        //    return RatingInfo(getTotal(playerColor), getHighest(playerColor));//ȡ��player������getHighest�ض���-100000
        //}
        result.lastStep = lastStep;
        result.info.highestScore = getHighest(playerColor);
        if (getTotal(playerColor) >= SCORE_5_CONTINUE)
        {
            result.info.totalScore = SCORE_5_CONTINUE + 100 - (lastStep.step - GameTreeNode::startStep);
        }
        else
        {
            result.info.totalScore = getTotal(playerColor) - basescore;
        }
    }
    else
    {
        RatingInfoDenfend tempThreat;
        result = childs[0]->getBestDefendRating(basescore);

        if (lastStep.getColor() == -playerColor)//AI�ڵ�
        {
            for (size_t i = 1; i < childs.size(); ++i)
            {
                tempThreat = childs[i]->getBestDefendRating(basescore);//�ݹ�
                if (tempThreat.info.totalScore > result.info.totalScore)//bestԭ��:player�¹��Ľڵ�player�÷�Խ��Խ��(Ĭ��player�����ŵ�)
                {
                    result = tempThreat;
                }
            }
        }
        else//player�ڵ�
        {
            for (size_t i = 1; i < childs.size(); ++i)
            {
                tempThreat = childs[i]->getBestDefendRating(basescore);//child��AI�ڵ�
                /*if (tempThreat.info.totalScore >= SCORE_5_CONTINUE && result.info.totalScore >= SCORE_5_CONTINUE)
                {
                    if (tempThreat.lastStep.step > result.lastStep.step)
                    {
                        result = tempThreat;
                    }
                }
                else */
                if (tempThreat.info.totalScore < result.info.totalScore)//bestԭ��:AI�¹��Ľڵ�player�÷�ԽСԽ��
                {
                    result = tempThreat;
                }
            }
        }
    }

    //result.moveList.push_back(lastStep);
    return result;
}

void GameTreeNode::buildDefendTreeNode(int basescore)
{
    RatingInfo info;
    int score;
    if (lastStep.getColor() == -playerColor)//build player
    {
        if (getDepth() >= maxSearchDepth)//���������������֤���һ����AI�µģ��ʶ�=maxSearchDepthʱ��ֱ�ӽ���
        {
            goto end;
        }
        if (getHighest(playerColor) >= SCORE_5_CONTINUE)
        {
            goto end;
        }
        else if (getHighest(-playerColor) >= SCORE_5_CONTINUE)//������
        {
            for (int i = 0; i < BOARD_ROW_MAX; ++i)
            {
                for (int j = 0; j < BOARD_COL_MAX; ++j)
                {
                    if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                    {
                        if (chessBoard->getPiece(i, j).getThreat(-playerColor) >= SCORE_5_CONTINUE)
                        {
                            score = chessBoard->getPiece(i, j).getThreat(playerColor);
                            if (score < 0)//player GG AI win
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
                            createChildNode(i, j);
                            if (buildDefendChildsAndPrune(basescore))
                            {
                                goto end;
                            }

                        }
                    }
                }
            }
        }
        else if (getHighest(-playerColor) >= SCORE_4_DOUBLE)//�����ġ����ģ����ǿɱ����Ľ����滻
        {
            for (int i = 0; i < BOARD_ROW_MAX; ++i)
            {
                for (int j = 0; j < BOARD_COL_MAX; ++j)
                {
                    if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                    {
                        if (chessBoard->getPiece(i, j).getThreat(-playerColor) >= SCORE_4_DOUBLE)
                        {
                            score = chessBoard->getPiece(i, j).getThreat(playerColor);
                            if (score < 0)//player GG AI win
                            {
                                continue;
                            }
                            createChildNode(i, j);
                            if (buildDefendChildsAndPrune(basescore))
                            {
                                goto end;
                            }

                        }
                    }
                }
            }
        }
        //����
        if (getHighest(playerColor) >= SCORE_4_DOUBLE && getHighest(-playerColor) < SCORE_5_CONTINUE)
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
                            createChildNode(i, j);
                            if (buildDefendChildsAndPrune(basescore))
                            {
                                goto end;
                            }
                        }
                    }
                }
            }
        }
        else if (getHighest(playerColor) > 99 && getHighest(-playerColor) < SCORE_5_CONTINUE)//����
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
                        score = chessBoard->getPiece(i, j).getThreat(playerColor);//player
                        if (score > 99)
                        {
                            createChildNode(i, j);
                            if (buildDefendChildsAndPrune(basescore))
                            {
                                goto end;
                            }
                        }
                        else if (ChessBoard::level >= AILEVEL_MASTER && score > 0 && score < 100 && getHighest(-playerColor) < SCORE_4_DOUBLE)//������������γ�����
                        {
                            tempBoard = *chessBoard;
                            tempBoard.doNextStep(i, j, playerColor);
                            tempBoard.updateThreat();
                            if (tempBoard.getRatingInfo(playerColor).highestScore >= SCORE_4_DOUBLE)
                            {
                                tempNode = new GameTreeNode(&tempBoard);
                                tempNode->hash = hash;
                                tempBoard.updateHashPair(tempNode->hash, i, j, -lastStep.getColor());
                                tempNode->alpha = alpha;
                                tempNode->beta = beta;
                                childs.push_back(tempNode);
                                if (buildDefendChildsAndPrune(basescore))
                                {
                                    goto end;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else //build AI
    {
        //player�ڵ�
        int score;
        //����
        if (getHighest(-playerColor) >= SCORE_5_CONTINUE)//player GG AI win 
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
                        if (chessBoard->getPiece(i, j).getThreat(-playerColor) >= SCORE_4_DOUBLE)
                        {
                            createChildNode(i, j);
                            if (buildDefendChildsAndPrune(basescore))
                            {
                                goto end;
                            }
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
                            if (score < 0)//�������� AI gg
                            {
                                goto end;//�����֣���������
                            }
                            createChildNode(i, j);
                            if (buildDefendChildsAndPrune(basescore))
                            {
                                goto end;
                            }
                            goto end;//�ض£���һ�������ˣ��������һ����ֱ������
                        }
                    }
                }
            }
        }
        else if (getHighest(playerColor) >= SCORE_3_DOUBLE)//��player�Ļ���(�����γɵ����ġ�����)
        {
            for (int i = 0; i < BOARD_ROW_MAX; ++i)
            {
                for (int j = 0; j < BOARD_COL_MAX; ++j)
                {
                    if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                    {
                        if (chessBoard->getPiece(i, j).getThreat(playerColor) >= SCORE_3_DOUBLE)//��player�Ļ����������γɵ�����
                        {
                            score = chessBoard->getPiece(i, j).getThreat(-playerColor);
                            if (score < 0)//��������
                            {
                                continue;
                            }
                            createChildNode(i, j);
                            if (buildDefendChildsAndPrune(basescore))
                            {
                                goto end;
                            }
                        }
                    }
                }
            }
        }
    }
end:
    delete chessBoard;
    chessBoard = 0;
}

RatingInfo GameTreeNode::buildDefendChildWithTransTable(GameTreeNode* child, int basescore)
{
    TransTableNodeData data;
    RatingInfoDenfend info;
    int depth = getDepth();
    if (depth < transTableMaxDepth)
    {
        transpositionTable[depth]->lock.lock_shared();
        if (transpositionTable[depth]->m.find(child->hash.z32key) != transpositionTable[depth]->m.end())//����
        {
            data = transpositionTable[depth]->m[child->hash.z32key];
            transpositionTable[depth]->lock.unlock_shared();
            if (data.checksum == child->hash.z64key)//У��ɹ�
            {
                hash_hit++;
                child->lastStep = data.lastStep;
                //����build�ˣ�ֱ�����ֳɵ�
                if (playerColor == STATE_CHESS_BLACK)
                {
                    child->black = data.black;
                    return data.black;
                }
                else
                {
                    child->white = data.white;
                    return data.white;
                }
            }
            else//��ͻ������
            {
                hash_clash++;
            }
        }
        else//δ����
        {
            transpositionTable[depth]->lock.unlock_shared();
            hash_miss++;
        }
    }

    child->buildDefendTreeNode(basescore);
    info = child->getBestDefendRating(basescore);
    if (playerColor == STATE_CHESS_BLACK)
    {
        data.black = info.info;
    }
    else
    {
        data.white = info.info;
    }

    if (depth < transTableMaxDepth)//�������û���
    {
        data.checksum = child->hash.z64key;
        data.lastStep = info.lastStep;
        transpositionTable[depth]->lock.lock();
        transpositionTable[depth]->m[child->hash.z32key] = data;
        transpositionTable[depth]->lock.unlock();
    }

    if (playerColor == STATE_CHESS_BLACK)
    {
        child->black = info.info;
    }
    else
    {
        child->white = info.info;
    }
    child->lastStep = info.lastStep;
    child->deleteChilds();
    return info.info;
}


bool GameTreeNode::buildDefendChildsAndPrune(int basescore)
{
    RatingInfo info = buildDefendChildWithTransTable(childs.back(), basescore);
    if (lastStep.getColor() == -playerColor)//build player
    {
        if (-info.totalScore < alpha || -info.totalScore < GameTreeNode::bestRating)//alpha��֦
        {
            return true;
        }
        //����betaֵ
        if (-info.totalScore < beta)
        {
            beta = -info.totalScore;
        }
    }
    else//build AI
    {
        if (-info.totalScore > beta)//beta��֦
        {
            return true;
        }
        //����alphaֵ
        if (-info.totalScore > alpha)
        {
            alpha = -info.totalScore;
        }
    }
    return false;
}


int GameTreeNode::buildAtackSearchTree(ThreadPool &pool)
{
    GameTreeNode::longtailmode = false;
    if (getHighest(-playerColor) >= SCORE_5_CONTINUE)//����5��������������
    {
        assert(0);//����������
    }
    //vector<int> index;
    bestIndex = -1;
    for (size_t i = 0; i < childs.size(); ++i)
    {
        //lastStepScore�ǽ���Ȩ��
        if (childsInfo[i].lastStepScore > 1000 && childs[i]->getHighest(playerColor) < SCORE_5_CONTINUE)
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
    for (size_t i = 0; i < childs.size(); ++i)
    {
        //lastStepScore�ǽ���Ȩ��
        if (childsInfo[i].lastStepScore <= 1000 && childsInfo[i].lastStepScore > 0 && getHighest(-playerColor) < SCORE_4_DOUBLE)//�������
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
                            if (buildAtackChildsAndPrune())
                            {
                                goto end;
                            }

                        }
                    }
                }
            }
        }
        else if (getHighest(-playerColor) > 99 && getHighest(-playerColor) < SCORE_4_DOUBLE)//����
        {
            int score;
            RatingInfo tempInfo = { 0,0 };
            for (int i = 0; i < BOARD_ROW_MAX; ++i)
            {
                for (int j = 0; j < BOARD_COL_MAX; ++j)
                {
                    if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                    {
                        score = chessBoard->getPiece(i, j).getThreat(-playerColor);
                        if (score > 0 && score < 100)
                        {
                            ChessBoard tempBoard = *chessBoard;
                            tempBoard.doNextStep(i, j, -playerColor);
                            tempBoard.updateThreat();
                            if (tempBoard.getRatingInfo(-playerColor).highestScore >= SCORE_4_DOUBLE)
                            {
                                GameTreeNode *tempNode = new GameTreeNode(&tempBoard);
                                tempNode->hash = hash;
                                tempBoard.updateHashPair(tempNode->hash, i, j, -lastStep.getColor());
                                tempNode->alpha = alpha;
                                tempNode->beta = beta;
                                childs.push_back(tempNode);
                                if (buildAtackChildsAndPrune())
                                {
                                    goto end;
                                }

                            }
                        }
                        else if (score > 99 && score < 10000)
                        {
                            createChildNode(i, j);
                            if (buildAtackChildsAndPrune())
                            {
                                goto end;
                            }
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
                            if (buildAtackChildsAndPrune())
                            {
                                goto end;
                            }
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
        if (getDepth() > alpha || getDepth() >= GameTreeNode::bestRating)
        {
            goto end;
        }
        if (getHighest(playerColor) >= SCORE_5_CONTINUE)
        {
            goto end;
        }
        else if (/*getHighest(playerColor) >= SCORE_4_DOUBLE &&*/ getHighest(-playerColor) < SCORE_5_CONTINUE)// û�м����γɵ�����������ȥ��ɱ����
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
                            if (buildAtackChildsAndPrune())
                            {
                                goto end;
                            }
                        }
                        else if (chessBoard->getPiece(i, j).getThreat(playerColor) > 900 && chessBoard->getPiece(i, j).getThreat(playerColor) < 1200)//����
                        {
                            /*if (chessBoard->getPiece(i, j).getThreat(-playerColor) >= 100)*/
                            {
                                createChildNode(i, j);
                                if (buildAtackChildsAndPrune())
                                {
                                    goto end;
                                }
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
                                if (playerColor == STATE_CHESS_BLACK)
                                {
                                    black.highestScore = -1;
                                }
                                else
                                {
                                    white.highestScore = -1;
                                }
                                goto end;//�����֣���������
                            }
                            createChildNode(i, j);
                            if (buildAtackChildsAndPrune())
                            {
                                goto end;
                            }
                            goto end;//��һ�������ˣ��������һ����ֱ�����ˣ�����������ζ�Ҫ��֦
                        }
                    }
                }
            }
        }
        else if (getHighest(-playerColor) >= SCORE_3_DOUBLE)//��player�Ļ���(�����γɵ����ġ����ġ�����)
        {
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
                            if (buildAtackChildsAndPrune())
                            {
                                goto end;//���info.depth < 0 ��goto end
                            }
                            /*if (info.depth > 0)*/
                            {
                                for (int n = 0; n < DIRECTION8_COUNT; ++n)//8������
                                {
                                    int r = i, c = j;
                                    int blankCount = 0, chessCount = 0;
                                    while (chessBoard->nextPosition(r, c, 1, n)) //����������߽�
                                    {
                                        if (chessBoard->pieces[r][c].state == 0)
                                        {
                                            blankCount++;
                                            if (!chessBoard->pieces[r][c].hot)
                                            {
                                                continue;
                                            }
                                            score = chessBoard->getPiece(r, c).getThreat(-playerColor);
                                            if (score >= 100 || score < 0)
                                            {
                                                score = chessBoard->getPiece(r, c).getThreat(playerColor);
                                                if (score < 0)//��������
                                                {
                                                    continue;
                                                }
                                                ChessBoard tempBoard = *chessBoard;
                                                tempBoard.doNextStep(r, c, playerColor);
                                                if (tempBoard.getStepScores(i, j, -playerColor, true) < SCORE_3_DOUBLE)
                                                {
                                                    tempBoard.updateThreat();
                                                    GameTreeNode *tempNode = new GameTreeNode(&tempBoard);
                                                    tempNode->hash = hash;
                                                    tempBoard.updateHashPair(tempNode->hash, r, c, -lastStep.getColor());
                                                    tempNode->alpha = alpha;
                                                    tempNode->beta = beta;
                                                    childs.push_back(tempNode);
                                                    if (buildAtackChildsAndPrune())
                                                    {
                                                        goto end;
                                                    }
                                                }
                                                else
                                                {
                                                    break;
                                                }
                                            }
                                        }
                                        else if (chessBoard->pieces[r][c].state == playerColor)
                                        {
                                            break;
                                        }
                                        else
                                        {
                                            chessCount++;
                                        }

                                        if (blankCount == 2
                                            || chessCount > 2)
                                        {
                                            break;
                                        }
                                    }
                                }
                            }

                        }
                        //else if (chessBoard->getPiece(i, j).getThreat(-playerColor) >= 100)//��Ӷ����������ġ�����
                        //{
                        //    int score = chessBoard->getPiece(i, j).getThreat(playerColor);
                        //    if (score < 0)//��������
                        //    {
                        //        continue;
                        //    }
                        //    tempBoard = *chessBoard;
                        //    tempBoard.doNextStep(i, j, playerColor);
                        //    tempBoard.updateThreat();
                        //    if (tempBoard.getRatingInfo(-playerColor).highestScore < SCORE_3_DOUBLE)
                        //    {
                        //        tempNode = new GameTreeNode(&tempBoard);
                        //        tempNode->hash = hash;
                        //        tempBoard.updateHashPair(tempNode->hash, i, j, -lastStep.getColor());
                        //        childs.push_back(tempNode);
                        //        RatingInfoAtack info = buildAtackChildWithTransTable(childs.back());
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
    if (GameTreeNode::longtailmode)
    {
        for (auto child : childs)
        {
            if (child->s.valid())
            {
                child->s.get();
            }
        }
    }
    delete chessBoard;
    chessBoard = 0;
}

RatingInfoAtack GameTreeNode::buildAtackChildWithTransTable(GameTreeNode* child)
{
    RatingInfoAtack info;
    TransTableNodeData data;
    int depth = getDepth();
    if (depth < transTableMaxDepth)
    {
        transpositionTable[depth]->lock.lock_shared();
        if (transpositionTable[depth]->m.find(child->hash.z32key) != transpositionTable[depth]->m.end())//����
        {
            data = transpositionTable[depth]->m[child->hash.z32key];
            transpositionTable[depth]->lock.unlock_shared();
            if (data.checksum == child->hash.z64key)//У��ɹ�
            {
                hash_hit++;
                //����build�ˣ�ֱ�����ֳɵ�
                child->black = data.black;
                child->white = data.white;
                child->lastStep = data.lastStep;
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
            transpositionTable[depth]->lock.unlock_shared();
            hash_miss++;
        }
    }

    child->buildAtackTreeNode();
    info = child->getBestAtackRating();

    if (depth < transTableMaxDepth)
    {
        data.checksum = child->hash.z64key;
        data.black = info.black;
        data.white = info.white;
        data.lastStep = info.lastStep;
        transpositionTable[depth]->lock.lock();
        transpositionTable[depth]->m[child->hash.z32key] = data;
        transpositionTable[depth]->lock.unlock();
    }

    child->black = info.black;
    child->white = info.white;
    child->lastStep = info.lastStep;
    child->deleteChilds();

    return info;
}

bool GameTreeNode::buildAtackChildsAndPrune()
{
    if (GameTreeNode::longtailmode && GameTreeNode::longtail_threadcount.load() < ThreadPool::num_thread
        && getDepth() < LONGTAILMODE_MAX_DEPTH)
    {
        GameTreeNode *child = childs.back();
        childs.back()->s = std::async(std::launch::async, [this, child]() {
            GameTreeNode::longtail_threadcount++;
            this->buildAtackChildWithTransTable(child);
            GameTreeNode::longtail_threadcount--;
        });
    }
    else
    {
        RatingInfoAtack info = buildAtackChildWithTransTable(childs.back());
        if (lastStep.getColor() == playerColor)//build AI, beta��֦
        {
            if (info.depth > -1)
            {
                if (info.depth < beta)//beta��֦
                {
                    return true;
                }
                //else 
                if (info.depth < alpha)//����alphaֵ
                {
                    alpha = info.depth;
                }
            }
        }
        else//buildplayer, alpha��֦
        {
            if (info.depth < 0 || info.depth > alpha || info.depth >= GameTreeNode::bestRating)//alpha��֦
            {
                return true;
            }
            else
            {
                if (info.depth > beta)//����betaֵ
                {
                    beta = info.depth;
                }
            }
        }
    }
    return false;
}

RatingInfoAtack GameTreeNode::getBestAtackRating()
{
    RatingInfoAtack result, temp;
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
        result.lastStep = lastStep;
        if (lastStep.getColor() == playerColor)//Ҷ�ӽڵ���player,��ʾ��ǰ����,AIȡʤ,����һ������AI
        {
            if (lastStep.step - startStep < 0)
            {
                result.depth = -1;
            }
            else if (getHighest(-playerColor) >= SCORE_5_CONTINUE)
            {
                result.depth = lastStep.step - startStep;
            }
            else
            {
                result.depth = -1;
            }
        }
        else//Ҷ�ӽڵ���AI,��ʾδ����
        {
            if (getHighest(-playerColor) >= SCORE_5_CONTINUE && getHighest(playerColor) < 0)//����
            {
                result.depth = lastStep.step - startStep + 1;
            }
            else
            {
                result.depth = -1;
            }
        }
    }
    else
    {
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
    }
    
    //result.moveList.push_back(lastStep);
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