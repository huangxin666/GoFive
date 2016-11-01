#include "stdafx.h"
#include "Piece.h"


Piece::Piece()
{
	hot = false;
	uState = STATE_EMPTY;
	threatFlag[0] = false;
	threatFlag[1] = false;
	threat[0] = 0;
	threat[1] = 0;
}


Piece::~Piece()
{
}

void Piece::clearThreat()
{
	threat[0] = 0;
	threat[1] = 0;
	threatFlag[0] = false;
	threatFlag[1] = false;
}

void Piece::setRow(UINT uRow){
	this->uRow = uRow;
}
void Piece::setCol(UINT uCol){
	this->uCol = uCol;
}
void Piece::setXY(UINT uRow, UINT uCol){
	this->uRow = uRow;
	this->uCol = uCol;
}
void Piece::setState(int uState){
	this->uState = uState;
}
void Piece::setHot(bool isHot){
	this->hot = isHot;
}

UINT Piece::getRow(){
	return uRow;
}
UINT Piece::getCol(){
	return uCol;
}

int Piece::getState(){
	return uState;
}
bool Piece::isHot(){
	return hot;
}

void Piece::setThreat(int score, int side)
{
	if (side == 1)
	{
		threat[0] = score;
		threatFlag[0] = true;
	}	
	else if (side == -1)
	{
		threat[1] = score;
		threatFlag[1] = true;
	}
}

int Piece::getThreat(int side)
{
	if (side == 1)
	{
		return threat[0];
	}
	else if (side == -1)
	{
		return threat[1];
	}
	else return 0;
}

bool Piece::isThreat(int side)// 检测是否标记威胁分数
{
	return threatFlag[side];
}
void Piece::save(CArchive &oar)
{
	oar << uState << hot;
}

void Piece::load(CArchive &oar)
{
	oar >> uState  >> hot;
}