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

struct AIParameter
{
    uint8_t maxSearchDepth;
    uint64_t maxSearchTimeMs;
    bool multiThread;
};

class Game
{
public:
    Game();
    ~Game();
    void initGame();
    
    int getPieceState(int row, int col);

    void doNextStep(int row, int col, bool ban);
    void doNextStepByAI(uint8_t level, bool ban, AIParameter setting);
    void stepBack();
    string debug(int mode);
    void printTable(uint8_t i);

    bool initTrieTree();
    bool initAIHelper(int num);
    Position getNextStepByAI(uint8_t level, bool ban, AIParameter setting);
    string getChessMode(int row, int col, int state);

    string getDebugString();
    void setGameState(uint8_t);
    void updateGameState();

    uint8_t getGameState()
    {
        return gameState;
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

    string getAITextOut()
    {
        if (ai != NULL)
        {
            ai->updateTextOut();
        }
        return AIEngine::textOut;
    }

private:
    vector<ChessStep> stepList;
    ChessBoard *currentBoard = NULL;
    uint8_t gameState;
    AIEngine *ai = NULL;
};

 
#endif