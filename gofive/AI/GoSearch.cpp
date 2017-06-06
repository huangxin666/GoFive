#include "GoSearch.h"
#include <algorithm>

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
    this->transTableStat = { 0,0,0 };
    this->board = board;
    this->startStep = lastStep;
    this->maxSearchTime = 10000;
}

uint8_t GoSearchEngine::getBestStep()
{
    global_OverTime = false;
    global_AlphaBetaDepth = 6;
    global_StartSearchTime = std::time(NULL);
    OptimalPath bestPath;
    bestPath.startStep = startStep.step;
    doAlphaBeta(board, INT_MIN, INT_MAX, bestPath);
    return bestPath.path[0].index;
}

void GoSearchEngine::doAlphaBeta(ChessBoard* board, int alpha, int beta, OptimalPath& optimalPath)
{
    
    ChessStep laststep;
    if (optimalPath.path.empty())
    {
        laststep = startStep;
    }
    else
    {
        laststep = optimalPath.path.back();
    }

    OptimalPath bestPath;
    if (laststep.black == startStep.black)//build AI
    {
        bestPath.boardScore = INT_MIN;
    }
    else
    {
        bestPath.boardScore = INT_MAX;
    }

    //≥ˆø⁄
    if (std::time(NULL) - global_StartSearchTime > maxSearchTime)
    {
        global_OverTime = true;
        return;
    }

    if (board->getHighestScore(util::otherside(laststep.getColor())) == util::type2score(CHESSTYPE_5))
    {
        optimalPath.boardScore = laststep.getColor() == startStep.getColor() ? util::type2score(CHESSTYPE_5) : -util::type2score(CHESSTYPE_5);
        ChessStep step;
        step.black = laststep.black ? false : true;
        step.chessType = CHESSTYPE_5;
        step.index = board->getHighestInfo(util::otherside(laststep.getColor())).index;
        step.step = laststep.step +1 ;
        optimalPath.path.push_back(step);
        return;
    }

    vector<GoTreeNode> childs;
    if (laststep.step - startStep.step > global_AlphaBetaDepth)
    {
        return;
        doKillCalculate();//À„…±
    }
    else
    {
        getNextSteps(board, util::otherside(laststep.getColor()), childs);
    }

    for (size_t i = 0; i < childs.size(); ++i)
    {
        OptimalPath tempPath;
        uint8_t oldindex = board->getLastStep().index;
        board->move(childs[i].index);
        
        tempPath.startStep = laststep.step;

        ChessStep step;
        step.black = childs[i].side == PIECE_BLACK ? true : false;
        step.chessType = childs[i].chessType;
        step.index = childs[i].index;
        step.step = tempPath.startStep + 1;

        tempPath.path.push_back(step);
        tempPath.boardScore = board->getSituationRating(getAISide());
        doAlphaBeta(board, alpha, beta, tempPath);
        board->unmove(childs[i].index, oldindex);
        if (global_OverTime)
        {
            return;
        }
        if (laststep.black == startStep.black)//build AI
        {
            if (tempPath.boardScore > bestPath.boardScore 
                ||(tempPath.boardScore == bestPath.boardScore && tempPath.path.size() < bestPath.path.size())
                )
            {
                bestPath.boardScore = tempPath.boardScore;
                bestPath.startStep = tempPath.startStep;
                bestPath.path.swap(tempPath.path);
            }

            if (tempPath.boardScore > beta)//beta cut
            {
                break;
            }
            if (tempPath.boardScore > alpha)//update alpha
            {
                alpha = tempPath.boardScore;
            }
        }
        else // build player
        {
            if (tempPath.boardScore < bestPath.boardScore 
                || (tempPath.boardScore == bestPath.boardScore && tempPath.path.size() > bestPath.path.size())
                )
            {
                bestPath.boardScore = tempPath.boardScore;
                bestPath.startStep = tempPath.startStep;
                bestPath.path.swap(tempPath.path);
            }

            if (tempPath.boardScore < alpha)//alpha cut
            {
                break;
            }
            if (tempPath.boardScore < beta)//update beta
            {
                beta = tempPath.boardScore;
            }
        }
    }

    if (!bestPath.path.empty())
    {
        optimalPath.boardScore = bestPath.boardScore;
        for (size_t i = 0; i < bestPath.path.size(); ++i)
        {
            optimalPath.path.push_back(bestPath.path[i]);
        }
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
        node.globalRating = board->getSituationRating(getAISide());
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

void GoSearchEngine::doKillCalculate()
{
}
