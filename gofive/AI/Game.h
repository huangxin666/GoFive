#ifndef _GAME_H_
#define _GAME_H_
#include "ChessBoard.h"

//��Ϸ״̬
#define GAME_STATE_RUN		0
#define GAME_STATE_BLACKWIN	1
#define GAME_STATE_WHITEWIN	2
#define GAME_STATE_DRAW     3 //ƽ��
#define GAME_STATE_BLACKBAN 4 //���ӽ��ָ渺
//#define GAME_STATE_WAIT		5 //�ȴ�AI����

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

    void doNextStep(int row, int col);
    void doNextStepByAI(uint8_t level, bool ban, AIParameter setting);
    void stepBack();
    string debug(int mode);
    void printTable(uint8_t i);

    bool initTrieTree();
    bool initAIHelper(int num);
    Position getNextStepByAI(uint8_t level, bool ban, AIParameter setting);
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


    AIRESULTFLAG getForecastStatus();
    HashStat getTransTableStat();
private:
    vector<ChessStep> stepList;
    ChessBoard *currentBoard;
    uint8_t gameState;
};

 
#endif