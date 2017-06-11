#include "GoSearch.h"
#include <algorithm>

HashStat GoSearchEngine::transTableStat;
string GoSearchEngine::textout;
int GoSearchEngine::maxKillSearchDepth = 0;

bool GoTreeNodeCmp(const GoTreeNode &a, const GoTreeNode &b)
{
    return a.stepScore > b.stepScore;
}

GoSearchEngine::GoSearchEngine() :board(NULL)
{
}

GoSearchEngine::~GoSearchEngine()
{

}

void GoSearchEngine::initSearchEngine(ChessBoard* board, ChessStep lastStep)
{
    GoSearchEngine::transTableStat = { 0,0,0 };
    this->board = board;
    this->startStep = lastStep;
    this->maxSearchTime = 20;
    textout.clear();
}

void GoSearchEngine::textOutSearchInfo(OptimalPath& optimalPath)
{
    char text[1024];
    int len = 0;
    len += snprintf(text + len, 1024, "currentDepth:%d-%d, caculatetime:%llus, rating:%d, next:%d,%d\r\n",
        global_currentMaxDepth, maxKillSearchDepth - startStep.step, std::time(NULL) - global_startSearchTime, optimalPath.situationRating, optimalPath.path[0].getRow(), optimalPath.path[0].getCol());
    textout += text;
}

void GoSearchEngine::textOutPathInfo(OptimalPath& optimalPath)
{
    char text[1024];
    int len = 0;
    len += snprintf(text + len, 1024, "path:");
    for (auto p : optimalPath.path)
    {
        len += snprintf(text + len, 1024, " %d,%d ", p.getRow(), p.getCol());
    }
    len += snprintf(text + len, 1024, "\r\n");
    textout += text;
}

uint8_t GoSearchEngine::getBestStep()
{
    global_isOverTime = false;

    global_startSearchTime = std::time(NULL);
    OptimalPath bestPath;
    //minAlphaBetaDepth = 7;
    for (global_currentMaxDepth = minAlphaBetaDepth; 
        global_currentMaxDepth <= 5/*maxAlphaBetaDepth*/; 
        ++global_currentMaxDepth)
    {
        maxKillSearchDepth = 0;
        if (std::time(NULL) - global_startSearchTime >= maxSearchTime / 5)
        {
            break;
        }
        OptimalPath temp;
        temp.startStep = startStep.step;
        doAlphaBetaSearch(board, INT_MIN, INT_MAX, temp);
        if (global_isOverTime)
        {
            break;
        }
        textOutSearchInfo(temp);
        bestPath = temp;
        if (temp.situationRating == util::type2score(CHESSTYPE_5) || temp.situationRating == -util::type2score(CHESSTYPE_5))
        {
            break;
        }
    }
    textOutPathInfo(bestPath);
    transTable.clear();
    return bestPath.path[0].index;
}

void GoSearchEngine::doAlphaBetaSearch(ChessBoard* board, int alpha, int beta, OptimalPath& optimalPath)
{
    //超时
    if (std::time(NULL) - global_startSearchTime > maxSearchTime)
    {
        global_isOverTime = true;
        return;
    }

    ChessStep laststep;
    if (optimalPath.path.empty())
    {
        laststep = startStep;
    }
    else
    {
        laststep = optimalPath.path.back();
    }

    //出口
    if (board->getHighestScore(util::otherside(laststep.getColor())) == util::type2score(CHESSTYPE_5))
    {
        optimalPath.situationRating = laststep.getColor() == startStep.getColor() ? util::type2score(CHESSTYPE_5) : -util::type2score(CHESSTYPE_5);
        ChessStep step;
        step.black = laststep.black ? false : true;
        step.chessType = CHESSTYPE_5;
        step.index = board->getHighestInfo(util::otherside(laststep.getColor())).index;
        step.step = laststep.step + 1;
        optimalPath.path.push_back(step);
        optimalPath.endStep = laststep.step + 1;
        return;
    }

    vector<GoTreeNode> childs;
    if (laststep.step - startStep.step >= global_currentMaxDepth)
    {
        if (board->getHighestInfo(laststep.getColor()).chessmode != CHESSTYPE_5)
        {
            doKillSearch(board, optimalPath, optimalPath.situationRating, util::otherside(laststep.getColor()));
            if (maxKillSearchDepth == 0)
            {
                maxKillSearchDepth = laststep.step;
            }
        }
        
        return;
    }
    else
    {
        getNextSteps(board, util::otherside(laststep.getColor()), childs);
    }

    OptimalPath bestPath;
    if (laststep.black == startStep.black)//build AI
    {
        bestPath.situationRating = INT_MIN;
    }
    else
    {
        bestPath.situationRating = INT_MAX;
    }

    for (size_t i = 0; i < childs.size(); ++i)
    {
        ChessStep oldindex = board->getLastStep();
        board->move(childs[i].index);
        ChessStep step;
        step.black = childs[i].side == PIECE_BLACK ? true : false;
        step.chessType = childs[i].chessType;
        step.index = childs[i].index;
        step.step = laststep.step + 1;

        OptimalPath tempPath;
        tempPath.startStep = laststep.step;
        tempPath.path.push_back(step);
        tempPath.situationRating = board->getSituationRating(getAISide());
        tempPath.endStep = step.step;

        if (step.step < startStep.step + global_currentMaxDepth)
        {
            TransTableData data;
            if (getTransTable(board->getBoardHash().z32key, data))
            {
                if (data.checkHash == board->getBoardHash().z64key)
                {
                    if ((data.isEnd() || data.endStep > startStep.step + global_currentMaxDepth))
                    {
                        transTableStat.hit++;
                        tempPath.situationRating = data.situationRating;
                        tempPath.endStep = data.endStep;
                    }
                    else
                    {
                        transTableStat.cover++;
                        doAlphaBetaSearch(board, alpha, beta, tempPath);
                        data.checkHash = board->getBoardHash().z64key;
                        data.endStep = tempPath.endStep;
                        data.endSide = tempPath.path.back().getColor();
                        data.situationRating = tempPath.situationRating;
                        putTransTable(board->getBoardHash().z32key, data);
                    }
                }
                else
                {
                    transTableStat.clash++;
                    doAlphaBetaSearch(board, alpha, beta, tempPath);
                    data.checkHash = board->getBoardHash().z64key;
                    data.endStep = tempPath.endStep;
                    data.endSide = tempPath.path.back().getColor();
                    data.situationRating = tempPath.situationRating;
                    putTransTable(board->getBoardHash().z32key, data);
                }
            }
            else
            {
                transTableStat.miss++;
                doAlphaBetaSearch(board, alpha, beta, tempPath);
                data.checkHash = board->getBoardHash().z64key;
                data.endStep = tempPath.endStep;
                data.endSide = tempPath.path.back().getColor();
                data.situationRating = tempPath.situationRating;
                putTransTable(board->getBoardHash().z32key, data);
            }
        }
        else
        {
            doAlphaBetaSearch(board, alpha, beta, tempPath);
        }


        board->unmove(childs[i].index, oldindex);
        if (global_isOverTime)
        {
            return;
        }
        if (laststep.black == startStep.black)//build AI
        {
            if (tempPath.situationRating > bestPath.situationRating
                || (tempPath.situationRating == bestPath.situationRating && tempPath.endStep < bestPath.endStep)
                )
            {
                bestPath.situationRating = tempPath.situationRating;
                bestPath.startStep = tempPath.startStep;
                bestPath.path.swap(tempPath.path);
                bestPath.endStep = tempPath.endStep;
            }

            if (tempPath.situationRating > beta)//beta cut
            {
                break;
            }
            if (tempPath.situationRating > alpha)//update alpha
            {
                alpha = tempPath.situationRating;
            }
        }
        else // build player
        {
            if (tempPath.situationRating < bestPath.situationRating
                || (tempPath.situationRating == bestPath.situationRating && tempPath.endStep > bestPath.endStep)
                )
            {
                bestPath.situationRating = tempPath.situationRating;
                bestPath.startStep = tempPath.startStep;
                bestPath.path.swap(tempPath.path);
                bestPath.endStep = tempPath.endStep;
            }

            if (tempPath.situationRating < alpha)//alpha cut
            {
                break;
            }
            if (tempPath.situationRating < beta)//update beta
            {
                beta = tempPath.situationRating;
            }
        }
    }

    if (!bestPath.path.empty())
    {
        optimalPath.situationRating = bestPath.situationRating;
        for (size_t i = 0; i < bestPath.path.size(); ++i)
        {
            optimalPath.path.push_back(bestPath.path[i]);
        }
        optimalPath.endStep = bestPath.endStep;
    }
    else if (board->getHighestScore(laststep.getColor())==util::type2score(CHESSTYPE_5))
    {
        optimalPath.situationRating = laststep.getColor() == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
    }
}

void GoSearchEngine::getNextSteps(ChessBoard* board, uint8_t side, vector<GoTreeNode>& childs)
{
    for (uint8_t index = 0; util::valid(index); ++index)
    {
        if (!board->canMove(index))
        {
            continue;
        }
        if (board->getThreat(index, side) == 0 && board->getThreat(index, util::otherside(side)) == 0)
        {
            continue;
        }
        if (board->getThreat(index, side) < 0)
        {
            continue;
        }
        if (util::isdead4(board->getChessType(index, side)) && board->getThreat(index, util::otherside(side)) == 0)
        {
            continue;//无意义冲四
        }
        GoTreeNode node;
        node.index = index;
        node.chessType = board->getChessType(index, side);
        node.side = side;
        node.stepScore = board->getThreat(index, side) + board->getThreat(index, util::otherside(side));
        childs.push_back(node);
    }
    std::sort(childs.begin(), childs.end(), GoTreeNodeCmp);
    if (childs.size() > MAX_CHILD_NUM)
    {
        childs.erase(childs.begin() + MAX_CHILD_NUM, childs.end());
    }
}

void GoSearchEngine::doKillSearch(ChessBoard* board, OptimalPath& optimalPath, int bestRating, uint8_t atackSide)
{
    //出口
    if (std::time(NULL) - global_startSearchTime > maxSearchTime)
    {
        global_isOverTime = true;
        return;
    }

    ChessStep laststep = optimalPath.path.back();

    if (board->getHighestScore(util::otherside(laststep.getColor())) == util::type2score(CHESSTYPE_5))
    {
        optimalPath.situationRating = laststep.getColor() == startStep.getColor() ? util::type2score(CHESSTYPE_5) : -util::type2score(CHESSTYPE_5);
        optimalPath.endStep = laststep.step + 1;
        return;
    }

    if (laststep.step - startStep.step >= maxKillToEndDepth)
    {
        return;
    }

    OptimalPath bestPath;
    bestPath.startStep = 255;//-1
    for (uint8_t index = 0; util::valid(index); ++index)
    {
        if (!board->canMove(index))
        {
            continue;
        }
        if (laststep.getColor() == atackSide)//防守
        {
            if (board->getChessType(index, laststep.getColor()) != CHESSTYPE_5 || board->getChessType(index, util::otherside(laststep.getColor()))< 0)
            {
                continue;
            }
        }
        else//进攻
        {
            if (!util::hasdead4(board->getChessType(index, util::otherside(laststep.getColor()))))
            {
                continue;
            }
        }

        ChessStep step;
        step.black = !laststep.black;
        step.chessType = board->getChessType(index, util::otherside(laststep.getColor()));
        step.index = index;
        step.step = laststep.step + 1;

        ChessStep oldindex = board->getLastStep();

        board->move(index);

        OptimalPath tempPath;
        tempPath.startStep = laststep.step;
        tempPath.path.push_back(step);
        tempPath.situationRating = board->getSituationRating(getAISide());
        tempPath.endStep = step.step;
        TransTableData data;
        if (getTransTable(board->getBoardHash().z32key, data))
        {
            if (data.checkHash == board->getBoardHash().z64key)
            {
                if ((data.isEnd() || data.endStep > startStep.step + global_currentMaxDepth))
                {
                    transTableStat.hit++;
                    tempPath.situationRating = data.situationRating;
                    tempPath.endStep = data.endStep;
                }
                else
                {
                    transTableStat.cover++;
                    doKillSearch(board, tempPath, bestRating, atackSide);
                    data.checkHash = board->getBoardHash().z64key;
                    data.endStep = tempPath.endStep;
                    data.endSide = tempPath.path.back().getColor();
                    data.situationRating = tempPath.situationRating;
                    putTransTable(board->getBoardHash().z32key, data);
                }
            }
            else
            {
                transTableStat.clash++;
                doKillSearch(board, tempPath, bestRating, atackSide);
                data.checkHash = board->getBoardHash().z64key;
                data.endStep = tempPath.endStep;
                data.endSide = tempPath.path.back().getColor();
                data.situationRating = tempPath.situationRating;
                putTransTable(board->getBoardHash().z32key, data);
            }
        }
        else
        {
            transTableStat.miss++;
            doKillSearch(board, tempPath, bestRating, atackSide);
            data.checkHash = board->getBoardHash().z64key;
            data.endStep = tempPath.endStep;
            data.endSide = tempPath.path.back().getColor();
            data.situationRating = tempPath.situationRating;
            putTransTable(board->getBoardHash().z32key, data);
        }

        board->unmove(index, oldindex);
        if (global_isOverTime)
        {
            return;
        }

        if (laststep.getColor() == atackSide)//防守
        {
            bestPath = tempPath;
            break;//只需要堵一个点，如果有两个点需要堵，肯定就输了
        }
        else//进攻
        {
            if (laststep.black == startStep.black)//build AI
            {
                if (tempPath.situationRating == util::type2score(CHESSTYPE_5))
                {
                    bestPath = tempPath;
                    bestRating = util::type2score(CHESSTYPE_5);
                    break;
                }
                if (tempPath.situationRating > bestRating)
                {
                    bestPath = tempPath;
                    bestRating = tempPath.situationRating;
                }
            }
            else // build player
            {
                if (tempPath.situationRating == -util::type2score(CHESSTYPE_5))
                {
                    bestPath = tempPath;
                    bestRating = -util::type2score(CHESSTYPE_5);
                    break;
                }
                if (tempPath.situationRating < bestRating)
                {
                    bestPath = tempPath;
                    bestRating = tempPath.situationRating;
                }
            }
        }
    }
    if (laststep.step >= maxKillSearchDepth)
    {
        maxKillSearchDepth = laststep.step;
    }
    if (bestPath.startStep != 255)
    {
        optimalPath.situationRating = bestPath.situationRating;
        optimalPath.endStep = bestPath.endStep;
    }
    else if (board->getHighestScore(laststep.getColor()) == util::type2score(CHESSTYPE_5))
    {
        optimalPath.situationRating = laststep.getColor() == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
    }

}
