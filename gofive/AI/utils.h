#ifndef __UTILS_H__
#define __UTILS_H__

#include "defines.h"
#include <algorithm>

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
//    { 0    , 0, 0,     0,  0 },           //CHESSTYPE_0,
//    { 10   , 1, 0,     1,  0 },           //CHESSTYPE_j2,
//    { 10   , 2, 2,     2,  1 },           //CHESSTYPE_2, 
//    { 10   , 1, 1,     4,  3 },           //CHESSTYPE_d3,
//    { 20   , 2, 2,     8,  5 },           //CHESSTYPE_d3p
//    { 80   , 3, 1,    20, 10 },           //CHESSTYPE_J3
//    { 100  , 4, 3,    50, 20 },           //CHESSTYPE_3, 
//    { 120  , 0, 3,    20, 10 },           //CHESSTYPE_d4,
//    { 150  , 4, 3,    60, 30 },           //CHESSTYPE_d4p
//    { 250  , 6, 4,   100, 40 },           //CHESSTYPE_33,
//    { 450  ,10, 5,   150, 50 },           //CHESSTYPE_43,
//    { 500  ,12, 5,   180, 55 },           //CHESSTYPE_44,
//    { 500  ,13, 6,   200, 60 },           //CHESSTYPE_4,
//    { 10000,15,15, 10000,100 },           //CHESSTYPE_5,
//    { -100 ,-9, 5,     0,  0 },           //CHESSTYPE_BAN,
//};
 
const ChessTypeInfo chesstypes[CHESSTYPE_COUNT] = {
    { 0    , 0, 0,     0,  0 },           //CHESSTYPE_0,
    { 10   , 2, 0,     1,  0 },           //CHESSTYPE_j2,
    { 10   , 2, 2,     2,  1 },           //CHESSTYPE_2, 
    { 10   , 2, 1,     4,  2 },           //CHESSTYPE_d3,
    { 20   , 2, 2,     6,  3 },           //CHESSTYPE_d3p
    { 80   , 3, 1,    12,  6 },           //CHESSTYPE_J3
    { 100  , 3, 3,    20, 10 },           //CHESSTYPE_3, 
    { 120  , 0, 2,    15,  7 },           //CHESSTYPE_d4,
    { 150  , 3, 3,    20, 10 },           //CHESSTYPE_d4p
    { 250  , 6, 4,    50, 15 },           //CHESSTYPE_33,
    { 450  ,10, 5,   100, 20 },           //CHESSTYPE_43,
    { 500  ,12, 5,   120, 25 },           //CHESSTYPE_44,
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
    inline bool isfourkill(uint8_t type)
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

    inline void myset_intersection(set<uint8_t>* set1, set<uint8_t>* set2, set<uint8_t>* dst)
    {
        vector<uint8_t> intersection_result(set1->size() > set2->size() ? set1->size() : set2->size());
        auto it = set_intersection(set1->begin(), set1->end(), set2->begin(), set2->end(), intersection_result.begin());
        dst->insert(intersection_result.begin(), it);
    }
};





#endif