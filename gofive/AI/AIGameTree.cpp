#include "AIEngine.h"
#include "GameTree.h"

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

void AISettings::defaultGameTree(AILEVEL level)
{

}

void AIGameTree::applyAISettings(AISettings setting)
{
    ChessBoard::setBan(setting.ban);
    GameTreeNode::initTree(setting.maxSearchDepth, setting.enableAtack, setting.extraSearch);
}

Position AIGameTree::getNextStep(ChessBoard *cb, time_t start_time)
{
    startSearchTime = system_clock::from_time_t(start_time);
    GameTreeNode root(cb);
    Position result = root.getBestStep(cb->getLastStep().getState(), cb->getLastStep().step);
    return result;
}
