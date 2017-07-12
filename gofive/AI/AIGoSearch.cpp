#include "AIEngine.h"
#include "GoSearch.h"
#include <chrono>
using namespace std::chrono;

void AIGoSearch::updateTextOut()
{
    char text[1024];
    snprintf(text, 1024, "hit: %llu \r\nmiss:%llu \r\nclash:%llu \r\ncover:%llu \r\n", GoSearchEngine::transTableStat.hit, GoSearchEngine::transTableStat.miss, GoSearchEngine::transTableStat.clash, GoSearchEngine::transTableStat.cover);
    textOut = text;
    textOut += GoSearchEngine::textout;
}

void AIGoSearch::applyAISettings(AISettings setting)
{

}

AIGoSearch::AIGoSearch()
{
}

AIGoSearch::~AIGoSearch()
{
}

Position AIGoSearch::getNextStep(ChessBoard *cb, AISettings setting, ChessStep lastStep, uint8_t side, uint8_t level, bool ban)
{
    ChessBoard::setBan(ban);

    GoSearchEngine engine;
    engine.initSearchEngine(cb, lastStep, system_clock::to_time_t(system_clock::now()), setting.maxSearchTime);
    uint8_t ret = engine.getBestStep();

    return Position(ret);
}

void AIGoSearch::getMoveList(ChessBoard* board, vector<pair<uint8_t, int>>& moves, int type, bool global)
{
    vector<StepCandidateItem> list;
    if (type == 1)
    {
        GoSearchEngine::getNormalSteps(board, list);
    }
    else if (type == 2)
    {
        GoSearchEngine::getVCTAtackSteps(board, list, NULL);
    }
    else if (type == 3)
    {
        GoSearchEngine::getVCFAtackSteps(board, list, NULL);
    }

    for (auto step : list)
    {
        moves.emplace_back(step.index, step.priority);
    }
}