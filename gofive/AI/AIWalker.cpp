#include "ChessAI.h"



AIWalker::AIWalker()
{
}


AIWalker::~AIWalker()
{
}

void AIWalker::applyAISettings(AISettings setting)
{

}

Position AIWalker::getNextStep(ChessBoard *cb, uint8_t side, uint8_t level, bool ban)
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
    int state = side;
    randomStep[0].row = 0;
    randomStep[0].col = 0;
    int randomCount = 0;
    int HighestScore = -500000;
    int HighestScoreTemp = -500000;
    int StepScore = 0;
    int score = 0;
    for (int i = 0; i < BOARD_ROW_MAX; ++i) 
    {
        for (int j = 0; j < BOARD_COL_MAX; ++j) 
        {
            if (currentBoard->isHot(i, j) && currentBoard->getState(i, j) == PIECE_BLANK)
            {
                StepScore = chess_ratings[currentBoard->getThreat(i, j, state)];
                currentBoard->move(util::xy2index(i, j), side);
                HighestScoreTemp = chess_ratings[currentBoard->getHighestInfo(util::otherside(state)).chessmode];
                StepScore = StepScore - HighestScoreTemp;
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
                currentBoard->unmove(util::xy2index(i, j));
            }
        }
    }

    int random = rand() % (randomCount + 1);
    return randomStep[random];
}

Position AIWalker::level2(ChessBoard *currentBoard, uint8_t side)
{
    int state = side;
    ChessBoard tempBoard;
    Position stepCurrent;
    Position randomStep[225];
    PieceInfo highest = { 0,0 }, tempInfo = { 0,0 };
    randomStep[0].row = 0;
    randomStep[0].col = 0;
    int randomCount = 0;
    int HighestScore = -500000;
    int StepScore = 0;
    for (int i = 0; i < BOARD_ROW_MAX; ++i) 
    {
        for (int j = 0; j < BOARD_COL_MAX; ++j) 
        {
            if (currentBoard->isHot(i, j) && currentBoard->getState(i, j) == PIECE_BLANK)
            {
                tempBoard = *currentBoard;
                //StepScore = tempBoard.getPiece(i, j).getThreat(state);
                StepScore = chess_ratings[tempBoard.getThreat(i, j, state)];
                tempBoard.move(util::xy2index(i, j),side);
                highest = tempBoard.getHighestInfo(util::otherside(state));
                //tempInfo = tempBoard.getThreatInfo(state);
                //出口
                if (StepScore >= chess_ratings[MODE_BASE_5]) 
                {
                    stepCurrent.row = i;
                    stepCurrent.col = j;
                    return stepCurrent;
                }
                else if (StepScore >= chess_ratings[MODE_ADV_43]) 
                {
                    if (chess_ratings[highest.chessmode] < chess_ratings[MODE_BASE_5]) 
                    {
                        stepCurrent.row = i;
                        stepCurrent.col = j;
                        return stepCurrent;
                    }
                }
                else if (StepScore >= chess_ratings[MODE_ADV_33]) 
                {
                    if (chess_ratings[highest.chessmode] < chess_ratings[MODE_ADV_43]) 
                    {
                        stepCurrent.row = i;
                        stepCurrent.col = j;
                        return stepCurrent;
                    }
                }//有一个小问题，如果有更好的选择就无法遍历到了。。
                 /*	StepScore = tempInfo.totalScore - info.totalScore;
                 if (info.HighestScore>99999)
                 {
                 StepScore = tempBoard.getPiece(i, j).getThreat(state);
                 }
                 else if (info.HighestScore > 9999 && tempInfo.HighestScore<100000)
                 {
                 StepScore = tempBoard.getPiece(i, j).getThreat(state);
                 }*/
                StepScore = StepScore - tempBoard.getTotalRating(util::otherside(state));

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
