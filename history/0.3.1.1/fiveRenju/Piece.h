#pragma once
class Piece
{
public:
	Piece();
	~Piece();
	void setRow(UINT uRow);
	void setCol(UINT uCol);
	void setXY(UINT uRow, UINT uCol);
	void setState(int uState);
	void setFocus(bool isFocus);
	void setFlag(bool f);
	void setHot(bool isHot);

	UINT getRow();
	UINT getCol();
	int getState();
	bool isFocus();
	bool isFlag();
	bool isHot();
private:
	UINT uRow;         //所在二维数组的行
	UINT uCol;         //所在二位数组的列
	int  uState;	   //格子状态：0表示无子；1表示黑；-1表示白
	bool focus;	   //是否显示焦点
	bool flag;		//是否被标记
	bool hot;			//是否应被搜索
};

