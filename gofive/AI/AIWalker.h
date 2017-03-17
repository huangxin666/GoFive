#ifndef AIWALKER_H
#define AIWALKER_H
#include "ChessAI.h"
class AIWalker :
    public ChessAI
{
public:
    AIWalker();
    virtual ~AIWalker();
    virtual Position getNextStep(ChessBoard *cb, AIParam param);
    AIStepResult level1(ChessBoard *currentBoard, AIParam parameter);
    AIStepResult level2(ChessBoard *currentBoard, AIParam parameter);
};

#endif
