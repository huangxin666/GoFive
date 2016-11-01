#pragma once
//窗口相关定义
#define BROARD_X	532	//棋盘X方向宽
#define BROARD_Y	532	//棋盘Y方向宽
#define FRAME_X
#define FRAME_Y		62	//窗口Y方向宽
#define CHESS_X		36	//棋子X方向宽
#define CHESS_Y		36	//棋子Y方向宽
#define BLANK		30
//五子棋方块定义
//State
#define STATE_EMPTY			0
#define STATE_CHESS_BLACK	1
#define STATE_CHESS_WHITE	-1

#define BELONGTOAI			0
#define BELONGTOMAN			1

#define GAME_STATE_WHITEWIN	2
#define GAME_STATE_BLACKWIN	1
#define GAME_STATE_RUN		0
#define GAME_STATE_DRAW     3 //平局

//棋盘大小
#define BOARD_ROW_MAX 15
#define BOARD_COL_MAX 15

//博弈树最大子节点
#define GAMETREE_CHILD_MAX 10
typedef struct
{
	int step;          //步数
	UINT uCol;
	UINT uRow;		//当前step	
} STEP;	// 五子棋步数

typedef struct
{
	int score;          //分数
	UINT x;
	UINT y;				//当前step	
} AISTEP;// 五子棋结构体