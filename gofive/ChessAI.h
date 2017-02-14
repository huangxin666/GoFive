#pragma once

#include "defines.h"
#include "ChessBoard.h"

class ChessAI
{
public:
	ChessAI();
	virtual ~ChessAI();
	virtual Position getNextStep(ChessBoard cb) = 0;
};

