#include "stdafx.h"
#include "Piece.h"


Piece::Piece()
{
	hot = false;
	uState = 0;
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
	}	
	else if (side == -1)
	{
		threat[1] = score;
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
	else if (side == 0)
	{
		return threat[0] + threat[1];
	}
	else return 0;
}