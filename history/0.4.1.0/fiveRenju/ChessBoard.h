#pragma once
#include "defines.h"
#include "Piece.h"

class ChessBoard
{
public:
	ChessBoard();
	~ChessBoard();
	bool doNextStep(int row, int col, int side);
	Piece &getPiece(int row,int col);//��ȡ����
	Piece &getPiece(STEP step);//��ȡ����
	Piece &getPiece();//��ȡ����
	void resetHotArea();//����������������ר�ã�
	void updateHotArea(int row,int col);
	//THREATINFO getThreatInfo(int color,int mode);
	THREATINFO getThreatInfo(int side);
	STEP stepListGetAt(int p);
	int getStepScores(int row, int col, int state);
	int getStepScores(int row, int col, int state,bool ban);
	int getStepScores(Piece thisStep);
	int getStepScores();
	int getDFS(ChessBoard cs);
	int getDFS(ChessBoard cs,int max);
	void setGlobalThreat(bool);//����Ϊһ��ȫɨgetStepScores*2
	void setThreat(int row, int col, int side,bool ban);//����Ϊһ��getStepScores
	void updateThreat(bool ban,int =0);
	void updateThreat(int row, int col, int side, bool ban);
	void setLastStep(STEP step);
	STEP getLastStep();
private:
	STEP lastStep;
	Piece m_pFive[BOARD_ROW_MAX][BOARD_COL_MAX];
};

