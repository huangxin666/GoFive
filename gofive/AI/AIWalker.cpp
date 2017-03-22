#include "AIWalker.h"



AIWalker::AIWalker()
{
}


AIWalker::~AIWalker()
{
}

Position AIWalker::getNextStep(ChessBoard *cb, AIParam param)
{
    AIStepResult result;
    if (param.level == 1)
    {
        result = level1(cb, param);
    }
    else if (param.level == 2)
    {
        result = level2(cb, param);
    }
    return Position{ result.x,result.y };
}

AIStepResult AIWalker::level1(ChessBoard *currentBoard, AIParam parameter)
{
    ChessBoard tempBoard;
    AIStepResult stepCurrent;
    AIStepResult randomStep[225];
    int state = -currentBoard->lastStep.getColor();
    randomStep[0].score = 0;
    randomStep[0].x = 0;
    randomStep[0].y = 0;
    int randomCount = 0;
    stepCurrent.score = 0;
    int HighestScore = -500000;
    int HighestScoreTemp = -500000;
    int StepScore = 0;
    int score = 0;
    for (int i = 0; i < BOARD_ROW_MAX; ++i) {
        for (int j = 0; j < BOARD_COL_MAX; ++j) {
            HighestScoreTemp = -500000;
            if (!currentBoard->getPiece(i, j).isHot())
                continue;
            if (currentBoard->getPiece(i, j).getState() == 0) {
                tempBoard = *currentBoard;
                tempBoard.doNextStep(i, j, state);
                StepScore = tempBoard.getStepScores(i, j, state, parameter.ban, false);
                for (int a = 0; a < BOARD_ROW_MAX; ++a) {
                    for (int b = 0; b < BOARD_COL_MAX; ++b) {
                        if (!tempBoard.getPiece(a, b).isHot())
                            continue;
                        if (tempBoard.getPiece(a, b).getState() == 0) {
                            tempBoard.getPiece(a, b).setState(-state);
                            score = tempBoard.getStepScores(a, b, -state, parameter.ban, true);
                            if (score > HighestScoreTemp) {
                                HighestScoreTemp = score;
                            }
                            tempBoard.getPiece(a, b).setState(0);
                        }

                    }
                }
                StepScore = StepScore - HighestScoreTemp;
                if (StepScore > HighestScore) {
                    HighestScore = StepScore;
                    stepCurrent.score = HighestScore;
                    stepCurrent.x = i;
                    stepCurrent.y = j;
                    randomCount = 0;
                    randomStep[randomCount] = stepCurrent;
                }
                else if (StepScore == HighestScore) {
                    stepCurrent.score = HighestScore;
                    stepCurrent.x = i;
                    stepCurrent.y = j;
                    randomCount++;
                    randomStep[randomCount] = stepCurrent;
                }
            }
        }
    }

    int random = rand() % (randomCount + 1);
    return randomStep[random];
}

AIStepResult AIWalker::level2(ChessBoard *currentBoard, AIParam parameter)
{
    currentBoard->setGlobalThreat(parameter.ban);
    int state = -currentBoard->lastStep.getColor();
    ChessBoard tempBoard;
    AIStepResult stepCurrent;
    AIStepResult randomStep[225];
    ThreatInfo info = { 0,0 }, tempInfo = { 0,0 };
    randomStep[0].score = 0;
    randomStep[0].x = 0;
    randomStep[0].y = 0;
    int randomCount = 0;
    stepCurrent.score = 0;
    int HighestScore = -500000;
    int StepScore = 0;
    for (int i = 0; i < BOARD_ROW_MAX; ++i) {
        for (int j = 0; j < BOARD_COL_MAX; ++j) {
            if (currentBoard->getPiece(i, j).isHot() && currentBoard->getPiece(i, j).getState() == 0)
            {
                tempBoard = *currentBoard;
                //StepScore = tempBoard.getPiece(i, j).getThreat(state);
                tempBoard.doNextStep(i, j, state);
                tempBoard.updateThreat(parameter.ban);
                StepScore = tempBoard.getLastStepScores(parameter.ban, false);
                info = tempBoard.getThreatInfo(-state);
                //tempInfo = tempBoard.getThreatInfo(state);
                //出口
                if (StepScore >= 100000) {
                    stepCurrent.score = StepScore;
                    stepCurrent.x = i;
                    stepCurrent.y = j;
                    return stepCurrent;
                }
                else if (StepScore >= 10000) {
                    if (info.HighestScore < 100000) {
                        stepCurrent.score = StepScore;
                        stepCurrent.x = i;
                        stepCurrent.y = j;
                        return stepCurrent;
                    }
                }
                else if (StepScore >= 8000) {
                    if (info.HighestScore < 10000) {
                        stepCurrent.score = StepScore;
                        stepCurrent.x = i;
                        stepCurrent.y = j;
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
                StepScore = StepScore - info.totalScore;

                if (StepScore > HighestScore) {
                    HighestScore = StepScore;
                    stepCurrent.score = HighestScore;
                    stepCurrent.x = i;
                    stepCurrent.y = j;
                    randomCount = 0;
                    randomStep[randomCount] = stepCurrent;
                    randomCount++;
                }
                else if (StepScore == HighestScore) {
                    stepCurrent.score = HighestScore;
                    stepCurrent.x = i;
                    stepCurrent.y = j;
                    randomStep[randomCount] = stepCurrent;
                    randomCount++;
                }
            }
        }
    }
    int random = rand() % randomCount;
    return randomStep[random];
}
