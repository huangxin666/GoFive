#ifndef CHESSAI_H
#define CHESSAI_H

#include "ChessBoard.h"
#include "defines.h"

class ChessAI
{
public:
    ChessAI();
    virtual ~ChessAI();
    virtual Position getNextStep(ChessBoard *cb, AIParam param) = 0;
};

#endif