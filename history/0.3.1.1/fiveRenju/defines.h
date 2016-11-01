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
#define GAMETREE_CHILD_MAX 225
typedef struct tagSTEP
{
public:
	UINT step;          //步数
	UINT uCol;
	UINT uRow;		//当前step	
	tagSTEP(UINT s, UINT r, UINT c){
		step = s;
		uRow = r;
		uCol = c;
	}
	tagSTEP(){

	}
}STEP;	// 五子棋步数stepList

typedef struct
{
	int score;          //分数
	UINT x;
	UINT y;				//当前step	
} AISTEP;// 五子棋结构体

typedef struct
{
	int totalScore;          //分数
	int HighestScore;	//最高分
} THREATINFO;// 五子棋结构体
//棋谱
#define STR_COUNT 20//总数
#define STR_FIVE_CONTINUE 0 //ooooo 死棋
#define STR_FOUR_CONTINUE 1 //?oooo? 死棋 ，自己有1可无视
#define STR_FOUR_CONTINUE_L 2 //?oooox 优先级max，一颗堵完 分：是对方的：优先级max；自己的：优先级可以缓一下
#define STR_FOUR_CONTINUE_R 3 //xoooo? 优先级max，一颗堵完
#define STR_FOUR_BLANK_R 4 //??ooo?o 优先级max，堵完就成10
#define STR_FOUR_BLANK_L 5 //o?ooo?? 优先级max，堵完就成11
#define STR_FOUR_BLANK_M 6 //oo?oo 优先级max，一颗堵完
#define STR_THREE_CONTINUE 7 //?ooo? 优先级max-1，不堵成1，堵完就成2,3,4,5,6 自己有2,3,4,5,6可无视
#define STR_THREE_BLANK_R 8 //?oo?o?
#define STR_THREE_BLANK_L 9 //?o?oo?
#define STR_THREE_CONTINUE_L 10 //??ooox
#define STR_THREE_CONTINUE_R 11 //xooo??
#define STR_THREE_BLANK_L1 12 //?o?oox
#define STR_THREE_BLANK_L2 13 //xo?oo?
#define STR_THREE_BLANK_R1 14 //?oo?ox
#define STR_THREE_BLANK_R2 15 //xoo?o?
#define STR_TOW_CONTINUE 16 //?oo?
#define STR_TOW_BLANK 17 //?o?o?
#define STR_FOUR_BLANK_R_DEAD 18 //xooo?o 优先级max，一颗堵完
#define STR_FOUR_BLANK_L_DEAD 19 //o?ooox 优先级max，一颗堵完