#include "AIEngine.h"
#include <random>
using namespace std;

static Position open1 = Position(7, 7);

void rotateChess(ChessBoard *cb, uint8_t center)
{

}


Position getOpen1(ChessBoard *cb)
{
    return open1;
}

Position getOpen2(ChessBoard *cb)
{
    ChessStep lastStep = cb->getLastStep();
    Position open(lastStep.index);
    default_random_engine e(4768);//fixed seed
    uniform_int_distribution<uint32_t> rd32;
    int safe_count = 0;
    while (1)
    {
        uint32_t direction = rd32(e) % 8;
        Position result = open.getNextPosition(direction / 2, direction % 2 == 1 ? 1 : -1);
        if (result.valid())
        {
            return result;
        }
        safe_count++;
        if (safe_count > 1000)
        {
            return Position(0);
        }
    }
}


bool checkOpen3(ChessBoard *cb)
{
    ChessStep lastStep = cb->getLastStep();
    Position open2(lastStep.index);
    if (open2.row - open1.row > 1 || open2.row - open1.row < -1)
    {
        return false;
    }
    if (open2.col - open1.col > 1 || open2.col - open1.col < -1)
    {
        return false;
    }
    return true;
}

Position getOpen3(ChessBoard *cb)
{
    ChessStep lastStep = cb->getLastStep();
    return Position();
}