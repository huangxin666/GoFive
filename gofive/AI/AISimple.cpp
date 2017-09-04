#include "AIEngine.h"

AISimple::AISimple()
{
}


AISimple::~AISimple()
{
}

bool AISimple::getMessage(string &msg)
{
    return false;
}

void AISimple::applyAISettings(AISettings setting)
{
    ChessBoard::setBan(setting.ban);
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
    ForEachPosition
    {
        if (cb->canMove(pos.row,pos.col))
        {
            if (tempBoard.getChessType(pos.row, pos.col, side) == CHESSTYPE_BAN)
            {
                StepScore = -CHESSTYPE_5_SCORE;
                continue;
            }
            else if (tempBoard.getChessType(pos.row, pos.col, side) == CHESSTYPE_5)
            {
                return Position(pos.row,pos.col);
            }
            else if (tempBoard.getChessType(pos.row, pos.col, side) >= CHESSTYPE_43)
            {
                if (tempBoard.getHighestInfo(Util::otherside(side)).chesstype < CHESSTYPE_5)
                {
                    return Position(pos.row,pos.col);
                }
            }
            else if (tempBoard.getChessType(pos.row, pos.col, side) >= CHESSTYPE_33)
            {
                if (tempBoard.getHighestInfo(Util::otherside(side)).chesstype < CHESSTYPE_43)
                {
                    return Position(pos.row,pos.col);
                }
            }
            else
            {
                tempBoard = *cb;
                tempBoard.move(pos.row, pos.col);
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
    }

    int random = rand() % randomCount;
    return randomStep[random];
}
