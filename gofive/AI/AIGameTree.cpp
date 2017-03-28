#include "AIGameTree.h"
#include "GameTree.h"

AIGameTree::AIGameTree()
{
}


AIGameTree::~AIGameTree()
{
}

Position AIGameTree::getNextStep(ChessBoard *cb, AIParam param)
{
    if (cb->lastStep.step == 0)
    {
        return Position{ 7,7 };
    }
    ChessBoard::setLevel(param.level);
    ChessBoard::setBan(param.ban);
    cb->setGlobalThreat();
    GameTreeNode root(cb, param.caculateSteps, 1);
    GameTreeNode::playerColor = (cb->lastStep.getColor());
    GameTreeNode::multiThread = param.multithread;
    Position result = root.getBestStep();

    return result;
}

