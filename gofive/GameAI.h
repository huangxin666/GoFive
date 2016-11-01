#pragma once

#include "defines.h"
#include "ChessBoard.h"

class GameAI
{
public:
	GameAI();
	virtual ~GameAI();
	virtual Position getNextStep(ChessBoard cb) = 0;
};

