#include "AIGameTree.h"
#include "TreeNode.h"

AIGameTree::AIGameTree()
{
}


AIGameTree::~AIGameTree()
{
}

Position AIGameTree::getNextStep(ChessBoard *cb, AIParam param)
{
    ChessBoard::setLevel(param.level);
    ChessBoard::setBan(param.ban);
    cb->setGlobalThreat();
    TreeNode root(cb, param.caculateSteps, 1);
    root.setPlayerColor(cb->lastStep.getColor());
    Position result = root.searchBest();

    return result;
}

