#include "AIEngine.h"

AIWalker::AIWalker(int type)
{
    AIType = type;
}


AIWalker::~AIWalker()
{
}

bool AIWalker::getMessage(string &msg)
{
    return false;
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
    uint8_t side = currentBoard->getLastStep().getOtherSide();
    Position stepCurrent;
    Position randomStep[BOARD_INDEX_BOUND];
    randomStep[0].row = 0;
    randomStep[0].col = 0;
    int randomCount = 0;
    int HighestScore = -500000;
    int HighestScoreTemp = -500000;
    int StepScore = 0;
    int score = 0;
    ChessStep oldstep = currentBoard->getLastStep();
    ForEachPosition
    {
        if (currentBoard->canMove(pos.row,pos.col))
        {
            StepScore = ChessBoard::getChessTypeInfo(currentBoard->getChessType(pos.row, pos.col, side)).rating;
            currentBoard->move(pos.row, pos.col);
            StepScore = StepScore - ChessBoard::getChessTypeInfo(currentBoard->getHighestInfo(Util::otherside(side)).chesstype).rating;
            if (StepScore > HighestScore)
            {
                HighestScore = StepScore;
                stepCurrent = pos;
                randomCount = 0;
                randomStep[randomCount] = stepCurrent;
            }
            else if (StepScore == HighestScore)
            {
                stepCurrent = pos;
                randomCount++;
                randomStep[randomCount] = stepCurrent;
            }
            currentBoard->unmove(pos.row, pos.col, oldstep);
        }
    }


    int random = rand() % (randomCount + 1);
    return randomStep[random];

}

Position AIWalker::level2(ChessBoard *currentBoard)
{
    uint8_t side = currentBoard->getLastStep().getOtherSide();
    ChessBoard tempBoard;
    Position stepCurrent;
    Position randomStep[BOARD_INDEX_BOUND];
    PieceInfo highest = { Position(),0 }, tempInfo = { Position(),0 };
    randomStep[0].row = 0;
    randomStep[0].col = 0;
    int randomCount = 0;
    int HighestScore = -500000;
    int StepScore = 0;
    ForEachPosition
    {
        if (currentBoard->canMove(pos.row,pos.col))
        {
            tempBoard = *currentBoard;
            StepScore = ChessBoard::getChessTypeInfo(tempBoard.getChessType(pos.row, pos.col, side)).rating;
            tempBoard.move(pos.row, pos.col);
            highest = tempBoard.getHighestInfo(Util::otherside(side));
            //³ö¿Ú
            if (StepScore >= CHESSTYPE_5_SCORE)
            {
                return Position(pos.row,pos.col);
            }
            else if (StepScore >= ChessBoard::getChessTypeInfo(CHESSTYPE_43).rating)
            {
                if (ChessBoard::getChessTypeInfo(highest.chesstype).rating < ChessBoard::getChessTypeInfo(CHESSTYPE_5).rating)
                {
                    return Position(pos.row,pos.col);
                }
            }
            else if (StepScore >= ChessBoard::getChessTypeInfo(CHESSTYPE_33).rating)
            {
                if (ChessBoard::getChessTypeInfo(highest.chesstype).rating < ChessBoard::getChessTypeInfo(CHESSTYPE_43).rating)
                {
                    return Position(pos.row,pos.col);
                }
            }
            StepScore = StepScore - tempBoard.getTotalRating(Util::otherside(side));

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
