#include "GameTree.h"
#include "utils.h"
#include "ThreadPool.h"
#include <thread>

static int countTreeNum = 0;
int8_t GameTreeNode::playerColor = 1;
bool GameTreeNode::multiThread = true;
size_t GameTreeNode::maxTaskNum = 0;
int GameTreeNode::bestRating = 0;
vector<map<string, GameTreeNode*>> GameTreeNode::historymaps(0);

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

GameTreeNode::GameTreeNode(ChessBoard *board, int depth, int tempdepth) :
    depth(depth), extraDepth(tempdepth)
{
    this->chessBoard = new ChessBoard;
    *(this->chessBoard) = *board;
    lastStep = chessBoard->lastStep;
    RatingInfo black = chessBoard->getRatingInfo(1);
    RatingInfo white = chessBoard->getRatingInfo(-1);
    this->blackThreat = black.totalScore;
    this->whiteThreat = white.totalScore;
    this->blackHighest = black.highestScore;
    this->whiteHighest = white.highestScore;
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
    depth = other.depth;
    extraDepth = other.extraDepth;
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
    delete chessBoard;
    chessBoard = 0;
}

void GameTreeNode::buildFirstChilds()
{
    //build AI step
    ChessBoard tempBoard;
    GameTreeNode *tempNode;
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
                        tempBoard = *chessBoard;
                        //score = chessBoard->getPiece(i, j).getThreat(-playerColor);
                        tempBoard.doNextStep(i, j, -playerColor);
                        tempBoard.updateThreat();
                        tempNode = new GameTreeNode(&tempBoard, depth - 1, extraDepth);
                        addChild(tempNode);
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
                        tempBoard = *chessBoard;
                        tempBoard.doNextStep(i, j, -playerColor);
                        tempBoard.updateThreat();
                        tempNode = new GameTreeNode(&tempBoard, depth - 1, extraDepth);//flag high-1
                        addChild(tempNode);
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
                    tempBoard = *chessBoard;
                    tempBoard.doNextStep(i, j, -lastStep.getColor());
                    tempBoard.updateThreat();
                    tempNode = new GameTreeNode(&tempBoard, depth - 1, extraDepth);
                    addChild(tempNode);
                }
            }
        }
    }
    
}

int GameTreeNode::searchBest2(ChildInfo *threatInfo, SortInfo *sortList)
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
        if (threatInfo[sortList[childs.size() - 1].key].hasSearch)//����������ĵ÷ֲ��������֮ǰ�ߣ����Ŀǰ�÷���ߵ��Ѿ������������ˣ��ٽ�������Ҳ�����ҵ��÷ֱ������ߵ���
        {
            //��ֹ������㷨����
            //���һ�����������������ڶ���δ���������ҵ÷ֵ������һ�����и���ѡȡ�������ڶ���
            int i = childs.size() - 1;
            while (i > 0 && sortList[i - 1].value == sortList[i].value)
            {
                i--;
                if (!threatInfo[sortList[i].key].hasSearch)//��ȵ÷�����û��������
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

        //�ȴ��߳�
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
                threatInfo[sortList[i].key].hasSearch = true;//hasSearchֵ��buildSortListInfo��Ӱ��
                //threatInfo[sortList[i].key] = childs[sortList[i].key]->getBestThreat();
                //childs[sortList[i].key]->deleteChild();
                buildSortListInfo(i, threatInfo, sortList);
            }
        }
        sort(sortList, 0, childs.size() - 1);
        searchNum += 10;
    }
    pool.stop();
    historymaps.clear();
    //�����
    int i = childs.size() - 1;
    while (i > 0 && sortList[i - 1].value == sortList[i].value) i--;
    return i + rand() % (childs.size() - i);
}

//���߳�

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
        if (threatInfo[sortList[childs.size() - 1].key].hasSearch)//����������ĵ÷ֲ��������֮ǰ�ߣ����Ŀǰ�÷���ߵ��Ѿ������������ˣ��ٽ�������Ҳ�����ҵ��÷ֱ������ߵ���
        {
            //��ֹ������㷨����
            //���һ�����������������ڶ���δ���������ҵ÷ֵ������һ�����и���ѡȡ�������ڶ���
            int i = childs.size() - 1;
            while (i > 0 && sortList[i - 1].value == sortList[i].value)
            {
                i--;
                if (!threatInfo[sortList[i].key].hasSearch)//��ȵ÷�����û��������
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

    //�����
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

    for (size_t i = 0; i < childs.size(); ++i)//��ʼ����˳���ҳ��������
    {
        score = childs[i]->chessBoard->getLastStepScores(false);
        if (score >= SCORE_5_CONTINUE)
        {
            result = Position{ childs[i]->lastStep.row, childs[i]->lastStep.col };
            goto endsearch;
        }
        /*else if (childs[i]->lastStepScore >= SCORE_4_DOUBLE && childs[i]->getHighest(-childs[i]->lastStep.getColor()) < SCORE_5_CONTINUE)
        {
            result = Position{ childs[i]->lastStep.row, childs[i]->lastStep.col };
            goto endsearch;
        }*/
        else if (score < 0)
        {
            if (childs.size() == 1)//ֻ����һ��
            {
                result = Position{ childs[i]->lastStep.row, childs[i]->lastStep.col };
                goto endsearch;
            }
            childs.erase(childs.begin() + i);//��֤���ֲ���
            i--;
            continue;
            //sortList[i].value -= SCORE_5_CONTINUE;//��֤���ֲ���
        }
        childsInfo[i].hasSearch = false;
        childsInfo[i].lastStepScore = score;
        sortList[i].key = i;
        buildSortListInfo(i, childsInfo, sortList);
    }


    int specialAtackStep = getSpecialAtack(childsInfo);//��ʱ��δ����sortList[specialAtackStep].key = specialAtackStep
    if (specialAtackStep >= 0)
    {
        childs[specialAtackStep]->buildPlayer();
        childsInfo[specialAtackStep].hasSearch = true;
        childsInfo[specialAtackStep].rating = childs[specialAtackStep]->getBestRating();
        buildSortListInfo(specialAtackStep, childsInfo, sortList);
        childs[specialAtackStep]->deleteChild();
    }

    int atackChildIndex = getAtackChild(childsInfo);
    if (!childsInfo[atackChildIndex].hasSearch)
    {
        childs[atackChildIndex]->buildPlayer();
        childsInfo[atackChildIndex].hasSearch = true;
        childsInfo[atackChildIndex].rating = childs[atackChildIndex]->getBestRating();
        buildSortListInfo(atackChildIndex, childsInfo, sortList);
        childs[atackChildIndex]->deleteChild();
    }

    //��ʼ�������
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
    else if (playerColor == STATE_CHESS_BLACK && lastStep.step < 20)//��ֹ���ֱ�����
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
            atackChildIndex = getDefendChild();//������������ҵ�����ȥ��
            if (chessBoard->getPiece(childs[atackChildIndex]->lastStep.row, childs[atackChildIndex]->lastStep.col).getThreat(lastStep.getColor()) > 2000)
                result = Position{ childs[atackChildIndex]->lastStep.row, childs[atackChildIndex]->lastStep.col };
            else
                result = Position{ childs[sortList[bestSearchPos].key]->lastStep.row, childs[sortList[bestSearchPos].key]->lastStep.col };
        }
    }
    else if (specialAtackStep > -1 && childsInfo[specialAtackStep].rating.highestScore < SCORE_3_DOUBLE)//add at 17.3.23 ��ֹ����ʧ��
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
    //historymap->clear();
    //delete historymap;
    //historymap = NULL;
    return result;
}

RatingInfo GameTreeNode::getBestRating()
{
    if (childs.size() == 0)
    {
        if (lastStep.getColor() != playerColor)//Ҷ�ӽڵ���AI
        {
            return RatingInfo(getTotal(playerColor), getHighest(playerColor));//ȡ��player����
        }
        else//Ҷ�ӽڵ���player,��ʾ��ǰ����,AIȡʤ,����һ������AI
        {
            return RatingInfo(getTotal(playerColor), getHighest(playerColor));//ȡ��player����
        }
    }

    RatingInfo tempThreat;
    RatingInfo best = (lastStep.getColor() == playerColor) ? RatingInfo{500000, 500000} : RatingInfo{0,0};//��ʼ��best

    if (lastStep.getColor() != playerColor)//AI�ڵ�
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

void GameTreeNode::buildSortListInfo(int n, ChildInfo *childsInfo, SortInfo *sortList)
{
    int i = sortList[n].key;
    if (!childsInfo[i].hasSearch)
    {
        sortList[n].value = childsInfo[i].lastStepScore - childs[i]->getHighest(lastStep.getColor()) - childs[i]->getTotal(lastStep.getColor()) / 10;
        if (sortList[n].value <= -SCORE_5_CONTINUE)//���Ӷ³��ĵ����ȼ�
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
            tempboard.setGlobalThreat(false);//����Ȩ��
            temp = tempboard.getAtackScore(childsInfo[i].lastStepScore, getTotal(lastStep.getColor()));
            if (temp > max)
            {
                max = temp;
                flag = i;
            }
            if (temp < 3000)//��С��������ĵ����ȼ�
            {
                if (chessBoard->getPiece(childs[i]->lastStep.row, childs[i]->lastStep.col).getThreat(lastStep.getColor()) < 8000)
                {
                    //childs[i]->currentScore -= 10000;
                    childsInfo[i].lastStepScore = 123; //modify at 17.3.23
                }
            }//���ܳ���Ȩ��BUG(�Ѿ����֣�3.23�޸�)
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

void GameTreeNode::buildPlayer(bool recursive)//�úøĸ�
{
    //lastStep.getColor()��AI�µ�
    if (getHighest(playerColor) >= SCORE_5_CONTINUE)
    {
        goto end;
    }

    if (depth > 0)
    {
        if (getHighest(-playerColor) >= SCORE_5_CONTINUE)
        {
            ChessBoard tempBoard;
            GameTreeNode *tempNode;
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
                            tempBoard = *chessBoard;
                            tempBoard.doNextStep(i, j, playerColor);
                            tempBoard.updateThreat();
                            tempNode = new GameTreeNode(&tempBoard, depth, extraDepth);//AI�ż�1
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
                                    childs[worst] = new GameTreeNode(&tempBoard, depth, extraDepth);
                                }
                            }
                            else
                            {
                                tempNode = new GameTreeNode(&tempBoard, depth, extraDepth);//AI�ż�1
                                addChild(tempNode);
                            }
                        }
                    }
                }
            }
        }
        else if (getHighest(playerColor) >= SCORE_4_DOUBLE)
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
                        if (score >= SCORE_4_DOUBLE)
                        {
                            tempBoard = *chessBoard;
                            tempBoard.doNextStep(i, j, playerColor);
                            tempBoard.updateThreat();
                            tempNode = new GameTreeNode(&tempBoard, depth, extraDepth);//AI�ż�1
                            addChild(tempNode);
                        }
                    }
                }
            }
        }
    }

end:
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
    //player�ڵ�
    ChessBoard tempBoard;
    GameTreeNode *tempNode;
    int score;
    int highest = getHighest(-playerColor);
    //����
    if (getHighest(-playerColor) >= SCORE_5_CONTINUE)
    {
        if (playerColor == 1)
        {
            blackHighest = -SCORE_5_CONTINUE;
            blackThreat = -SCORE_5_CONTINUE;
        }
        else
        {
            whiteHighest = -SCORE_5_CONTINUE;
            whiteThreat = -SCORE_5_CONTINUE;
        }
        goto end;
    }
    else if (getHighest(-playerColor) >= SCORE_4_DOUBLE && getHighest(playerColor) < SCORE_5_CONTINUE)// playerColor û�м����γɵ�����������ȥ��ɱ
    {
        for (int i = 0; i < BOARD_ROW_MAX; ++i)
        {
            for (int j = 0; j < BOARD_COL_MAX; ++j)
            {
                if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
                {
                    if (chessBoard->getPiece(i, j).getThreat(-playerColor) >= SCORE_4_DOUBLE && chessBoard->getPiece(i, j).getThreat(playerColor)<SCORE_4_DOUBLE)//��ֹ�ͺ����ظ�
                    {
                        tempBoard = *chessBoard;
                        //score = chessBoard->getPiece(i, j).getThreat(-playerColor);
                        tempBoard.doNextStep(i, j, -playerColor);
                        tempBoard.updateThreat();
                        tempNode = new GameTreeNode(&tempBoard, depth - 1, extraDepth);
                        addChild(tempNode);
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
                        tempBoard = *chessBoard;
                        score = chessBoard->getPiece(i, j).getThreat(-playerColor);
                        if (score < 0)//��������
                        {
                            goto end;//�����֣���������
                        }
                        tempBoard.doNextStep(i, j, -playerColor);
                        tempBoard.updateThreat();
                        tempNode = new GameTreeNode(&tempBoard, extraDepth > 0 ? depth : depth - 1, extraDepth > 0 ? extraDepth - 1 : extraDepth);//flag high-1
                        addChild(tempNode);
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
                        tempBoard = *chessBoard;
                        score = chessBoard->getPiece(i, j).getThreat(-playerColor);
                        if (score < 0)//��������
                        {
                            continue;
                        }
                        tempBoard.doNextStep(i, j, -playerColor);
                        tempBoard.updateThreat();
                        tempNode = new GameTreeNode(&tempBoard, depth - 1, extraDepth);
                        addChild(tempNode);
                    }
                }
            }
        }
    }

end:
    if (recursive)//��Ҫ�ݹ�
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