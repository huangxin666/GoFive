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

    cb->setGlobalThreat();
    GameTreeNode root(cb);
    root.initTree(param, cb->lastStep.getColor());
    Position result = root.getBestStep();

    return result;
}

