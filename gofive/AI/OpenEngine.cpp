#include "AIEngine.h"
#include <random>
using namespace std;

static Position open1 = Position(6, 6);

//map<uint64_t, OpenEngine::OpenInfo> OpenEngine::openMap;

void rotateChess(ChessBoard *cb, uint8_t center)
{

}


Position OpenEngine::getOpen1(ChessBoard *cb)
{
    return open1;
}

bool OpenEngine::checkOpen2(ChessBoard *cb)
{
    if (cb->getLastStep().pos == Position(7,7))
    {
        return true;
    }
    return false;
}

Position OpenEngine::getOpen2(ChessBoard *cb)
{
    ChessStep lastStep = cb->getLastStep();
    default_random_engine e((uint32_t)time(NULL));
    uniform_int_distribution<uint32_t> rd32;
    int safe_count = 0;
    while (1)
    {
        uint32_t direction = rd32(e) % 8;
        Position result = lastStep.pos.getNextPosition(direction / 2, direction % 2 == 1 ? 1 : -1);
        if (result.valid())
        {
            return result;
        }
        safe_count++;
        if (safe_count > 1000)
        {
            return Position(7, 7);
        }
    }
}


bool OpenEngine::checkOpen3(ChessBoard *cb)
{
    ChessStep lastStep = cb->getLastStep();
    if (lastStep.pos.row - open1.row > 1 || lastStep.pos.row - open1.row < -1)
    {
        return false;
    }
    if (lastStep.pos.col - open1.col > 1 || lastStep.pos.col - open1.col < -1)
    {
        return false;
    }
    return true;
}

Position OpenEngine::getOpen3(ChessBoard *cb)
{
    ChessStep lastStep = cb->getLastStep();
    return Position();
}