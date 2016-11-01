#pragma once
#include "stdafx.h"
#include "Piece.h"

class ChessBoard
{
public:
	ChessBoard();
	~ChessBoard();
	
	Piece m_pFive[BOARD_ROW_MAX][BOARD_COL_MAX];

	bool doNextStep(int row, int col, int side);
	Piece * getPiece(int row,int col);//获取棋子
	Piece * getPiece(STEP step);//获取棋子
	Piece * getPiece();//获取棋子
	BOOL stepListIsEmpty();
	void stepListAddTail(STEP step);
	void stepListRemoveTail();
	int stepListGetCount();
	void stepBack(int playerSide);
	void resetHotArea();//重置搜索区（悔棋专用）
	int getGlobalScore(int color);
	STEP stepListGetAt(int p);
	int getStepScores(int col, int row, int state);//row和col相反
	int getStepScores(Piece thisStep);//row和col相反
	bool isFiveContinue(int derection[]);		// 00000
	bool isFourContinue(int derection[]);		// 0000 
	bool isFourContinueSide(int derection[]);	// 0000X||X0000
	bool isFourContinueVar1(int derection[]);	// 000 0||0 000
	bool isFourContinueVar2(int derection[]);	// 00 00
	bool isThreeContinue(int derection[]);		// 000
	bool isThreeContinueSide(int derection[]);	// 000X||X000
	bool isThreeContinueVar(int derection[]);	// 00 0||0 00
	bool isThreeContinueVarSide(int derection[]);// 0 00X || X0 00 || 00 0X || X00 0
	bool isTwoContinue(int derection[]);			// 00
	bool isTwoContinueVar(int derection[]);		// 0 0
	bool isDeadTwo(int direction[]);
private:
	std::vector<STEP> stepList;
	int nextStep;
};

