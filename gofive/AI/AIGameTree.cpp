#include "AIEngine.h"
#include "GameTree.h"

AIGameTree::AIGameTree()
{
}


AIGameTree::~AIGameTree()
{
}

static time_point<system_clock> startSearchTime;
static bool finished = false;
bool AIGameTree::getMessage(string &msg)
{
    if (!finished)
    {
        return false;
    }
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
    msg = text;
    return true;
}

void AISettings::defaultGameTree(uint8_t level)
{

}

void AIGameTree::applyAISettings(AISettings setting)
{
    ChessBoard::setBan(setting.ban);
    GameTreeNode::initTree(setting.maxSearchDepth, setting.enableAtack, setting.extraSearch);
}

Position AIGameTree::getNextStep(ChessBoard *cb, time_t start_time)
{
    finished = false;
    startSearchTime = system_clock::from_time_t(start_time);
    GameTreeNode root(cb);
    Position result = root.getBestStep(cb->getLastStep().getState(), cb->getLastStep().step);
    finished = true;
    return result;
}
