#include "pisqpipe.h"
#include "../Game.h"
#include <windows.h>

const char *infotext = "name=\"Random\", author=\"Petr Lastovicka\", version=\"3.2\", country=\"Czech Republic\", www=\"http://petr.lastovicka.sweb.cz\"";

#define MAX_BOARD 100

Game *game = NULL;

void brain_init()
{
    stringstream ss;
    if (!(width == 15 && height == 15)
        && !(width == 20 && height == 20)) {
        pipeOut("ERROR size of the board, only support 15*15 or 20*20");
        return;
    }
    Util::setBoardSize(width);
    game = new Game();
    if (!game->initAIHelper(0))
    {
        pipeOut("ERROR init game failed!");
        return;
    }
    game->initGame();
    pipeOut("OK");
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
        //game->putChess(x, y, PIECE_BLACK, info_renju == 1);
        game->doNextStep(x, y, info_renju == 1);
    }
    else {
        pipeOut("ERROR my move [%d,%d]", x, y);
    }
}

void brain_opponents(int x, int y)
{
    if (isFree(x, y)) {
        //game->putChess(x, y, PIECE_WHITE, info_renju == 1);
        game->doNextStep(x, y, info_renju == 1);
    }
    else {
        pipeOut("ERROR opponents's move [%d,%d]", x, y);
    }
}

void brain_block(int x, int y)
{
    if (isFree(x, y)) {
        //game->doNextStep(x, y, info_renju == 1);
        pipeOut("ERROR winning move [%d,%d]", x, y);
    }
    else {
        pipeOut("ERROR winning move [%d,%d]", x, y);
    }
}

int brain_takeback(int x, int y)
{
    if (x >= 0 && y >= 0 && x < width && y < height && game->getPieceState(x, y) != PIECE_BLANK) {
        game->stepBack();
        return 0;
    }
    return 2;
}

void brain_turn()
{
    AISettings setting;
    setting.defaultGoSearch(AILEVEL_UNLIMITED);
    setting.enableDebug = false;
    setting.maxStepTimeMs = info_timeout_turn;
    setting.restMatchTimeMs = info_time_left;
    setting.startTimeMs = start_time;
    setting.maxMemoryBytes = info_max_memory;
    //setting.fullSearch = true;

    setting.ban = info_renju == 1;
    Position ret = game->getNextStepByAI(AIGOSEARCH, setting);
    game->getAITextOut();
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
