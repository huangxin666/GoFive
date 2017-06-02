#ifndef AI_UTILS_H
#define AI_UTILS_H
#include <stdint.h>




enum CHESSTYPE //初级棋型
{
    CHESSTYPE_0, //null
    CHESSTYPE_J2,//"?o?o?"
    CHESSTYPE_2,//"?oo?"
    CHESSTYPE_D3,//"xoo?o?" and "?ooo?" and "xooo??"
    CHESSTYPE_D3P,//"xo?oo?"
    CHESSTYPE_3,//"?oo?o?" "??ooo?"
    CHESSTYPE_D4,  //"o?ooo" "oo?oo"  "xoooo?"
    CHESSTYPE_D4P, // "o?ooo??"
    CHESSTYPE_4, //"?oooo?"
    CHESSTYPE_5,
    CHESSTYPE_BAN, //禁手
    CHESSTYPE_33, //双活三
    CHESSTYPE_43, // 三四
    CHESSTYPE_44, // (同一条线上的)双四
    CHESSTYPE_COUNT
};

const int32_t chesstype2rating[CHESSTYPE_COUNT] = {
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
    inline int32_t type2score(uint8_t type)
    {
        return chesstype2rating[type];
    }
    inline bool isdead4(uint8_t type)
    {
        return (type == CHESSTYPE_D4P || type == CHESSTYPE_D4);
    }
    inline bool isdead3(uint8_t type)
    {
        return (type == CHESSTYPE_D3P || type == CHESSTYPE_D3);
    }
    inline bool isalive2(uint8_t type)
    {
        return (type == CHESSTYPE_J2 || type == CHESSTYPE_2);
    }
};


#endif
