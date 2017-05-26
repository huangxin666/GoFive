#ifndef AI_UTILS_H
#define AI_UTILS_H

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
};


#endif
