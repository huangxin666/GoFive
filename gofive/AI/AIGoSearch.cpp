#include "AIEngine.h"
#include "GoSearch.h"

void AIGoSearch::updateTextOut()
{
    char text[1024];
    snprintf(text, 1024, "hit: %llu \n miss:%llu \n clash:%llu \n", GoSearchEngine::transTableStat.hit, GoSearchEngine::transTableStat.miss, GoSearchEngine::transTableStat.clash);
    textOut = text;
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
    ChessBoard::setLevel(level);
    ChessBoard::setBan(ban);
    Position result;

    GoSearchEngine engine;
    engine.initSearchEngine(cb, lastStep);
    uint8_t ret = engine.getBestStep();
    result.row = util::getrow(ret);
    result.col = util::getcol(ret);

    return result;
}