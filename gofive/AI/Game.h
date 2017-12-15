#ifndef __GAME_H__
#define __GAME_H__
#include "AIEngine.h"
//游戏状态
#define GAME_STATE_RUN		0
#define GAME_STATE_BLACKWIN	1
#define GAME_STATE_WHITEWIN	2
#define GAME_STATE_DRAW     3 //平局
#define GAME_STATE_BLACKBAN 4 //黑子禁手告负

enum GAME_MODE
{
    PLAYER_FIRST,
    AI_FIRST,
    NO_AI,
    NO_PLAYER
};

class Game
{
public:
    Game();
    ~Game();
    void initGame();
    bool initAIHelper(int num);

    int getPieceState(int row, int col);

    void putChess(int row, int col, uint8_t side, GAME_RULE ban);
    void doNextStep(int row, int col, GAME_RULE ban);
    void doNextStepByAI(AIENGINE type, AISettings setting);
    void stepBack(GAME_RULE ban);
    string debug(int mode, AISettings setting);
    void printTable(uint8_t i);

    Position getNextStepByAI(AIENGINE type, AISettings setting);

    void setGameState(uint8_t);
    void updateGameState();
    void stopSearching();
    uint8_t getGameState()
    {
        return gameState;
    }

    uint8_t getChessType(int row, int col, uint8_t side)
    {
        return currentBoard->getChessType(Position(row, col), side);
    }

    ChessStep getLastStep()
    {
        if (stepList.size() > 0)
        {
            return stepList.back();
        }
        else
        {
            return ChessStep();
        }
    }
    size_t getStepsCount()
    {
        return stepList.size();
    }
    ChessStep getStep(size_t steps)
    {
        if (steps < stepList.size())
        {
            return stepList[steps];
        }
        else
        {
            return ChessStep();
        }
    }

private:
    vector<ChessStep> stepList;
    ChessBoard *currentBoard = NULL;
    uint8_t gameState;
    AIEngine *ai = NULL;
};


#endif