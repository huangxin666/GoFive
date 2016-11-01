#pragma once
#include "ChessBoard.h"
#include "TreeNode.h"
#include "defines.h"
#include <sstream>
#include <string>
#include <fstream>
using namespace std;

class Game
{
public:
	Game();
	~Game();
	bool saveBoard(CString path);
	bool loadBoard(CString path);
	AISTEP getBestStepAI1(ChessBoard currentBoard, int state);
	AISTEP getBestStepAI2(ChessBoard currentBoard, int state);
	AISTEP getBestStepAI3(ChessBoard currentBoard, int state);
	void init();
	BOOL isVictory();
	void stepBack();
	void AIWork();
	void AIHelp();
	ChessBoard* getCurrentBoard();
	int getGameState();
	int getPlayerSide();
	void setAIlevel(int);
	void setHelpLevel(int);
	void updateGameState();
	int getAIlevel();
	int getHelpLevel();
	void changeSide(int side);
	void playerWork(int row,int col);
	ChessBoard killDeadFour(int max, int side, ChessBoard cs);//�ݹ���������
	ChessBoard killAliveThree(int max, int side, ChessBoard cs);//�ݹ���������
	THREATINFO getDFS(ChessBoard cs, int max,int side);
	CString debug(int mode);
	bool stepListIsEmpty();
	bool isPlayerToPlayer();
	void setPlayerToPlayer(bool);
private:
	bool playerToPlayer;
	int AIlevel;
	int HelpLevel;
	int uGameState;
	int playerSide; //������ӵ���ɫ��1�����֣�
	ChessBoard * currentBoard;
	bool ban;
	std::vector<STEP> stepList;
};

