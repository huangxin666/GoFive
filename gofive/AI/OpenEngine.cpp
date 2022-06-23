#include "AIEngine.h"
#include <random>
using namespace std;

//map<uint64_t, OpenEngine::OpenInfo> OpenEngine::openMap;

void rotateChess(ChessBoard *cb, uint8_t center)
{

}


Position OpenEngine::getOpen1(ChessBoard *cb)
{
    return Position(Util::BoardSize / 2 - 1, Util::BoardSize / 2 - 1);
}

bool OpenEngine::checkOpen2(ChessBoard *cb)
{
    if (cb->getLastStep().pos == Position(Util::BoardSize / 2, Util::BoardSize / 2))
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
    uint32_t direction = rd32(e) % 8;
    Position result = lastStep.pos;
    result.displace8(direction);

    return result;
}


bool OpenEngine::checkOpen3(ChessBoard *cb)
{
    ChessStep lastStep = cb->getLastStep();

    return true;
}

Position OpenEngine::getOpen3(ChessBoard *cb)
{
    ChessStep lastStep = cb->getLastStep();
    return Position();
}