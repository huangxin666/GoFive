#include "AIEngine.h"
#include "GoSearch.h"

AIGoSearch::AIGoSearch()
{
}

AIGoSearch::~AIGoSearch()
{
}

void AISettings::defaultGoSearch(uint8_t level)
{
    enableDebug = false;
    maxAlphaBetaDepth = 20;
    minAlphaBetaDepth = 2;
    VCFExpandDepth = 10;//³åËÄ
    VCTExpandDepth = 0;//×·Èý
    useTransTable = true;
    useDBSearch = true;
    multithread = false;
}

Position AIGoSearch::getNextStep(ChessBoard *cb, time_t start_time, AISettings setting)
{
    GoSearchEngine engine;
    engine.applySettings(setting);
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
        board->getNormalCandidates(list, false);
    }
    else if (type == 2)
    {
        board->getVCTCandidates(list, NULL);
    }
    else if (type == 3)
    {
        board->getVCFCandidates(list, NULL);
    }

    for (auto step : list)
    {
        moves.emplace_back(step.pos, step.priority);
    }
}