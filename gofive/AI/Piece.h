#pragma once
#include <stdint.h>
class Piece
{
public:
	Piece();
	~Piece();
	void setRow(uint8_t uRow);
	void setCol(uint8_t uCol);
	void setXY(uint8_t uRow, uint8_t uCol);
	void setState(int uState);
	void setHot(bool isHot);
	uint8_t getRow();
	uint8_t getCol();
	int getState();
	bool isHot();
	void setThreat(int score,int side);// 0为黑棋 1为白棋
	int getThreat(int side);// 0为黑棋 1为白棋
	void clearThreat();
private:	
	int  uState;	   //格子状态：0表示无子；1表示黑；-1表示白	
	int threat[2];		//威胁分数 0为黑棋 1为白棋	
	uint8_t uRow;         //所在二维数组的行
	uint8_t uCol;         //所在二位数组的列
	bool hot;			//是否应被搜索
};

