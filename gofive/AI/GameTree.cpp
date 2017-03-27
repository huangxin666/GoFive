#include "GameTree.h"
#include "utils.h"
#include "ThreadPool.h"
#include <thread>

static int countTreeNum = 0;
int8_t GameTreeNode::playerColor = 1;
bool GameTreeNode::multiThread = true;
int GameTreeNode::maxTaskNum = 0;
unordered_map<string, GameTreeNode*>* GameTreeNode::historymap = NULL;
ChildInfo *sortList;

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

GameTreeNode::GameTreeNode(ChessBoard *chessBoard, int depth, int tempdepth, int score) :
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

GameTreeNode::~GameTreeNode()
{
    deleteChild();
    if (currentBoard) delete currentBoard;
}

const GameTreeNode& GameTreeNode::operator=(const GameTreeNode& other)
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

void GameTreeNode::deleteChild()
{
    for (size_t i = 0; i < childs.size(); i++)
    {
        delete childs[i];
    }
    childs.clear();
}

void GameTreeNode::deleteChessBoard()
{
    delete currentBoard;
    currentBoard = 0;
}

int GameTreeNode::findBestChild(int *childrenInfo)
{
    int bestScore = -500000;
    int randomStep[100];
    int randomCount = 0;
    for (size_t i = 0; i < childs.size(); i++)
    {
        if (childs[i]->currentScore >= SCORE_5_CONTINUE)
        {
            return i;
        }
        else if (childs[i]->currentScore >= SCORE_4_DOUBLE && childs[i]->getHighest(lastStep.getColor()) < SCORE_5_CONTINUE)
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

void GameTreeNode::buildChildrenInfo(int *childrenInfo, int i)
{
    if (childs[i]->getChildNum() == 0)
    {
        childrenInfo[i] = childs[i]->currentScore - childs[i]->getHighest(lastStep.getColor()) - childs[i]->getTotal(lastStep.getColor()) / 10;
        if (childrenInfo[i] <= -SCORE_5_CONTINUE)//���Ӷ³��ĵ����ȼ�
        {
            childrenInfo[i] -= SCORE_5_CONTINUE;
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

ThreatInfo GameTreeNode::getBestThreat()
{
    ThreatInfo tempThreat(0, 0), best(0, 0);
    if (lastStep.getColor() == playerColor)//��ʼ��best
    {
        best.HighestScore = 500000;
        best.totalScore = 500000;
    }

    if (lastStep.getColor() != playerColor && childs.size() == 0)//Ҷ�ӽڵ���AI
    {
        return ThreatInfo(getTotal(-lastStep.getColor()), getHighest(-lastStep.getColor()));//ȡ��player����
    }

    if (lastStep.getColor() == playerColor && childs.size() == 0)//Ҷ�ӽڵ���player,��ʾ��ǰ����,���ȡʤ,����һ������AI
    {
        return ThreatInfo(getTotal(lastStep.getColor()), getHighest(lastStep.getColor()));//ȡ��player����
    }

    for (size_t i = 0; i < childs.size(); ++i)
    {
        if ((lastStep.getColor() != playerColor))//AI�ڵ�
        {
            tempThreat = childs[i]->getBestThreat();//�ݹ�
            if (tempThreat.totalScore > best.totalScore)//bestԭ��:player�¹��Ľڵ�player�÷�Խ��Խ��(Ĭ��player�����ŵ�)
            {
                if (tempThreat.HighestScore > best.HighestScore / 2)
                {
                    best = tempThreat;
                }
            }
        }
        else//player�ڵ�
        {
            tempThreat = childs[i]->getBestThreat();//child��AI�ڵ�
            if (tempThreat.totalScore < best.totalScore)//bestԭ��:AI�¹��Ľڵ�player�÷�ԽСԽ��
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

void GameTreeNode::buildAllChild()
{
    ChessBoard tempBoard;
    GameTreeNode *tempNode;
    int score;
    for (int i = 0; i < BOARD_ROW_MAX; ++i)
    {
        for (int j = 0; j < BOARD_COL_MAX; ++j)
        {
            if (currentBoard->getPiece(i, j).hot && currentBoard->getPiece(i, j).state == 0)
            {
                tempBoard = *currentBoard;
                //score = currentBoard->getPiece(i, j).getThreat(-side);
                tempBoard.doNextStep(i, j, -lastStep.getColor());
                tempBoard.updateThreat();
                score = tempBoard.getLastStepScores(false);
                tempNode = new GameTreeNode(&tempBoard, depth - 1, tempdepth, score);
                addChild(tempNode);
            }
        }
    }
}

int GameTreeNode::searchBest2(bool *hasSearch, ThreatInfo *threatInfo)
{
    size_t searchNum = 10;
    ThreadPool pool(7);
    pool.start();
    sort(sortList, 0, childs.size() - 1);
    while (true)
    {
        if (hasSearch[sortList[childs.size() - 1].key])//����������ĵ÷ֲ��������֮ǰ�ߣ����Ŀǰ�÷���ߵ��Ѿ������������ˣ��ٽ�������Ҳ�����ҵ��÷ֱ������ߵ���
        {
            //��ֹ������㷨
            //���һ�����������������ڶ���δ���������ҵ÷ֵ������һ�����и���ѡȡ�������ڶ�����������������ڶ�������������Ȼ��С�����һ��
            int i = childs.size() - 1;
            while (i > 0 && sortList[i - 1].value == sortList[i].value)
            {
                i--;
                if (!hasSearch[sortList[i].key])//��ȵ÷�����û��������
                {
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

        int tasknum = 0;
        for (size_t i = childs.size() - searchNum; i < childs.size(); i++)
        {
            if (!hasSearch[sortList[i].key])
            {
                Task t;
                t.index = sortList[i].key;
                //t.hasSearch = hasSearch;
                t.node = childs[sortList[i].key];
                pool.run(t);
                //tasknum++;
                //if (tasknum > 5)
                //{
                //    //�ȴ��߳�
                //    while (true)
                //    {
                //        if (pool.getTaskNum() == 0 && pool.getWorkNum() == 0)
                //        {
                //            break;
                //        }
                //        this_thread::sleep_for(std::chrono::milliseconds(100));
                //    }
                //    tasknum = 0;
                //}
            }
        }

        //�ȴ��߳�
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
                hasSearch[sortList[i].key] = true;//hasSearchֵ��buildSortListInfo��Ӱ��
                threatInfo[sortList[i].key] = childs[sortList[i].key]->getBestThreat();
                childs[sortList[i].key]->deleteChild();
                buildSortListInfo(i, threatInfo, hasSearch);
            }
        }
        sort(sortList, 0, childs.size() - 1);
        searchNum += 10;
    }
    pool.stop();
    //�����
    int i = childs.size() - 1;
    while (i > 0 && sortList[i - 1].value == sortList[i].value) i--;
    return i + rand() % (childs.size() - i);
}

//���߳�

void GameTreeNode::buildTreeThreadFunc(int n, ThreatInfo *threatInfo, GameTreeNode *child)
{
    child->buildPlayer();
    threatInfo[sortList[n].key] = child->getBestThreat();
    delete child;
}

int GameTreeNode::searchBest(bool *hasSearch, ThreatInfo *threatInfo)
{
    size_t searchNum = 10;
    thread buildTreeThread[MAXTHREAD];
    sort(sortList, 0, childs.size() - 1);
    while (true)
    {
        if (hasSearch[sortList[childs.size() - 1].key])//����������ĵ÷ֲ��������֮ǰ�ߣ����Ŀǰ�÷���ߵ��Ѿ������������ˣ��ٽ�������Ҳ�����ҵ��÷ֱ������ߵ���
        {
            break;
        }

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

    //�����
    int i = childs.size() - 1;
    while (i > 0 && sortList[i - 1].value == sortList[i].value) i--;
    return i + rand() % (childs.size() - i);
}

Position GameTreeNode::getBestStep()
{
    bool needSearch = true;
    int bestPos;
    buildAllChild();
    bool *hasSearch = new bool[childs.size()];
    ThreatInfo *threatInfo = new ThreatInfo[childs.size()];
    sortList = new ChildInfo[childs.size()];

    for (size_t i = 0; i < childs.size(); ++i)
    {
        hasSearch[i] = false;
        sortList[i].key = i;
        buildSortListInfo(i, threatInfo, hasSearch);
        if (childs[i]->currentScore >= SCORE_5_CONTINUE)
        {
            bestPos = i;
            needSearch = false;
        }
        else if (childs[i]->currentScore >= SCORE_4_DOUBLE && childs[i]->getHighest(-childs[i]->lastStep.getColor()) < SCORE_5_CONTINUE)
        {
            bestPos = i;
            needSearch = false;
        }
        else if (childs[i]->currentScore < 0)
        {
            sortList[i].value -= SCORE_5_CONTINUE;//��֤���ֲ���
        }
    }


    int specialAtackStep = getSpecialAtack();//��ʱ��δ����sortList[specialAtackStep].key = specialAtackStep
    if (specialAtackStep >= 0)
    {
        //int specialindex;
        //for (size_t i = 0; i < childs.size(); ++i)
        //{
        //    if (specialAtackStep == sortList[i].key)
        //    {
        //        specialindex = i; break;
        //    }
        //}
        if (!hasSearch[specialAtackStep])//���û������������������������ֹ����ʧ��
        {
            childs[specialAtackStep]->buildPlayer();
            hasSearch[specialAtackStep] = true;
            threatInfo[specialAtackStep] = childs[specialAtackStep]->getBestThreat();
            buildSortListInfo(specialAtackStep, threatInfo, hasSearch);
            childs[specialAtackStep]->deleteChild();
        }
    }

    int planB = getAtack();
    /*int planBindex;

    for (size_t i = 0; i < childs.size(); ++i)
    {
        if (planB == sortList[i].key)
        {
            planB = i; break;
        }
    }*/
    if (!hasSearch[planB])//���û������������������������ֹ����ʧ��
    {
        childs[planB]->buildPlayer();
        hasSearch[planB] = true;
        threatInfo[planB] = childs[planB]->getBestThreat();
        //childs[sortList[planB].key]->printTree();
        buildSortListInfo(planB, threatInfo, hasSearch);
        childs[planB]->deleteChild();
    }

    if (needSearch)
    {
        bestPos = multiThread ? searchBest2(hasSearch, threatInfo) : searchBest(hasSearch, threatInfo);
    }

    Position result;



    //childs[39]->buildPlayer();
    //hasSearch[39] = true;
    //threatInfo[39] = childs[39]->getBestThreat();
    //childs[39]->printTree();
    //childs[39]->deleteChild();

    if (childs[planB]->currentScore >= SCORE_5_CONTINUE ||
        (childs[planB]->currentScore >= 10000 && childs[planB]->getHighest(lastStep.getColor()) < SCORE_5_CONTINUE && threatInfo[planB].HighestScore < SCORE_5_CONTINUE))
    {
        result = Position{ childs[planB]->lastStep.row, childs[planB]->lastStep.col };
    }
    else if (playerColor == STATE_CHESS_BLACK && lastStep.step < 20)//��ֹ���ֱ�����
    {
        planB = getDefense();
        result = Position{ childs[planB]->lastStep.row, childs[planB]->lastStep.col };
    }
    else if (threatInfo[sortList[bestPos].key].HighestScore > 80000 ||
        (threatInfo[sortList[bestPos].key].HighestScore >= 10000 && (childs[sortList[bestPos].key]->currentScore <= 1200 || (childs[sortList[bestPos].key]->currentScore >= 8000 && childs[sortList[bestPos].key]->currentScore < 10000))))
    {
        if (specialAtackStep > -1 && childs[specialAtackStep]->currentScore > 1200 && childs[specialAtackStep]->getHighest(lastStep.getColor()) < SCORE_5_CONTINUE)
        {
            result = Position{ childs[specialAtackStep]->lastStep.row, childs[specialAtackStep]->lastStep.col };
        }
        else
        {
            planB = getDefense();//������������ҵ�����ȥ��
            if (currentBoard->getPiece(childs[planB]->lastStep.row, childs[planB]->lastStep.col).getThreat(lastStep.getColor()) > 2000)
                result = Position{ childs[planB]->lastStep.row, childs[planB]->lastStep.col };
            else
                result = Position{ childs[sortList[bestPos].key]->lastStep.row, childs[sortList[bestPos].key]->lastStep.col };
        }
    }
    else if (specialAtackStep > -1 && threatInfo[specialAtackStep].HighestScore < SCORE_3_DOUBLE)//add at 17.3.23 ��ֹ����ʧ��
    {
        result = Position{ childs[specialAtackStep]->lastStep.row, childs[specialAtackStep]->lastStep.col };
    }
    else if (threatInfo[planB].HighestScore <= SCORE_3_DOUBLE && childs[planB]->currentScore > 1000)
    {
        result = Position{ childs[planB]->lastStep.row, childs[planB]->lastStep.col };
    }
    else
    {
        result = Position{ childs[sortList[bestPos].key]->lastStep.row, childs[sortList[bestPos].key]->lastStep.col };
    }

    delete[] sortList;
    sortList = 0;
    delete[] threatInfo;
    delete[] hasSearch;
    return result;
}

int GameTreeNode::getAtack()
{
    int max = INT_MIN, flag = 0, temp;
    for (size_t i = 0; i < childs.size(); ++i)
    {
        temp = currentBoard->getPiece(childs[i]->lastStep.row, childs[i]->lastStep.col).getThreat(lastStep.getColor()) / 50;
        if (childs[i]->currentScore + temp > max)
        {
            max = childs[i]->currentScore + temp;
            flag = i;
        }
    }
    return flag;
}

int GameTreeNode::getSpecialAtack()
{
    if (getHighest(lastStep.getColor()) >= SCORE_5_CONTINUE)
        return -1;
    int max = 3000, flag = -1, temp;
    for (size_t i = 0; i < childs.size(); ++i)
    {
        if (childs[i]->currentScore >= 1210 && childs[i]->currentScore < 2000 /*||
            (getHighest(side) < 10000 && childs[i]->currentScore < 1210 && childs[i]->currentScore>1000)*/)
        {
            ChessBoard tempboard = *childs[i]->currentBoard;
            tempboard.setGlobalThreat(false);//����Ȩ��
            temp = tempboard.getAtackScore(childs[i]->currentScore, getTotal(lastStep.getColor()));
            if (temp > max)
            {
                max = temp;
                flag = i;
            }
            if (temp < 3000)//��С��������ĵ����ȼ�
            {
                if (currentBoard->getPiece(childs[i]->lastStep.row, childs[i]->lastStep.col).getThreat(lastStep.getColor()) < 8000)
                {
                    //childs[i]->currentScore -= 10000;
                    childs[i]->currentScore = 111; //modify at 17.3.23
                }
            }//���ܳ���Ȩ��BUG
        }
    }
    return flag;
}

int GameTreeNode::getDefense()
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
        temp = currentBoard->getPiece(childs[i]->lastStep.row, childs[i]->lastStep.col).getThreat(-lastStep.getColor()) / 50;
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

void GameTreeNode::buildSortListInfo(int n, ThreatInfo *threatInfo, bool *hasSearch)
{
    int i = sortList[n].key;
    if (!hasSearch[i])
    {
        sortList[n].value = childs[i]->currentScore - childs[i]->getHighest(lastStep.getColor()) - childs[i]->getTotal(lastStep.getColor()) / 10;
        if (sortList[n].value <= -SCORE_5_CONTINUE)//���Ӷ³��ĵ����ȼ�
        {
            sortList[n].value -= SCORE_5_CONTINUE;
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

int GameTreeNode::findWorstChild()
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

void GameTreeNode::buildPlayer(bool recursive)//�úøĸ�
{
    if (getHighest(-lastStep.getColor()) >= SCORE_5_CONTINUE)
    {
        delete currentBoard;
        currentBoard = 0;
        return;
    }
    else if (getHighest(-lastStep.getColor()) >= 10000 && getHighest(lastStep.getColor()) < SCORE_5_CONTINUE)
    {
        if (getHighest(-lastStep.getColor()) < 12000)
        {
            ChessBoard tempBoard;
            GameTreeNode *tempNode;
            int score;
            for (int i = 0; i < BOARD_ROW_MAX; ++i)
            {
                for (int j = 0; j < BOARD_COL_MAX; ++j)
                {
                    if (currentBoard->getPiece(i, j).hot && currentBoard->getPiece(i, j).state == 0)
                    {
                        if (currentBoard->getPiece(i, j).getThreat(-lastStep.getColor()) >= 10000)
                        {
                            score = currentBoard->getPiece(i, j).getThreat(-lastStep.getColor());
                            tempBoard = *currentBoard;
                            tempBoard.doNextStep(i, j, -lastStep.getColor());
                            tempBoard.updateThreat();
                            tempNode = new GameTreeNode(&tempBoard, depth, tempdepth, score);//AI�ż�1
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
        if (getHighest(lastStep.getColor()) >= SCORE_5_CONTINUE)
        {
            ChessBoard tempBoard;
            GameTreeNode *tempNode;
            int score;
            for (int i = 0; i < BOARD_ROW_MAX; ++i)
            {
                for (int j = 0; j < BOARD_COL_MAX; ++j)
                {
                    if (currentBoard->getPiece(i, j).hot && currentBoard->getPiece(i, j).state == 0)
                    {
                        if (currentBoard->getPiece(i, j).getThreat(lastStep.getColor()) >= SCORE_5_CONTINUE)
                        {
                            score = currentBoard->getPiece(i, j).getThreat(-lastStep.getColor());
                            tempBoard = *currentBoard;
                            tempBoard.doNextStep(i, j, -lastStep.getColor());
                            tempBoard.updateThreat();
                            tempNode = new GameTreeNode(&tempBoard, depth, tempdepth, score);//AI�ż�1
                            addChild(tempNode);
                        }
                    }
                }
            }
        }
        else if (getHighest(-lastStep.getColor()) > 99 && getHighest(-lastStep.getColor()) < 10000)
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
                    if (currentBoard->getPiece(i, j).hot && currentBoard->getPiece(i, j).state == 0)
                    {
                        score = currentBoard->getPiece(i, j).getThreat(-lastStep.getColor());//player
                        if (score > 99 && score < 10000)
                        {
                            tempBoard = *currentBoard;
                            tempBoard.doNextStep(i, j, -lastStep.getColor());
                            tempBoard.updateThreat();
                            highTemp = tempBoard.getThreatInfo(lastStep.getColor()).HighestScore;//AI

                            if (highTemp >= SCORE_5_CONTINUE) continue;
                            else if (highTemp >= 8000 && (score > 1100 || score < 900)) continue;

                            if (childs.size() > 4)
                            {
                                tempInfo = tempBoard.getThreatInfo(-lastStep.getColor());
                                worst = findWorstChild();
                                if (childs[worst]->getTotal(-lastStep.getColor()) < tempInfo.totalScore)
                                {
                                    delete childs[worst];
                                    childs[worst] = new GameTreeNode(&tempBoard, depth, tempdepth, score);
                                }
                            }
                            else
                            {
                                tempNode = new GameTreeNode(&tempBoard, depth, tempdepth, score);//AI�ż�1
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
            GameTreeNode *tempNode;
            int score;
            for (int i = 0; i < BOARD_ROW_MAX; ++i)
            {
                for (int j = 0; j < BOARD_COL_MAX; ++j)
                {
                    if (currentBoard->getPiece(i, j).hot && currentBoard->getPiece(i, j).state == 0)
                    {
                        score = currentBoard->getPiece(i, j).getThreat(-lastStep.getColor());
                        if (score >= 10000)
                        {
                            tempBoard = *currentBoard;
                            tempBoard.doNextStep(i, j, -lastStep.getColor());
                            tempBoard.updateThreat();
                            tempNode = new GameTreeNode(&tempBoard, depth, tempdepth, score);//AI�ż�1
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
    if (recursive)//��Ҫ�ݹ�
    {
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
    }
    delete currentBoard;
    currentBoard = 0;
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

void GameTreeNode::buildAI(bool recursive)
{
    ChessBoard tempBoard;
    GameTreeNode *tempNode;
    int score;
    int highest = getHighest(-lastStep.getColor());

    if (getHighest(-lastStep.getColor()) >= SCORE_5_CONTINUE)
    {

        for (int m = 0; m < BOARD_ROW_MAX; ++m)
        {
            for (int n = 0; n < BOARD_COL_MAX; ++n)
            {
                if (currentBoard->getPiece(m, n).hot && currentBoard->getPiece(m, n).state == 0)
                {
                    if (currentBoard->getPiece(m, n).getThreat(-lastStep.getColor()) >= SCORE_5_CONTINUE)
                    {
                        tempBoard = *currentBoard;
                        score = currentBoard->getPiece(m, n).getThreat(-lastStep.getColor());
                        tempBoard.doNextStep(m, n, -lastStep.getColor());
                        tempBoard.updateThreat();
                        tempNode = new GameTreeNode(&tempBoard, 0, 0, score);
                        if (lastStep.getColor() == 1)
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
    else if (getHighest(-lastStep.getColor()) >= 10000 && getHighest(lastStep.getColor()) < SCORE_5_CONTINUE)// >=10000 ��ȷ��������-�������
    {
        for (int m = 0; m < BOARD_ROW_MAX; ++m)
        {
            for (int n = 0; n < BOARD_COL_MAX; ++n)
            {
                if (currentBoard->getPiece(m, n).hot && currentBoard->getPiece(m, n).state == 0)
                {
                    if (currentBoard->getPiece(m, n).getThreat(-lastStep.getColor()) >= 10000)
                    {
                        tempBoard = *currentBoard;
                        score = currentBoard->getPiece(m, n).getThreat(-lastStep.getColor());
                        tempBoard.doNextStep(m, n, -lastStep.getColor());
                        tempBoard.updateThreat();
                        tempNode = new GameTreeNode(&tempBoard, 0, 0, score);
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
            if (currentBoard->getPiece(m, n).hot && currentBoard->getPiece(m, n).state == 0)
            {
                if (currentBoard->getPiece(m, n).getThreat(lastStep.getColor()) >= SCORE_5_CONTINUE)
                {
                    tempBoard = *currentBoard;
                    score = currentBoard->getPiece(m, n).getThreat(-lastStep.getColor());
                    tempBoard.doNextStep(m, n, -lastStep.getColor());
                    tempBoard.updateThreat();
                    tempNode = new GameTreeNode(&tempBoard, tempdepth > 0 ? depth : depth - 1, tempdepth > 0 ? tempdepth - 1 : tempdepth, score);//flag high-1
                    addChild(tempNode);
                    goto end;
                }
                else if (currentBoard->getPiece(m, n).getThreat(lastStep.getColor()) >= 10000 &&
                    highest < SCORE_5_CONTINUE)
                {
                    tempBoard = *currentBoard;
                    score = currentBoard->getPiece(m, n).getThreat(-lastStep.getColor());
                    tempBoard.doNextStep(m, n, -lastStep.getColor());
                    tempBoard.updateThreat();
                    tempNode = new GameTreeNode(&tempBoard, depth - 1, tempdepth, score);
                    addChild(tempNode);
                }
                //else if (getHighest(side) < 100000 && getHighest(side)>=10000)//�������Ƿ���
                //{
                //	if (currentBoard->getPiece(m, n).getThreat(-side)>900 && currentBoard->getPiece(m, n).getThreat(-side) < 1100)
                //	{
                //		tempBoard = *currentBoard;
                //		score = currentBoard->getPiece(m, n).getThreat(-side);
                //		tempBoard.doNextStep(m, n, -side);
                //		tempBoard.updateThreat(ban);
                //		tempNode = new TreeNode(tempBoard, temphigh>0 ? high : high - 1, temphigh - 1, score);//����
                //		addChild(tempNode);
                //	}
                //}
                //else if (getHighest(side) < 10000)
                //{
                //	if (currentBoard->getPiece(m, n).getThreat(-side)>1199 && currentBoard->getPiece(m, n).getThreat(-side) < 1300)//����û����
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
    if (0 == childs.size())//�п�����δ֪������
    {
        int best = 0;
        int i, j;
        for (int m = 0; m < BOARD_ROW_MAX; ++m)
        {
            for (int n = 0; n < BOARD_COL_MAX; ++n)
            {
                if (currentBoard->getPiece(m, n).hot && currentBoard->getPiece(m, n).state == 0)
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
        tempBoard.updateThreat();
        tempNode = new GameTreeNode(&tempBoard, depth - 1, tempdepth, score);
        addChild(tempNode);
    }
end:
    if (recursive)//��Ҫ�ݹ�
    {
        for (size_t i = 0; i < childs.size(); i++)
        {
            childs[i]->buildPlayer();
        }
    }
    delete currentBoard;
    currentBoard = 0;
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