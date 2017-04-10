#pragma once
#include "ChessBoard.h"
#include "defines.h"

class Game
{
public:
    Game();
    ~Game();
    bool saveBoard(CString path);
    bool loadBoard(CString path);
    void initGame();
    bool checkVictory();
    void stepBack();
    void AIWork();
    int getPieceState(int row, int col);
    void updateGameState();
    void setGameState(int);

    void changeSide(int side);
    void playerWork(int row, int col);
    CString debug(int mode);

    bool initTrieTree();
    bool initAIHelper(int num);
    Position getNextStepByAI(byte AIlevel);
    void getChessMode(char *str, int row, int col, int state);
    bool isBan();
    void setBan(bool b);
public:
    vector<ChessStep> stepList;
    AIParam parameter;
    ChessBoard *currentBoard;
    int playerSide; //玩家棋子的颜色（1黑先手）
    bool playerToPlayer;
    bool showStep;
    byte AIlevel;
    byte HelpLevel;
    byte gameState;
};