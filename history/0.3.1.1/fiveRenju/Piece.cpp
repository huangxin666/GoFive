#include "stdafx.h"
#include "Piece.h"


Piece::Piece()
{
	hot = false;
	uState = STATE_EMPTY;
	focus = false;
	flag = false;
}


Piece::~Piece()
{
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
void Piece::setFocus(bool isFocus){
	this->focus = isFocus;
}
void Piece::setFlag(bool f){
	this->flag = f;
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
bool Piece::isFocus(){
	return focus;
}
bool Piece::isFlag(){
	return flag;
}
bool Piece::isHot(){
	return hot;
}
