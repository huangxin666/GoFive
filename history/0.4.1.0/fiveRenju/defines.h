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

//游戏状态
#define GAME_STATE_WHITEWIN	2
#define GAME_STATE_BLACKWIN	1
#define GAME_STATE_RUN		0
#define GAME_STATE_DRAW     3 //平局
#define GAME_STATE_BLACKBAN 4 //黑子禁手告负
#define GAME_STATE_WAIT		5 //等待AI计算
//棋盘大小
#define BOARD_ROW_MAX 15
#define BOARD_COL_MAX 15
#define UPDATETHREAT_SEARCH_MAX 4


//博弈树最大子节点
#define GAMETREE_CHILD_MAX 225
#define GAMETREE_CHILD_SEARCH 10//初始子树最大搜索数

//多线程
#define MultipleThread_MAXIMUM 128 //同时最大线程数

struct STEP
{
public:      
	UINT uRow;
	UINT uCol;	
	UINT step;//步数,当前step	
	STEP(UINT s, UINT r, UINT c){
		step = s;
		uRow = r;
		uCol = c;
	};
	STEP(){};
};	// 五子棋步数stepList

struct AISTEP
{
	int score;          //分数
	UINT x;
	UINT y;				//当前step	
	AISTEP(UINT i, UINT j, int s){
		score = s; x = i; y = j;
	};
	AISTEP(){ score = 0; x = 0; y = 0; };
};// 五子棋结构体

struct  THREATINFO
{
	int totalScore;     //分数
	int HighestScore;	//最高分
	THREATINFO(int total, int Highest){
		totalScore = total; HighestScore = Highest;
	};
	THREATINFO(){ totalScore = 0; HighestScore = 0; };
};// 五子棋结构体


//棋型
#define STR_COUNT				15//总数
#define STR_SIX_CONTINUE		0	//oooooo 禁手，非禁手等同于1
#define STR_FIVE_CONTINUE		1	//ooooo 死棋
#define STR_FOUR_CONTINUE		2	//?oooo? 死棋 ，自己有1可无视
#define STR_FOUR_CONTINUE_DEAD	3	//?oooox 优先级max，一颗堵完 分：是对方的：优先级max；自己的：优先级可以缓一下
#define STR_FOUR_BLANK			4	//o?ooo?? 优先级max，堵完就成11
#define STR_FOUR_BLANK_DEAD		5	//ooo?o 优先级max，一颗堵完
#define STR_FOUR_BLANK_M		6	//oo?oo 优先级max，一颗堵完
#define STR_THREE_CONTINUE		7	//?ooo?? 活三
#define STR_THREE_CONTINUE_F	8	//?ooo? 假活三
#define STR_THREE_BLANK			9	//?o?oo?
#define STR_THREE_CONTINUE_DEAD	10	//??ooox
#define STR_THREE_BLANK_DEAD1	11	//?o?oox
#define STR_THREE_BLANK_DEAD2	12	//?oo?ox
#define STR_TOW_CONTINUE		13	//?oo?
#define STR_TOW_BLANK			14	//?o?o?
