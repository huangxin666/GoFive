#pragma once
#include "GameAI.h"
class AIGameTree :
	public GameAI
{
public:
	AIGameTree();
	virtual ~AIGameTree();
	virtual Position getNextStep(ChessBoard cb);
};

