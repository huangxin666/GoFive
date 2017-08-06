#ifndef __UTILS_H__
#define __UTILS_H__

#include "defines.h"
#include <algorithm>

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
//    { 10   , 0, 0,     0,  0 },           //CHESSTYPE_j2,
//    { 10   , 1, 1,     4,  2 },           //CHESSTYPE_2, 
//    { 10   , 1, 1,     6,  3 },           //CHESSTYPE_d3,
//    { 20   , 1, 1,     8,  4 },           //CHESSTYPE_d3p
//    { 80   , 1, 1,    12,  6 },           //CHESSTYPE_J3
//    { 100  , 2, 2,    25, 12 },           //CHESSTYPE_3, 
//    { 120  , 0, 2,    20, 16 },           //CHESSTYPE_d4,
//    { 150  , 2, 2,    30, 20 },           //CHESSTYPE_d4p
//    { 250  , 8, 6,   100, 40 },           //CHESSTYPE_33,
//    { 450  ,10, 6,   200, 50 },           //CHESSTYPE_43,
//    { 500  ,12, 6,   250, 45 },           //CHESSTYPE_44,
//    { 500  ,13,10,   500, 50 },           //CHESSTYPE_4,
//    { 10000,15,15, 10000,100 },           //CHESSTYPE_5,
//    { -100 ,-9, 5,   -10, -5 },           //CHESSTYPE_BAN,
//};

const ChessTypeInfo chesstypes[CHESSTYPE_COUNT] = {
    { 0    , 0, 0,     0,  0 },           //CHESSTYPE_0,
    { 10   , 0, 0,     0,  0 },           //CHESSTYPE_j2,
    { 10   , 2, 1,     4,  1 },           //CHESSTYPE_2, 
    { 10   , 1, 1,     6,  4 },           //CHESSTYPE_d3,
    { 20   , 2, 1,     8,  6 },           //CHESSTYPE_d3p
    { 80   , 2, 1,    12,  6 },           //CHESSTYPE_J3
    { 100  , 3, 3,    25, 12 },           //CHESSTYPE_3, 
    { 120  , 0, 2,    20, 16 },           //CHESSTYPE_d4,
    { 150  , 3, 3,    30, 20 },           //CHESSTYPE_d4p
    { 250  , 8, 6,   100, 40 },           //CHESSTYPE_33,
    { 450  ,10, 6,   200, 50 },           //CHESSTYPE_43,
    { 500  ,12, 6,   250, 60 },           //CHESSTYPE_44,
    { 500  ,13,10,   500, 70 },           //CHESSTYPE_4,
    { 10000,15,15, 10000,100 },           //CHESSTYPE_5,
    { -100 ,-9, 5,   -10, -5 },           //CHESSTYPE_BAN,
};


namespace util
{
    inline int32_t type2score(uint8_t type)
    {
        return chesstypes[type].rating;
    }

    inline void myset_intersection(set<uint8_t>* set1, set<uint8_t>* set2, set<uint8_t>* dst)
    {
        vector<uint8_t> intersection_result(set1->size() > set2->size() ? set1->size() : set2->size());
        auto it = set_intersection(set1->begin(), set1->end(), set2->begin(), set2->end(), intersection_result.begin());
        dst->clear();
        dst->insert(intersection_result.begin(), it);
    }
};

#endif