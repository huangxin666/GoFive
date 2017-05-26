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
    AIStepResult result;
    if (level == 1)
    {
        result = level1(cb, side);
    }
    else if (level == 2)
    {
        result = level2(cb, side);
    }
    return Position{ int8_t(result.row),int8_t(result.col) };
}

AIStepResult AIWalker::level1(ChessBoard *currentBoard, uint8_t side)
{
    ChessBoard tempBoard;
    AIStepResult stepCurrent;
    AIStepResult randomStep[225];
    int state = side;
    randomStep[0].score = 0;
    randomStep[0].row = 0;
    randomStep[0].col = 0;
    int randomCount = 0;
    stepCurrent.score = 0;
    int HighestScore = -500000;
    int HighestScoreTemp = -500000;
    int StepScore = 0;
    int score = 0;
    for (int i = 0; i < BOARD_ROW_MAX; ++i) {
        for (int j = 0; j < BOARD_COL_MAX; ++j) {
            HighestScoreTemp = -500000;
            if (!currentBoard->isHot(i, j))
                continue;
            if (currentBoard->getState(i, j) == PIECE_BLANK) {
                tempBoard = *currentBoard;
                StepScore = chess_ratings[tempBoard.getThreat(i, j, state)];
                tempBoard.move(i, j);
                for (int a = 0; a < BOARD_ROW_MAX; ++a) {
                    for (int b = 0; b < BOARD_COL_MAX; ++b) {
                        if (!tempBoard.isHot(a, b))
                            continue;
                        if (tempBoard.getState(a, b) == PIECE_BLANK) {
                            score = chess_ratings[tempBoard.getThreat(i, j, util::otherside(state))];
                            tempBoard.move(a, b);
                            if (score > HighestScoreTemp) {
                                HighestScoreTemp = score;
                            }
                            tempBoard.unmove(util::xy2index(a, b));
                        }
                    }
                }
                StepScore = StepScore - HighestScoreTemp;
                if (StepScore > HighestScore) {
                    HighestScore = StepScore;
                    stepCurrent.score = HighestScore;
                    stepCurrent.row = i;
                    stepCurrent.col = j;
                    randomCount = 0;
                    randomStep[randomCount] = stepCurrent;
                }
                else if (StepScore == HighestScore) {
                    stepCurrent.score = HighestScore;
                    stepCurrent.row = i;
                    stepCurrent.col = j;
                    randomCount++;
                    randomStep[randomCount] = stepCurrent;
                }
            }
        }
    }

    int random = rand() % (randomCount + 1);
    return randomStep[random];
}

AIStepResult AIWalker::level2(ChessBoard *currentBoard, uint8_t side)
{
    int state = side;
    ChessBoard tempBoard;
    AIStepResult stepCurrent;
    AIStepResult randomStep[225];
    PieceInfo highest = { 0,0 }, tempInfo = { 0,0 };
    randomStep[0].score = 0;
    randomStep[0].row = 0;
    randomStep[0].col = 0;
    int randomCount = 0;
    stepCurrent.score = 0;
    int HighestScore = -500000;
    int StepScore = 0;
    for (int i = 0; i < BOARD_ROW_MAX; ++i) {
        for (int j = 0; j < BOARD_COL_MAX; ++j) {
            if (currentBoard->isHot(i, j) && currentBoard->getState(i, j) == PIECE_BLANK)
            {
                tempBoard = *currentBoard;
                //StepScore = tempBoard.getPiece(i, j).getThreat(state);
                StepScore = chess_ratings[tempBoard.getThreat(i, j, state)];
                tempBoard.move(i, j);
                highest = tempBoard.getHighestInfo(util::otherside(state));
                //tempInfo = tempBoard.getThreatInfo(state);
                //出口
                if (StepScore >= chess_ratings[MODE_BASE_5]) {
                    stepCurrent.score = StepScore;
                    stepCurrent.row = i;
                    stepCurrent.col = j;
                    return stepCurrent;
                }
                else if (StepScore >= chess_ratings[MODE_ADV_43]) {
                    if (chess_ratings[highest.chessmode] < chess_ratings[MODE_BASE_5]) {
                        stepCurrent.score = StepScore;
                        stepCurrent.row = i;
                        stepCurrent.col = j;
                        return stepCurrent;
                    }
                }
                else if (StepScore >= chess_ratings[MODE_ADV_33]) {
                    if (chess_ratings[highest.chessmode] < chess_ratings[MODE_ADV_43]) {
                        stepCurrent.score = StepScore;
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

                if (StepScore > HighestScore) {
                    HighestScore = StepScore;
                    stepCurrent.score = HighestScore;
                    stepCurrent.row = i;
                    stepCurrent.col = j;
                    randomCount = 0;
                    randomStep[randomCount] = stepCurrent;
                    randomCount++;
                }
                else if (StepScore == HighestScore) {
                    stepCurrent.score = HighestScore;
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
