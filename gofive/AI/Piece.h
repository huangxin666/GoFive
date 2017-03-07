#ifndef PIECE_H
#define PIECE_H

#include <stdint.h>
class Piece
{
public:
	Piece();
	~Piece();
	void setState(int uState);
	void setHot(bool isHot);
	int getState();
	bool isHot();
	void setThreat(int score,int side);// 0为黑棋 1为白棋
	int getThreat(int side);// 0为黑棋 1为白棋
	void clearThreat();
private:	
	int  uState;	   //格子状态：0表示无子；1表示黑；-1表示白	
	int threat[2];		//威胁分数 0为黑棋 1为白棋	
	bool hot;			//是否应被搜索
};

#endif