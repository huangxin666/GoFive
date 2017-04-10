#ifndef AI_UTILS_H
#define AI_UTILS_H

#include <cstdint>
#include <sstream>
#include <string>
#include <fstream>
#include <vector>
using namespace std;

//�����巽�鶨��
//State
#define STATE_EMPTY			0
#define STATE_CHESS_BLACK	1
#define STATE_CHESS_WHITE	-1

//���̴�С
#define BOARD_ROW_MAX 15
#define BOARD_COL_MAX 15

//����������ӽڵ�
#define GAMETREE_CHILD_MAX 225
#define GAMETREE_CHILD_SEARCH 10//��ʼ�������������
#define UPDATETHREAT_SEARCH_MAX   4
#define UPDATETHREAT_SEARCH_RANGE 9
//���߳�
#define MAXTHREAD 128 //ͬʱ����߳���

#define MAP_IGNORE_DEPTH      3

enum AIRESULTFLAG
{
    AIRESULTFLAG_NORMAL,
    AIRESULTFLAG_WIN,
    AIRESULTFLAG_FAIL,
    AIRESULTFLAG_NEARWIN,
    AIRESULTFLAG_TAUNT
};

enum AILEVEL
{
    AILEVEL_PRIMARY=1,
    AILEVEL_INTERMEDIATE,
    AILEVEL_HIGH,
    AILEVEL_MASTER,
    AILEVEL_UNLIMITED
};

enum AITYPE
{
    AITYPE_WALKER,
    AITYPE_GAMETREE
};

//����(4��)
enum DIRECTION4
{
    DIRECTION4_R,       //as����
    DIRECTION4_D,       //as����
    DIRECTION4_RD,		//as�I�K
    DIRECTION4_RU,	    //as�L�J
    DIRECTION4_COUNT
};

//����(8��)
enum DIRECTION8
{
    DIRECTION8_L,	  //as��
    DIRECTION8_R,	  //as��
    DIRECTION8_U,	  //as��
    DIRECTION8_D,	  //as��
    DIRECTION8_LU,	  //as�I
    DIRECTION8_RD,	  //as�K
    DIRECTION8_LD,	  //as�L
    DIRECTION8_RU,	  //as�J
    DIRECTION8_COUNT
};

struct Position
{
    int row;
    int col;
};

struct AIParam
{
    uint8_t caculateSteps;
    uint8_t level;
    bool ban;
    bool multithread;
};

struct ChessStep
{
public:
    uint8_t row;
    uint8_t col;
    uint8_t step;//����,��ǰstep
    bool    black;
    ChessStep(uint8_t s, uint8_t r, uint8_t c, bool b) :step(s), black(b), row(r), col(c) { };
    ChessStep() :step(0) { };
    inline int getColor()
    {
        return black ? 1 : -1;
    }
    inline void setColor(int color)
    {
        black = (color == 1) ? true : false;
    }
};	// �����岽��stepList

struct AIStepResult
{
    int score;          //����
    uint8_t row;
    uint8_t col;				//��ǰstep	
    AIStepResult(uint8_t i, uint8_t j, int s) :score(s), row(i), col(j) { };
    AIStepResult() :row(0), col(0), score(0) { };
};

struct RatingInfo
{
    int totalScore;     //����
    int highestScore;	//��߷�
    RatingInfo() :totalScore(0), highestScore(0) {};
    RatingInfo(int total, int high) :totalScore(total), highestScore(high) {};
};

struct RatingInfo2
{
    RatingInfo black;
    RatingInfo white;
    int8_t depth;
};

struct ChildInfo
{
    RatingInfo rating;
    int lastStepScore;
    int8_t depth;
    bool hasSearch;
};

struct SortInfo
{
    int key;
    int value;
};

inline void interchange(SortInfo *list, int a, int b)
{
    SortInfo temp = list[a];
    list[a] = list[b];
    list[b] = temp;
}

void insert(SortInfo e, SortInfo * a, int left, int right);

void insertionsort(SortInfo * a, int left, int right);

void quicksort(SortInfo * a, int left, int right);

inline void sort(SortInfo * a, int left, int right)
{
    quicksort(a, left, right);
    insertionsort(a, left, right);
}

int fastfind(int f[], const string &p, int size_o, char o[], int range);

//�Ȳ��ó��ĸ��Ƕ̵Ĳ��ԣ������߿���ʹ������ĸ�������ģ�
enum CHESSMODE2
{
    TRIE_4_DOUBLE_BAN1,         //"o?ooo?o",  12000, 12000,                     4                                   ��������
    TRIE_4_DOUBLE_BAN2,         //"oo?oo?oo", 12000, 12000,                     4                                   ��������
    TRIE_4_DOUBLE_BAN3,         //"ooo?o?ooo",12000, 12000,                     4                                   ��������
    TRIE_4_CONTINUE_BAN,        //"o?oooo?",  12000, 12000,                     5                                   ��������
    TRIE_4_CONTINUE_BAN_R,      //"?oooo?o",  12000, 12000,                     4                                   ��������
    TRIE_4_CONTINUE_DEAD_BAN,   //"o?oooox",  1211,  1000,                      5                                   ��������
    TRIE_4_CONTINUE_DEAD_BAN_R, //"xoooo?o",  1211,  1000,                      4                                   ��������
    TRIE_4_BLANK_BAN,           //"oo?ooo??", 1300,  1030,                      5                                   ��������
    TRIE_4_BLANK_BAN_R,         //"??ooo?oo", 1300,  1030,                      4                                   ��������
    TRIE_4_BLANK_DEAD_BAN,      //"ooo?oo",   1210,  999,                       5                                   ��������
    TRIE_4_BLANK_DEAD_BAN_R,    //"oo?ooo",   1210,  999,                       5                                   ��������
    TRIE_6_CONTINUE,		    //"oooooo",   SCORE_5_CONTINUE,SCORE_5_CONTINUE,5                                   ��������,����,�ǽ��ֵ�ͬ��TRIE_5_CONTINUE
    TRIE_5_CONTINUE,			//"ooooo",    SCORE_5_CONTINUE,SCORE_5_CONTINUE,4
    TRIE_4_CONTINUE,			//"?oooo?",   12000, 12000,                     4
    TRIE_4_CONTINUE_DEAD,       //"?oooox",   1211,  1000,                      4                                   ���ȼ�max��һ�Ŷ��꣬�Է��ģ����ȼ�max���Լ��ģ����ȼ����Ի�һ��
    TRIE_4_CONTINUE_DEAD_R,     //"xoooo?",   1211,  1000,                      4
    TRIE_4_BLANK,			    //"o?ooo??",  1300,  1030,                      4                                   ���ȼ�max
    TRIE_4_BLANK_R,             //"??ooo?o",  1300,  1030,                      4
    TRIE_4_BLANK_DEAD,		    //"ooo?o",    1210,  999,                       4                                   ���ȼ�max��һ�Ŷ���
    TRIE_4_BLANK_DEAD_R,        //"o?ooo",    1210,  999,                       4
    TRIE_4_BLANK_M,			    //"oo?oo",    1210,  999,                       4                                   ���ȼ�max��һ�Ŷ���
    TRIE_3_CONTINUE,			//"?ooo??",   1100,  1200,                      3                                   ����
    TRIE_3_CONTINUE_R,			//"??ooo?",   1100,  1200,                      4                                   ����
    TRIE_3_BLANK,			    //"?o?oo?",   1080,  100,                       5                                   ����
    TRIE_3_BLANK_R,			    //"?oo?o?",   1080,  100,                       5                                   ����
    TRIE_3_CONTINUE_F,		    //"?ooo?",    20,    20,                        3                                   �ٻ���
    TRIE_3_CONTINUE_DEAD,	    //"??ooox",   20,    20,                        4
    TRIE_3_CONTINUE_DEAD_R,	    //"xooo??",   20,    20,                        3
    TRIE_3_BLANK_DEAD1,		    //"?o?oox",   5,     5,                         4
    TRIE_3_BLANK_DEAD1_R,		//"xoo?o?",   5,     5,                         4
    TRIE_3_BLANK_DEAD2,		    //"?oo?ox",   10,    10,                        4
    TRIE_3_BLANK_DEAD2_R,		//"xo?oo?",   10,    10,                        4
    TRIE_2_CONTINUE,			//"?oo?",     35,    10,                        2
    TRIE_2_BLANK,			    //"?o?o?",    30,    5,                         3
    TRIE_COUNT
};

//����
#define FORMAT_LENGTH  11
#define FORMAT_LAST_INDEX 10
#define SEARCH_LENGTH  5
#define SEARCH_MIDDLE  4

#define SCORE_5_CONTINUE 100000 //"ooooo"
#define SCORE_4_CONTINUE 12000  //"?oooo?"
#define SCORE_4_DOUBLE   10001  //˫�ġ�����
#define SCORE_3_DOUBLE   8000   //˫��

#define BAN_LONGCONTINUITY  (-3)  // ��������
#define BAN_DOUBLEFOUR      (-2)  // ˫�Ľ���
#define BAN_DOUBLETHREE     (-1)  // ˫��������

/*
char* pat;
int evaluation;
int evaluation_defend;
uint8_t range;
uint8_t pat_len;
*/
struct ChessModeData
{
    char* pat;
    int evaluation;
    int evaluation_defend;
    uint8_t left_offset;//�����ң���֤���Ͱ������м���ǿ�����
    uint8_t pat_len;
};

struct SearchResult
{
    int chessMode;
    int pos;
};


//enum CHESSMODE
//{
//    STR_6_CONTINUE = 0,		//oooooo ���֣��ǽ��ֵ�ͬ��1
//    STR_5_CONTINUE,			//ooooo ����
//    STR_4_CONTINUE,			//?oooo? ���� ���Լ���1������
//    STR_4_CONTINUE_DEAD,	//?oooox ���ȼ�max��һ�Ŷ��� �֣��ǶԷ��ģ����ȼ�max���Լ��ģ����ȼ����Ի�һ��
//    STR_4_BLANK,			//o?ooo?? ���ȼ�max������ͳ�11
//    STR_4_BLANK_DEAD,		//ooo?o ���ȼ�max��һ�Ŷ���
//    STR_4_BLANK_M,			//oo?oo ���ȼ�max��һ�Ŷ���
//    STR_3_CONTINUE,			//?ooo?? ����
//    STR_3_BLANK,			//?o?oo?
//    STR_3_CONTINUE_F,		//?ooo? �ٻ���
//    STR_3_CONTINUE_DEAD,	//??ooox
//    STR_3_BLANK_DEAD1,		//?o?oox
//    STR_3_BLANK_DEAD2,		//?oo?ox
//    STR_2_CONTINUE,			//?oo?
//    STR_2_BLANK,			//?o?o?
//    STR_COUNT
//};

#endif