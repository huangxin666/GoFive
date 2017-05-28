#ifndef AI_UTILS_H
#define AI_UTILS_H
#include <stdint.h>




enum CHESSMODE //初级棋型
{
    MODE_BASE_0, //null
    MODE_BASE_j2,//"?o?o?"
    MODE_BASE_2,//"?oo?"
    MODE_BASE_d3,//"xoo?o?" and "?ooo?" and "xooo??"
    MODE_BASE_d3p,//"xo?oo?"
    MODE_BASE_3,//"?oo?o?" "??ooo?"
    MODE_BASE_d4,  //"o?ooo" "oo?oo"  "xoooo?"
    MODE_BASE_d4p, // "o?ooo??"
    MODE_BASE_4, //"?oooo?"
    MODE_BASE_5,
    MODE_ADV_BAN, //禁手
    MODE_ADV_33, //双活三
    MODE_ADV_43, // 三四
    MODE_ADV_44, // (同一条线上的)双四
    MODE_COUNT
};

const int32_t chess_ratings[MODE_COUNT] = {
    0,            //MODE_BASE_0,
    5,            //MODE_BASE_j2,
    5,            //MODE_BASE_2, 
    8,            //MODE_BASE_d3,
    10,           //MODE_BASE_d3p
    100,          //MODE_BASE_3, 
    120,          //MODE_BASE_d4,
    150,          //MODE_BASE_d4p
    1000,         //MODE_BASE_4,
    10000,        //MODE_BASE_5,
    -100,         //MODE_ADV_BAN,
    500,          //MODE_ADV_33,
    800,          //MODE_ADV_43,
    1000          //MODE_ADV_44,
};

namespace util
{

    inline uint8_t xy2index(int8_t row, int8_t col)
    {
        return row * 15 + col;
    }
    inline int8_t getRow(uint8_t index)
    {
        return index / 15;
    }
    inline int8_t getCol(uint8_t index)
    {
        return index % 15;
    }
    inline bool valid(uint8_t index)
    {
        if (index < 225) return true;
        else return false;
    }
    inline uint8_t otherside(uint8_t x)
    {
        return ((~x) & 1);
    }
    inline int mode2score(uint8_t mode)
    {
        return chess_ratings[mode];
    }
    inline bool is5continus()
    {

    }
};


#endif
