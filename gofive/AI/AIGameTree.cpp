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
        AIsay = ("哈哈，你马上要输了！\r\n");
    }
    else if (GameTreeNode::resultFlag == AIRESULTFLAG_FAIL)
    {
        AIsay = (("你牛，我服！\r\n"));
    }
    else if (GameTreeNode::resultFlag == AIRESULTFLAG_TAUNT)
    {
        AIsay = (("别挣扎了，没用的\r\n"));
    }
    else if (GameTreeNode::resultFlag == AIRESULTFLAG_COMPLAIN)
    {
        AIsay = (("你牛，差点中招！\r\n"));
    }

    snprintf(text, 1024, "hit: %llu \r\nmiss:%llu \r\nclash:%llu \r\n %s\r\n 用时:%lldms",
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
