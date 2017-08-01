#include "AIEngine.h"
#include "utils.h"
AIWalker::AIWalker(int type)
{
    AIType = type;
}


AIWalker::~AIWalker()
{
}

void AIWalker::updateTextOut()
{

}

void AIWalker::applyAISettings(AISettings setting)
{
    ChessBoard::setBan(setting.ban);
}

Position AIWalker::getNextStep(ChessBoard *cb, time_t start_time_ms)
{
    Position result;
    if (AIType == 1)
    {
        result = level1(cb);
    }
    else if (AIType == 2)
    {
        result = level2(cb);
    }
    return result;
}

Position AIWalker::level1(ChessBoard *currentBoard)
{
    PIECE_STATE side = currentBoard->getLastStep().getOtherSide();
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
                StepScore = util::type2score(currentBoard->getChessType(i, j, side));
                currentBoard->move(Util::xy2index(i, j));
                StepScore = StepScore - util::type2score(currentBoard->getHighestInfo(Util::otherside(side)).chesstype);
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
                currentBoard->unmove(Util::xy2index(i, j), oldIndex);
            }
        }
    }

    int random = rand() % (randomCount + 1);
    return randomStep[random];
    
}

Position AIWalker::level2(ChessBoard *currentBoard)
{
    PIECE_STATE side = currentBoard->getLastStep().getOtherSide();
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
                StepScore = util::type2score(tempBoard.getChessType(i, j, side));
                tempBoard.move(Util::xy2index(i, j));
                highest = tempBoard.getHighestInfo(Util::otherside(side));
                //³ö¿Ú
                if (StepScore >= chesstypes[CHESSTYPE_5].rating)
                {
                    return Position{ i,j };
                }
                else if (StepScore >= chesstypes[CHESSTYPE_43].rating)
                {
                    if (chesstypes[highest.chesstype].rating < chesstypes[CHESSTYPE_5].rating)
                    {
                        return Position{ i,j };
                    }
                }
                else if (StepScore >= chesstypes[CHESSTYPE_33].rating)
                {
                    if (chesstypes[highest.chesstype].rating < chesstypes[CHESSTYPE_43].rating)
                    {
                        return Position{ i,j };
                    }
                }
                StepScore = StepScore - tempBoard.getTotalRating(Util::otherside(side));

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
