#include "stdafx.h"
#include "Piece.h"


Piece::Piece()
{
	isHot = false;
	uState = STATE_EMPTY;
	isFocus = false;
	isFlag = false;
}


Piece::~Piece()
{
}

void Piece::setFlag(bool f){
	isFlag = f;
}
