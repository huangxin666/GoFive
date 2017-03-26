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
    TreeNode::playerColor = (cb->lastStep.getColor());
    TreeNode::multiThread = param.multithread;
    Position result = root.getBestStep();

    return result;
}

