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
    cb->setGlobalThreat();//要在setBan之后调用
    GameTreeNode root(cb);
    root.initTree(param, cb->lastStep.getColor());
    Position result = root.getBestStep();

    return result;
}

