#pragma once
#include "defines.h"
class Piece
{
public:
	Piece();
	~Piece();
	void setRow(UINT uRow);
	void setCol(UINT uCol);
	void setXY(UINT uRow, UINT uCol);
	void setState(int uState);
	void setHot(bool isHot);
	UINT getRow();
	UINT getCol();
	int getState();
	bool isHot();
	void save(CArchive &oar);
	void load(CArchive &oar);
	void setThreat(int score,int side);// 0为黑棋 1为白棋
	int getThreat(int side);// 0为黑棋 1为白棋
	void clearThreat();
private:
	UINT uRow;         //所在二维数组的行
	UINT uCol;         //所在二位数组的列
	int  uState;	   //格子状态：0表示无子；1表示黑；-1表示白
	bool hot;			//是否应被搜索
	int threat[2];		//威胁分数 0为黑棋 1为白棋
};

