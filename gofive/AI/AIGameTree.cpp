#include "AIGameTree.h"
#include "GameTree.h"
#include "ThreadPool.h"

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
