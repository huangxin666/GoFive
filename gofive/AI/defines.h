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

//���̴�С
#define BOARD_ROW_MAX 15
#define BOARD_COL_MAX 15
#define BOARD_INDEX_BOUND (BOARD_ROW_MAX*BOARD_COL_MAX)

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
    AILEVEL_GOSEARCH,
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
    uint64_t cover;
};

enum CHESSTYPE 
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
    CHESSTYPE_33, //˫����
    CHESSTYPE_43, // ����
    CHESSTYPE_44, // (ͬһ�����ϵ�)˫��
    CHESSTYPE_4, //"?oooo?"
    CHESSTYPE_5,
    CHESSTYPE_BAN, //����
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
};	// �����岽��stepList

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

    bool nextPosition(int& row, int& col, int offset, uint8_t direction)
    {
        switch (direction)
        {
        case DIRECTION8_L:
            col -= offset;
            if (col < 0) return false;
            break;
        case DIRECTION8_R:
            col += offset;
            if (col > 14) return false;
            break;
        case DIRECTION8_U:
            row -= offset;
            if (row < 0) return false;
            break;
        case DIRECTION8_D:
            row += offset;
            if (row > 14) return false;
            break;
        case DIRECTION8_LU:
            row -= offset; col -= offset;
            if (row < 0 || col < 0) return false;
            break;
        case DIRECTION8_RD:
            col += offset; row += offset;
            if (row > 14 || col > 14) return false;
            break;
        case DIRECTION8_LD:
            col -= offset; row += offset;
            if (row > 14 || col < 0) return false;
            break;
        case DIRECTION8_RU:
            col += offset; row -= offset;
            if (row < 0 || col > 14) return false;
            break;
        default:
            return false;
            break;
        }
        return true;
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