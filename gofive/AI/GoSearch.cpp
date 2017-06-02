#include "GoSearch.h"
#include <algorithm>

GoSearchEngine::GoSearchEngine(ChessBoard * board, ChessStep lastStep) :board(board), startStep(lastStep)
{
}

GoSearchEngine::~GoSearchEngine()
{

}

void GoSearchEngine::initSearchEngine()
{
    transTableStat = { 0,0,0 };
}

uint8_t GoSearchEngine::getBestStep()
{
    vector<GoTreeNode> childs;
    getNextSteps(board, util::otherside(startStep.getColor()), childs);
    return uint8_t();
}

void GoSearchEngine::doAlphaBeta(ChessBoard* board, int alpha, int beta, OptimalPath& optimalPath)
{
    vector<GoTreeNode> childs;
    OptimalPath bestPath;

    ChessStep laststep;
    if (optimalPath.path.empty())
    {
        laststep = startStep;
    }
    else
    {
        laststep = optimalPath.path.back();
    }

    //≥ˆø⁄
    if (std::time(NULL) - global_StartSearchTime > maxSearchTime)
    {
        global_OverTime = true;
        return;
    }
    if (laststep.step - startStep.step > global_AlphaBetaDepth)
    {
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
        tempPath.startStep = optimalPath.startStep + optimalPath.path.size();
        tempPath.path.emplace_back(childs[i].index, childs[i].chessType, tempPath.startStep, childs[i].side == PIECE_BLACK ? true : false);
        //path.boardScore = childs[i].globalRating;
        doAlphaBeta(board, alpha, beta, tempPath);
        board->unmove(childs[i].index);
        if (global_OverTime)
        {
            return;
        }
    }

    if (bestPath.path.empty())
    {

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
        childs.push_back(GoTreeNode{ index, board->getChessType(index, side), side, board->getBoardRating(startStep.getColor(),util::otherside(side)) });
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
