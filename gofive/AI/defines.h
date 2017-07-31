#ifndef __AI_DEFINES_H__
#define __AI_DEFINES_H__

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
#include <utility>
#include <set>

using namespace std;

//棋盘大小
#define BOARD_ROW_MAX 15
#define BOARD_COL_MAX 15
#define BOARD_INDEX_BOUND (BOARD_ROW_MAX*BOARD_COL_MAX)

typedef uint8_t csidx;//chess index

enum PIECE_STATE :uint8_t
{
    PIECE_BLACK,
    PIECE_WHITE,
    PIECE_BLANK
};


enum AIRESULTFLAG :uint8_t
{
    AIRESULTFLAG_NORMAL,
    AIRESULTFLAG_WIN,
    AIRESULTFLAG_FAIL,
    AIRESULTFLAG_NEARWIN,
    AIRESULTFLAG_TAUNT,
    AIRESULTFLAG_COMPLAIN
};

enum AILEVEL :uint8_t
{
    AILEVEL_PRIMARY = 1,
    AILEVEL_INTERMEDIATE,
    AILEVEL_HIGH,
    AILEVEL_MASTER,
    AILEVEL_GOSEARCH,
    AILEVEL_UNLIMITED
};

enum AITYPE :uint8_t
{
    AITYPE_WALKER,
    AITYPE_GAMETREE
};

//方向(4向)
enum DIRECTION4 :uint8_t
{
    DIRECTION4_LR,       //as←→
    DIRECTION4_UD,       //as↑↓
    DIRECTION4_RD,		//as↖↘
    DIRECTION4_RU,	    //as↗↙
    DIRECTION4_COUNT
};

//方向(8向)
enum DIRECTION8 :uint8_t
{
    DIRECTION8_L,	  //as←
    DIRECTION8_R,	  //as→
    DIRECTION8_U,	  //as↑
    DIRECTION8_D,	  //as↓
    DIRECTION8_LU,	  //as↖
    DIRECTION8_RD,	  //as↘
    DIRECTION8_LD,	  //as↙
    DIRECTION8_RU,	  //as↗
    DIRECTION8_COUNT
};


struct HashStat
{
    uint64_t hit;
    uint64_t clash;
    uint64_t miss;
    uint64_t cover;
};

enum CHESSTYPE :uint8_t
{
    CHESSTYPE_0, //null
    CHESSTYPE_J2,//"?o?o?"
    CHESSTYPE_2,//"?oo?"
    CHESSTYPE_D3,//"xoo?o?" and "?ooo?" and "xooo??"
    CHESSTYPE_D3P,//"xo?oo?"
    CHESSTYPE_J3,//"?oo?o?"
    CHESSTYPE_3,// "??ooo?"
    CHESSTYPE_D4,  //"o?ooo" "oo?oo"  "xoooo?"
    CHESSTYPE_D4P, // "o?ooo??"
    CHESSTYPE_33, //双活三
    CHESSTYPE_43, // 三四
    CHESSTYPE_44, // (同一条线上的)双四
    CHESSTYPE_4, //"?oooo?"
    CHESSTYPE_5,
    CHESSTYPE_BAN, //禁手
    CHESSTYPE_COUNT
};

namespace util
{

    inline uint8_t xy2index(int8_t row, int8_t col)
    {
        return row * 15 + col;
    }
    inline int8_t getrow(uint8_t index)
    {
        return index / 15;
    }
    inline int8_t getcol(uint8_t index)
    {
        return index % 15;
    }
    inline bool valid(uint8_t index)
    {
        if (index < BOARD_INDEX_BOUND) return true;
        else return false;
    }
    inline uint8_t otherside(uint8_t x)
    {
        return ((~x) & 1);
    }
}



//uint8_t index;
//uint8_t chessMode;
//uint8_t step;
//bool    black;
struct ChessStep
{
public:
    uint8_t index;
    uint8_t chessType;
    uint8_t step;//步数,当前step
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
        return util::getrow(index);
    }
    inline int8_t getCol()
    {
        return util::getcol(index);
    }
    inline uint8_t getSide()
    {
        return black ? PIECE_BLACK : PIECE_WHITE;
    }
    inline void setColor(int color)
    {
        black = (color == PIECE_BLACK) ? true : false;
    }
};	// 五子棋步数stepList

struct Position
{
    int8_t row;
    int8_t col;
    Position()
    {
        row = 0;
        col = 0;
    }
    Position(int8_t a, int8_t b)
    {
        row = a;
        col = b;
    }
    Position(uint8_t index)
    {
        row = util::getrow(index);
        col = util::getcol(index);
    }
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
        case DIRECTION4::DIRECTION4_LR:
            return Position{ row,col + offset };
            break;
        case DIRECTION4::DIRECTION4_UD:
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

    inline uint8_t toIndex()
    {
        return util::xy2index(row, col);
    }
};

#endif