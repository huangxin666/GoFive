#include "ChessAI.h"
#include "GameTree.h"
#include "ThreadPool.h"

AIGameTree::AIGameTree()
{
}


AIGameTree::~AIGameTree()
{
}

void AIGameTree::applyAISettings(AISettings setting)
{

}

Position AIGameTree::getNextStep(ChessBoard *cb, AISettings setting,ChessStep lastStep, uint8_t side, uint8_t level, bool ban)
{
    ChessBoard::setLevel(level);
    ChessBoard::setBan(ban);
    GameTreeNode::initTree(setting, lastStep.getColor(), lastStep.step);
    GameTreeNode root(cb, lastStep);
    Position result = root.getBestStep();
    return result;
}

void AIGameTree::setThreadPoolSize(int num)
{
    ThreadPool::num_thread = num;
}


AIRESULTFLAG AIGameTree::getResultFlag()
{
    return GameTreeNode::resultFlag;
}

HashStat AIGameTree::getTransTableHashStat()
{
    return GameTreeNode::transTableHashStat;
}
