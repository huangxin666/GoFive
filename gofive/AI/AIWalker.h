#ifndef AIWALKER_H
#define AIWALKER_H

#include "ChessAI.h"


class AIWalker :
    public ChessAI
{
public:
    AIWalker();
    virtual ~AIWalker();
    virtual Position getNextStep(ChessBoard *cb, uint8_t side, uint8_t level, bool ban);
    virtual void applyAISettings(AISettings setting);
    AIStepResult level1(ChessBoard *cb, uint8_t side);
    AIStepResult level2(ChessBoard *cb, uint8_t side);
};

#endif