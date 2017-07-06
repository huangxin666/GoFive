#ifndef __UTILS_H__
#define __UTILS_H__

#include "defines.h"

const uint8_t direction_offset_index[DIRECTION4_COUNT] = { 1, 15, 16, 14 };

struct ChessTypeInfo
{
    int32_t rating;
    int8_t atackPriority;
    int8_t defendPriority;
    int16_t atackFactor;
    int16_t defendFactor;
};

//const ChessTypeInfo chesstypes[CHESSTYPE_COUNT] = {
//    {0    , 0, 0,     0,  0},           //CHESSTYPE_0,
//    {10   , 1, 0,     1,  0},           //CHESSTYPE_j2,
//    {10   , 2, 1,     2,  1},           //CHESSTYPE_2, 
//    {10   , 1, 1,     4,  2},           //CHESSTYPE_d3,
//    {20   , 2, 2,     8,  4},           //CHESSTYPE_d3p
//    {80   , 3, 1,    10,  5},           //CHESSTYPE_J3
//    {100  , 4, 3,    12,  6},           //CHESSTYPE_3, 
//    {120  , 0, 3,     5,  5},           //CHESSTYPE_d4,
//    {150  , 5, 4,    15,  7},           //CHESSTYPE_d4p
//    {250  , 6, 5,    50, 25},           //CHESSTYPE_33,
//    {450  , 7, 5,   100, 50},           //CHESSTYPE_43,
//    {500  , 8, 5,   100, 50},           //CHESSTYPE_44,
//    {500  , 8, 8,   150,100},           //CHESSTYPE_4,
//    {10000, 9, 9, 10000,200},           //CHESSTYPE_5,
//    {-100 ,-9, 5,     0,  0},           //CHESSTYPE_BAN,
//};
 
const ChessTypeInfo chesstypes[CHESSTYPE_COUNT] = {
    { 0    , 0, 0,     0,  0 },           //CHESSTYPE_0,
    { 10   , 1, 0,     1,  0 },           //CHESSTYPE_j2,
    { 10   , 2, 1,     2,  1 },           //CHESSTYPE_2, 
    { 10   , 2, 0,     4,  2 },           //CHESSTYPE_d3,
    { 20   , 3, 1,     8,  4 },           //CHESSTYPE_d3p
    { 80   , 3, 1,    15,  8 },           //CHESSTYPE_J3
    { 100  , 4, 3,    25, 10 },           //CHESSTYPE_3, 
    { 120  , 0, 3,    20, 15 },           //CHESSTYPE_d4,
    { 150  , 4, 4,    30, 20 },           //CHESSTYPE_d4p
    { 250  , 5, 5,    50, 30 },           //CHESSTYPE_33,
    { 450  ,10, 5,   100, 30 },           //CHESSTYPE_43,
    { 500  ,12, 5,   100, 40 },           //CHESSTYPE_44,
    { 500  ,13, 6,   150, 50 },           //CHESSTYPE_4,
    { 10000,15,15, 10000,200 },           //CHESSTYPE_5,
    { -100 ,-9, 5,     0,  0 },           //CHESSTYPE_BAN,
};


namespace util
{

    inline bool inLocalArea(uint8_t index, uint8_t center, int8_t length)
    {
        if (getrow(index) < getrow(center) - length || getrow(index) > getrow(center) + length || getcol(index) < getcol(center) - length || getcol(index) > getcol(center) + length)
        {
            return false;
        }
        return true;
    }
    inline int32_t type2score(uint8_t type)
    {
        return chesstypes[type].rating;
    }
    inline bool hasfourkill(uint8_t type)
    {
        return (type == CHESSTYPE_4 || type == CHESSTYPE_43 || type == CHESSTYPE_44);
    }
    inline bool hasdead4(uint8_t type)
    {
        return (type == CHESSTYPE_D4P || type == CHESSTYPE_D4 || type == CHESSTYPE_4 || type == CHESSTYPE_43 || type == CHESSTYPE_44);
    }
    inline bool isdead4(uint8_t type)
    {
        return (type == CHESSTYPE_D4P || type == CHESSTYPE_D4);
    }
    inline bool isalive3(uint8_t type)
    {
        return (type == CHESSTYPE_J3 || type == CHESSTYPE_3);
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