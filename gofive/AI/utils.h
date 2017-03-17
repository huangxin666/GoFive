#ifndef AI_UTILS_H
#define AI_UTILS_H

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
#define MAXTHREAD 128 //同时最大线程数


struct AIParam
{
    uint8_t caculateSteps;
    uint8_t level;
    bool ban;
    bool multithread;
};

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

struct AIStepResult
{
    int score;          //分数
    uint8_t x;
    uint8_t y;				//当前step	
    AIStepResult(uint8_t i, uint8_t j, int s) {
        score = s; x = i; y = j;
    };
    AIStepResult() { score = 0; x = 0; y = 0; };
};// 五子棋结构体

struct ThreatInfo
{
    int totalScore;     //分数
    int HighestScore;	//最高分
    ThreatInfo() :totalScore(0), HighestScore(0) {};
    ThreatInfo(int total, int high) :totalScore(total), HighestScore(high) {};
};// 五子棋结构体

struct Position
{
    int row;
    int col;
};

//方向(4向)
enum DIRECTION4
{
    DIRECTION4_ROW,
    DIRECTION4_COL,
    DIRECTION4_RC1,		 //从左到右斜下
    DIRECTION4_RC2		 //从左到右斜上
};

//方向(8向)
enum DIRECTION8
{
    DIRECTION8_L,	  //←
    DIRECTION8_R,	  //→
    DIRECTION8_U,	  //↑
    DIRECTION8_D,	  //↓
    DIRECTION8_LU,	  //↖
    DIRECTION8_RD,	  //↘
    DIRECTION8_LD,	  //↙
    DIRECTION8_RU	  //↗
};

//棋型
struct ChessStrInfo
{
    int continues;
    int stops;
    int blanks;
};

const int STR_MAX_LENGTH = 6;

enum ChessMode
{
    STR_SIX_CONTINUE = 0,		//oooooo 禁手，非禁手等同于1
    STR_FIVE_CONTINUE,			//ooooo 死棋
    STR_FOUR_CONTINUE,			//?oooo? 死棋 ，自己有1可无视
    STR_FOUR_CONTINUE_DEAD,		//?oooox 优先级max，一颗堵完 分：是对方的：优先级max；自己的：优先级可以缓一下
    STR_FOUR_BLANK,				//o?ooo?? 优先级max，堵完就成11
    STR_FOUR_BLANK_DEAD,		//ooo?o 优先级max，一颗堵完
    STR_FOUR_BLANK_M,			//oo?oo 优先级max，一颗堵完
    STR_THREE_CONTINUE,			//?ooo?? 活三
    STR_THREE_BLANK,			//?o?oo?
    STR_THREE_CONTINUE_F,		//?ooo? 假活三
    STR_THREE_CONTINUE_DEAD,	//??ooox
    STR_THREE_BLANK_DEAD1,		//?o?oox
    STR_THREE_BLANK_DEAD2,		//?oo?ox
    STR_TOW_CONTINUE,			//?oo?
    STR_TOW_BLANK,				//?o?o?
    STR_COUNT
};



struct ChildInfo
{
    int key;
    int value;
};

inline void insert(ChildInfo e, ChildInfo * a, int left, int right)
{
    while (right >= left&&e.value < a[right].value)
    {
        a[right + 1] = a[right];
        right--;
    }
    a[right + 1] = e;
}

inline void insertionsort(ChildInfo * a, int left, int right)
{
    for (int i = left + 1; i <= right; i++)
    {
        insert(a[i], a, left, i - 1);
    }
}

inline void interchange(ChildInfo *list, int a, int b)
{
    ChildInfo temp = list[a];
    list[a] = list[b];
    list[b] = temp;
}

inline void quicksort(ChildInfo * a, int left, int right)
{
    if (left < right)
    {
        int l = left, r = right + 1;
        while (l < r)
        {
            do l++; while (a[l].value < a[left].value&&l < right);
            do r--; while (a[r].value > a[left].value);
            if (l < r) interchange(a, l, r);
        }
        interchange(a, left, r);
        if (r - 1 - left > 9)
            quicksort(a, left, r - 1);
        //个数小于9就不管了		
        if (right - r - 1 > 9)
            quicksort(a, r + 1, right);
    }
}

inline void sort(ChildInfo * a, int left, int right)
{
    quicksort(a, left, right);
    insertionsort(a, left, right);
}


#endif