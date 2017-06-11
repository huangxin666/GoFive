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
    snprintf(text,1024,"hit: %llu \r\nmiss:%llu \r\nclash:%llu \r\n", GameTreeNode::transTableHashStat.hit, GameTreeNode::transTableHashStat.miss, GameTreeNode::transTableHashStat.clash);
    textOut = text;
    if (GameTreeNode::resultFlag == AIRESULTFLAG_NEARWIN)
    {
        textOut += ("������������Ҫ���ˣ�\r\n");
    }
    else if (GameTreeNode::resultFlag == AIRESULTFLAG_FAIL)
    {
        textOut += (("��ţ���ҷ���\r\n"));
    }
    else if (GameTreeNode::resultFlag == AIRESULTFLAG_TAUNT)
    {
        textOut += (("�������ˣ�û�õ�\r\n"));
    }
    else if (GameTreeNode::resultFlag == AIRESULTFLAG_COMPLAIN)
    {
        textOut += (("��ţ��������У�\r\n"));
    }
}

void AIGameTree::applyAISettings(AISettings setting)
{

}

Position AIGameTree::getNextStep(ChessBoard *cb, AISettings setting,ChessStep lastStep, uint8_t side, uint8_t level, bool ban)
{
    ChessBoard::setLevel(level);
    ChessBoard::setBan(ban);
    GameTreeNode::initTree(setting.maxSearchDepth,setting.multiThread, lastStep.getColor(), lastStep.step);
    GameTreeNode root(cb, lastStep);
    Position result = root.getBestStep();
    
    return result;
}

void AIGameTree::setThreadPoolSize(int num)
{
    ThreadPool::num_thread = num;
}
