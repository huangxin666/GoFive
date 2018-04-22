#include "AIEngine.h"
#include "AIConfig.h"

AISimple::AISimple()
{
}


AISimple::~AISimple()
{
}

Position AISimple::getNextStep(ChessBoard *cb, time_t start_time_ms)
{
    uint8_t side = cb->getLastStep().getOtherSide();
    ChessBoard tempBoard;
    Position stepCurrent;
    Position randomStep[BOARD_INDEX_BOUND];
    PieceInfo highest = { Position(),0 }, tempInfo = { Position(),0 };
    randomStep[0].row = 0;
    randomStep[0].col = 0;
    int randomCount = 0;
    int HighestScore = INT_MIN;
    int StepScore = 0;
    ForEachMove(cb)
    {

        if (tempBoard.getChessType(pos, side) == CHESSTYPE_BAN)
        {
            StepScore = -CHESSTYPE_5_SCORE;
            continue;
        }
        else if (tempBoard.getChessType(pos, side) == CHESSTYPE_5)
        {
            return Position(pos.row,pos.col);
        }
        else if (tempBoard.getChessType(pos, side) >= CHESSTYPE_43)
        {
            if (!tempBoard.hasChessType(Util::otherside(side), CHESSTYPE_5))
            {
                return Position(pos.row,pos.col);
            }
        }
        else if (tempBoard.getChessType(pos, side) >= CHESSTYPE_33)
        {
            if (tempBoard.getHighestType(Util::otherside(side)) < CHESSTYPE_43)
            {
                return Position(pos.row,pos.col);
            }
        }
        else
        {
            tempBoard = *cb;
            tempBoard.move(pos, AIConfig::getInstance()->rule);
            StepScore = tempBoard.getGlobalEvaluate(side);
        }

        if (StepScore > HighestScore)
        {
            HighestScore = StepScore;
            stepCurrent = pos;
            randomCount = 0;
            randomStep[randomCount] = stepCurrent;
            randomCount++;
        }
        else if (StepScore == HighestScore)
        {
            stepCurrent = pos;
            randomStep[randomCount] = stepCurrent;
            randomCount++;
        }
        
    }

    int random = rand() % randomCount;
    return randomStep[random];
}
