#include "GoSearch.h"
#include <algorithm>

HashStat GoSearchEngine::transTableStat;

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
    this->maxSearchTime = 10000;
}

uint8_t GoSearchEngine::getBestStep()
{
    global_isOverTime = false;
    global_currentMaxDepth = 10;
    global_startSearchTime = std::time(NULL);
    OptimalPath bestPath;
    bestPath.startStep = startStep.step;
    doAlphaBetaSearch(board, INT_MIN, INT_MAX, bestPath);
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
        step.step = laststep.step +1 ;
        optimalPath.path.push_back(step);
        optimalPath.endStep = laststep.step + 1;
        return;
    }

    vector<GoTreeNode> childs;
    if (laststep.step - startStep.step > global_currentMaxDepth)
    {
        //doKillSearch(board, optimalPath, util::otherside(board->getLastStep().getColor()));
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
        uint8_t oldindex = board->getLastStep().index;
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
                    if ((data.isEnd() || data.endStep >= startStep.step + global_currentMaxDepth))
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
                ||(tempPath.situationRating == bestPath.situationRating && tempPath.endStep < bestPath.endStep)
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

}

void GoSearchEngine::getNextSteps(ChessBoard* board, uint8_t side, vector<GoTreeNode>& childs)
{
    for (uint8_t index = 0; util::valid(index); ++index)
    {
        if (!board->canMove(index))
        {
            continue;
        }
        if (board->getThreat(index,side) == 0 && board->getThreat(index, util::otherside(side)) == 0)
        {
            continue;
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

void GoSearchEngine::doKillSearch(ChessBoard* board, OptimalPath& optimalPath, uint8_t atackSide)
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

    if (laststep.step - startStep.step > maxKillToEndDepth)
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
        if (laststep.getColor() == atackSide )//防守
        {
            if (board->getChessType(index, laststep.getColor()) != CHESSTYPE_5)
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

        uint8_t oldindex = board->getLastStep().index;

        board->move(index);

        OptimalPath tempPath;
        tempPath.startStep = laststep.step;
        tempPath.path.push_back(step);
        tempPath.situationRating = board->getSituationRating(getAISide());

        TransTableData data;
        if (getTransTable(board->getBoardHash().z32key, data))
        {
            if (data.checkHash == board->getBoardHash().z64key)
            {
                if ((data.isEnd() || data.endStep >= startStep.step + global_currentMaxDepth))
                {
                    transTableStat.hit++;
                    tempPath.situationRating = data.situationRating;
                    tempPath.endStep = data.endStep;
                }
                else
                {
                    transTableStat.cover++;
                    doKillSearch(board, tempPath, atackSide);
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
                doKillSearch(board, tempPath, atackSide);
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
            doKillSearch(board, tempPath, atackSide);
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
                if(tempPath.situationRating == util::type2score(CHESSTYPE_5))
                {
                    bestPath = tempPath;
                    break;
                }
            }
            else // build player
            {
                if (tempPath.situationRating == -util::type2score(CHESSTYPE_5))
                {
                    bestPath = tempPath;
                    break;
                }
            }
        }
    }

    if (bestPath.startStep != 255)
    {
        if (bestPath.situationRating == util::type2score(CHESSTYPE_5) || bestPath.situationRating == -util::type2score(CHESSTYPE_5))
        {
            optimalPath.situationRating = bestPath.situationRating;
            optimalPath.endStep = bestPath.endStep;
        }
    }
}
