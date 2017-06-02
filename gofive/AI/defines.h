#ifndef AI_DEFINES_H
#define AI_DEFINES_H

#include <cstdint>
#include <sstream>
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <shared_mutex>
#include <memory>
#include <ctime>

#include "utils.h"
using namespace std;

//���̴�С
#define BOARD_ROW_MAX 15
#define BOARD_COL_MAX 15

#define BOARD_INDEX_BOUND 225

//����������ӽڵ�
#define GAMETREE_CHILD_MAX 225
#define GAMETREE_CHILD_SEARCH 10//��ʼ�������������
//���߳�
#define MAXTHREAD 128 //ͬʱ����߳���

enum PIECE_STATE
{
    PIECE_BLACK,
    PIECE_WHITE,
    PIECE_BLANK
};

enum AIRESULTFLAG
{
    AIRESULTFLAG_NORMAL,
    AIRESULTFLAG_WIN,
    AIRESULTFLAG_FAIL,
    AIRESULTFLAG_NEARWIN,
    AIRESULTFLAG_TAUNT,
    AIRESULTFLAG_COMPLAIN
};

enum AILEVEL
{
    AILEVEL_PRIMARY = 1,
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
    DIRECTION4_RU,	    //as�J�L
    DIRECTION4_COUNT
};

const uint8_t direction_offset_index[DIRECTION4_COUNT] = { 1, 15, 16, 14 };

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


struct HashStat
{
    uint64_t hit;
    uint64_t clash;
    uint64_t miss;
};

//uint8_t index;
//uint8_t chessMode;
//uint8_t step;
//bool    black;
struct ChessStep
{
public:
    uint8_t index;
    uint8_t chessType;
    uint8_t step;//����,��ǰstep
    bool    black;
    ChessStep(int8_t row, int8_t col, uint8_t step, uint8_t chessMode, bool black) :step(step), black(black), chessType(chessMode)
    {
        index = util::xy2index(row, col);
    }
    ChessStep(uint8_t index, uint8_t step, uint8_t chessMode, bool black) :index(index), step(step), black(black), chessType(chessMode)
    {
    }
    ChessStep() :step(0)
    {
    }
    inline int8_t getRow()
    {
        return util::getRow(index);
    }
    inline int8_t getCol()
    {
        return util::getCol(index);
    }
    inline uint8_t getColor()
    {
        return black ? PIECE_BLACK : PIECE_WHITE;
    }
    inline void setColor(int color)
    {
        black = (color == PIECE_BLACK) ? true : false;
    }
};	// �����岽��stepList

struct Position
{
    int8_t row;
    int8_t col;
    Position& operator++() // ++i
    {
        if (col++ == 14)
        {
            ++row;
            col = 0;
        }
        return *this;
    }
    Position& operator--() // --i
    {
        if (col-- == 0)
        {
            --row;
            col = 14;
        }
        return *this;
    }
    Position getNextPosition(uint8_t direction, int8_t offset)
    {
        switch (direction)
        {
        case DIRECTION4::DIRECTION4_R:
            return Position{ row,col + offset };
            break;
        case DIRECTION4::DIRECTION4_D:
            return Position{ row + offset,col };
            break;
        case DIRECTION4::DIRECTION4_RD:
            return Position{ row + offset,col + offset };
            break;
        case DIRECTION4::DIRECTION4_RU:
            return Position{ row + offset,col - offset };
            break;
        default:
            return *this;
            break;
        }
    }
    inline bool valid()
    {
        if (row > -1 && row < BOARD_ROW_MAX && col > -1 && col < BOARD_COL_MAX)
        {
            return true;
        }
        return false;
    }

    inline uint16_t toIndex()
    {
        return util::xy2index(row, col);
    }
};

//uint8_t index;

struct AISettings
{
    uint8_t maxSearchDepth;
    uint64_t maxSearchTimeMs;
    bool multiThread;
};



struct AIStepResult
{
    int score;          //����
    int8_t row;
    int8_t col;				//��ǰstep	
    AIStepResult(int8_t i, int8_t j, int s) :score(s), row(i), col(j) { };
    AIStepResult() :row(0), col(0), score(0) { };
};

struct RatingInfo
{
    int totalScore;     //����
    int highestScore;	//��߷�
    RatingInfo() :totalScore(0), highestScore(0) {};
    RatingInfo(int total, int high) :totalScore(total), highestScore(high) {};
};

struct PieceInfo
{
    uint8_t index;
    uint8_t chessmode;
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


//�Ȳ��ó��ĸ��Ƕ̵Ĳ��ԣ������߿���ʹ������ĸ�������ģ�
enum CHESSMODE2
{
    TRIE_6_CONTINUE,		    //"oooooo",   SCORE_5_CONTINUE,SCORE_5_CONTINUE,5                                   ��������,����,�ǽ��ֵ�ͬ��TRIE_5_CONTINUE
    TRIE_5_CONTINUE,			//"ooooo",    SCORE_5_CONTINUE,SCORE_5_CONTINUE,4
    TRIE_4_DOUBLE_BAN1,         //"o?ooo?o",  12000, 12000,                     4                                   ��������
    TRIE_4_DOUBLE_BAN2,         //"oo?oo?oo", 12000, 12000,                     4                                   ��������
    TRIE_4_DOUBLE_BAN3,         //"ooo?o?ooo",12000, 12000,                     4                                   ��������
    TRIE_4_CONTINUE_BAN,        //"o?oooo?",  12000, 12000,                     5                                   ��������
    TRIE_4_CONTINUE_BAN_R,      //"?oooo?o",  12000, 12000,                     4                                   ��������
    TRIE_4_BLANK_BAN,           //"oo?ooo??", 1300,  1030,                      5                                   ��������
    TRIE_4_BLANK_BAN_R,         //"??ooo?oo", 1300,  1030,                      4                                   ��������
    TRIE_4_CONTINUE_DEAD_BAN,   //"o?oooox",  1211,  1000,                      5                                   ��������
    TRIE_4_CONTINUE_DEAD_BAN_R, //"xoooo?o",  1211,  1000,                      4                                   ��������
    TRIE_4_BLANK_DEAD_BAN,      //"ooo?oo",   1210,  999,                       5                                   ��������
    TRIE_4_BLANK_DEAD_BAN_R,    //"oo?ooo",   1210,  999,                       5                                   ��������
    TRIE_4_CONTINUE,			//"?oooo?",   12000, 12000,                     4
    TRIE_4_BLANK,			    //"o?ooo??",  1300,  1030,                      4                                   ���ȼ�max
    TRIE_4_BLANK_R,             //"??ooo?o",  1300,  1030,                      4
    TRIE_4_CONTINUE_DEAD,       //"?oooox",   1211,  1001,                      4                                   ���ȼ�max��һ�Ŷ��꣬�Է��ģ����ȼ�max���Լ��ģ����ȼ����Ի�һ��
    TRIE_4_CONTINUE_DEAD_R,     //"xoooo?",   1211,  1001,                      4
    TRIE_4_BLANK_DEAD,		    //"ooo?o",    1210,  999,                       4                                   ���ȼ�max��һ�Ŷ���
    TRIE_4_BLANK_DEAD_R,        //"o?ooo",    1210,  999,                       4
    TRIE_4_BLANK_M,			    //"oo?oo",    1210,  999,                       4                                   ���ȼ�max��һ�Ŷ���
    TRIE_3_CONTINUE,			//"?ooo??",   1100,  1200,                      3                                   ����
    TRIE_3_CONTINUE_R,			//"??ooo?",   1100,  1200,                      4                                   ����
    TRIE_3_BLANK,			    //"?o?oo?",   1080,  100,                       5                                   ����
    TRIE_3_BLANK_R,			    //"?oo?o?",   1080,  100,                       5                                   ����
    TRIE_3_BLANK_DEAD2,		    //"?oo?ox",   10,    10,                        4
    TRIE_3_BLANK_DEAD2_R,		//"xo?oo?",   10,    10,                        4
    TRIE_3_CONTINUE_F,		    //"?ooo?",    20,    20,                        3                                   �ٻ���
    TRIE_3_CONTINUE_DEAD,	    //"??ooox",   20,    20,                        4
    TRIE_3_CONTINUE_DEAD_R,	    //"xooo??",   20,    20,                        3
    TRIE_3_BLANK_DEAD1,		    //"?o?oox",   5,     5,                         4
    TRIE_3_BLANK_DEAD1_R,		//"xoo?o?",   5,     5,                         4
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

const ChessModeData chessMode[TRIE_COUNT] = {
    { "oooooo",   SCORE_5_CONTINUE,SCORE_5_CONTINUE,5, 6 },
    { "ooooo",    SCORE_5_CONTINUE,SCORE_5_CONTINUE,4, 5 },
    { "o?ooo?o",  12000, 12000,                     4, 7 },
    { "oo?oo?oo", 12000, 12000,                     4, 8 },
    { "ooo?o?ooo",12000, 12000,                     4, 9 },
    { "o?oooo?",  12000, 12000,                     5, 7 },
    { "?oooo?o",  12000, 12000,                     4, 7 },
    { "oo?ooo??", 1300,  1030,                      5, 8 },
    { "??ooo?oo", 1300,  1030,                      4, 8 },
    { "o?oooox",  1211,  1000,                      5, 7 },
    { "xoooo?o",  1211,  1000,                      4, 7 },
    { "ooo?oo",   1210,  999,                       5, 6 },
    { "oo?ooo",   1210,  999,                       5, 6 },
    { "?oooo?",   12000, 12000,                     4, 6 },
    { "o?ooo??",  1300,  1030,                      4, 7 },
    { "??ooo?o",  1300,  1030,                      4, 7 },
    { "?oooox",   1211,  1001,                      4, 6 },
    { "xoooo?",   1211,  1001,                      4, 6 },
    { "ooo?o",    1210,  999,                       4, 5 },
    { "o?ooo",    1210,  999,                       4, 5 },
    { "oo?oo",    1210,  999,                       4, 5 },
    { "?ooo??",   1100,  1200,                      3, 6 },
    { "??ooo?",   1100,  1200,                      4, 6 },
    { "?o?oo?",   1080,  100,                       5, 6 },
    { "?oo?o?",   1080,  100,                       5, 6 },
    { "?oo?ox",   25,    25,                        4, 6 },
    { "xo?oo?",   25,    25,                        4, 6 },
    { "?ooo?",    10,    10,                        3, 5 },
    { "??ooox",   10,    10,                        4, 6 },
    { "xooo??",   10,    10,                        3, 6 },
    { "?o?oox",   10,    10,                        4, 6 },
    { "xoo?o?",   10,    10,                        4, 6 },
    { "?oo?",     10,    10,                        2, 4 },
    { "?o?o?",    10,    10,                        3, 5 },
};

#endif