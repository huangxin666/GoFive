#include "AIEngine.h"
#include "GoSearch.h"

AIGoSearch::AIGoSearch()
{
}

AIGoSearch::~AIGoSearch()
{
}

void AIGoSearch::updateTextOut()
{
    char text[1024];
    snprintf(text, 1024, "hit: %llu \r\nmiss:%llu \r\nclash:%llu \r\ncover:%llu \r\n", GoSearchEngine::transTableStat.hit, GoSearchEngine::transTableStat.miss, GoSearchEngine::transTableStat.clash, GoSearchEngine::transTableStat.cover);
    textOut = text;
    textOut = GoSearchEngine::textout + textOut;
}

void AISettings::defaultGoSearch(AILEVEL level)
{
    enableDebug = true;
    maxAlphaBetaDepth = 12;
    minAlphaBetaDepth = 4;
    VCFExpandDepth = 14;//³åËÄ
    VCTExpandDepth = 8;//×·Èý
    useTranTable = false;
}

void AIGoSearch::applyAISettings(AISettings setting)
{
    ChessBoard::setBan(setting.ban);
    GoSearchEngine::applySettings(
        setting.maxSearchTimeMs,
        setting.minAlphaBetaDepth,
        setting.maxAlphaBetaDepth,
        setting.VCFExpandDepth,
        setting.VCTExpandDepth,
        setting.enableDebug,
        setting.useTranTable
    );
}

Position AIGoSearch::getNextStep(ChessBoard *cb, time_t start_time)
{
    GoSearchEngine engine;
    engine.initSearchEngine(cb);
    uint8_t ret = engine.getBestStep(system_clock::to_time_t(system_clock::now()));
    return Position(ret);
}

void AIGoSearch::getMoveList(ChessBoard* board, vector<pair<uint8_t, int>>& moves, int type, bool global)
{
    GoSearchEngine engine;
    engine.initSearchEngine(board);
    vector<StepCandidateItem> list;
    if (type == 1)
    {
        set<uint8_t> myset;
        //engine.getNormalRelatedSet(board, myset);
        GoSearchEngine::getNormalSteps(board, list, myset.empty() ? NULL : &myset);
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