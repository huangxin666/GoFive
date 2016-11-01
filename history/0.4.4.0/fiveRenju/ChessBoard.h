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
	THREATINFO getThreatInfo(int side);
	STEP stepListGetAt(int p);
	int getStepSituation(int row, int col, int state);
	int getStepScores(bool,bool);
	int getStepScores(int row, int col, int state, bool ban);
	int getStepScores(int row, int col, int state,bool ban,bool);
	void setGlobalThreat(bool);//����Ϊһ��ȫɨgetStepScores*2
	void setThreat(int row, int col, int side,bool ban);//����Ϊһ��getStepScores
	void updateThreat(bool ban,int =0);
	void updateThreat(int row, int col, int side, bool ban);
	void setLastStep(STEP step);
	STEP getLastStep();
	int getAtackScore(int currentScore,int threat,bool ban);
	int getAtackScoreHelp(int,int,int,int &,char, char);
	bool getDirection(int&,int&,int,int);
public:
	Piece pieces[BOARD_ROW_MAX][BOARD_COL_MAX];
private:
	STEP lastStep;
};

