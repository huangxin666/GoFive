#pragma once
//所有头文件
#include <sstream>
#include <string>
#include <fstream>
#include <vector>
using namespace std;

//窗口相关定义
#define BROARD_X	532	//棋盘X方向宽
#define BROARD_Y	532	//棋盘Y方向宽
#define FRAME_X
#define FRAME_Y		62	//窗口Y方向宽
#define CHESS_X		36	//棋子X方向宽
#define CHESS_Y		36	//棋子Y方向宽
#define BLANK		30


//游戏状态
#define GAME_STATE_WHITEWIN	2
#define GAME_STATE_BLACKWIN	1
#define GAME_STATE_RUN		0
#define GAME_STATE_DRAW     3 //平局
#define GAME_STATE_BLACKBAN 4 //黑子禁手告负
#define GAME_STATE_WAIT		5 //等待AI计算

#define DEFAULT_DPI 96

struct CursorPosition
{
	int row;
	int col;
	bool enable;
};

inline bool operator==(const CursorPosition &a, const CursorPosition &b) 
{
	if (a.col != b.col) return false;
	if (a.row != b.row) return false;
	if (a.enable != b.enable) return false;
	return true;
}