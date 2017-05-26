#ifndef CHESSAI_H
#define CHESSAI_H

#include "ChessBoard.h"
#include "defines.h"

class ChessAI
{
public:
    ChessAI()
    {

    }
    virtual ~ChessAI()
    {

    }
    virtual Position getNextStep(ChessBoard *cb, uint8_t side, uint8_t level, bool ban) = 0;
    virtual void applyAISettings(AISettings setting) = 0;
};

#endif