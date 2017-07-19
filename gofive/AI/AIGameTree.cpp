#include "AIEngine.h"
#include "GameTree.h"
#include "ThreadPool.h"
#include <chrono>
using namespace std::chrono;
AIGameTree::AIGameTree()
{
}


AIGameTree::~AIGameTree()
{
}

static time_point<system_clock> startSearchTime;

void AIGameTree::updateTextOut()
{
    char text[1024];

    string AIsay;
    if (GameTreeNode::resultFlag == AIRESULTFLAG_NEARWIN)
    {
        AIsay = ("������������Ҫ���ˣ�\r\n");
    }
    else if (GameTreeNode::resultFlag == AIRESULTFLAG_FAIL)
    {
        AIsay = (("��ţ���ҷ���\r\n"));
    }
    else if (GameTreeNode::resultFlag == AIRESULTFLAG_TAUNT)
    {
        AIsay = (("�������ˣ�û�õ�\r\n"));
    }
    else if (GameTreeNode::resultFlag == AIRESULTFLAG_COMPLAIN)
    {
        AIsay = (("��ţ��������У�\r\n"));
    }

    snprintf(text, 1024, "hit: %llu \r\nmiss:%llu \r\nclash:%llu \r\n %s\r\n ��ʱ:%lldms",
        GameTreeNode::transTableHashStat.hit, GameTreeNode::transTableHashStat.miss, GameTreeNode::transTableHashStat.clash,
        AIsay.c_str(), duration_cast<milliseconds>(system_clock::now() - startSearchTime).count());
    textOut = text;
}

void AIGameTree::applyAISettings(AISettings setting)
{

}

Position AIGameTree::getNextStep(ChessBoard *cb, AISettings setting, ChessStep lastStep, uint8_t side, uint8_t level, bool ban)
{
    startSearchTime = system_clock::now();
    ChessBoard::setBan(ban);
    GameTreeNode::initTree(setting.maxSearchDepth, setting.multiThread, lastStep.getSide(), lastStep.step);
    GameTreeNode root(cb, lastStep);
    GameTreeNode::level = level;
    Position result = root.getBestStep();
    return result;
}

void AIGameTree::setThreadPoolSize(int num)
{
    ThreadPool::num_thread = num;
}
