#ifndef AI_UTILS_H
#define AI_UTILS_H

#include <cstdint>
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

class Piece
{
    int blackscore;
    int whitescore;
    int8_t state;	    //格子状态：0表示无子；1表示黑；-1表示白	
    bool hot;			//是否应被搜索
public:
    Piece() : hot(false), state(0), blackscore(0), whitescore(0)
    {

    }

    inline void setState(int uState) {
        this->state = uState;
    }
    inline void setHot(bool isHot) {
        this->hot = isHot;
    }
    inline int getState() {
        return state;
    }
    inline bool isHot() {
        return hot;
    }
    inline void clearThreat()
    {
        blackscore = 0;
        blackscore = 0;
    }
    void setThreat(int score, int side)// 0为黑棋 1为白棋
    {
        if (side == 1)
        {
            blackscore = score;
        }
        else if (side == -1)
        {
            whitescore = score;
        }
    }
    int getThreat(int side)// 0为黑棋 1为白棋
    {
        if (side == 1)
        {
            return blackscore;
        }
        else if (side == -1)
        {
            return whitescore;
        }
        else if (side == 0)
        {
            return blackscore + whitescore;
        }
        else return 0;
    }
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
    AIStepResult(uint8_t i, uint8_t j, int s):score(s),x(i),y(j) {
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
    DIRECTION8_L,	  //as←
    DIRECTION8_R,	  //as→
    DIRECTION8_U,	  //as↑
    DIRECTION8_D,	  //as↓
    DIRECTION8_LU,	  //as↖
    DIRECTION8_RD,	  //as↘
    DIRECTION8_LD,	  //as↙
    DIRECTION8_RU	  //as↗
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


int fastfind(int f[], const string &p, int size_o, char o[], int range);


//棋型
const int FORMAT_LENGTH = 15;
const int SEARCH_LENGTH = 7;
const int SEARCH_MIDDLE = 6;
enum CHESSMODE
{
    STR_6_CONTINUE = 0,		//oooooo 禁手，非禁手等同于1
    STR_5_CONTINUE,			//ooooo 死棋
    STR_4_CONTINUE,			//?oooo? 死棋 ，自己有1可无视
    STR_4_CONTINUE_DEAD,	//?oooox 优先级max，一颗堵完 分：是对方的：优先级max；自己的：优先级可以缓一下
    STR_4_BLANK,			//o?ooo?? 优先级max，堵完就成11
    STR_4_BLANK_DEAD,		//ooo?o 优先级max，一颗堵完
    STR_4_BLANK_M,			//oo?oo 优先级max，一颗堵完
    STR_3_CONTINUE,			//?ooo?? 活三
    STR_3_BLANK,			//?o?oo?
    STR_3_CONTINUE_F,		//?ooo? 假活三
    STR_3_CONTINUE_DEAD,	//??ooox
    STR_3_BLANK_DEAD1,		//?o?oox
    STR_3_BLANK_DEAD2,		//?oo?ox
    STR_2_CONTINUE,			//?oo?
    STR_2_BLANK,			//?o?o?
    STR_COUNT
};


//oooooo  6,6,0+,0+,0+,0+,0+
//ooooo   5,5,0+,0+,0+,0+,0+
//?oooo?  4,4,0,0 ,0 ,2 ,?
//?oooox  4,4,0,1 ,0 ,1 ,?
//o?ooo?? 4,3,1,0 ,1 ,? ,1+
//ooo?o   4,3,1,0 ,1 ,? ,?
//oo?oo   4,2,0,0 ,1 ,? ,?
//?ooo??  3,3,0,0 ,0 ,1 ,1
//?o?oo?  3,2,1,0 ,1 ,2 ,?
//?ooo?   3,3,0,0 ,0 ,2 ,?
//??ooox  3,3,0,1 ,0 ,1 ,1
//?o?oox  3,2,1,1 ,1 ,1 ,?
//?oo?ox  3,2,1,1 ,1 ,1 ,?
//?oo?    2,2,0,0 ,0 ,2 ,?
//?o?o?   2,0,2,0 ,1 ,2 ,?
//count, continus, alone, stop, blank_in, blank_side, blank_two

enum CHESSMODE2
{
    TRIE_6_CONTINUE = 0,		//"oooooo",   100000,100000,5             禁手，非禁手等同于1
    TRIE_5_CONTINUE,			//"ooooo",    100000,100000,4
    TRIE_4_CONTINUE,			//"?oooo?",   12000, 12000, 4
    TRIE_4_CONTINUE_BAN,        //"o?oooo?",  12000, 12000, 5              禁手棋
    TRIE_4_CONTINUE_BAN_R,      //"?oooo?o",  12000, 12000, 6              禁手棋
    TRIE_4_DOUBLE_BAN1,         //"o?ooo?o",
    TRIE_4_DOUBLE_BAN2,         //"oo?oo?oo",
    TRIE_4_DOUBLE_BAN3,         //"ooo?o?ooo",
    TRIE_4_CONTINUE_DEAD,       //"?oooox",   1211,  1000,  4              优先级max，一颗堵完，对方的：优先级max；自己的：优先级可以缓一下
    TRIE_4_CONTINUE_DEAD_R,     //"xoooo?",   1211,  1000,  4
    TRIE_4_CONTINUE_DEAD_BAN,   //"o?oooox",  1211,  1000,  5             禁手棋
    TRIE_4_CONTINUE_DEAD_BAN_R, //"xoooo?o",  1211,  1000,  6             禁手棋
    TRIE_4_BLANK,			    //"o?ooo??",  1300,  1030,  4             优先级max
    TRIE_4_BLANK_R,             //"??ooo?o",  1300,  1030,  6
    TRIE_4_BLANK_BAN,           //"oo?ooo??", 1300,  1030,  5              禁手棋
    TRIE_4_BLANK_BAN_R,         //"??ooo?oo", 1300,  1030,  7              禁手棋
    TRIE_4_BLANK_DEAD,		    //"ooo?o",    1210,  999,   4              优先级max，一颗堵完
    TRIE_4_BLANK_DEAD_R,        //"o?ooo",    1210,  999,   4
    TRIE_4_BLANK_DEAD_BAN,      //"ooo?oo",   1210,  999,   5             禁手棋
    TRIE_4_BLANK_DEAD_BAN_R,    //"oo?ooo",   1210,  999,   5             禁手棋
    TRIE_4_BLANK_M,			    //"oo?oo",    1210,  999,   4             优先级max，一颗堵完
    TRIE_3_CONTINUE,			//"?ooo??",   1100,  1200,  3             活三
    TRIE_3_CONTINUE_R,			//"??ooo?",   1100,  1200,  4             活三
    TRIE_3_BLANK,			    //"?o?oo?",   1080,  100,   5             活三
    TRIE_3_BLANK_R,			    //"?oo?o?",   1080,  100,   5             活三
    TRIE_3_CONTINUE_F,		    //"?ooo?",    20,    20,    3             假活三
    TRIE_3_CONTINUE_DEAD,	    //"??ooox",   20,    20,    4
    TRIE_3_CONTINUE_DEAD_R,	    //"xooo??",   20,    20,    3
    TRIE_3_BLANK_DEAD1,		    //"?o?oox",   5,     5,     4
    TRIE_3_BLANK_DEAD1_R,		//"xoo?o?",   5,     5,     4
    TRIE_3_BLANK_DEAD2,		    //"?oo?ox",   10,    10,    4
    TRIE_3_BLANK_DEAD2_R,		//"xo?oo?",   10,    10,    4
    TRIE_2_CONTINUE,			//"?oo?",     35,    10,    2
    TRIE_2_BLANK,			    //"?o?o?",    30,    5,     3
    TRIE_COUNT
};


/*
char* pat;
int evaluation;
int evaluation_defend;
uint8_t range;
*/
struct ChessModeData
{
    char* pat;
    int evaluation;
    int evaluation_defend;
    uint8_t max_offset;//从左到右，保证棋型包含最中间的那颗棋子
};

class TrieTreeNode 
{
public:
    TrieTreeNode* childs[3];// 0-o,1-x,2-?
    int chessType;
    TrieTreeNode()
    {
        chessType = -1;
        childs[0] = childs[1] = childs[2] = NULL;
    };
    void clearStringTree();
    bool buildStringTree();
    int search(char *str, uint8_t result[TRIE_COUNT]);
    inline int char2index(char a)
    {
        if (a == 'o')
        {
            return 0;
        }
        else if (a == 'x')
        {
            return 1;
        }
        else if (a == '?')
        {
            return 2;
        }
        else
        {
            return -1;
        }
    };
};

#endif