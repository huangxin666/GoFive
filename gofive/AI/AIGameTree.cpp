#include "AIEngine.h"
#include "GameTree.h"
#include "ThreadPool.h"

AIGameTree::AIGameTree()
{
}


AIGameTree::~AIGameTree()
{
}

void AIGameTree::updateTextOut()
{
    char text[1024];
    snprintf(text,1024,"hit: %llu \n miss:%llu \n clash:%llu \n", GameTreeNode::transTableHashStat.hit, GameTreeNode::transTableHashStat.miss, GameTreeNode::transTableHashStat.clash);
    textOut = text;
    if (GameTreeNode::resultFlag == AIRESULTFLAG_NEARWIN)
    {
        textOut += ("哈哈，你马上要输了！\n");
    }
    else if (GameTreeNode::resultFlag == AIRESULTFLAG_FAIL)
    {
        textOut += (("你牛，我服！\n"));
    }
    else if (GameTreeNode::resultFlag == AIRESULTFLAG_TAUNT)
    {
        textOut += (("别挣扎了，没用的\n"));
    }
    else if (GameTreeNode::resultFlag == AIRESULTFLAG_COMPLAIN)
    {
        textOut += (("你牛，差点中招！\n"));
    }
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
