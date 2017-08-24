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
    enableDebug = false;
    maxAlphaBetaDepth = 20;
    minAlphaBetaDepth = 2;
    VCFExpandDepth = 15;//³åËÄ
    VCTExpandDepth = 6;//×·Èý
    useTranTable = true;
    fullSearch = false;
    multithread = false;
}

void AIGoSearch::applyAISettings(AISettings setting)
{
    ChessBoard::setBan(setting.ban);
    this->setting = setting;
}

Position AIGoSearch::getNextStep(ChessBoard *cb, time_t start_time)
{
    GoSearchEngine engine;
    engine.applySettings(
        setting.maxStepTimeMs,
        setting.restMatchTimeMs,
        setting.maxMemoryBytes,
        setting.minAlphaBetaDepth,
        setting.maxAlphaBetaDepth,
        setting.VCFExpandDepth,
        setting.VCTExpandDepth,
        setting.enableDebug,
        setting.useTranTable,
        setting.fullSearch,
        setting.multithread
    );
    engine.initSearchEngine(cb);
    return engine.getBestStep(system_clock::to_time_t(system_clock::now()));
}

void AIGoSearch::getMoveList(ChessBoard* board, vector<pair<Position, int>>& moves, int type, bool global)
{
    GoSearchEngine engine;
    engine.initSearchEngine(board);
    vector<StepCandidateItem> list;
    if (type == 1)
    {
        set<Position> myset;
        //engine.getNormalRelatedSet(board, myset);
        GoSearchEngine::getNormalSteps(board, list, myset.empty() ? NULL : &myset, false);
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
        moves.emplace_back(step.pos, step.priority);
    }
}