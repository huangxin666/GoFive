#pragma once
#include "ChessBoard.h"
#include "TreeNode.h"
#include "defines.h"
#include <vector>


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
	Piece &getPiece(int row, int col);
	int getGameState();
	int getPlayerSide();
	void setAIlevel(int);
	void setHelpLevel(int);
	void updateGameState();
	void setGameState(int);
	int getAIlevel();
	int getHelpLevel();
	void changeSide(int side);
	void playerWork(int row,int col);
	CString debug(int mode);
	bool stepListIsEmpty();
	bool isPlayerToPlayer();
	const std::vector<STEP> &getStepList();
	void setPlayerToPlayer(bool);
	bool isBan();
	void setBan(bool);
	bool isMultithread();
	void setMultithread(bool);
	void setCaculateStep(UINT);
	byte getCaculateStep();
	void setJoseki(std::vector<Position> &);
	void setShowStep(bool b);
	bool isShowStep();
private:
	byte caculateStep;
	bool playerToPlayer;
	bool showStep;
	byte AIlevel;
	byte HelpLevel;
	byte uGameState;
	int playerSide; //玩家棋子的颜色（1黑先手）
	ChessBoard * currentBoard;
	bool ban;
	std::vector<STEP> stepList;
	bool multithread;
};