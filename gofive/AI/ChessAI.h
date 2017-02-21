#ifndef CHESSAI_H
#define CHESSAI_H

#include "ChessBoard.h"

class ChessAI
{
public:
	ChessAI();
	virtual ~ChessAI();
	virtual Position getNextStep(ChessBoard cb) = 0;
};

#endif