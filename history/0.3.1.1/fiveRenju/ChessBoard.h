#pragma once
#include "stdafx.h"
#include "Piece.h"

class ChessBoard
{
public:
	ChessBoard();
	~ChessBoard();
	bool doNextStep(int row, int col, int side);
	Piece &getPiece(int row,int col);//获取棋子
	Piece &getPiece(STEP step);//获取棋子
	Piece &getPiece();//获取棋子
	BOOL stepListIsEmpty();
	void stepListAddTail(STEP step);
	void stepListRemoveTail();
	int stepListGetCount();
	void stepBack(int playerSide);
	void resetHotArea();//重置搜索区（悔棋专用）
	THREATINFO getThreatInfo(int color,int mode);
	STEP stepListGetAt(int p);
	int getStepScores(int col, int row, int state);
	int getStepScores(Piece thisStep);
	int getStepScores();
	int getDFS(ChessBoard cs);
private:
	std::vector<STEP> stepList;
	Piece m_pFive[BOARD_ROW_MAX][BOARD_COL_MAX];
public:
	bool saveBoard(CString path);
	bool loadBoard(CString path);
};

