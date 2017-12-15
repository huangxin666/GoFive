#ifndef __AI_DEFINES_H__
#define __AI_DEFINES_H__

#include <cstdint>
#include <sstream>
#include <string>
#include <fstream>
#include <vector>
#include <shared_mutex>
#include <memory>
#include <ctime>
#include <utility>
#include <set>
#include <algorithm>
#include <bitset>
#include <queue>

using namespace std;

#define HOME_PAGE_URL "github.com/huangxin666/GoFive"
//棋盘大小
#define BOARD_SIZE_MAX 20
#define BOARD_INDEX_BOUND (BOARD_SIZE_MAX*BOARD_SIZE_MAX)

enum GAME_RULE :uint8_t
{
    FREESTYLE,  // 无禁手
    STANDARD,   // 禁长连
    RENJU,      // 禁手
};

enum PIECE_STATE :uint8_t
{
    PIECE_BLACK,
    PIECE_WHITE,
    PIECE_BLANK,
    PIECE_TYPE_COUNT,
    PIECE_BLOCK
};

//方向(4向)
enum DIRECTION4 :uint8_t
{
    DIRECTION4_LR,       //as→
    DIRECTION4_UD,       //as↓
    DIRECTION4_RD,		//as ↘
    DIRECTION4_RU,	    //as ↗
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

const int direct4_offset_row[DIRECTION4_COUNT] = { 0,1,1,-1 };
const int direct4_offset_col[DIRECTION4_COUNT] = { 1,0,1, 1 };
const int direct8_offset_row[DIRECTION8_COUNT] = { 0,0,-1,1,-1,1, 1,-1 };
const int direct8_offset_col[DIRECTION8_COUNT] = { -1,1, 0,0,-1,1,-1, 1 };

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
    CHESSTYPE_DJ2, //"?o??o?"
    CHESSTYPE_J2, //"??o?o??"
    CHESSTYPE_2, //"??oo??"
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

typedef void(*MessageCallBack)(string&);
struct AISettings
{
    //common
    GAME_RULE rule;
    bool multithread;
    uint32_t maxStepTimeMs;
    uint32_t restMatchTimeMs;
    uint32_t maxMemoryBytes;
    time_t startTimeMs;
    MessageCallBack msgfunc;
    //

    //GameTree
    uint8_t atack_payment;
    bool enableAtack;
    bool extraSearch;
    //

    //GoSearch
    int minAlphaBetaDepth;
    int maxAlphaBetaDepth;
    int VCFExpandDepth;
    int VCTExpandDepth;
    bool enableDebug;//若开启，会输出更多调试信息
    bool useTransTable;
    bool useDBSearch;//若开启，alphabeta搜索时会搜索全部节点，否则会放弃一些评价不好的节点（可能会导致关键节点丢失）
                    //
    void defaultBase()
    {
        rule = FREESTYLE;
        maxStepTimeMs = 10000;
    }

    void defaultGameTree(uint8_t level);

    void defaultGoSearch(uint8_t level);

};


struct Rect
{
    int row_lower, row_upper;
    int col_lower, col_upper;
};

struct Position;

class Util
{
public:
    static bool needBreak;
    static int SizeUpper;
    static int8_t BoardSize;
    static inline void setBoardSize(int8_t size)
    {
        BoardSize = size;
        SizeUpper = size - 1;
    }

    static inline uint8_t otherside(uint8_t x)
    {
        //return ((~x) & 1);
        return 1 - x;
    }
    static inline bool isRealFourKill(uint8_t type)
    {
        return (type == CHESSTYPE_4 || type == CHESSTYPE_44);
    }
    static inline bool isDoubleThreat(uint8_t type)
    {
        return (type == CHESSTYPE_4 || type == CHESSTYPE_43 || type == CHESSTYPE_44);
    }
    static inline bool hasdead4(uint8_t type)
    {
        return (type == CHESSTYPE_D4P || type == CHESSTYPE_D4 || type == CHESSTYPE_4 || type == CHESSTYPE_43 || type == CHESSTYPE_44);
    }
    static inline bool iscontinue4(uint8_t type)
    {
        return (type == CHESSTYPE_D4P || type == CHESSTYPE_D4 || type == CHESSTYPE_4);
    }
    static inline bool isdead4(uint8_t type)
    {
        return (type == CHESSTYPE_D4P || type == CHESSTYPE_D4);
    }
    static inline bool isalive3or33(uint8_t type)
    {
        return (type == CHESSTYPE_J3 || type == CHESSTYPE_3 || type == CHESSTYPE_33);
    }
    static inline bool isSpecialType(uint8_t type)
    {
        return (type == CHESSTYPE_43 || type == CHESSTYPE_44 || type == CHESSTYPE_33);
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
    static inline bool isthreat(uint8_t type)
    {
        return type > CHESSTYPE_D3 && type < CHESSTYPE_BAN;
    }

    static inline void myset_intersection(set<uint8_t>* set1, set<uint8_t>* set2, set<uint8_t>* dst)
    {
        vector<uint8_t> intersection_result(set1->size() > set2->size() ? set1->size() : set2->size());
        auto it = set_intersection(set1->begin(), set1->end(), set2->begin(), set2->end(), intersection_result.begin());
        dst->clear();
        dst->insert(intersection_result.begin(), it);
    }

    static inline void rect_fix(Rect &rect)
    {
        if (rect.row_lower < 0) rect.row_lower = 0;
        if (rect.col_lower < 0) rect.col_lower = 0;
        if (rect.row_upper > SizeUpper) rect.row_upper = SizeUpper;
        if (rect.col_upper > SizeUpper) rect.col_upper = SizeUpper;
    }
    static inline Rect generate_rect(int row, int col, int offset)
    {
        Rect rect{ row - offset, row + offset, col - offset, col + offset };
        rect_fix(rect);
        return rect;
    }

    static inline bool inRect(int row, int col, int center_row, int center_col, int offset)
    {
        if (row < center_row - offset || row > center_row + offset || col < center_col - offset || col > center_col + offset) return false;
        return true;
    }
};

struct Position
{

    int8_t row;
    int8_t col;
public:
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
    inline int8_t getRow()
    {
        return row;
    }

    inline int8_t getCol()
    {
        return col;
    }

    inline void set(int8_t r, int8_t c)
    {
        row = r;
        col = c;
    }

    //位移 bool ret是否越界
    inline bool displace8(int8_t offset, uint8_t direction)
    {
        row += direct8_offset_row[direction] * offset;
        col += direct8_offset_col[direction] * offset;
        return (row > -1 && row < Util::BoardSize && col > -1 && col < Util::BoardSize);
    }

    inline bool displace8(uint8_t direction)
    {
        row += direct8_offset_row[direction];
        col += direct8_offset_col[direction];
        return (row > -1 && row < Util::BoardSize && col > -1 && col < Util::BoardSize);
    }

    inline bool displace4(int8_t offset, uint8_t direction)
    {
        row += direct4_offset_row[direction] * offset;
        col += direct4_offset_col[direction] * offset;
        return (row > -1 && row < Util::BoardSize && col > -1 && col < Util::BoardSize);
    }

    inline bool displace4(uint8_t direction)
    {
        row += direct4_offset_row[direction];
        col += direct4_offset_col[direction];
        return (row > -1 && row < Util::BoardSize && col > -1 && col < Util::BoardSize);
    }

    inline bool valid()
    {
        return (row > -1 && row < Util::BoardSize && col > -1 && col < Util::BoardSize);
    }

    inline bool not_over_upper_bound()
    {
        return row < Util::BoardSize;
    }

    Position &operator++()      //++i
    {
        if (++col < Util::BoardSize)
        {
            return *this;
        }

        col = 0;
        ++row;
        return *this;
    }

    const Position operator++(int) //i++
    {
        Position old(row, col);
        ++(*this);
        return old;
    }

    bool operator==(const Position &other)
    {
        return row == other.row && col == other.col;
    }
    bool operator<(const Position &other) const
    {
        return row < other.row || (row == other.row && col < other.col);
    }
};

//pos
#define ForEachPosition for (Position pos; pos.not_over_upper_bound(); ++pos)

#define ForEachMove(board) for (Position pos; pos.not_over_upper_bound(); ++pos)\
                           if (board->getState(pos) == PIECE_BLANK)


struct StepCandidateItem
{
    Position pos;
    int16_t value;
    uint8_t direction;
    uint8_t type;
    StepCandidateItem(Position i, int16_t value, uint8_t direction = 0) :pos(i), value(value), direction(direction)
    {};
    StepCandidateItem(Position i, int16_t value, uint8_t direction, uint8_t type) :pos(i), value(value), direction(direction), type(type)
    {};
    bool operator<(const StepCandidateItem& other)  const
    {
        return pos < other.pos;
    }
};

inline bool CandidateItemCmp(const StepCandidateItem &a, const StepCandidateItem &b)
{
    return a.value > b.value;
}

//Position pos;
//uint8_t chessMode;
//uint8_t step;
//uint8_t state;
struct ChessStep
{
public:
    Position pos;
    uint16_t step;//步数,当前step
    uint8_t chessType;
    uint8_t state;
    ChessStep()
    {
        chessType = 0;
        step = 0;
        state = PIECE_WHITE;
    }
    ChessStep(int8_t row, int8_t col, uint16_t step, uint8_t type, uint8_t state) :step(step), state(state), chessType(type)
    {
        pos.set(row, col);
    }
    ChessStep(Position pos, uint16_t step, uint8_t type, uint8_t state) :pos(pos), step(step), state(state), chessType(type)
    {
    }
    inline int8_t getRow()
    {
        return pos.getRow();
    }
    inline int8_t getCol()
    {
        return pos.getCol();
    }
    inline void set(int8_t row, int8_t col)
    {
        pos.set(row, col);
    }
    inline uint8_t getOtherSide()
    {
        return Util::otherside(state);
    }
    inline void setState(uint8_t s)
    {
        state = s;
    }
    inline void changeSide()
    {
        state = Util::otherside(state);
    }
};	// 五子棋步数stepList

#endif