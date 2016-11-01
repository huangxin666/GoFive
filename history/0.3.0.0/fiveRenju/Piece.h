#pragma once
class Piece
{
public:
	UINT uRow;         //所在二维数组的行
	UINT uCol;         //所在二位数组的列
	int  uState;	   //格子状态：0表示无子；1表示黑；-1表示白
	bool isFocus;	   //是否显示焦点
	bool isFlag;		//是否被标记
	bool isHot;			//是否应被搜索
	Piece();
	~Piece();
	void setFlag(bool f);
};

