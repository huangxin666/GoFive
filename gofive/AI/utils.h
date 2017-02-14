#pragma once
#include <stdint.h>
#include <sstream>
#include <string>
#include <fstream>
#include <vector>
using namespace std;

//五子棋方块定义
//State
#define STATE_EMPTY			0
#define STATE_CHESS_BLACK	1
#define STATE_CHESS_WHITE	-1

#define BELONGTOAI			0
#define BELONGTOMAN			1

//棋盘大小
#define BOARD_ROW_MAX 15
#define BOARD_COL_MAX 15

//博弈树最大子节点
#define GAMETREE_CHILD_MAX 225
#define GAMETREE_CHILD_SEARCH 10//初始子树最大搜索数
#define UPDATETHREAT_SEARCH_MAX 4
//多线程
#define MultipleThread_MAXIMUM 128 //同时最大线程数

struct STEP
{
public:
	uint8_t uRow;
	uint8_t uCol;
	uint8_t step;//步数,当前step
	bool isBlack;
	STEP(uint8_t s, uint8_t r, uint8_t c, bool b) {
		step = s;
		uRow = r;
		uCol = c;
		isBlack = b;
	};
	STEP() {};
	int getColor()
	{
		return isBlack ? 1 : -1;
	}
	void setColor(int color)
	{
		isBlack = (color == 1) ? true : false;
	}
};	// 五子棋步数stepList

struct AISTEP
{
	int score;          //分数
	uint8_t x;
	uint8_t y;				//当前step	
	AISTEP(uint8_t i, uint8_t j, int s) {
		score = s; x = i; y = j;
	};
	AISTEP() { score = 0; x = 0; y = 0; };
};// 五子棋结构体

struct  THREATINFO
{
	int totalScore;     //分数
	int HighestScore;	//最高分
	THREATINFO(int total, int Highest)
	{
		totalScore = total; HighestScore = Highest;
	};
	THREATINFO() { totalScore = 0; HighestScore = 0; };
};// 五子棋结构体

struct Position
{
	int row, col;
	Position(int r, int c) :row(r), col(c) {};
	Position() {};
};

//方向(4向)
#define DIRECTION4_ROW 0
#define DIRECTION4_COL 1
#define DIRECTION4_RC1 2  //从左到右斜下
#define DIRECTION4_RC2 3	 //从左到右斜上
//方向(8向)
#define DIRECTION8_L	 0 //←
#define DIRECTION8_R	 1 //→
#define DIRECTION8_U     2 //↑
#define DIRECTION8_D     3 //↓
#define DIRECTION8_LU    4 //↖
#define DIRECTION8_RD    5 //↘
#define DIRECTION8_LD    6 //↙
#define DIRECTION8_RU    7 //↗

//棋型
struct STR_INFO
{
	int continues, stops, blanks;
	STR_INFO()
	{
		stops = 0;
		continues = 0;
		blanks = 0;
	}
};
#define STR_MAX_LENGTH			6
#define STR_COUNT				15	//总数
#define STR_SIX_CONTINUE		0	//oooooo 禁手，非禁手等同于1
#define STR_FIVE_CONTINUE		1	//ooooo 死棋
#define STR_FOUR_CONTINUE		2	//?oooo? 死棋 ，自己有1可无视
#define STR_FOUR_CONTINUE_DEAD	3	//?oooox 优先级max，一颗堵完 分：是对方的：优先级max；自己的：优先级可以缓一下
#define STR_FOUR_BLANK			4	//o?ooo?? 优先级max，堵完就成11
#define STR_FOUR_BLANK_DEAD		5	//ooo?o 优先级max，一颗堵完
#define STR_FOUR_BLANK_M		6	//oo?oo 优先级max，一颗堵完
#define STR_THREE_CONTINUE		7	//?ooo?? 活三
#define STR_THREE_BLANK			8	//?o?oo?
#define STR_THREE_CONTINUE_F	9	//?ooo? 假活三
#define STR_THREE_CONTINUE_DEAD	10	//??ooox
#define STR_THREE_BLANK_DEAD1	11	//?o?oox
#define STR_THREE_BLANK_DEAD2	12	//?oo?ox
#define STR_TOW_CONTINUE		13	//?oo?
#define STR_TOW_BLANK			14	//?o?o?



struct CHILDINFO
{
	int value;
	int key;
	CHILDINFO(){};
	CHILDINFO(int k, int v){ key = k; value = v; };
};
class hxtools
{
public:
	static void insert(CHILDINFO e, CHILDINFO * a, int left, int right)
	{
		while (right >= left&&e.value<a[right].value)
		{
			a[right + 1] = a[right];
			right--;
		}
		a[right + 1] = e;
	}

	static void insertionsort(CHILDINFO * a, int left, int right)
	{
		for (int i = left + 1; i <= right; i++)
		{
			insert(a[i], a, left, i - 1);
		}
	}

	static void interchange(CHILDINFO *list, int a, int b)
	{
		CHILDINFO temp = list[a];
		list[a] = list[b];
		list[b] = temp;
	}

	static void quicksort(CHILDINFO * a, int left, int right)
	{
		if (left < right)
		{
			int l = left , r = right+1;
			while (l<r)
			{
				do l++; while (a[l].value < a[left].value&&l < right);
				do r--; while (a[r].value > a[left].value);
				if (l < r) interchange(a, l, r);
			}
			interchange(a, left, r);
			if (r - 1 - left>9)
				quicksort(a, left, r - 1);
			//个数小于9就不管了		
			if (right - r - 1>9)
				quicksort(a, r + 1, right);
		}
	}

	static void sort(CHILDINFO * a, int left, int right)
	{
		quicksort(a,left,right);
		insertionsort(a, left, right);
	}
};