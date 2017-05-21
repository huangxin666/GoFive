#include "GameTree.h"
#include "utils.h"
#include "ThreadPool.h"
#include <thread>
#include <assert.h>

#define LONGTAILMODE_MAX_DEPTH 6

ChildInfo *GameTreeNode::childsInfo = NULL;

int8_t GameTreeNode::playerColor = 1;
bool GameTreeNode::enableAtack = true;
AIRESULTFLAG GameTreeNode::resultFlag = AIRESULTFLAG_NORMAL;
uint8_t GameTreeNode::startStep = 0;
uint8_t GameTreeNode::maxSearchDepth = 0;
uint8_t GameTreeNode::transTableMaxDepth = 0;
size_t GameTreeNode::maxTaskNum = 0;
int GameTreeNode::bestRating = 0;
int GameTreeNode::bestIndex = -1;
trans_table GameTreeNode::transTable_atack(0);
HashStat GameTreeNode::transTableHashStat = { 0,0,0 };
bool GameTreeNode::longtailmode = false;
bool  GameTreeNode::iterative_deepening = false;
atomic<int> GameTreeNode::longtail_threadcount = 0;

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
    black = other.black;
    white = other.white;
    hash = other.hash;
    alpha = other.alpha;
    beta = other.beta;
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
    transTableHashStat = { 0,0,0 };
    hash = chessBoard->toHash();
    if (transTable_atack.size() < maxSearchDepth)
    {
        for (auto t : transTable_atack)
        {
            if (t)
            {
                delete t;
            }
        }
        transTable_atack.resize(maxSearchDepth);
        for (size_t i = 0; i < transTable_atack.size(); i++)
        {
            transTable_atack[i] = new SafeMap;
        }
    }
}

void GameTreeNode::clearTransTable()
{
    for (auto t : transTable_atack)
    {
        if (t)
        {
            t->m.clear();
        }
    }
}

void GameTreeNode::popHeadTransTable()
{
    if (transTable_atack[0])
    {
        delete transTable_atack[0];
    }
    transTable_atack.erase(transTable_atack.begin());
    transTable_atack.push_back(new SafeMap);
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

int GameTreeNode::buildDefendSearchTree(ThreadPool &pool)
{
    GameTreeNode::longtailmode = false;
    GameTreeNode::longtail_threadcount = 0;

    size_t index[2];
    if (childs.size() % 2 == 0)
    {
        index[0] = childs.size() / 2 - 1;
        index[1] = childs.size() / 2;
    }
    else
    {
        index[0] = childs.size() / 2;
        index[1] = childs.size() / 2;
    }
    for (; index[1] < childs.size(); index[0]--, index[1]++)
    {
        for (int n = 0; n < 2; ++n)
        {
            int i = index[n];
            if (!childsInfo[i].hasSearch)
            {
                childsInfo[i].hasSearch = true;
                Piece p = chessBoard->getPiece(childs[i]->lastStep.row, childs[i]->lastStep.col);
                if (p.getThreat(playerColor) < 100 && lastStep.step > 10)//active���ֻ��䣬�ŵ����ȫ���ҷ�ֹʧ�ܵ��߷�
                {
                    continue;
                }
                TaskItems t;
                t.node = childs[i];
                t.index = i;
                t.type = TASKTYPE_DEFEND;
                pool.run(bind(threadPoolWorkFunc, t));
            }
        }
    }
    //for (size_t i = 0; i < childs.size(); ++i)
    //{
    //    if (!childsInfo[i].hasSearch)
    //    {
    //        Piece p = chessBoard->getPiece(childs[i]->lastStep.row, childs[i]->lastStep.col);
    //        if (p.getThreat(playerColor) < 100 && lastStep.step > 10)
    //        {
    //            childsInfo[i].hasSearch = true;
    //            //sortList[i].value = -1000000;
    //            continue;
    //        }
    //        Task t;
    //        t.node = childs[i];
    //        t.index = i;
    //        //t.threatInfo = childsInfo;
    //        t.type = TASKTYPE_DEFEND;
    //        pool.run(t);
    //    }
    //}


    //�ȴ��߳�
    pool.wait();
    //for (size_t i = 0; i < childs.size(); i++)
    //{
    //    if (!childsInfo[sortList[i].key].hasSearch)
    //    {
    //        childsInfo[sortList[i].key].hasSearch = true;//hasSearchֵ��buildSortListInfo��Ӱ��
    //        sortList[i].value = childsInfo[sortList[i].key].rating.totalScore;
    //    }
    //}
    //sort(sortList, 0, childs.size() - 1);

    ////�����
    //int i = childs.size() - 1;
    //while (i > 0 && (sortList[i - 1].value == sortList[i].value)) i--;
    //return i + rand() % (childs.size() - i);
    return GameTreeNode::bestIndex;
}

Position GameTreeNode::getBestStep()
{
    ThreadPool pool;
    Position result;
    int bestDefendPos;
    buildAllChilds();
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
    }

    pool.start();

    if (ChessBoard::level >= AILEVEL_MASTER && enableAtack)
    {
        clearTransTable();
        GameTreeNode::bestRating = 100;//������
        int atackSearchTreeResult = buildAtackSearchTree(pool);
        if (atackSearchTreeResult > -1)
        {
            if (childsInfo[atackSearchTreeResult].lastStepScore < 1200
                || (childsInfo[atackSearchTreeResult].lastStepScore >= SCORE_3_DOUBLE && childsInfo[atackSearchTreeResult].lastStepScore < SCORE_4_DOUBLE))
            {
                GameTreeNode* simpleSearchNode = new GameTreeNode();
                *simpleSearchNode = *childs[atackSearchTreeResult];
                simpleSearchNode->buildDefendTreeNodeSimple(4);//����4��
                tempinfo = simpleSearchNode->getBestDefendRating(childsInfo[atackSearchTreeResult].lastStepScore);
                simpleSearchNode->deleteChilds();
                delete simpleSearchNode;
                if (tempinfo.rating.highestScore >= SCORE_5_CONTINUE)
                {
                    resultFlag = AIRESULTFLAG_COMPLAIN;
                    clearTransTable();
                    goto beginDefend;
                }
            }
            resultFlag = AIRESULTFLAG_NEARWIN;
            result = Position{ childs[atackSearchTreeResult]->lastStep.row, childs[atackSearchTreeResult]->lastStep.col };
            popHeadTransTable();
            popHeadTransTable();
            goto endsearch;
        }
    }

    resultFlag = AIRESULTFLAG_NORMAL;
    clearTransTable();

    GameTreeNode::longtailmode = true;
    GameTreeNode::longtail_threadcount = 0;
    GameTreeNode::bestRating = INT32_MIN;
    GameTreeNode::iterative_deepening = false;
    int activeChildIndex;
    if (lastStep.step > 10)
    {
        activeChildIndex = getActiveChild();
    }
    else
    {
        activeChildIndex = getDefendChild();
    }
    
    //���ⱻ��֦���ȵ�����
    childs[activeChildIndex]->alpha = GameTreeNode::bestRating;
    childs[activeChildIndex]->beta = INT32_MAX;
    childs[activeChildIndex]->buildDefendTreeNode(childsInfo[activeChildIndex].lastStepScore);
    childsInfo[activeChildIndex].hasSearch = true;
    tempinfo = childs[activeChildIndex]->getBestDefendRating(childsInfo[activeChildIndex].lastStepScore);
    childsInfo[activeChildIndex].rating = tempinfo.rating;
    childsInfo[activeChildIndex].depth = tempinfo.lastStep.step - GameTreeNode::startStep;
    childs[activeChildIndex]->deleteChilds();

    GameTreeNode::bestRating = childsInfo[activeChildIndex].rating.totalScore;
    GameTreeNode::bestIndex = activeChildIndex;


    if (childsInfo[activeChildIndex].rating.highestScore < SCORE_3_DOUBLE)
    {
        //��������������ᵼ������ʧ�ܣ���������������������10�����Ȳ�����
        result = Position{ childs[activeChildIndex]->lastStep.row, childs[activeChildIndex]->lastStep.col };
        clearTransTable();
        goto endsearch;
    }
beginDefend:
    //��ʼ�������
    bestDefendPos = buildDefendSearchTree(pool);

    //transpositionTable.clear();
    clearTransTable();

    if (childsInfo[bestDefendPos].rating.highestScore >= SCORE_5_CONTINUE)
    {
        if (resultFlag != AIRESULTFLAG_COMPLAIN)
        {
            resultFlag = AIRESULTFLAG_FAIL;
        }
        bestDefendPos = getDefendChild();
        result = Position{ childs[bestDefendPos]->lastStep.row, childs[bestDefendPos]->lastStep.col };
    }
    else
    {
        result = Position{ childs[bestDefendPos]->lastStep.row, childs[bestDefendPos]->lastStep.col };
    }
endsearch:
    delete[] childsInfo;
    childsInfo = NULL;
    pool.stop();
    return result;
}

int GameTreeNode::getActiveChild()
{
    int maxAI = INT_MIN, maxPlayer = INT_MIN;
    int tempAI, tempPlayer;
    vector<int> results;
    for (size_t i = 0; i < childs.size(); ++i)
    {
        ChessBoard tempBoard = *(childs[i]->chessBoard);
        int highest = tempBoard.getRatingInfo(-playerColor).highestScore;
        for (Position pos = { 0,0 }; pos.valid(); ++pos)
        {
            if (tempBoard.pieces[pos.row][pos.col].getThreat(-playerColor) >= highest)
            {
                tempBoard.doNextStep(pos.row, pos.col, playerColor);
                tempBoard.updateThreat(-playerColor);
                break;
            }
        }
        tempAI = tempBoard.getRatingInfo(-playerColor).totalScore;
        tempAI = tempAI / 100 * 100;
        tempPlayer = tempBoard.getRatingInfo(playerColor).totalScore;
        tempPlayer = tempPlayer / 100 * 100;

        tempAI = tempAI - tempPlayer / 10;
        //temp = childsInfo[i].lastStepScore + childs[i]->getTotal(-playerColor) - childs[i]->getTotal(playerColor) / 10;
        //if (temp >= SCORE_5_CONTINUE && childsInfo[i].lastStepScore > 1200 && childsInfo[i].lastStepScore < 1400)
        //{
        //    temp -= SCORE_5_CONTINUE;//������������ĵ����ȼ�
        //}
        if (tempAI > maxAI)
        {
            results.clear();
            maxAI = tempAI;
            results.push_back(i);
        }
        else if (tempAI == maxAI)
        {
            results.push_back(i);
        }
    }
    return results[rand() % results.size()];;
}

int GameTreeNode::getDefendChild()
{
    int min = INT_MAX, temp;
    vector<int> results;
    for (size_t i = 0; i < childs.size(); ++i)
    {
        temp = childs[i]->getTotal(playerColor) + childsInfo[i].lastStepScore / 10;
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

void GameTreeNode::buildDefendTreeNodeSimple(int deepen)
{
    RatingInfo info;
    int score;
    if (lastStep.getColor() == -playerColor)//build player
    {
        if (getDepth() >= maxSearchDepth + deepen) //���������������֤���һ����AI�µģ��ʶ�=maxSearchDepthʱ��ֱ�ӽ���           
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
                            childs.back()->buildDefendTreeNodeSimple(deepen);
                            if (childs.back()->getBestDefendRating(deepen).rating.totalScore >= SCORE_5_CONTINUE)
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
                            childs.back()->buildDefendTreeNodeSimple(deepen);
                            if (childs.back()->getBestDefendRating(deepen).rating.totalScore >= SCORE_5_CONTINUE)
                            {
                                goto end;
                            }
                        }
                    }
                }
            }
        }
        else if (getHighest(playerColor) > 99 && getHighest(-playerColor) < SCORE_5_CONTINUE)
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
                            tempBoard = *chessBoard;
                            tempBoard.doNextStep(i, j, playerColor);
                            score = tempBoard.updateThreat(playerColor);
                            if (score >= SCORE_5_CONTINUE)//����
                            {
                                tempBoard.updateThreat(-playerColor);
                                tempNode = new GameTreeNode(&tempBoard);
                                tempNode->hash = hash;
                                tempBoard.updateHashPair(tempNode->hash, i, j, -lastStep.getColor());
                                tempNode->alpha = alpha;
                                tempNode->beta = beta;
                                childs.push_back(tempNode);
                                childs.back()->buildDefendTreeNodeSimple(deepen);
                                if (childs.back()->getBestDefendRating(deepen).rating.totalScore >= SCORE_5_CONTINUE)
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
                            childs.back()->buildDefendTreeNodeSimple(deepen);
                            goto end;//�ض£���һ�������ˣ��������һ����ֱ������
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

void GameTreeNode::setAlpha(int score, int type)
{
    try
    {
        if (type == CUTTYPE_DEFEND)
        {
            if (score > alpha)
            {
                for (size_t i = 0; i < childs.size(); ++i)
                {
                    childs[i]->setAlpha(score, type);
                }
                alpha = score;
            }
        }
        else if (type == CUTTYPE_ATACK)
        {
            if (score > -1 && score < alpha)
            {
                for (size_t i = 0; i < childs.size(); ++i)
                {
                    childs[i]->setAlpha(score, type);
                }
                alpha = score;
            }
        }
    }
    catch (std::exception)//���ܻ�outofrange����invalid address
    {
        return;
    }
}

void GameTreeNode::setBeta(int score, int type)
{
    try
    {
        if (type == CUTTYPE_DEFEND)
        {
            if (score < beta)
            {

                for (size_t i = 0; i < childs.size(); ++i)
                {
                    childs[i]->setBeta(score, type);
                }
                beta = score;
            }
        }
        else if (type == CUTTYPE_ATACK)
        {
            if (score > -1 && score > beta)
            {

                for (size_t i = 0; i < childs.size(); ++i)
                {
                    childs[i]->setBeta(score, type);
                }
                beta = score;
            }
        }
    }
    catch (std::exception)//���ܻ�outofrange����invalid address
    {
        return;
    }
}

void GameTreeNode::buildDefendTreeNode(int basescore)
{
    /*if (black.highestScore == 1001 && black.totalScore == 2305 && white.highestScore == 1210 && white.totalScore == 6200)
    {
        int a  = 1;
    }*/
    RatingInfo info;
    int score;
    if (lastStep.getColor() == -playerColor)//build player
    {
        if ((getDepth() >= maxSearchDepth))//���������������֤���һ����AI�µģ��ʶ�=maxSearchDepthʱ��ֱ�ӽ���
            //&& GameTreeNode::iterative_deepening)//GameTreeNode::iterative_deepeningΪfalse��ʱ����Լ�������
            //|| getDepth() >= maxSearchDepth + 2)//���24��
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
        //else if (getHighest(-playerColor) >= SCORE_4_DOUBLE)//�����ġ����ģ����ǿɱ����Ľ����滻
        //{
        //    for (int i = 0; i < BOARD_ROW_MAX; ++i)
        //    {
        //        for (int j = 0; j < BOARD_COL_MAX; ++j)
        //        {
        //            if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
        //            {
        //                if (chessBoard->getPiece(i, j).getThreat(-playerColor) >= SCORE_4_DOUBLE)
        //                {
        //                    score = chessBoard->getPiece(i, j).getThreat(playerColor);
        //                    if (score < 0)
        //                    {
        //                        continue;
        //                    }
        //                    createChildNode(i, j);
        //                    if (buildDefendChildsAndPrune(basescore))
        //                    {
        //                        goto end;
        //                    }

        //                }
        //            }
        //        }
        //    }
        //}
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
        else if (/*getHighest(playerColor) > 99 && */getHighest(-playerColor) < SCORE_5_CONTINUE)
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
                            tempBoard = *chessBoard;
                            tempBoard.doNextStep(i, j, playerColor);
                            score = tempBoard.updateThreat(playerColor);
                            tempBoard.updateThreat(-playerColor);
                            if (tempBoard.getRatingInfo(-playerColor).highestScore < SCORE_4_DOUBLE || score >= SCORE_5_CONTINUE)
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
                            //createChildNode(i, j);
                        }
                        else if (ChessBoard::level >= AILEVEL_MASTER && getDepth() < maxSearchDepth - 4 && score > 0)//������������γ�����
                        {
                            tempBoard = *chessBoard;
                            tempBoard.doNextStep(i, j, playerColor);
                            score = tempBoard.updateThreat(playerColor);
                            tempBoard.updateThreat(-playerColor);
                            if (tempBoard.getRatingInfo(-playerColor).highestScore < SCORE_4_DOUBLE)
                            {
                                if (score >= SCORE_4_DOUBLE)
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
                                else if (getDepth() < 4 && score > 2000) // �������
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
        //else if (getHighest(-playerColor) >= SCORE_4_DOUBLE && getHighest(playerColor) < SCORE_5_CONTINUE)// û�м����γɵ�����������ȥ��ɱ
        //{
        //    for (int i = 0; i < BOARD_ROW_MAX; ++i)
        //    {
        //        for (int j = 0; j < BOARD_COL_MAX; ++j)
        //        {
        //            if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
        //            {
        //                if (chessBoard->getPiece(i, j).getThreat(-playerColor) >= SCORE_4_DOUBLE)
        //                {
        //                    createChildNode(i, j);
        //                    if (buildDefendChildsAndPrune(basescore))
        //                    {
        //                        goto end;
        //                    }
        //                }
        //            }
        //        }
        //    }
        //}

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
                            createChildNode(i, j);//ֱ�Ӷ�
                            if (buildDefendChildsAndPrune(basescore))
                            {
                                goto end;
                            }
                            if (ChessBoard::level >= AILEVEL_MASTER && getDepth() < maxSearchDepth - 3)//��Ӷ�
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
                                            score = chessBoard->getPiece(r, c).getThreat(playerColor);
                                            if (score >= 100 || score < 0)
                                            {
                                                score = chessBoard->getPiece(r, c).getThreat(-playerColor);
                                                if (score < 0)//��������
                                                {
                                                    continue;
                                                }
                                                ChessBoard tempBoard = *chessBoard;
                                                tempBoard.doNextStep(r, c, -playerColor);
                                                if (tempBoard.setThreat(i, j, playerColor) < SCORE_3_DOUBLE)
                                                {
                                                    tempBoard.updateThreat();
                                                    GameTreeNode *tempNode = new GameTreeNode(&tempBoard);
                                                    tempNode->hash = hash;
                                                    tempBoard.updateHashPair(tempNode->hash, r, c, -lastStep.getColor());
                                                    tempNode->alpha = alpha;
                                                    tempNode->beta = beta;
                                                    childs.push_back(tempNode);
                                                    if (buildDefendChildsAndPrune(basescore))
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
                                        else if (chessBoard->pieces[r][c].state == -playerColor)
                                        {
                                            break;
                                        }
                                        else
                                        {
                                            chessCount++;
                                        }

                                        if (blankCount == 2
                                            || chessCount > 3)
                                        {
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        else if (getHighest(-playerColor) >= SCORE_4_DOUBLE)//����ס�ͽ���
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
        else if (getHighest(playerColor) > 900 /*&& getHighest(-playerColor) < 100*/)//�³��ġ�����
        {
            for (int i = 0; i < BOARD_ROW_MAX; ++i)
            {
                for (int j = 0; j < BOARD_COL_MAX; ++j)
                {
                    if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                    {
                        score = chessBoard->getPiece(i, j).getThreat(playerColor);
                        if (score > 900/* && score < 1200*/)
                        {
                            if ((score == 999 || score == 1001 || score == 1030))//������ĳ���
                            {
                                continue;
                            }
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

RatingInfoDenfend GameTreeNode::buildDefendChildWithTransTable(GameTreeNode* child, int basescore)
{
    TransTableNodeData data;
    RatingInfoDenfend info;
    int depth = getDepth();
    if (depth < transTableMaxDepth)
    {
        transTable_atack[depth]->lock.lock_shared();
        if (transTable_atack[depth]->m.find(child->hash.z32key) != transTable_atack[depth]->m.end())//����
        {
            data = transTable_atack[depth]->m[child->hash.z32key];
            transTable_atack[depth]->lock.unlock_shared();
            if (data.checksum == child->hash.z64key)//У��ɹ�
            {
                transTableHashStat.hit++;
                //����build�ˣ�ֱ�����ֳɵ�
                child->lastStep = data.lastStep;
                child->black = data.black;
                child->white = data.white;
                info = child->getBestDefendRating(basescore);
                return info;
            }
            else//��ͻ������
            {
                transTableHashStat.clash++;
            }
        }
        else//δ����
        {
            transTable_atack[depth]->lock.unlock_shared();
            transTableHashStat.miss++;
        }
    }

    child->buildDefendTreeNode(basescore);
    info = child->getBestDefendRating(basescore);

    if (depth < transTableMaxDepth && child->childs.size() > 0)//�������û���
    {
        data.checksum = child->hash.z64key;
        data.lastStep = info.lastStep;
        data.black = info.black;
        data.white = info.white;
        transTable_atack[depth]->lock.lock();
        transTable_atack[depth]->m[child->hash.z32key] = data;
        transTable_atack[depth]->lock.unlock();
    }


    child->black = info.black;
    child->white = info.white;
    child->lastStep = info.lastStep;
    child->deleteChilds();

    return info;
}


bool GameTreeNode::buildDefendChildsAndPrune(int basescore)
{
    if (GameTreeNode::longtailmode && GameTreeNode::longtail_threadcount.load() < ThreadPool::num_thread - 1
        && getDepth() < LONGTAILMODE_MAX_DEPTH)
    {
        GameTreeNode *child = childs.back();
        childs.back()->s = std::async(std::launch::async, [this, child, basescore]() {
            GameTreeNode::longtail_threadcount++;
            RatingInfoDenfend info = this->buildDefendChildWithTransTable(child, basescore);
            //if (lastStep.getColor() == -playerColor)//build player
            //{
            //    this->setBeta(info.rating.totalScore, CUTTYPE_DEFEND);
            //}
            //else
            //{
            //    this->setAlpha(info.rating.totalScore, CUTTYPE_DEFEND);
            //}

            GameTreeNode::longtail_threadcount--;
        });
    }
    else
    {
        RatingInfoDenfend info = buildDefendChildWithTransTable(childs.back(), basescore);
        if (lastStep.getColor() == -playerColor)//build player
        {
            if (info.rating.totalScore < -SCORE_5_CONTINUE || info.rating.totalScore <= alpha || info.rating.totalScore <= GameTreeNode::bestRating)//alpha��֦
            {
                return true;
            }
            //����betaֵ
            if (info.rating.totalScore < beta)
            {
                beta = info.rating.totalScore;
            }
        }
        else//build AI
        {
            if (info.rating.totalScore >= beta)//beta��֦
            {
                return true;
            }
            //����alphaֵ
            if (info.rating.totalScore > alpha)
            {
                alpha = info.rating.totalScore;
            }
        }
    }

    return false;
}

RatingInfoDenfend GameTreeNode::getBestDefendRating(int basescore)
{
    RatingInfoDenfend result;
    if (childs.size() == 0)
    {
        result.lastStep = lastStep;
        result.black = black;
        result.white = white;
        result.rating.highestScore = getHighest(playerColor);
        if (getTotal(playerColor) >= SCORE_5_CONTINUE)
        {
            result.rating.totalScore = -SCORE_5_CONTINUE - 100 + (lastStep.step - GameTreeNode::startStep);
        }
        else
        {
            result.rating.totalScore = -getTotal(playerColor);
            //result.rating.totalScore = result.rating.totalScore / 10 * 10;
            if (result.rating.totalScore < 0 && result.rating.totalScore > -1000)
            {
                result.rating.totalScore = result.rating.totalScore / 100 * 100;
            }
            else
            {
                result.rating.totalScore = result.rating.totalScore / 1000 * 1000;
            }
            if (ChessBoard::level == AILEVEL_HIGH || lastStep.step < 10)
            {
                result.rating.totalScore += basescore;
            }
            //result.rating.totalScore += basescore;
            if (lastStep.getColor() == playerColor)
            {
                result.rating.totalScore /= 10;//Ϊ��ʹȨ��ƽ�⣬��Ȼ���һ����playerColor�µĵĻ���Ȩ��ʼ�����������һ����AI�µ�
                                               //����������֮���BUG
            }
        }

    }
    else
    {
        RatingInfoDenfend tempThreat;
        result = childs[0]->getBestDefendRating(basescore);

        if (lastStep.getColor() == -playerColor)//AI�ڵ� build player
        {
            for (size_t i = 1; i < childs.size(); ++i)
            {
                tempThreat = childs[i]->getBestDefendRating(basescore);//�ݹ�
                if (tempThreat.rating.totalScore < result.rating.totalScore)//bestԭ��:player�¹��Ľڵ�player�÷�Խ��Խ��(Ĭ��player�����ŵ�)
                {
                    result = tempThreat;
                }
            }
        }
        else//player�ڵ� build AI
        {
            for (size_t i = 1; i < childs.size(); ++i)
            {
                tempThreat = childs[i]->getBestDefendRating(basescore);//child��AI�ڵ�
                if (tempThreat.rating.totalScore > result.rating.totalScore)//bestԭ��:AI�¹��Ľڵ�player�÷�ԽСԽ��
                {
                    result = tempThreat;
                }
            }
        }
    }

    //result.moveList.push_back(lastStep);

    return result;
}


int GameTreeNode::buildAtackSearchTree(ThreadPool &pool)
{
    GameTreeNode::longtailmode = false;
    GameTreeNode::longtail_threadcount = 0;
    if (getHighest(-playerColor) >= SCORE_5_CONTINUE)//����5��������������
    {
        assert(0);//����������
    }
    //vector<int> index;
    GameTreeNode::bestIndex = -1;
    for (size_t i = 0; i < childs.size(); ++i)
    {
        //lastStepScore�ǽ���Ȩ��
        int flag = false;
        if ((childsInfo[i].lastStepScore > 1000 && childs[i]->getHighest(playerColor) < SCORE_5_CONTINUE))
        {
            flag = true;
        }
        else if (childsInfo[i].lastStepScore > 0)
        {
            if (getHighest(-playerColor) < SCORE_4_DOUBLE && childs[i]->getHighest(-playerColor) >= SCORE_4_DOUBLE)
            {
                flag = true;
            }
            else if (childs[i]->chessBoard->updateThreat(-playerColor) > 4000)
            {
                flag = true;
            }
        }
        if (flag)
        {
            TaskItems t;
            t.node = new GameTreeNode();
            *t.node = *childs[i];
            t.index = i;
            t.type = TASKTYPE_ATACK;
            pool.run(bind(threadPoolWorkFunc, t));
            //index.push_back(i);
        }
    }
    pool.wait();

    return GameTreeNode::bestIndex;
}

void GameTreeNode::buildAtackTreeNode(int deepen)
{
    int oldalpha = alpha;
    if (lastStep.getColor() == playerColor)//build AI ������
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
                            if (buildAtackChildsAndPrune(deepen))
                            {
                                goto end;
                            }

                        }
                    }
                }
            }
        }
        else if (getHighest(-playerColor) >= SCORE_4_DOUBLE)//����
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
                            if (buildAtackChildsAndPrune(deepen))
                            {
                                goto end;
                            }
                        }
                    }
                }
            }
        }
        else if (getHighest(-playerColor) > 99)//����
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
                        if (score > 99 && score < SCORE_4_DOUBLE)
                        {
                            createChildNode(i, j);
                            if (buildAtackChildsAndPrune(deepen))
                            {
                                goto end;
                            }
                        }
                        else if (score > 0 && getDepth() < maxSearchDepth - 4)
                        {
                            ChessBoard tempBoard = *chessBoard;
                            tempBoard.doNextStep(i, j, -playerColor);
                            score = tempBoard.updateThreat(-playerColor);

                            if (score >= SCORE_4_DOUBLE)
                            {
                                tempBoard.updateThreat(playerColor);
                                GameTreeNode *tempNode = new GameTreeNode(&tempBoard);
                                tempNode->hash = hash;
                                tempBoard.updateHashPair(tempNode->hash, i, j, -lastStep.getColor());
                                tempNode->alpha = alpha;
                                tempNode->beta = beta;
                                childs.push_back(tempNode);
                                if (buildAtackChildsAndPrune(deepen))
                                {
                                    goto end;
                                }

                            }
                            else if (getDepth() < 4 && score > 2000) // �������
                            {
                                tempBoard.updateThreat(playerColor);
                                GameTreeNode *tempNode = new GameTreeNode(&tempBoard);
                                tempNode->hash = hash;
                                tempBoard.updateHashPair(tempNode->hash, i, j, -lastStep.getColor());
                                tempNode->alpha = alpha;
                                tempNode->beta = beta;
                                childs.push_back(tempNode);
                                if (buildAtackChildsAndPrune(deepen))
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
    else//buildplayer ���ط�
    {
        //����
        int score;
        bool flag = false;
        if (getDepth() >= maxSearchDepth)//���������������֤���һ����AI�µģ��ʶ�=maxSearchDepthʱ��ֱ�ӽ���
        {
            goto end;
        }
        if (getDepth() >= alpha || getDepth() >= GameTreeNode::bestRating)
        {
            goto end;
        }
        if (getHighest(playerColor) >= SCORE_5_CONTINUE)
        {
            goto end;
        }
        else if (getHighest(-playerColor) < SCORE_5_CONTINUE)// û�м����γɵ�����������ȥ��ɱ�����һ���
        {
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
                            if (buildAtackChildsAndPrune(deepen))
                            {
                                goto end;
                            }
                        }
                        else if (score > 900 && score < 1200 && getHighest(-playerColor) >= SCORE_3_DOUBLE)//���ڷ��ط���������Ϊ���һ��ᣬ�������׳�
                        {
                            if (/*(score == 999 || score == 1001 || score == 1030) && */
                                chessBoard->getPiece(i, j).getThreat(-playerColor) < 100)//���˵�������ĳ���
                            {
                                continue;
                            }
                            //flag = true;//��֤��������
                            createChildNode(i, j);
                            if (buildAtackChildsAndPrune(deepen + 2))//���Ĳ����������������
                            {
                                goto end;
                            }
                        }
                    }
                }
            }
        }
        //if (deepen > 0)//ֻҪ���Ĺ����Ͳ����ڽ�������������
        //{
        //    goto end;
        //}
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
                            if (buildAtackChildsAndPrune(deepen))
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
                            createChildNode(i, j);//ֱ�Ӷ�
                            if (buildAtackChildsAndPrune(deepen))
                            {
                                goto end;//���info.depth < 0 ��goto end
                            }
                            /*if (info.depth > 0)*/
                            {
                                //��Ӷ�
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
                                                if (tempBoard.setThreat(i, j, -playerColor) < SCORE_3_DOUBLE)
                                                {
                                                    tempBoard.updateThreat();
                                                    GameTreeNode *tempNode = new GameTreeNode(&tempBoard);
                                                    tempNode->hash = hash;
                                                    tempBoard.updateHashPair(tempNode->hash, r, c, -lastStep.getColor());
                                                    tempNode->alpha = alpha;
                                                    tempNode->beta = beta;
                                                    childs.push_back(tempNode);
                                                    if (buildAtackChildsAndPrune(deepen))
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
                                            || chessCount > 3)
                                        {
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else if (getHighest(-playerColor) > 900 && getHighest(playerColor) < 100)//�³��ġ�����
        {
            for (int i = 0; i < BOARD_ROW_MAX; ++i)
            {
                for (int j = 0; j < BOARD_COL_MAX; ++j)
                {
                    if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                    {
                        score = chessBoard->getPiece(i, j).getThreat(-playerColor);
                        if (score > 900)
                        {
                            if ((score == 999 || score == 1001 || score == 1030))//������ĳ���
                            {
                                continue;
                            }
                            score = chessBoard->getPiece(i, j).getThreat(playerColor);
                            if (score < 0)//��������
                            {
                                continue;
                            }
                            createChildNode(i, j);
                            if (buildAtackChildsAndPrune(deepen))
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

    //    if (lastStep.getColor() == -playerColor && getHighest(-playerColor) < SCORE_5_CONTINUE 
    //        && alpha > 0 && getDepth() < 6 )//�����һ���
    //    {
    //        if (chessBoard->pieces[5][9].state == 1 )
    //        {
    //            chessBoard->pieces[5][9].state = 1;
    //        }
    //        for (int i = 0; i < BOARD_ROW_MAX; ++i)
    //        {
    //            for (int j = 0; j < BOARD_COL_MAX; ++j)
    //            {
    //                if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
    //                {
    //                    int score = chessBoard->getPiece(i, j).getThreat(playerColor);
    //                    if (score > 900 && score < 1200)
    //                    {
    //                        createChildNode(i, j);
    //                        childs.back()->buildDefendTreeNodeSimple(getDepth());
    //                        RatingInfoDenfend tempinfo = childs.back()->getBestDefendRating(0);
    //                        childs.back()->deleteChilds();
    //                        if (tempinfo.rating.highestScore >= SCORE_5_CONTINUE)
    //                        {
    //                            childs.back()->lastStep.step = 0;
    //                            childs.back()->black = { 0,0 };
    //                            childs.back()->white = { 0,0 };
    //                            goto gotodelete;
    //                        }
    //                        else
    //                        {
    //                            childs.pop_back();
    //                        }
    //                    }
    //                }
    //            }
    //        }
    //
    //    }
    //gotodelete:
    delete chessBoard;
    chessBoard = 0;
}

RatingInfoAtack GameTreeNode::buildAtackChildWithTransTable(GameTreeNode* child, int deepen)
{
    RatingInfoAtack info;
    TransTableNodeData data;
    int depth = getDepth();
    if (depth < transTableMaxDepth)
    {
        transTable_atack[depth]->lock.lock_shared();
        if (transTable_atack[depth]->m.find(child->hash.z32key) != transTable_atack[depth]->m.end())//����
        {
            data = transTable_atack[depth]->m[child->hash.z32key];
            transTable_atack[depth]->lock.unlock_shared();
            if (data.checksum == child->hash.z64key)//У��ɹ�
            {
                transTableHashStat.hit++;
                //����build�ˣ�ֱ�����ֳɵ�
                child->black = data.black;
                child->white = data.white;
                child->lastStep = data.lastStep;
                info = child->getBestAtackRating();
                return info;
            }
            else//��ͻ������
            {
                transTableHashStat.clash++;
            }
        }
        else//δ����
        {
            transTable_atack[depth]->lock.unlock_shared();
            transTableHashStat.miss++;
        }
    }

    child->buildAtackTreeNode(deepen);
    info = child->getBestAtackRating();

    if (depth < transTableMaxDepth && child->childs.size() > 0)
    {
        data.checksum = child->hash.z64key;
        data.black = info.black;
        data.white = info.white;
        data.lastStep = info.lastStep;
        transTable_atack[depth]->lock.lock();
        transTable_atack[depth]->m[child->hash.z32key] = data;
        transTable_atack[depth]->lock.unlock();
    }

    child->black = info.black;
    child->white = info.white;
    child->lastStep = info.lastStep;
    child->deleteChilds();
    return info;
}

bool GameTreeNode::buildAtackChildsAndPrune(int deepen)
{
    if (GameTreeNode::longtailmode && GameTreeNode::longtail_threadcount.load() < ThreadPool::num_thread
        && getDepth() < LONGTAILMODE_MAX_DEPTH)
    {
        GameTreeNode *child = childs.back();
        childs.back()->s = std::async(std::launch::async, [this, child, deepen]() {
            GameTreeNode::longtail_threadcount++;
            RatingInfoAtack info = this->buildAtackChildWithTransTable(child, deepen);
            //if (lastStep.getColor() == playerColor)//build AI, beta��֦
            //{
            //    this->setAlpha(info.depth, CUTTYPE_ATACK);
            //}
            //else
            //{
            //    this->setBeta(info.depth, CUTTYPE_ATACK);
            //}

            GameTreeNode::longtail_threadcount--;
        });
    }
    else
    {
        RatingInfoAtack info = buildAtackChildWithTransTable(childs.back(), deepen);
        if (lastStep.getColor() == playerColor)//build AI, beta��֦
        {
            if (info.depth > -1)
            {
                if (info.depth <= beta)//beta��֦
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
            if (info.depth < 0 || info.depth >= alpha || info.depth >= GameTreeNode::bestRating)//alpha��֦
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

void GameTreeNode::threadPoolWorkFunc(TaskItems t)
{
    if (t.type == TASKTYPE_DEFEND)
    {
        t.node->alpha = GameTreeNode::bestRating;
        t.node->beta = INT32_MAX;
        t.node->buildDefendTreeNode(GameTreeNode::childsInfo[t.index].lastStepScore);
        RatingInfoDenfend info = t.node->getBestDefendRating(GameTreeNode::childsInfo[t.index].lastStepScore);
        GameTreeNode::childsInfo[t.index].rating = info.rating;
        GameTreeNode::childsInfo[t.index].depth = info.lastStep.step - GameTreeNode::startStep;
        if (t.index == 30)
        {
            t.index = 30;
        }
        t.node->deleteChilds();
        if (GameTreeNode::childsInfo[t.index].rating.totalScore > GameTreeNode::bestRating)
        {
            GameTreeNode::bestRating = GameTreeNode::childsInfo[t.index].rating.totalScore;
            GameTreeNode::bestIndex = t.index;
        }
    }
    else if (t.type == TASKTYPE_ATACK)
    {
        t.node->alpha = GameTreeNode::bestRating;
        t.node->beta = 0;
        //if (t.index == 24)
        //{
        //    t.index = 24;
        //}
        t.node->buildAtackTreeNode(0);
        RatingInfoAtack info = t.node->getBestAtackRating();
        GameTreeNode::childsInfo[t.index].rating = (t.node->playerColor == STATE_CHESS_BLACK) ? info.white : info.black;
        GameTreeNode::childsInfo[t.index].depth = info.depth;
        if (GameTreeNode::childsInfo[t.index].rating.highestScore >= SCORE_5_CONTINUE && info.depth > -1)
        {
            if (info.depth < GameTreeNode::bestRating)
            {
                GameTreeNode::bestRating = info.depth;
                GameTreeNode::bestIndex = t.index;
            }
        }
        if (t.index == 10)
        {
            t.index = 10;
        }
        t.node->deleteChilds();
        delete t.node;
    }
}