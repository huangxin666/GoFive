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
#include <algorithm>

using namespace std;

#define HOME_PAGE_URL "github.com/huangxin666/GoFive"
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
    CHESSTYPE_0,  //null
    CHESSTYPE_J2, //"?o?o?"
    CHESSTYPE_2, //"?oo?"
    CHESSTYPE_D3, //"xoo?o?" and "?ooo?" and "xooo??" and "xo?oo?"
    CHESSTYPE_J3, //"?oo?o?" and "x?ooo??"
    CHESSTYPE_3,  // "??ooo??"
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

#define CHESSTYPE_5_SCORE 10000

class Util
{
public:
    static int8_t BoardSize;
    static int BoardIndexUpper;

    static inline void setBoardSize(int8_t size)
    {
        BoardSize = size;
        BoardIndexUpper = size * size;
    }

    static inline csidx xy2index(int8_t row, int8_t col)
    {
        return row * BoardSize + col;
    }
    static inline int8_t getrow(csidx index)
    {
        return index / BoardSize;
    }
    static inline int8_t getcol(csidx index)
    {
        return index % BoardSize;
    }
    static inline bool valid(csidx index)
    {
        if (index < BoardIndexUpper) return true;
        else return false;
    }
    static inline uint8_t otherside(uint8_t x)
    {
        return ((~x) & 1);
    }
    static inline bool isfourkill(uint8_t type)
    {
        return (type == CHESSTYPE_4 || type == CHESSTYPE_43 || type == CHESSTYPE_44);
    }
    static inline bool hasdead4(uint8_t type)
    {
        return (type == CHESSTYPE_D4P || type == CHESSTYPE_D4 || type == CHESSTYPE_4 || type == CHESSTYPE_43 || type == CHESSTYPE_44);
    }
    static inline bool isdead4(uint8_t type)
    {
        return (type == CHESSTYPE_D4P || type == CHESSTYPE_D4);
    }
    static inline bool isalive3(uint8_t type)
    {
        return (type == CHESSTYPE_J3 || type == CHESSTYPE_3);
    }
    static inline bool isdead3(uint8_t type)
    {
        return type == CHESSTYPE_D3;
    }
    static inline bool isalive2(uint8_t type)
    {
        return (type == CHESSTYPE_J2 || type == CHESSTYPE_2);
    }

    static inline uint8_t get_index_offset(uint8_t direction)
    {
        switch (direction)
        {
        case DIRECTION4::DIRECTION4_LR:
            return 1;
        case DIRECTION4::DIRECTION4_UD:
            return BoardSize;
        case DIRECTION4::DIRECTION4_RD:
            return BoardSize + 1;
        case DIRECTION4::DIRECTION4_RU:
            return BoardSize - 1;
        default:
            return 0;
        }
    }

    //位移 bool ret是否越界
    static inline bool displace(int& row, int& col, int8_t offset, uint8_t direction)
    {
        switch (direction)
        {
        case DIRECTION8_L:
            col -= offset;
            if (col > -1) return true;
            break;
        case DIRECTION8_R:
            col += offset;
            if (col < BoardSize) return true;
            break;
        case DIRECTION8_U:
            row -= offset;
            if (row > -1) return true;
            break;
        case DIRECTION8_D:
            row += offset;
            if (row < BoardSize) return true;
            break;
        case DIRECTION8_LU:
            row -= offset; col -= offset;
            if (row > -1 && col > -1) return true;
            break;
        case DIRECTION8_RD:
            col += offset; row += offset;
            if (row < BoardSize && col < BoardSize) return true;
            break;
        case DIRECTION8_LD:
            col -= offset; row += offset;
            if (row < BoardSize && col > -1) return true;
            break;
        case DIRECTION8_RU:
            col += offset; row -= offset;
            if (row > -1 && col < BoardSize) return true;
            break;
        default:
            return false;
        }
        return false;
    }

    inline void myset_intersection(set<uint8_t>* set1, set<uint8_t>* set2, set<uint8_t>* dst)
    {
        vector<uint8_t> intersection_result(set1->size() > set2->size() ? set1->size() : set2->size());
        auto it = set_intersection(set1->begin(), set1->end(), set2->begin(), set2->end(), intersection_result.begin());
        dst->clear();
        dst->insert(intersection_result.begin(), it);
    }
};



//uint8_t index;
//uint8_t chessMode;
//uint8_t step;
//bool    black;
struct ChessStep
{
public:
    csidx index;
    uint8_t chessType;
    uint8_t step;//步数,当前step
    bool    black;
    ChessStep(int8_t row, int8_t col, uint8_t step, uint8_t chessMode, bool black) :step(step), black(black), chessType(chessMode)
    {
        index = Util::xy2index(row, col);
    }
    ChessStep(csidx index, uint8_t step, uint8_t chessMode, bool black) :index(index), step(step), black(black), chessType(chessMode)
    {
    }
    ChessStep() :step(0)
    {
    }
    inline int8_t getRow()
    {
        return Util::getrow(index);
    }
    inline int8_t getCol()
    {
        return Util::getcol(index);
    }
    inline PIECE_STATE getSide()
    {
        return black ? PIECE_BLACK : PIECE_WHITE;
    }
    inline PIECE_STATE getOtherSide()
    {
        return black ? PIECE_WHITE : PIECE_BLACK;
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
    Position(csidx index)
    {
        row = Util::getrow(index);
        col = Util::getcol(index);
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
        if (row > -1 && row < Util::BoardSize && col > -1 && col < Util::BoardSize)
        {
            return true;
        }
        return false;
    }

    inline csidx toIndex()
    {
        return Util::xy2index(row, col);
    }

    inline csidx toIndexWithCheck()
    {
        if (!valid()) return Util::BoardIndexUpper;
        return Util::xy2index(row, col);
    }
};

#endif