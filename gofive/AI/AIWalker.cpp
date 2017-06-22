#include "AIEngine.h"
#include "GoSearch.h"


AIWalker::AIWalker()
{
}


AIWalker::~AIWalker()
{
}

void AIWalker::updateTextOut()
{

}

void AIWalker::applyAISettings(AISettings setting)
{

}

Position AIWalker::getNextStep(ChessBoard *cb, AISettings setting, ChessStep lastStep, uint8_t side, uint8_t level, bool ban)
{
    ChessBoard::setLevel(level);
    ChessBoard::setBan(ban);
    Position result;
    if (level == 1)
    {
        result = level1(cb, side);
    }
    else if (level == 2)
    {
        result = level2(cb, side);
    }
    return result;
}

Position AIWalker::level1(ChessBoard *currentBoard, uint8_t side)
{
    Position stepCurrent;
    Position randomStep[225];
    randomStep[0].row = 0;
    randomStep[0].col = 0;
    int randomCount = 0;
    int HighestScore = -500000;
    int HighestScoreTemp = -500000;
    int StepScore = 0;
    int score = 0;
    ChessStep oldIndex = currentBoard->getLastStep();
    for (int i = 0; i < BOARD_ROW_MAX; ++i)
    {
        for (int j = 0; j < BOARD_COL_MAX; ++j)
        {
            if (currentBoard->canMove(i, j))
            {
                StepScore = currentBoard->getThreat(i, j, side);
                currentBoard->move(util::xy2index(i, j));
                StepScore = StepScore - currentBoard->getHighestScore(util::otherside(side));
                if (StepScore > HighestScore)
                {
                    HighestScore = StepScore;
                    stepCurrent.row = i;
                    stepCurrent.col = j;
                    randomCount = 0;
                    randomStep[randomCount] = stepCurrent;
                }
                else if (StepScore == HighestScore)
                {
                    stepCurrent.row = i;
                    stepCurrent.col = j;
                    randomCount++;
                    randomStep[randomCount] = stepCurrent;
                }
                currentBoard->unmove(util::xy2index(i, j), oldIndex);
            }
        }
    }

    int random = rand() % (randomCount + 1);
    return randomStep[random];
    
}

Position AIWalker::level2(ChessBoard *currentBoard, uint8_t side)
{
    ChessBoard tempBoard;
    Position stepCurrent;
    Position randomStep[225];
    PieceInfo highest = { 0,0 }, tempInfo = { 0,0 };
    randomStep[0].row = 0;
    randomStep[0].col = 0;
    int randomCount = 0;
    int HighestScore = -500000;
    int StepScore = 0;
    for (int8_t i = 0; i < BOARD_ROW_MAX; ++i)
    {
        for (int8_t j = 0; j < BOARD_COL_MAX; ++j)
        {
            if (currentBoard->canMove(i, j))
            {
                tempBoard = *currentBoard;
                StepScore = tempBoard.getThreat(i, j, side);
                tempBoard.move(util::xy2index(i, j));
                highest = tempBoard.getHighestInfo(util::otherside(side));
                //³ö¿Ú
                if (StepScore >= chesstypes[CHESSTYPE_5].rating)
                {
                    return Position{ i,j };
                }
                else if (StepScore >= chesstypes[CHESSTYPE_43].rating)
                {
                    if (chesstypes[highest.chessmode].rating < chesstypes[CHESSTYPE_5].rating)
                    {
                        return Position{ i,j };
                    }
                }
                else if (StepScore >= chesstypes[CHESSTYPE_33].rating)
                {
                    if (chesstypes[highest.chessmode].rating < chesstypes[CHESSTYPE_43].rating)
                    {
                        return Position{ i,j };
                    }
                }
                StepScore = StepScore - tempBoard.getTotalRating(util::otherside(side));

                if (StepScore > HighestScore)
                {
                    HighestScore = StepScore;
                    stepCurrent.row = i;
                    stepCurrent.col = j;
                    randomCount = 0;
                    randomStep[randomCount] = stepCurrent;
                    randomCount++;
                }
                else if (StepScore == HighestScore)
                {
                    stepCurrent.row = i;
                    stepCurrent.col = j;
                    randomStep[randomCount] = stepCurrent;
                    randomCount++;
                }
            }
        }
    }
    int random = rand() % randomCount;
    return randomStep[random];
}
