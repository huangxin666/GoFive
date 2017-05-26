#ifndef _GAME_H_
#define _GAME_H_
#include "ChessBoard.h"

//游戏状态
#define GAME_STATE_WHITEWIN	2
#define GAME_STATE_BLACKWIN	1
#define GAME_STATE_RUN		0
#define GAME_STATE_DRAW     3 //平局
#define GAME_STATE_BLACKBAN 4 //黑子禁手告负
//#define GAME_STATE_WAIT		5 //等待AI计算

enum GAME_MODE
{
    PLAYER_FIRST,
    AI_FIRST,
    NO_AI,
    NO_PLAYER
};

struct AIParameter
{
    uint8_t caculateSteps;
    uint8_t level;
    bool ban;
    bool multithread;
};

class Game
{
public:
    Game();
    ~Game();
    void initGame();
    void stepBack();
    int getPieceState(int row, int col);

    void doNextStep(int row, int col);
    void doNextStepByAI(uint8_t level, bool ban, AISettings setting);
    CString debug(int mode);

    bool initTrieTree();
    bool initAIHelper(int num);
    Position getNextStepByAI(uint8_t level, bool ban, AISettings setting);
    string getChessMode(int row, int col, int state);


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
    int getStepsCount()
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


    AIRESULTFLAG getForecastStatus();
    HashStat getTransTableStat();
private:
    vector<ChessStep> stepList;
    ChessBoard *currentBoard;
    uint8_t gameState;
};

 
#endif