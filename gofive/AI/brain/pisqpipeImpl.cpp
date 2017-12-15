#include "pisqpipe.h"
#include "../Game.h"
#include <windows.h>

const char *infotext = "name=\"gofive\", author=\"HuangXin\", version=\"0.6.1.0\", country=\"China\", www=\"\"";

#define MAX_BOARD 100

Game *game = NULL;

void brain_init()
{
    game = new Game();

    stringstream ss;
    if (!(width == 15 && height == 15)
        && !(width == 20 && height == 20)) {
        pipeOut("ERROR size of the board, only support 15*15 or 20*20");
        return;
    }
    Util::setBoardSize(width);

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    int thread_num;
    if (si.dwNumberOfProcessors > 4)
    {
        thread_num = si.dwNumberOfProcessors - 1;
    }
    else
    {
        thread_num = si.dwNumberOfProcessors;
    }

    if (!game->initAIHelper(thread_num))
    {
        pipeOut("ERROR init game failed!");
        return;
    }
    game->initGame();
    pipeOut("OK");
}

void msgCallBack(string &msg)
{
    pipeOut("DEBUG %s", msg.c_str());
}

void brain_restart()
{
    game->initGame();
    pipeOut("OK");
}

int isFree(int x, int y)
{
    return x >= 0 && y >= 0 && x < width && y < height && game->getPieceState(x, y) == PIECE_BLANK;
}

void brain_my(int x, int y)
{
    if (isFree(x, y)) {
        game->doNextStep(x, y, info_renju == 1 ? RENJU : (info_exact5 == 1 ? STANDARD : FREESTYLE));
    }
    else {
        pipeOut("ERROR my move [%d,%d]", x, y);
    }
}

void brain_opponents(int x, int y)
{
    if (isFree(x, y)) {
        game->doNextStep(x, y, info_renju == 1 ? RENJU : (info_exact5 == 1 ? STANDARD : FREESTYLE));
    }
    else {
        pipeOut("ERROR opponents's move [%d,%d]", x, y);
    }
}

void brain_block(int x, int y)
{
    if (isFree(x, y)) {
        pipeOut("ERROR winning move [%d,%d]", x, y);
    }
    else {
        pipeOut("ERROR winning move [%d,%d]", x, y);
    }
}

int brain_takeback(int x, int y)
{
    if (x >= 0 && y >= 0 && x < width && y < height && game->getPieceState(x, y) != PIECE_BLANK) {
        game->stepBack(info_renju == 1 ? RENJU : (info_exact5 == 1 ? STANDARD : FREESTYLE));
        return 0;
    }
    return 2;
}

void brain_turn()
{
    AISettings setting;
    setting.msgfunc = msgCallBack;
    setting.defaultGoSearch(AILEVEL_UNLIMITED);
    setting.enableDebug = true;
    setting.maxStepTimeMs = info_timeout_turn;
    setting.restMatchTimeMs = info_time_left;
    setting.startTimeMs = start_time;
    setting.maxMemoryBytes = info_max_memory;
    setting.useDBSearch = true;

    setting.rule = info_renju == 1 ? RENJU : (info_exact5 == 1 ? STANDARD : FREESTYLE);
    Position ret = game->getNextStepByAI(AIGOSEARCH, setting);

    do_mymove(ret.row, ret.col);
}

void brain_end()
{
    if (game) delete game;
}

#ifdef DEBUG_EVAL
#include <windows.h>

void brain_eval(int x, int y)
{
    HDC dc;
    HWND wnd;
    RECT rc;
    char c;
    wnd = GetForegroundWindow();
    dc = GetDC(wnd);
    GetClientRect(wnd, &rc);
    c = (char)(board[x][y] + '0');
    TextOut(dc, rc.right - 15, 3, &c, 1);
    ReleaseDC(wnd, dc);
}

#endif
