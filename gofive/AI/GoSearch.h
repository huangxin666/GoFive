#ifndef __GOSEARCH_H__
#define __GOSEARCH_H__
#include "ChessBoard.h"
#include "defines.h"

class GoSearchEngine
{
private:
    ChessBoard board;
    vector<uint8_t> optimal_path;
};



#endif // !__GOSEARCH_H__
