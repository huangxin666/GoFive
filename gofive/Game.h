#pragma once
#include "ChessBoard.h"


//��Ϸ״̬
#define GAME_STATE_WHITEWIN	2
#define GAME_STATE_BLACKWIN	1
#define GAME_STATE_RUN		0
#define GAME_STATE_DRAW     3 //ƽ��
#define GAME_STATE_BLACKBAN 4 //���ӽ��ָ渺
#define GAME_STATE_WAIT		5 //�ȴ�AI����

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
    void AIWork(int level, int side);
    int getPieceState(int row, int col);
    void updateGameState();
    void setGameState(int);

    void changeSide(int side);
    void playerWork(int row, int col);
    CString debug(int mode);

    bool initTrieTree();
    bool initAIHelper(int num);
    Position getNextStepByAI(byte AIlevel);
    string getChessMode(int row, int col, int state);
    bool isBan();
    void setBan(bool b);

    AIRESULTFLAG getForecastStatus();
    HashStat getTransTableStat();
public:
    vector<ChessStep> stepList;
    AIParam parameter;
    ChessBoard *currentBoard;
    int playerSide; //������ӵ���ɫ��1�����֣�
    bool playerToPlayer;
    bool showStep;
    byte AIlevel;
    byte HelpLevel;
    byte gameState;
};