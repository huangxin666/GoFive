#pragma once
#include "ChessAI.h"
class AIGameTree :
	public ChessAI
{
public:
	AIGameTree();
	virtual ~AIGameTree();
	virtual Position getNextStep(ChessBoard cb);
};

