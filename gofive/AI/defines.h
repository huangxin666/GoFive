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

using namespace std;

#define HOME_PAGE_URL "github.com/huangxin666/GoFive"
//棋盘大小
#define BOARD_SIZE_MAX 20
#define BOARD_INDEX_BOUND (BOARD_SIZE_MAX*BOARD_SIZE_MAX)

enum PIECE_STATE :uint8_t
{
    PIECE_BLACK,
    PIECE_WHITE,
    PIECE_BLANK,
    PIECE_TYPE_COUNT
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
struct AISettings
{
    //common
    bool ban;
    bool multithread;
    uint32_t maxStepTimeMs;
    uint32_t restMatchTimeMs;
    uint32_t maxMemoryBytes;
    time_t startTimeMs;
    //

    //GameTree
    uint8_t maxSearchDepth;
    bool enableAtack;
    bool extraSearch;
    //

    //GoSearch
    int minAlphaBetaDepth;
    int maxAlphaBetaDepth;
    int VCFExpandDepth;
    int VCTExpandDepth;
    bool enableDebug;//若开启，会输出更多调试信息
    bool useTranTable;
    bool fullSearch;//若开启，alphabeta搜索时会搜索全部节点，否则会放弃一些评价不好的节点（可能会导致关键节点丢失）
                    //
    void defaultBase()
    {
        ban = false;
        maxStepTimeMs = 10000;
    }

    void defaultGameTree(uint8_t level);

    void defaultGoSearch(uint8_t level);

};

struct Position;

class Util
{
public:
    static AISettings settings;
    static int8_t BoardSize;
    static int BoardIndexUpper;
    static set<Position> board_range;
    static inline void setBoardSize(int8_t size)
    {
        BoardSize = size;
        BoardIndexUpper = size * size;
        initBoardRange();
    }
    static void initBoardRange();


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

    inline void myset_intersection(set<uint8_t>* set1, set<uint8_t>* set2, set<uint8_t>* dst)
    {
        vector<uint8_t> intersection_result(set1->size() > set2->size() ? set1->size() : set2->size());
        auto it = set_intersection(set1->begin(), set1->end(), set2->begin(), set2->end(), intersection_result.begin());
        dst->clear();
        dst->insert(intersection_result.begin(), it);
    }

    template<class T>
    inline void myvector_unique(vector<T>& src, vector<T>& dst)
    {
        set<T> helpset;
        size_t len = src.size();
        for (size_t i = 0; i < len; ++i)
        {
            if (helpset.insert(src[i]).second)
            {
                dst.push_back(src[i]);
            }
        }
    }
    template<class T>
    inline size_t special_vector_unique(vector<T>& src, size_t partition, vector<T>& dst)
    {
        set<T> helpset;
        size_t len = src.size();
        size_t i = 0;

        for (; i < partition; ++i)
        {
            if (helpset.insert(src[i]).second)
            {
                dst.push_back(src[i]);
            }
        }
        size_t new_partition = dst.size();
        for (; i < len; ++i)
        {
            if (helpset.insert(src[i]).second)
            {
                dst.push_back(src[i]);
            }
        }
        return new_partition;
    }

};

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

    inline void set(int8_t r, int8_t c)
    {
        row = r;
        col = c;
    }

    inline bool equel(int8_t r, int8_t c)
    {
        return row == r && col == c;
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

    //位移 bool ret是否越界
    inline bool displace8(int8_t offset, uint8_t direction)
    {
        switch (direction)
        {
        case DIRECTION8_L:
            col -= offset;
            if (col > -1) return true;
            break;
        case DIRECTION8_R:
            col += offset;
            if (col < Util::BoardSize) return true;
            break;
        case DIRECTION8_U:
            row -= offset;
            if (row > -1) return true;
            break;
        case DIRECTION8_D:
            row += offset;
            if (row < Util::BoardSize) return true;
            break;
        case DIRECTION8_LU:
            row -= offset; col -= offset;
            if (row > -1 && col > -1) return true;
            break;
        case DIRECTION8_RD:
            col += offset; row += offset;
            if (row < Util::BoardSize && col < Util::BoardSize) return true;
            break;
        case DIRECTION8_RU:
            col += offset; row -= offset;
            if (row > -1 && col < Util::BoardSize) return true;
            break;
        case DIRECTION8_LD:
            col -= offset; row += offset;
            if (row < Util::BoardSize && col > -1) return true;
            break;
        default:
            return false;
        }
        return false;
    }

    inline bool displace4(int8_t offset, uint8_t direction)
    {
        switch (direction)
        {
        case DIRECTION4::DIRECTION4_LR:
            col += offset;
            if (col > -1 && col < Util::BoardSize) return true;
            break;
        case DIRECTION4::DIRECTION4_UD:
            row += offset;
            if (row > -1 && row < Util::BoardSize) return true;
            break;
        case DIRECTION4::DIRECTION4_RD:
            row += offset; col += offset;
            if (row > -1 && col > -1 && row < Util::BoardSize && col < Util::BoardSize) return true;
            break;
        case DIRECTION4::DIRECTION4_RU:
            row += offset; col -= offset;
            if (row > -1 && col > -1 && row < Util::BoardSize && col < Util::BoardSize) return true;
            break;
        default:
            return false;
        }
        return false;
    }

    inline bool valid()
    {
        if (row > -1 && row < Util::BoardSize && col > -1 && col < Util::BoardSize)
        {
            return true;
        }
        return false;
    }

    inline bool over_upper_bound()
    {
        if (row < Util::BoardSize)
        {
            return false;
        }
        return true;
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

#define ForEachPosition for (Position pos(0,0); !pos.over_upper_bound(); ++pos) //pos

//Position pos;
//uint8_t chessMode;
//uint8_t step;
//uint8_t state;
struct ChessStep
{
public:
    Position pos;
    uint8_t chessType;
    uint8_t step;//步数,当前step
    uint8_t state;
    ChessStep()
    {
        chessType = 0;
        step = 0;
        state = PIECE_WHITE;
    }
    ChessStep(int8_t row, int8_t col, uint8_t step, uint8_t type, uint8_t state) :step(step), state(state), chessType(type)
    {
        pos.row = row;
        pos.col = col;
    }
    ChessStep(Position pos, uint8_t step, uint8_t type, uint8_t state) :pos(pos), step(step), state(state), chessType(type)
    {
    }
    inline int8_t getRow()
    {
        return pos.row;
    }
    inline int8_t getCol()
    {
        return pos.col;
    }
    inline uint8_t getState()
    {
        return state;
    }
    inline void set(int8_t row, int8_t col)
    {
        pos.row = row;
        pos.col = col;
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