#include "AIEngine.h"
#include "GoSearch.h"

AIGoSearch::AIGoSearch()
{
}

AIGoSearch::~AIGoSearch()
{
}

Position AIGoSearch::getNextStep(ChessBoard *cb, time_t start_time)
{
    GoSearchEngine engine;
    engine.applySettings();
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
        board->getThreatCandidates(2, list);
    }

    for (auto step : list)
    {
        moves.emplace_back(step.pos, step.value);
    }
}