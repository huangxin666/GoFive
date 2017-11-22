#include "GameTree.h"
#include "ThreadPool.h"

ChildInfo *GameTreeNode::childsInfo = NULL;
GAME_RULE GameTreeNode::ban = FREESTYLE;
bool GameTreeNode::extraSearch = true;
uint8_t GameTreeNode::playerColor = 1;
bool GameTreeNode::enableAtack = true;
AIRESULTFLAG GameTreeNode::resultFlag = AIRESULTFLAG_NORMAL;
uint16_t GameTreeNode::startStep = 0;
uint8_t GameTreeNode::maxSearchDepth = 0;
uint8_t GameTreeNode::transTableMaxDepth = 0;
size_t GameTreeNode::maxTaskNum = 0;
int GameTreeNode::bestRating = 0;
int GameTreeNode::bestIndex = -1;
trans_table GameTreeNode::transTable_atack(0);
HashStat GameTreeNode::transTableHashStat = { 0,0,0 };

GameTreeNode::GameTreeNode()
{

}

GameTreeNode::GameTreeNode(ChessBoard *board)
{
    this->chessBoard = new ChessBoard;
    *(this->chessBoard) = *board;
    lastStep = board->getLastStep();
    black.highestScore = ChessBoard::getChessTypeInfo(chessBoard->getHighestType(PIECE_BLACK)).rating;
    black.totalScore = chessBoard->getSimpleTotalScore(PIECE_BLACK);
    white.highestScore = ChessBoard::getChessTypeInfo(chessBoard->getHighestType(PIECE_WHITE)).rating;
    white.totalScore = chessBoard->getSimpleTotalScore(PIECE_WHITE);
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
    alpha = other.alpha;
    beta = other.beta;
    return *this;
}

void GameTreeNode::initTree(uint8_t maxDepth, bool multiThread, bool extra, GAME_RULE ban)
{
    //init static param
    GameTreeNode::ban = ban;
    extraSearch = extra;
    enableAtack = multiThread;
    maxSearchDepth = maxDepth;
    transTableMaxDepth = maxSearchDepth > 1 ? maxSearchDepth - 1 : 0;

    transTableHashStat = { 0,0,0 };
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
    tempBoard.move(Position(row, col), ban);
    GameTreeNode *tempNode = new GameTreeNode(&tempBoard);
    tempNode->alpha = alpha;
    tempNode->beta = beta;
    childs.push_back(tempNode);
}

void GameTreeNode::buildAllChilds()
{
    //build AI step
    if (getHighest(Util::otherside(playerColor)) >= CHESSTYPE_5_SCORE)
    {
        ForEachPosition
        {
            if (chessBoard->canMove(pos.row, pos.col))
            {
                if (chessBoard->getChessType(pos.row, pos.col, Util::otherside(playerColor)) == CHESSTYPE_5)
                {
                    createChildNode(pos.row, pos.col);
                }
            }
        }

    }
    else if (getHighest(playerColor) >= CHESSTYPE_5_SCORE)
    {
        ForEachPosition
        {
            if (chessBoard->canMove(pos.row, pos.col))
            {
                if (chessBoard->getChessType(pos.row, pos.col, playerColor) == CHESSTYPE_5)//堵player即将形成的五连
                {
                    createChildNode(pos.row, pos.col);
                }
            }
        }

    }
    else
    {
        //int score;
        ForEachPosition
        {
            if (chessBoard->canMove(pos.row, pos.col) && chessBoard->useful(pos.row, pos.col))
            {
                createChildNode(pos.row, pos.col);
            }
        }
    }

}

int GameTreeNode::buildDefendSearchTree()
{
    for (size_t i = 0; i < childs.size(); i++)
    {
        if (!childsInfo[i].hasSearch)
        {
            childsInfo[i].hasSearch = true;
            if (chessBoard->getChessType(childs[i]->lastStep.pos, playerColor) < CHESSTYPE_J3
                && lastStep.step > 10)//active发现会输，才到这里，全力找防止失败的走法
            {
                continue;
            }
            TaskItems t;
            t.node = childs[i];
            t.index = (int)i;
            t.type = TASKTYPE_DEFEND;
            ThreadPool::getInstance()->run(bind(threadPoolWorkFunc, t));
        }
    }

    //等待线程
    ThreadPool::getInstance()->wait();

    return GameTreeNode::bestIndex;
}

Position GameTreeNode::getBestStep(uint8_t playercolor, uint16_t startstep)
{
    playerColor = playercolor;
    startStep = startstep;
    Position result;
    int bestDefendPos;
    buildAllChilds();
    childsInfo = new ChildInfo[childs.size()];
    int score;
    RatingInfoDenfend tempinfo;

    if (childs.size() == 1)//只有这一个
    {
        if ((resultFlag == AIRESULTFLAG_NEARWIN || resultFlag == AIRESULTFLAG_TAUNT) &&
            getHighest(Util::otherside(playerColor)) < CHESSTYPE_5_SCORE && getHighest(playerColor) >= CHESSTYPE_5_SCORE)//垂死冲四
        {
            resultFlag = AIRESULTFLAG_TAUNT;//嘲讽
        }
        else
        {
            resultFlag = AIRESULTFLAG_NORMAL;
        }

        result = Position{ childs[0]->lastStep.getRow(),childs[0]->lastStep.getCol() };
        goto endsearch;
    }

    for (size_t i = 0; i < childs.size(); ++i)//初始化，顺便找出特殊情况
    {
        score = ChessBoard::getChessTypeInfo(chessBoard->getChessType(childs[i]->lastStep.getRow(), childs[i]->lastStep.getCol(), childs[i]->lastStep.state)).rating;//进攻权重
        if (score >= CHESSTYPE_5_SCORE)
        {
            resultFlag = AIRESULTFLAG_WIN;
            result = Position{ childs[i]->lastStep.getRow(),childs[i]->lastStep.getCol() };
            goto endsearch;
        }
        else if (score < 0)
        {
            if (childs.size() == 1)//只有这一个,只能走禁手了
            {
                resultFlag = AIRESULTFLAG_FAIL;
                result = Position{ childs[i]->lastStep.getRow(),childs[i]->lastStep.getCol() };
                goto endsearch;
            }
            delete childs[i];
            childs.erase(childs.begin() + i);//保证禁手不走
            i--;
            continue;
        }
        childsInfo[i].hasSearch = false;
        childsInfo[i].lastStepScore = score;
    }

    if (extraSearch && enableAtack)
    {
        clearTransTable();
        GameTreeNode::bestRating = 100;//代表步数
        int atackSearchTreeResult = buildAtackSearchTree();
        if (atackSearchTreeResult > -1)
        {
            if (childsInfo[atackSearchTreeResult].lastStepScore < ChessBoard::getChessTypeInfo(CHESSTYPE_D4).rating
                || (childsInfo[atackSearchTreeResult].lastStepScore >= ChessBoard::getChessTypeInfo(CHESSTYPE_33).rating
                    && childsInfo[atackSearchTreeResult].lastStepScore < ChessBoard::getChessTypeInfo(CHESSTYPE_44).rating))
            {
                GameTreeNode* simpleSearchNode = new GameTreeNode();
                *simpleSearchNode = *childs[atackSearchTreeResult];
                simpleSearchNode->buildDefendTreeNodeSimple(4);//多算4步
                tempinfo = simpleSearchNode->getBestDefendRating(childsInfo[atackSearchTreeResult].lastStepScore);
                simpleSearchNode->deleteChilds();
                delete simpleSearchNode;
                if (tempinfo.rating.highestScore >= CHESSTYPE_5_SCORE)
                {
                    resultFlag = AIRESULTFLAG_COMPLAIN;
                    clearTransTable();
                    goto beginDefend;
                }
            }
            resultFlag = AIRESULTFLAG_NEARWIN;
            result = Position{ childs[atackSearchTreeResult]->lastStep.getRow(), childs[atackSearchTreeResult]->lastStep.getCol() };
            popHeadTransTable();
            popHeadTransTable();
            goto endsearch;
        }
    }

    resultFlag = AIRESULTFLAG_NORMAL;
    clearTransTable();

    GameTreeNode::bestRating = INT32_MIN;

    int activeChildIndex;
    if (lastStep.step > 10)
    {
        activeChildIndex = getActiveChild();
    }
    else
    {
        activeChildIndex = getDefendChild();
    }
    //activeChildIndex = 34;
    //避免被剪枝，先单独算
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


    if (childsInfo[activeChildIndex].rating.highestScore < ChessBoard::getChessTypeInfo(CHESSTYPE_33).rating)
    {
        //如果主动出击不会导致走向失败，则优先主动出击，开局10步内先不作死
        result = Position{ childs[activeChildIndex]->lastStep.getRow(), childs[activeChildIndex]->lastStep.getCol() };
        clearTransTable();
        goto endsearch;
    }
beginDefend:
    //开始深度搜索
    bestDefendPos = buildDefendSearchTree();

    //transpositionTable.clear();
    clearTransTable();

    if (childsInfo[bestDefendPos].rating.highestScore >= CHESSTYPE_5_SCORE)
    {
        if (resultFlag != AIRESULTFLAG_COMPLAIN)
        {
            resultFlag = AIRESULTFLAG_FAIL;
        }
        bestDefendPos = getDefendChild();
        result = Position{ childs[bestDefendPos]->lastStep.getRow(), childs[bestDefendPos]->lastStep.getCol() };
    }
    else
    {
        result = Position{ childs[bestDefendPos]->lastStep.getRow(), childs[bestDefendPos]->lastStep.getCol() };
    }
endsearch:
    delete[] childsInfo;
    childsInfo = NULL;
    return result;
}

int GameTreeNode::getActiveChild()
{
    int maxAI = INT_MIN, maxPlayer = INT_MIN;
    //int tempAI, tempPlayer;
    int temp;
    vector<int> results;
    for (size_t i = 0; i < childs.size(); ++i)
    {
        //ChessBoard tempBoard = *(childs[i]->chessBoard);

        //uint8_t highestPos = tempBoard.getHighestInfo(Util::otherside(playerColor)).index;
        //tempBoard.move(highestPos);
        //tempAI = tempBoard.getTotalRating(Util::otherside(playerColor));
        //tempAI = tempAI / 10 * 10;
        //tempPlayer = tempBoard.getTotalRating(playerColor);

        //tempAI = tempAI - tempPlayer / 10;
        ////temp = childsInfo[i].lastStepScore + childs[i]->getTotal(util::otherside(playerColor)) - childs[i]->getTotal(playerColor) / 10;
        ////if (temp >= SCORE_5_CONTINUE && childsInfo[i].lastStepScore > 1200 && childsInfo[i].lastStepScore < 1400)
        ////{
        ////    temp -= SCORE_5_CONTINUE;//降低无意义冲四的优先级
        ////}
        //if (tempAI > maxAI)
        //{
        //    results.clear();
        //    maxAI = tempAI;
        //    results.push_back((int)i);
        //}
        //else if (tempAI == maxAI)
        //{
        //    results.push_back((int)i);
        //}
        temp = chessBoard->getRelatedFactor(childs[i]->lastStep.pos, childs[i]->lastStep.state);
        if (temp > maxAI)
        {
            results.clear();
            maxAI = temp;
            results.push_back((int)i);
        }
        else if (temp == maxAI)
        {
            results.push_back((int)i);
        }
    }
    return results[rand() % results.size()];;
}

int GameTreeNode::getDefendChild()
{
    int best = INT_MIN, temp;
    vector<int> results;
    for (size_t i = 0; i < childs.size(); ++i)
    {
        //temp = childs[i]->getTotal(playerColor) + childsInfo[i].lastStepScore / 10;
        temp = chessBoard->getRelatedFactor(childs[i]->lastStep.pos, childs[i]->lastStep.getOtherSide(), true);
        if (temp > best)
        {
            results.clear();
            best = temp;
            results.push_back((int)i);
        }
        else if (temp == best)
        {
            results.push_back((int)i);
        }
    }
    return results[rand() % results.size()];
}

void GameTreeNode::buildDefendTreeNodeSimple(int deepen)
{
    RatingInfo info;
    if (lastStep.state == Util::otherside(playerColor))//build player
    {
        if (getDepth() >= maxSearchDepth + deepen) //除非特殊情况，保证最后一步是AI下的，故而=maxSearchDepth时就直接结束           
        {
            goto end;
        }
        if (getHighest(playerColor) >= CHESSTYPE_5_SCORE)
        {
            goto end;
        }
        else if (getHighest(Util::otherside(playerColor)) >= CHESSTYPE_5_SCORE)//防五连
        {
            ForEachPosition
            {
                if (chessBoard->canMove(pos.row, pos.col))
                {
                    if (chessBoard->getChessType(pos.row, pos.col, Util::otherside(playerColor)) == CHESSTYPE_5)
                    {
                        if (chessBoard->getChessType(pos.row, pos.col, playerColor) == CHESSTYPE_BAN)//player GG AI win
                        {
                            if (playerColor == PIECE_BLACK)
                            {
                                black = { -CHESSTYPE_5_SCORE , -CHESSTYPE_5_SCORE };
                            }
                            else
                            {
                                white = { -CHESSTYPE_5_SCORE , -CHESSTYPE_5_SCORE };
                            }
                            goto end;
                        }
                        createChildNode(pos.row, pos.col);
                        childs.back()->buildDefendTreeNodeSimple(deepen);
                        if (childs.back()->getBestDefendRating(deepen).rating.totalScore >= CHESSTYPE_5_SCORE)
                        {
                            goto end;
                        }
                    }
                }
            }
        }

        //进攻
        if (getHighest(playerColor) >= ChessBoard::getChessTypeInfo(CHESSTYPE_43).rating && getHighest(Util::otherside(playerColor)) < CHESSTYPE_5_SCORE)
        {
            ForEachPosition
            {
                if (chessBoard->canMove(pos.row, pos.col))
                {
                    if (ChessBoard::getChessTypeInfo(chessBoard->getChessType(pos.row, pos.col, playerColor)).rating >= ChessBoard::getChessTypeInfo(CHESSTYPE_43).rating)
                    {
                        createChildNode(pos.row, pos.col);
                        childs.back()->buildDefendTreeNodeSimple(deepen);
                        if (childs.back()->getBestDefendRating(deepen).rating.totalScore >= CHESSTYPE_5_SCORE)
                        {
                            goto end;
                        }
                    }
                }
            }

        }
        else if (getHighest(Util::otherside(playerColor)) < CHESSTYPE_5_SCORE)
        {
            ChessBoard tempBoard;
            GameTreeNode *tempNode;
            int score;
            ForEachPosition
            {
                if (chessBoard->canMove(pos.row, pos.col))
                {
                    score = ChessBoard::getChessTypeInfo(chessBoard->getChessType(pos.row, pos.col, playerColor)).rating;//player
                    if (score >= ChessBoard::getChessTypeInfo(CHESSTYPE_J3).rating)
                    {
                        tempBoard = *chessBoard;
                        tempBoard.move(pos, ban);
                        if (tempBoard.hasChessType(playerColor, CHESSTYPE_5))//冲四
                        {
                            tempNode = new GameTreeNode(&tempBoard);
                            tempNode->alpha = alpha;
                            tempNode->beta = beta;
                            childs.push_back(tempNode);
                            childs.back()->buildDefendTreeNodeSimple(deepen);
                            if (childs.back()->getBestDefendRating(deepen).rating.totalScore >= CHESSTYPE_5_SCORE)
                            {
                                goto end;
                            }
                        }
                    }
                }

            }
        }
    }
    else //build AI
    {
        //player节点
        //进攻
        if (getHighest(Util::otherside(playerColor)) >= CHESSTYPE_5_SCORE)//player GG AI win 
        {
            if (playerColor == PIECE_BLACK)
            {
                black = { -CHESSTYPE_5_SCORE , -CHESSTYPE_5_SCORE };
            }
            else
            {
                white = { -CHESSTYPE_5_SCORE , -CHESSTYPE_5_SCORE };
            }
            goto end;
        }

        //防守
        if (getHighest(playerColor) >= CHESSTYPE_5_SCORE)//堵playerd的冲四(即将形成的五连)
        {
            ForEachPosition
            {
                if (chessBoard->canMove(pos.row, pos.col))
                {
                    if (chessBoard->getChessType(pos.row, pos.col, playerColor) == CHESSTYPE_5)//堵player即将形成的五连
                    {
                        if (chessBoard->getChessType(pos.row, pos.col, Util::otherside(playerColor)) == CHESSTYPE_BAN)//被禁手了 AI gg
                        {
                            goto end;//被禁手，必输无疑
                        }
                        createChildNode(pos.row, pos.col);
                        childs.back()->buildDefendTreeNodeSimple(deepen);
                        goto end;//必堵，堵一个就行了，如果还有一个就直接输了
                    }
                }
            }
        }
    }
end:
    int a = 0;
    //delete chessBoard;
    //chessBoard = 0;
}

void GameTreeNode::buildDefendTreeNode(int basescore)
{
    /*if (black.highestScore == 1001 && black.totalScore == 2305 && white.highestScore == 1210 && white.totalScore == 6200)
    {
        int a  = 1;
    }*/
    RatingInfo info;
    if (lastStep.state == Util::otherside(playerColor))//build player
    {
        if ((getDepth() >= maxSearchDepth))//除非特殊情况，保证最后一步是AI下的，故而=maxSearchDepth时就直接结束
            //&& GameTreeNode::iterative_deepening)//GameTreeNode::iterative_deepening为false的时候可以继续迭代
            //|| getDepth() >= maxSearchDepth + 2)//最多24步
        {
            goto end;
        }
        if (getHighest(playerColor) >= CHESSTYPE_5_SCORE)
        {
            goto end;
        }
        else if (getHighest(Util::otherside(playerColor)) >= CHESSTYPE_5_SCORE)//防五连
        {
            ForEachPosition
            {
                if (chessBoard->canMove(pos.row, pos.col))
                {
                    if (chessBoard->getChessType(pos.row, pos.col, Util::otherside(playerColor)) == CHESSTYPE_5)
                    {
                        if (chessBoard->getChessType(pos.row, pos.col, playerColor) == CHESSTYPE_BAN)//player GG AI win
                        {
                            if (playerColor == PIECE_BLACK)
                            {
                                black = { -CHESSTYPE_5_SCORE , -CHESSTYPE_5_SCORE };
                            }
                            else
                            {
                                white = { -CHESSTYPE_5_SCORE , -CHESSTYPE_5_SCORE };
                            }
                            goto end;
                        }
                        createChildNode(pos.row, pos.col);
                        if (buildDefendChildsAndPrune(basescore))
                        {
                            goto end;
                        }

                    }
                }

            }
        }
        //else if (getHighest(util::otherside(playerColor)) >= SCORE_4_DOUBLE)//防三四、活四，但是可被冲四进攻替换
        //{
        //    for (int i = 0; i < BOARD_ROW_MAX; ++i)
        //    {
        //        for (int j = 0; j < BOARD_COL_MAX; ++j)
        //        {
        //            if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
        //            {
        //                if (chessBoard->getPiece(i, j).getThreat(util::otherside(playerColor)) >= SCORE_4_DOUBLE)
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
        //进攻
        if (getHighest(playerColor) >= ChessBoard::getChessTypeInfo(CHESSTYPE_43).rating && getHighest(Util::otherside(playerColor)) < CHESSTYPE_5_SCORE)
        {
            int score;
            ForEachPosition
            {
                if (chessBoard->canMove(pos.row, pos.col))
                {
                    score = ChessBoard::getChessTypeInfo(chessBoard->getChessType(pos.row, pos.col, playerColor)).rating;
                    if (score >= ChessBoard::getChessTypeInfo(CHESSTYPE_43).rating)
                    {
                        createChildNode(pos.row, pos.col);
                        if (buildDefendChildsAndPrune(basescore))
                        {
                            goto end;
                        }
                    }
                }

            }
        }
        else if (/*getHighest(playerColor) > 99 && */getHighest(Util::otherside(playerColor)) < CHESSTYPE_5_SCORE)
        {
            ChessBoard tempBoard;
            GameTreeNode *tempNode;
            int score;
            ForEachPosition
            {
                if (chessBoard->canMove(pos.row, pos.col))
                {
                    score = ChessBoard::getChessTypeInfo(chessBoard->getChessType(pos.row, pos.col, playerColor)).rating;//player
                    if (score > ChessBoard::getChessTypeInfo(CHESSTYPE_J3).rating)
                    {
                        tempBoard = *chessBoard;
                        tempBoard.move(pos, ban);
                        if (tempBoard.getHighestType(Util::otherside(playerColor)) < CHESSTYPE_43
                            || tempBoard.hasChessType(playerColor, CHESSTYPE_5))
                        {
                            tempNode = new GameTreeNode(&tempBoard);
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
                    else if (extraSearch && getDepth() < maxSearchDepth - 4 && score > 0)//特殊情况，会形成三四
                    {
                        tempBoard = *chessBoard;
                        tempBoard.move(pos, ban);
                        if (tempBoard.getHighestType(Util::otherside(playerColor)) < CHESSTYPE_43)
                        {
                            if (tempBoard.getHighestType(playerColor) >= CHESSTYPE_43)
                            {
                                tempNode = new GameTreeNode(&tempBoard);
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
        //player节点
        int score;
        //进攻
        if (getHighest(Util::otherside(playerColor)) >= CHESSTYPE_5_SCORE)//player GG AI win 
        {
            if (playerColor == PIECE_BLACK)
            {
                black = { -CHESSTYPE_5_SCORE , -CHESSTYPE_5_SCORE };
            }
            else
            {
                white = { -CHESSTYPE_5_SCORE , -CHESSTYPE_5_SCORE };
            }
            goto end;
        }
        //else if (getHighest(util::otherside(playerColor)) >= SCORE_4_DOUBLE && getHighest(playerColor) < SCORE_5_CONTINUE)// 没有即将形成的五连，可以去绝杀
        //{
        //    for (int i = 0; i < BOARD_ROW_MAX; ++i)
        //    {
        //        for (int j = 0; j < BOARD_COL_MAX; ++j)
        //        {
        //            if (chessBoard->getPiece(i, j).hot && chessBoard->getPiece(i, j).state == 0)
        //            {
        //                if (chessBoard->getPiece(i, j).getThreat(util::otherside(playerColor)) >= SCORE_4_DOUBLE)
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

        //防守
        if (getHighest(playerColor) >= CHESSTYPE_5_SCORE)//堵playerd的冲四(即将形成的五连)
        {
            ForEachPosition
            {
                if (chessBoard->canMove(pos.row, pos.col))
                {
                    if (chessBoard->getChessType(pos.row, pos.col,playerColor) == CHESSTYPE_5)//堵player即将形成的五连
                    {
                        if (chessBoard->getChessType(pos.row, pos.col, Util::otherside(playerColor)) == CHESSTYPE_BAN)//被禁手了 AI gg
                        {
                            goto end;//被禁手，必输无疑
                        }
                        createChildNode(pos.row, pos.col);
                        if (buildDefendChildsAndPrune(basescore))
                        {
                            goto end;
                        }
                        goto end;//必堵，堵一个就行了，如果还有一个就直接输了
                    }
                }
            }
        }
        else if (getHighest(playerColor) >= ChessBoard::getChessTypeInfo(CHESSTYPE_33).rating)//堵player的活三(即将形成的三四、活四)
        {
            ForEachPosition
            {
                if (chessBoard->canMove(pos.row, pos.col))
                {
                    if (ChessBoard::getChessTypeInfo(chessBoard->getChessType(pos.row, pos.col, playerColor)).rating >= ChessBoard::getChessTypeInfo(CHESSTYPE_33).rating)//堵player的活三、即将形成的三四
                    {
                        if (chessBoard->getChessType(pos.row, pos.col, Util::otherside(playerColor)) == CHESSTYPE_BAN)//被禁手了
                        {
                            continue;
                        }
                        createChildNode(pos.row, pos.col);//直接堵
                        if (buildDefendChildsAndPrune(basescore))
                        {
                            goto end;
                        }
                        if (extraSearch && getDepth() < maxSearchDepth - 3)//间接堵
                        {
                            for (int n = 0; n < DIRECTION8_COUNT; ++n)//8个方向
                            {
                                Position temppos = pos;
                                int blankCount = 0, chessCount = 0;
                                while (temppos.displace8(1, n)) //如果不超出边界
                                {
                                    if (chessBoard->getState(temppos.row, temppos.col) == PIECE_BLANK)
                                    {
                                        blankCount++;

                                        score = ChessBoard::getChessTypeInfo(chessBoard->getChessType(temppos.row, temppos.col, playerColor)).rating;
                                        if (score >= ChessBoard::getChessTypeInfo(CHESSTYPE_J3).rating || score < 0)
                                        {
                                            if (chessBoard->getChessType(temppos.row, temppos.col, Util::otherside(playerColor)) == CHESSTYPE_BAN)//被禁手了
                                            {
                                                continue;
                                            }
                                            ChessBoard tempBoard = *chessBoard;
                                            tempBoard.move(temppos, ban);
                                            if (tempBoard.getChessType(pos.row, pos.col, playerColor) < CHESSTYPE_33)
                                            {
                                                GameTreeNode *tempNode = new GameTreeNode(&tempBoard);
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
                                    else if (chessBoard->getState(temppos.row, temppos.col) == Util::otherside(playerColor))
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
                    else if (getHighest(Util::otherside(playerColor)) >= ChessBoard::getChessTypeInfo(CHESSTYPE_43).rating)//防不住就进攻
                    {
                        createChildNode(pos.row, pos.col);
                        if (buildDefendChildsAndPrune(basescore))
                        {
                            goto end;
                        }
                    }
                }

            }
        }
        else if (getHighest(playerColor) >= ChessBoard::getChessTypeInfo(CHESSTYPE_J3).rating /*&& getHighest(util::otherside(playerColor)) < 100*/)//堵冲四、活三
        {
            ForEachPosition
            {
                if (chessBoard->canMove(pos.row, pos.col))
                {
                    score = ChessBoard::getChessTypeInfo(chessBoard->getChessType(pos.row, pos.col, playerColor)).rating;
                    if (score >= ChessBoard::getChessTypeInfo(CHESSTYPE_J3).rating/* && score < 1200*/)
                    {
                        //if ((score == 999 || score == 1001 || score == 1030))//无意义的冲四
                        //{
                        //    continue;
                        //}
                        if (chessBoard->getChessType(pos.row, pos.col, Util::otherside(playerColor)) == CHESSTYPE_BAN)//被禁手了
                        {
                            continue;
                        }
                        createChildNode(pos.row, pos.col);
                        if (buildDefendChildsAndPrune(basescore))
                        {
                            goto end;
                        }
                    }
                }

            }
        }
    }
end:
    int a = 1;
    /*delete chessBoard;
    chessBoard = 0;*/
}

RatingInfoDenfend GameTreeNode::buildDefendChildWithTransTable(GameTreeNode* child, int basescore)
{
    TransTableNodeData data;
    RatingInfoDenfend info;
    int depth = getDepth();
    if (depth < transTableMaxDepth)
    {
        transTable_atack[depth]->lock.lock_shared();
        if (transTable_atack[depth]->m.find(child->chessBoard->hash.check_key) != transTable_atack[depth]->m.end())//命中
        {
            data = transTable_atack[depth]->m[child->chessBoard->hash.check_key];
            transTable_atack[depth]->lock.unlock_shared();
            if (data.checksum == child->chessBoard->hash.hash_key)//校验成功
            {
                transTableHashStat.hit++;
                //不用build了，直接用现成的
                child->lastStep = data.lastStep;
                child->black = data.black;
                child->white = data.white;
                info = child->getBestDefendRating(basescore);
                return info;
            }
            else//冲突，覆盖
            {
                transTableHashStat.clash++;
            }
        }
        else//未命中
        {
            transTable_atack[depth]->lock.unlock_shared();
            transTableHashStat.miss++;
        }
    }

    child->buildDefendTreeNode(basescore);
    info = child->getBestDefendRating(basescore);

    if (depth < transTableMaxDepth && child->childs.size() > 0)//缓存入置换表
    {
        data.checksum = child->chessBoard->hash.hash_key;
        data.lastStep = info.lastStep;
        data.black = info.black;
        data.white = info.white;
        transTable_atack[depth]->lock.lock();
        transTable_atack[depth]->m[child->chessBoard->hash.check_key] = data;
        transTable_atack[depth]->lock.unlock();
    }


    child->black = info.black;
    child->white = info.white;
    child->lastStep = info.lastStep;
#ifndef GAMETREE_DEBUG
    child->deleteChilds();
#endif // !GAMETREE_DEBUG

    return info;
}


bool GameTreeNode::buildDefendChildsAndPrune(int basescore)
{

    RatingInfoDenfend info = buildDefendChildWithTransTable(childs.back(), basescore);
    if (lastStep.state == Util::otherside(playerColor))//build player
    {
        if (info.rating.totalScore < -CHESSTYPE_5_SCORE || info.rating.totalScore <= alpha || info.rating.totalScore <= GameTreeNode::bestRating)//alpha剪枝
        {
            return true;
        }
        //设置beta值
        if (info.rating.totalScore < beta)
        {
            beta = info.rating.totalScore;
        }
    }
    else//build AI
    {
        if (info.rating.totalScore >= beta)//beta剪枝
        {
            return true;
        }
        //设置alpha值
        if (info.rating.totalScore > alpha)
        {
            alpha = info.rating.totalScore;
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
        if (getTotal(playerColor) >= CHESSTYPE_5_SCORE)
        {
            result.rating.totalScore = -CHESSTYPE_5_SCORE - 100 + (lastStep.step - GameTreeNode::startStep);
        }
        else
        {
            result.rating.totalScore = -getTotal(playerColor);

            if (!extraSearch || lastStep.step < 10)
            {
                result.rating.totalScore += basescore;
            }
            //result.rating.totalScore += basescore;
            if (lastStep.state == playerColor)
            {
                result.rating.totalScore /= 10;//为了使权重平衡，不然最后一步是playerColor下的的话，权重始终是优于最后一步是AI下的
                                               //可能有意料之外的BUG
            }
        }

    }
    else
    {
        RatingInfoDenfend tempThreat;
        result = childs[0]->getBestDefendRating(basescore);

        if (lastStep.state == Util::otherside(playerColor))//AI节点 build player
        {
            for (size_t i = 1; i < childs.size(); ++i)
            {
                tempThreat = childs[i]->getBestDefendRating(basescore);//递归
                if (tempThreat.rating.totalScore < result.rating.totalScore)//best原则:player下过的节点player得分越大越好(默认player走最优点)
                {
                    result = tempThreat;
                }
            }
        }
        else//player节点 build AI
        {
            for (size_t i = 1; i < childs.size(); ++i)
            {
                tempThreat = childs[i]->getBestDefendRating(basescore);//child是AI节点
                if (tempThreat.rating.totalScore > result.rating.totalScore)//best原则:AI下过的节点player得分越小越好
                {
                    result = tempThreat;
                }
            }
        }
    }
#ifdef GAMETREE_DEBUG
    result.moveList.push_back(lastStep);
#endif
    return result;
}


int GameTreeNode::buildAtackSearchTree()
{
    //vector<int> index;
    GameTreeNode::bestIndex = -1;
    for (size_t i = 0; i < childs.size(); ++i)
    {
        //lastStepScore是进攻权重
        int flag = false;
        if ((childsInfo[i].lastStepScore >= ChessBoard::getChessTypeInfo(CHESSTYPE_D4).rating && childs[i]->getHighest(playerColor) < CHESSTYPE_5_SCORE))
        {
            flag = true;
        }
        else if (childsInfo[i].lastStepScore > 0)
        {
            if (getHighest(Util::otherside(playerColor)) < ChessBoard::getChessTypeInfo(CHESSTYPE_43).rating && childs[i]->getHighest(Util::otherside(playerColor)) >= ChessBoard::getChessTypeInfo(CHESSTYPE_43).rating)
            {
                flag = true;
            }
        }
        if (flag)
        {
            TaskItems t;
            t.node = new GameTreeNode();
            *t.node = *childs[i];
            t.index = (int)i;
            t.type = TASKTYPE_ATACK;
            ThreadPool::getInstance()->run(bind(threadPoolWorkFunc, t));
            //index.push_back(i);
        }
    }
    ThreadPool::getInstance()->wait();

    return GameTreeNode::bestIndex;
}

void GameTreeNode::buildAtackTreeNode(int deepen)
{
    int oldalpha = alpha;
    if (lastStep.state == playerColor)//build AI 进攻方
    {
        if (getHighest(Util::otherside(playerColor)) >= CHESSTYPE_5_SCORE)//成功
        {
            goto end;
        }

        if (getHighest(playerColor) >= CHESSTYPE_5_SCORE)//防冲四
        {
            ForEachPosition
            {
                if (chessBoard->canMove(pos.row, pos.col))
                {
                    if (chessBoard->getChessType(pos.row, pos.col, playerColor) == CHESSTYPE_5)
                    {
                        if (chessBoard->getChessType(pos.row, pos.col, Util::otherside(playerColor)) == CHESSTYPE_BAN)
                        {
                            goto end;
                        }
                        createChildNode(pos.row, pos.col);
                        if (buildAtackChildsAndPrune(deepen))
                        {
                            goto end;
                        }

                    }
                }
            }
        }
        else if (getHighest(Util::otherside(playerColor)) >= ChessBoard::getChessTypeInfo(CHESSTYPE_44).rating)//进攻
        {
            int score;
            ForEachPosition
            {
                if (chessBoard->canMove(pos.row, pos.col))
                {
                    score = ChessBoard::getChessTypeInfo(chessBoard->getChessType(pos.row, pos.col, Util::otherside(playerColor))).rating;
                    if (score >= ChessBoard::getChessTypeInfo(CHESSTYPE_43).rating)
                    {
                        createChildNode(pos.row, pos.col);
                        if (buildAtackChildsAndPrune(deepen))
                        {
                            goto end;
                        }
                    }
                }

            }
        }
        else if (getHighest(Util::otherside(playerColor)) >= ChessBoard::getChessTypeInfo(CHESSTYPE_J3).rating)//进攻
        {
            int score;
            RatingInfo tempInfo = { 0,0 };
            ForEachPosition
            {
                if (chessBoard->canMove(pos.row, pos.col))
                {
                    score = chessBoard->getChessType(pos.row, pos.col, Util::otherside(playerColor));
                    if (score >= CHESSTYPE_J3 && score < CHESSTYPE_43)
                    {
                        createChildNode(pos.row, pos.col);
                        if (buildAtackChildsAndPrune(deepen))
                        {
                            goto end;
                        }
                    }
                    else if (score > 0 && getDepth() < maxSearchDepth - 4)
                    {
                        ChessBoard tempBoard = *chessBoard;
                        tempBoard.move(pos, ban);

                        if (tempBoard.getHighestType(Util::otherside(playerColor)) >= CHESSTYPE_43)
                        {
                            GameTreeNode *tempNode = new GameTreeNode(&tempBoard);
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
    else//buildplayer 防守方
    {
        //五连
        int score;
        bool flag = false;
        if (getDepth() >= maxSearchDepth)//除非特殊情况，保证最后一步是AI下的，故而=maxSearchDepth时就直接结束
        {
            goto end;
        }
        if (getDepth() >= alpha || getDepth() >= GameTreeNode::bestRating)
        {
            goto end;
        }
        if (getHighest(playerColor) >= CHESSTYPE_5_SCORE)
        {
            goto end;
        }
        else if (getHighest(Util::otherside(playerColor)) < CHESSTYPE_5_SCORE)// 没有即将形成的五连，可以去绝杀进攻找机会
        {
            ForEachPosition
            {
                if (chessBoard->canMove(pos.row, pos.col))
                {
                    score = ChessBoard::getChessTypeInfo(chessBoard->getChessType(pos.row, pos.col, (playerColor))).rating;
                    if (score >= ChessBoard::getChessTypeInfo(CHESSTYPE_44).rating)
                    {
                        createChildNode(pos.row, pos.col);
                        if (buildAtackChildsAndPrune(deepen))
                        {
                            goto end;
                        }
                    }
                    else if ((score == (CHESSTYPE_D4) || score == CHESSTYPE_D4P) && getHighest(Util::otherside(playerColor)) >= ChessBoard::getChessTypeInfo(CHESSTYPE_33).rating)//对于防守方，冲四是为了找机会，不会轻易冲
                    {
                        if (/*(score == 999 || score == 1001 || score == 1030) && */
                            chessBoard->getChessType(pos.row, pos.col, Util::otherside(playerColor)) < CHESSTYPE_J3)//过滤掉无意义的冲四
                        {
                            continue;
                        }
                        //flag = true;//保证连续冲四
                        createChildNode(pos.row, pos.col);
                        if (buildAtackChildsAndPrune(deepen + 2))//冲四不消耗搜索深度限制
                        {
                            goto end;
                        }
                    }
                }

            }
        }
        //if (deepen > 0)//只要冲四过，就不能在进行其他操作了
        //{
        //    goto end;
        //}
        //防守
        if (getHighest(Util::otherside(playerColor)) >= CHESSTYPE_5_SCORE)//堵playerd的冲四(即将形成的五连)
        {
            ForEachPosition
            {
                if (chessBoard->canMove(pos.row, pos.col))
                {
                    if (chessBoard->getChessType(pos.row, pos.col, Util::otherside(playerColor)) == CHESSTYPE_5)//堵player即将形成的五连
                    {
                        if (chessBoard->getChessType(pos.row, pos.col, (playerColor)) == CHESSTYPE_BAN)//被禁手了
                        {
                            if (playerColor == PIECE_BLACK)
                            {
                                black.highestScore = -1;
                            }
                            else
                            {
                                white.highestScore = -1;
                            }
                            goto end;//被禁手，必输无疑
                        }
                        createChildNode(pos.row, pos.col);
                        if (buildAtackChildsAndPrune(deepen))
                        {
                            goto end;
                        }
                        goto end;//堵一个就行了，如果还有一个就直接输了，所以无论如何都要剪枝
                    }
                }
            }
        }
        else if (getHighest(Util::otherside(playerColor)) >= ChessBoard::getChessTypeInfo(CHESSTYPE_33).rating)//堵player的活三(即将形成的三四、活四、三三)
        {
            ForEachPosition
            {
                if (chessBoard->canMove(pos.row, pos.col)
                    && chessBoard->getChessType(pos.row, pos.col, (playerColor)) < CHESSTYPE_44)//防止和前面重复
                {
                    if (ChessBoard::getChessTypeInfo(chessBoard->getChessType(pos.row, pos.col, Util::otherside(playerColor))).rating >= ChessBoard::getChessTypeInfo(CHESSTYPE_33).rating)//堵player的活三、即将形成的三四
                    {
                        if (chessBoard->getChessType(pos.row, pos.col, playerColor) == CHESSTYPE_BAN)//被禁手了
                        {
                            continue;
                        }
                        createChildNode(pos.row, pos.col);//直接堵
                        if (buildAtackChildsAndPrune(deepen))
                        {
                            goto end;//如果info.depth < 0 就goto end
                        }
                        /*if (info.depth > 0)*/
                        {
                            //间接堵
                            for (int n = 0; n < DIRECTION8_COUNT; ++n)//8个方向
                            {
                                Position temppos = pos;
                                int blankCount = 0, chessCount = 0;
                                while (temppos.displace8(1, n)) //如果不超出边界
                                {
                                    if (chessBoard->getState(temppos.row, temppos.col) == PIECE_BLANK)
                                    {
                                        blankCount++;
                                        //if (!chessBoard->isHot(r, c))
                                        //{
                                        //    continue;
                                        //}
                                        score = ChessBoard::getChessTypeInfo(chessBoard->getChessType(temppos.row, temppos.col, Util::otherside(playerColor))).rating;
                                        if (score >= ChessBoard::getChessTypeInfo(CHESSTYPE_J3).rating || score < 0)
                                        {
                                            if (chessBoard->getChessType(temppos.row, temppos.col, playerColor) == CHESSTYPE_BAN)//被禁手了
                                            {
                                                continue;
                                            }
                                            ChessBoard tempBoard = *chessBoard;
                                            tempBoard.move(temppos, ban);
                                            if (tempBoard.getChessType(pos.row,pos.col, Util::otherside(playerColor)) < CHESSTYPE_33)
                                            {
                                                GameTreeNode *tempNode = new GameTreeNode(&tempBoard);
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
                                    else if (chessBoard->getState(temppos.row, temppos.col) == playerColor)
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
        else if (getHighest(Util::otherside(playerColor)) >= ChessBoard::getChessTypeInfo(CHESSTYPE_J3).rating && getHighest(playerColor) < ChessBoard::getChessTypeInfo(CHESSTYPE_J3).rating)//堵冲四、活三
        {
            ForEachPosition
            {
                if (chessBoard->canMove(pos.row, pos.col))
                {
                    score = ChessBoard::getChessTypeInfo(chessBoard->getChessType(pos.row, pos.col, Util::otherside(playerColor))).rating;
                    if (score >= ChessBoard::getChessTypeInfo(CHESSTYPE_J3).rating)
                    {
                        //if ((score == 999 || score == 1001 || score == 1030))//无意义的冲四
                        //{
                        //    continue;
                        //}
                        if (chessBoard->getChessType(pos.row, pos.col, playerColor) == CHESSTYPE_BAN)//被禁手了
                        {
                            continue;
                        }
                        createChildNode(pos.row, pos.col);
                        if (buildAtackChildsAndPrune(deepen))
                        {
                            goto end;
                        }
                    }
                }
            }
        }
    }
end:
    int a = 0;
    /*delete chessBoard;
    chessBoard = 0;*/
}

RatingInfoAtack GameTreeNode::buildAtackChildWithTransTable(GameTreeNode* child, int deepen)
{
    RatingInfoAtack info;
    TransTableNodeData data;
    int depth = getDepth();
    if (depth < transTableMaxDepth)
    {
        transTable_atack[depth]->lock.lock_shared();
        if (transTable_atack[depth]->m.find(child->chessBoard->hash.check_key) != transTable_atack[depth]->m.end())//命中
        {
            data = transTable_atack[depth]->m[child->chessBoard->hash.check_key];
            transTable_atack[depth]->lock.unlock_shared();
            if (data.checksum == child->chessBoard->hash.hash_key)//校验成功
            {
                transTableHashStat.hit++;
                //不用build了，直接用现成的
                child->black = data.black;
                child->white = data.white;
                child->lastStep = data.lastStep;
                info = child->getBestAtackRating();
                return info;
            }
            else//冲突，覆盖
            {
                transTableHashStat.clash++;
            }
        }
        else//未命中
        {
            transTable_atack[depth]->lock.unlock_shared();
            transTableHashStat.miss++;
        }
    }

    child->buildAtackTreeNode(deepen);
    info = child->getBestAtackRating();

    if (depth < transTableMaxDepth && child->childs.size() > 0)
    {
        data.checksum = child->chessBoard->hash.hash_key;
        data.black = info.black;
        data.white = info.white;
        data.lastStep = info.lastStep;
        transTable_atack[depth]->lock.lock();
        transTable_atack[depth]->m[child->chessBoard->hash.check_key] = data;
        transTable_atack[depth]->lock.unlock();
    }

    child->black = info.black;
    child->white = info.white;
    child->lastStep = info.lastStep;
#ifndef GAMETREE_DEBUG
    child->deleteChilds();
#endif // !GAMETREE_DEBUG
    return info;
}

bool GameTreeNode::buildAtackChildsAndPrune(int deepen)
{
    RatingInfoAtack info = buildAtackChildWithTransTable(childs.back(), deepen);
    if (lastStep.state == playerColor)//build AI, beta剪枝
    {
        if (info.depth > -1)
        {
            if (info.depth <= beta)//beta剪枝
            {
                return true;
            }
            //else 
            if (info.depth < alpha)//设置alpha值
            {
                alpha = info.depth;
            }
        }
    }
    else//buildplayer, alpha剪枝
    {
        if (info.depth < 0 || info.depth >= alpha || info.depth >= GameTreeNode::bestRating)//alpha剪枝
        {
            return true;
        }
        else
        {
            if (info.depth > beta)//设置beta值
            {
                beta = info.depth;
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
        if (playerColor == PIECE_BLACK)
        {
            result.black = RatingInfo{ getTotal(playerColor), getHighest(playerColor) };
            result.white = RatingInfo{ getTotal(Util::otherside(playerColor)) , getHighest(Util::otherside(playerColor)) };
        }
        else
        {
            result.white = RatingInfo{ getTotal(playerColor), getHighest(playerColor) };
            result.black = RatingInfo{ getTotal(Util::otherside(playerColor)), getHighest(Util::otherside(playerColor)) };
        }
        result.lastStep = lastStep;
        if (lastStep.state == playerColor)//叶子节点是player,表示提前结束,AI取胜,否则一定会是AI
        {
            if (lastStep.step - startStep < 0)
            {
                result.depth = -1;
            }
            else if (getHighest(Util::otherside(playerColor)) >= CHESSTYPE_5_SCORE)
            {
                result.depth = lastStep.step - startStep;
            }
            else
            {
                result.depth = -1;
            }
        }
        else//叶子节点是AI,表示未结束
        {
            if (getHighest(Util::otherside(playerColor)) >= CHESSTYPE_5_SCORE && getHighest(playerColor) < 0)//禁手
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
        if (lastStep.state != playerColor)//节点是AI
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
                else if (playerColor == PIECE_BLACK)
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
                    if (playerColor == PIECE_BLACK)
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
#ifdef GAMETREE_DEBUG
    result.moveList.push_back(lastStep);
#endif
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
        GameTreeNode::childsInfo[t.index].rating = (GameTreeNode::playerColor == PIECE_BLACK) ? info.white : info.black;
        GameTreeNode::childsInfo[t.index].depth = info.depth;
        if (GameTreeNode::childsInfo[t.index].rating.highestScore >= CHESSTYPE_5_SCORE && info.depth > -1)
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