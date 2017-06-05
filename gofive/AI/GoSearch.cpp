#include "GoSearch.h"
#include <algorithm>

bool GoTreeNodeCmp(const GoTreeNode &a, const GoTreeNode &b)
{
    return a.stepScore < b.stepScore;
}

GoSearchEngine::GoSearchEngine() :board(NULL)
{
}

GoSearchEngine::~GoSearchEngine()
{

}

void GoSearchEngine::initSearchEngine(ChessBoard* board, ChessStep lastStep)
{
    transTableStat = { 0,0,0 };
    this->board = board;
    this->startStep = lastStep;
}

uint8_t GoSearchEngine::getBestStep()
{
    global_OverTime = false;
    global_AlphaBetaDepth = 6;
    global_StartSearchTime = std::time(NULL);
    OptimalPath bestPath;
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
        step.step = optimalPath.startStep + (uint8_t)optimalPath.path.size();
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
        board->move(childs[i].index, childs[i].side);
        tempPath.startStep = optimalPath.startStep + (uint8_t)optimalPath.path.size();
        tempPath.path.emplace_back(childs[i].index, childs[i].chessType, tempPath.startStep, childs[i].side == PIECE_BLACK ? true : false);
        tempPath.boardScore = board->getTotalRating(getAISide());
        doAlphaBeta(board, alpha, beta, tempPath);
        board->unmove(childs[i].index);
        if (global_OverTime)
        {
            return;
        }
        if (laststep.black == startStep.black)//build AI
        {
            if (tempPath.boardScore > bestPath.boardScore ||
                (tempPath.boardScore == bestPath.boardScore && tempPath.path.size() < bestPath.path.size()))
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
            if (tempPath.boardScore < bestPath.boardScore ||
                (tempPath.boardScore == bestPath.boardScore && tempPath.path.size() > bestPath.path.size()))
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
        GoTreeNode node;
        node.index = index;
        node.chessType = board->getChessType(index, side);
        node.globalRating = board->getBoardRating(getAISide(), util::otherside(side));
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
