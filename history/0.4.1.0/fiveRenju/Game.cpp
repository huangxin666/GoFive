#include "stdafx.h"
#include "Game.h"
#include <sstream>
#include <string>
#include <fstream>
using namespace std;

Game::Game()
{
	stepList.reserve(225);
	srand(unsigned int(time(0)));
	currentBoard = NULL;
	playerSide = 1;
	AIlevel = 1;
	HelpLevel = 1;
	ban = false;
	playerToPlayer = false;
	multithread = true;
	caculateStep = 4;
	init();
}


Game::~Game()
{
}

bool Game::isMultithread()
{
	return multithread;
}
void Game::setMultithread(bool s)
{
	if (uGameState != GAME_STATE_WAIT)
		multithread = s;
}

ChessBoard* Game::getCurrentBoard()
{
	return currentBoard;
}

int Game::getGameState()
{
	return uGameState;
}
int Game::getPlayerSide()
{
	return playerSide;
}

bool Game::stepListIsEmpty()
{
	return stepList.empty();
}

void Game::setAIlevel(int level)
{
	if (uGameState != GAME_STATE_WAIT)
		AIlevel = level;
}
void Game::setHelpLevel(int level)
{
	if (uGameState != GAME_STATE_WAIT)
		HelpLevel = level;
}

int Game::getAIlevel()
{
	return AIlevel;
}
int Game::getHelpLevel()
{
	return HelpLevel;
}

bool Game::isPlayerToPlayer()
{
	return playerToPlayer;
}

void Game::setPlayerToPlayer(bool s)
{
	if (uGameState != GAME_STATE_WAIT)
		playerToPlayer = s;
}

void Game::changeSide(int side)
{
	if (playerSide != side)
	{
		playerSide = side;
		if (uGameState == GAME_STATE_RUN&&!playerToPlayer)
		{
				AIWork();
		}
	}
}

void Game::setCaculateStep(UINT s)
{
	caculateStep = s;
}
UINT Game::getCaculateStep()
{
	return caculateStep;
}

bool Game::isBan()
{
	return ban;
}

void Game::setBan(bool b)
{
	if(uGameState!= GAME_STATE_WAIT)
		ban = b;
}

void Game::updateGameState()
{
	if (!isVictory())
	{
		if (stepList.size() == 225){
			uGameState = GAME_STATE_DRAW;
		}
		else
			uGameState = GAME_STATE_RUN;
	}
}

void Game::setGameState(int state)
{
	uGameState = state;
}

void Game::init()
{
	uGameState = GAME_STATE_RUN;
	if (currentBoard)
	{
		delete currentBoard;
	}
	currentBoard = new ChessBoard();
	stepList.clear();

	if (playerToPlayer)
		playerSide = 1;
	if (!playerToPlayer&&playerSide == -1){
		//棋子操作
		AIWork();
	}
}

BOOL Game::isVictory()
{
	if (stepList.empty())
		return false;
	int state = currentBoard->getPiece().getState();
	int score = currentBoard->getStepScores(currentBoard->getPiece().getRow(), currentBoard->getPiece().getCol(), 
		currentBoard->getPiece().getState(), ban);
	if (ban&&state == 1 && score < 0)//禁手判断
	{
		uGameState = GAME_STATE_BLACKBAN;
		return true;
	}
	if (score >= 100000){
		if (state == 1){
			uGameState = GAME_STATE_BLACKWIN;
		}
		else if (state == -1){
			uGameState = GAME_STATE_WHITEWIN;
		}
		return true;
	}
	return false;
}

AISTEP Game::getBestStepAI1(ChessBoard currentBoard, int state)
{
	ChessBoard tempBoard;
	AISTEP stepCurrent;
	AISTEP randomStep[225];
	randomStep[0].score = 0;
	randomStep[0].x = 0;
	randomStep[0].y = 0;
	int randomCount = 0;
	stepCurrent.score = 0;
	int HighestScore = -500000;
	int HighestScoreTemp = -500000;
	int StepScore = 0;
	int score = 0;
	for (int i = 0; i < BOARD_ROW_MAX; ++i){
		for (int j = 0; j < BOARD_COL_MAX; ++j){
			HighestScoreTemp = -500000;
			if (!currentBoard.getPiece(i, j).isHot())
				continue;
			if (currentBoard.getPiece(i, j).getState() == 0){
				tempBoard = currentBoard;
				tempBoard.doNextStep(i, j, state);
				StepScore = tempBoard.getStepScores(tempBoard.getPiece());
				//score = currentBoard.getGlobalScore(state);
				for (int a = 0; a < BOARD_ROW_MAX; ++a){
					for (int b = 0; b<BOARD_COL_MAX; ++b){
						if (!tempBoard.getPiece(a, b).isHot())
							continue;
						if (tempBoard.getPiece(a, b).getState() == 0){
							tempBoard.getPiece(a, b).setState(-state);
							score = tempBoard.getStepScores(a, b, -state);
							//score = currentBoard.getGlobalScore(state);
							if (score>HighestScoreTemp){
								HighestScoreTemp = score;
							}
							tempBoard.getPiece(a, b).setState(0);
						}

					}
				}
				StepScore = StepScore - HighestScoreTemp;
				if (StepScore > HighestScore){
					HighestScore = StepScore;
					stepCurrent.score = HighestScore;
					stepCurrent.x = i;
					stepCurrent.y = j;
					randomCount = 0;
					randomStep[randomCount] = stepCurrent;
				}
				else if (StepScore == HighestScore){
					stepCurrent.score = HighestScore;
					stepCurrent.x = i;
					stepCurrent.y = j;
					randomCount++;
					randomStep[randomCount] = stepCurrent;
				}
			}
		}
	}

	int random = rand() % (randomCount + 1);
	return randomStep[random];
}

//AISTEP Game::getBestStepAI2(ChessBoard currentBoard, int state)
//{
//	ChessBoard tempBoard;
//	AISTEP stepCurrent;
//	AISTEP randomStep[225];
//	randomStep[0].score = 0;
//	randomStep[0].x = 0;
//	randomStep[0].y = 0;
//	int randomCount = 0;
//	stepCurrent.score = 0;
//	int HighestScore = -500000;
//	int HighestScoreTemp = 0;
//	int StepScore = 0;
//	int totalScore = 0;
//	for (int i = 0; i < BOARD_ROW_MAX; ++i){
//		for (int j = 0; j < BOARD_COL_MAX; ++j){
//			if (currentBoard.getPiece(i, j).isHot()&&currentBoard.getPiece(i, j).getState() == 0)
//			{
//				HighestScoreTemp = 0;
//				totalScore = 0;
//				tempBoard = currentBoard;
//				tempBoard.doNextStep(i, j, state);
//				StepScore = tempBoard.getStepScores(tempBoard.getPiece());
//				//score = currentBoard.getGlobalScore(state);
//				THREATINFO info = tempBoard.getThreatInfo(-state, 1);
//				totalScore = info.totalScore;
//				HighestScoreTemp = info.HighestScore;
//				if (StepScore >= 100000){
//					stepCurrent.score = StepScore;
//					stepCurrent.x = i;
//					stepCurrent.y = j;
//					return stepCurrent;
//				}
//				else if (StepScore >= 10000){
//					if (HighestScoreTemp < 100000){
//						stepCurrent.score = StepScore;
//						stepCurrent.x = i;
//						stepCurrent.y = j;
//						return stepCurrent;
//					}
//				}
//				else if (StepScore >= 8000){
//					if (HighestScoreTemp < 10000){
//						stepCurrent.score = StepScore;
//						stepCurrent.x = i;
//						stepCurrent.y = j;
//						return stepCurrent;
//					}
//				}
//				StepScore = StepScore - totalScore;
//				if (StepScore>HighestScore){
//					HighestScore = StepScore;
//					stepCurrent.score = HighestScore;
//					stepCurrent.x = i;
//					stepCurrent.y = j;
//					randomCount = 0;
//					randomStep[randomCount] = stepCurrent;
//				}
//				else if (StepScore == HighestScore){
//					stepCurrent.score = HighestScore;
//					stepCurrent.x = i;
//					stepCurrent.y = j;
//					randomCount++;
//					randomStep[randomCount] = stepCurrent;
//				}
//			}
//		}
//	}
//	int random = rand() % (randomCount + 1);
//	return randomStep[random];
//}

AISTEP Game::getBestStepAI2(ChessBoard currentBoard, int state)
{
	currentBoard.setGlobalThreat(ban);
	ChessBoard tempBoard;
	AISTEP stepCurrent;
	AISTEP randomStep[225];
	THREATINFO info(0, 0), tempInfo(0, 0);
	randomStep[0].score = 0;
	randomStep[0].x = 0;
	randomStep[0].y = 0;
	int randomCount = 0;
	stepCurrent.score = 0;
	int HighestScore = -500000;
	int StepScore = 0;
	for (int i = 0; i < BOARD_ROW_MAX; ++i){
		for (int j = 0; j < BOARD_COL_MAX; ++j){
			if (currentBoard.getPiece(i, j).isHot() && currentBoard.getPiece(i, j).getState() == 0)
			{
				tempBoard = currentBoard;
				StepScore = tempBoard.getPiece(i, j).getThreat(state);
				tempBoard.doNextStep(i, j, state);
				tempBoard.updateThreat(ban);
				info = tempBoard.getThreatInfo(-state);
				//tempInfo = tempBoard.getThreatInfo(state);
				//出口
				if (StepScore >= 100000){
					stepCurrent.score = StepScore;
					stepCurrent.x = i;
					stepCurrent.y = j;
					return stepCurrent;
				}
				else if (StepScore >= 10000){
					if (info.HighestScore < 100000){
						stepCurrent.score = StepScore;
						stepCurrent.x = i;
						stepCurrent.y = j;
						return stepCurrent;
					}
				}
				else if (StepScore >= 8000){
					if (info.HighestScore < 10000){
						stepCurrent.score = StepScore;
						stepCurrent.x = i;
						stepCurrent.y = j;
						return stepCurrent;
					}
				}//有一个小问题，如果有更好的选择就无法遍历到了。。
				/*	StepScore = tempInfo.totalScore - info.totalScore;
					if (info.HighestScore>99999)
					{
					StepScore = tempBoard.getPiece(i, j).getThreat(state);
					}
					else if (info.HighestScore > 9999 && tempInfo.HighestScore<100000)
					{
					StepScore = tempBoard.getPiece(i, j).getThreat(state);
					}*/
				StepScore = StepScore - info.totalScore;

				if (StepScore > HighestScore){
					HighestScore = StepScore;
					stepCurrent.score = HighestScore;
					stepCurrent.x = i;
					stepCurrent.y = j;
					randomCount = 0;
					randomStep[randomCount] = stepCurrent;
					randomCount++;
				}
				else if (StepScore == HighestScore){
					stepCurrent.score = HighestScore;
					stepCurrent.x = i;
					stepCurrent.y = j;	
					randomStep[randomCount] = stepCurrent;
					randomCount++;
				}
			}
		}
	}
	int random = rand() % randomCount;
	return randomStep[random];
}

AISTEP Game::getBestStepAI3(ChessBoard currentBoard, int state)
{
	currentBoard.setGlobalThreat(ban);	
	if (multithread)
	{
		TreeNode root(currentBoard, caculateStep, false);
		root.setBan(ban);
		return root.searchBest2();
	}
	else
	{
		TreeNode root(currentBoard, 4, false);
		root.setBan(ban);
		return root.searchBest();
	}
}


void Game::stepBack()
{
	if (playerToPlayer)
	{
		if (stepList.size() > 0)
		{
			currentBoard->getPiece(stepList.back().uRow, stepList.back().uCol).setState(0);
			stepList.pop_back();
			playerSide = -playerSide;
			STEP step;
			if (stepList.empty())
				step.step = 0;
			else
				step = stepList.back();
			currentBoard->setLastStep(step);
			currentBoard->resetHotArea();
			if (uGameState != GAME_STATE_RUN)
				uGameState = GAME_STATE_RUN;
		}
	}
	else
	{
		if (stepList.size() > 1)
		{
			if (currentBoard->getPiece().getState() == playerSide)
			{
				currentBoard->getPiece(stepList.back().uRow, stepList.back().uCol).setState(0);
				stepList.pop_back();
			}
			else
			{
				currentBoard->getPiece(stepList.back().uRow, stepList.back().uCol).setState(0);
				stepList.pop_back();
				currentBoard->getPiece(stepList.back().uRow, stepList.back().uCol).setState(0);
				stepList.pop_back();
			}
			STEP step; 
			if (stepList.empty())
				step.step = 0;
			else
				step = stepList.back();	
			currentBoard->setLastStep(step);
			currentBoard->resetHotArea();
			if (uGameState != GAME_STATE_RUN)
				uGameState = GAME_STATE_RUN;
		}
	}
}

void Game::playerWork(int row, int col)
{
	currentBoard->doNextStep(row, col, playerSide);
	stepList.push_back(STEP(stepList.size() + 1,row, col));
	updateGameState();
	if (playerToPlayer)
		playerSide = -playerSide;
}

void Game::AIWork()
{
	if (stepList.empty())
	{
		currentBoard->doNextStep(7, 7, -playerSide);
		stepList.push_back(STEP(stepList.size() + 1, 7, 7));
	}
	else
	{
		AISTEP AIstep;
		if (AIlevel == 1)
			AIstep = getBestStepAI1(*currentBoard, -playerSide);
		else if (AIlevel == 2)
			AIstep = getBestStepAI2(*currentBoard, -playerSide);
		else if (AIlevel == 3)
			AIstep = getBestStepAI3(*currentBoard, -playerSide);
		//棋子操作
		currentBoard->doNextStep(AIstep.x, AIstep.y, -playerSide);
		stepList.push_back(STEP(stepList.size() + 1, AIstep.x, AIstep.y));
		updateGameState();
	}
}

void Game::AIHelp()
{
	if (uGameState == GAME_STATE_RUN){
		//棋子操作
		if (stepList.empty()){
			currentBoard->doNextStep(7, 7, playerSide);
			stepList.push_back(STEP(stepList.size() + 1, 7, 7));
		}
		else{
			AISTEP AIstep;
			if (HelpLevel == 1)
				AIstep = getBestStepAI1(*currentBoard, playerSide);
			else if (HelpLevel == 2)
				AIstep = getBestStepAI2(*currentBoard, playerSide);
			else if (HelpLevel == 3)
				AIstep = getBestStepAI2(*currentBoard, playerSide);
			//棋子操作
			currentBoard->doNextStep(AIstep.x, AIstep.y, playerSide);
			stepList.push_back(STEP(stepList.size() + 1, AIstep.x, AIstep.y));
		}
		//temp.current->isFocus=false;
		updateGameState();

		if (uGameState == GAME_STATE_RUN)
		{
			AIWork();
			updateGameState();
		}
	}
}

bool Game::saveBoard(CString path)
{
	CFile oFile(path, CFile::
		modeCreate | CFile::modeWrite);
	CArchive oar(&oFile, CArchive::store);
	//读入m_pFive
	for (int i = 0; i < BOARD_ROW_MAX; i++)
	{
		for (int j = 0; j < BOARD_COL_MAX; j++)
		{
			oar <<currentBoard->getPiece(i,j).getState()
				<< currentBoard->getPiece(i, j).isHot();
		}
	}
	//读入stepList
	int size = stepList.size();
	for (int i = 0; i < size; i++)
	{
		oar << stepList[i].step << stepList[i].uRow << stepList[i].uCol;
	}
	oar.Close();
	oFile.Close();
	return true;
}

bool Game::loadBoard(CString path)
{
	CFile oFile(path, CFile::modeRead);
	//CFile oFile;
	//CFileException fileException;
	//if (!oFile.Open(path, CFile::modeRead, &fileException))
	//{
	//	return false;
	//}

	CArchive oar(&oFile, CArchive::load);

	int state;
	bool Hot;
	//读入m_pFive
	for (int i = 0; i < BOARD_ROW_MAX; i++)
	{
		for (int j = 0; j < BOARD_COL_MAX; j++)
		{
			oar >> state >> Hot;
			currentBoard->getPiece(i, j).setState(state);
			currentBoard->getPiece(i, j).setHot(Hot);
		}
	}

	//读入stepList
	UINT step, uRow, uCol;
	while (!oar.IsBufferEmpty())
	{
		oar >> step >> uRow >> uCol;
		stepList.push_back(STEP(step, uRow, uCol));
	}
	currentBoard->setLastStep(stepList.back());
	updateGameState();

	if (playerToPlayer)
	{
		if (stepListIsEmpty())
			playerSide = 1;
		else if (currentBoard->getPiece().getState() == playerSide)
			playerSide = -playerSide;
	}
	else
	{
		if (uGameState == GAME_STATE_RUN&&!stepList.empty())
		{
			if (currentBoard->getPiece().getState() == playerSide){
				AIWork();
			}
		}
	}
	oar.Close();
	oFile.Close();
	return true;
}



CString Game::debug(int mode)
{
	CString info;
	string s;
	stringstream ss(s);
	fstream of("debug.txt", ios::out);
	ChessBoard temp = *currentBoard;
	if (mode == 1)
	{
		temp.setGlobalThreat(ban);
		ss << " " << "\t";
		for (int j = 0; j < BOARD_COL_MAX; ++j)
			ss << j << "\t";
		ss << "\n";
		for (int i = 0; i < BOARD_ROW_MAX; ++i)
		{
			ss << i << "\t";
			for (int j = 0; j < BOARD_COL_MAX; ++j)
			{
				ss << temp.getPiece(i, j).getThreat(1) << "|"
					<< temp.getPiece(i, j).getThreat(-1) << "\t";
			}
			ss << "\n\n\n";
		}
		ss << temp.getThreatInfo(playerSide).HighestScore << "," << temp.getThreatInfo(playerSide) .totalScore
			<< "|" << temp.getThreatInfo(-playerSide).HighestScore <<"," << temp.getThreatInfo(-playerSide).totalScore;


	}
	else if (mode == 3)
	{
		ss << sizeof(TreeNode) << "\n" << sizeof(ChessBoard)<<"\n" << sizeof(Piece);
	}
	of << ss.str().c_str();
	of.close();
	return CString(ss.str().c_str());
}

