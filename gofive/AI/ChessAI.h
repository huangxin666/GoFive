#ifndef CHESSAI_H
#define CHESSAI_H

#include <stdint.h>
#include "ChessBoard.h"
#include "utils.h"

class ChessAI
{
public:
	ChessAI();
	virtual ~ChessAI();
	virtual Position getNextStep(ChessBoard cb, AIParam param) = 0;
};

#endif