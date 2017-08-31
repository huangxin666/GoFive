#include "AIEngine.h"
#include "GoSearch.h"

AIGoSearch::AIGoSearch()
{
}

AIGoSearch::~AIGoSearch()
{
}

bool AIGoSearch::getMessage(string &msg)
{
    return GoSearchEngine::getDebugMessage(msg);
}

void AISettings::defaultGoSearch(uint8_t level)
{
    enableDebug = false;
    maxAlphaBetaDepth = 20;
    minAlphaBetaDepth = 2;
    VCFExpandDepth = 10;//����
    VCTExpandDepth = 0;//׷��
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