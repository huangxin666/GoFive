#include "stdafx.h"
#include "Game.h"
#include "AIGameTree.h"
#include "AIWalker.h"


Game::Game()
{
	stepList.reserve(225);
	srand(unsigned int(time(0)));
	currentBoard = NULL;
	playerSide = 1;
	AIlevel = 3;
	HelpLevel = 1;
	parameter.ban = true;
	playerToPlayer = false;
    parameter.multithread = true;
	showStep = false;
    parameter.caculateSteps = 4;
	init();
}


Game::~Game()
{
}

void Game::setShowStep(bool b)
{
	showStep = b;
}

bool Game::isShowStep()
{
	return showStep;
}

bool Game::isMultithread()
{
	return parameter.multithread;
}
void Game::setMultithread(bool s)
{
	if (uGameState != GAME_STATE_WAIT)
        parameter.multithread = s;
}

Piece &Game::getPiece(int row, int col)
{
	return currentBoard->getPiece(row, col);
}

const std::vector<STEP> &Game::getStepList()
{
	return stepList;
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
		if (uGameState == GAME_STATE_RUN && !playerToPlayer)
		{
			AIWork();
		}
	}
}

void Game::setCaculateStep(UINT s)
{
    parameter.caculateSteps = s;
}
byte Game::getCaculateStep()
{
	return parameter.caculateSteps;
}

bool Game::isBan()
{
	return parameter.ban;
}

void Game::setBan(bool b)
{
	if (uGameState != GAME_STATE_WAIT)
        parameter.ban = b;
}

void Game::updateGameState()
{
	if (!isVictory())
	{
		if (stepList.size() == 225) {
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
	if (!playerToPlayer&&playerSide == -1) {
		//���Ӳ���
		AIWork();
	}
}

BOOL Game::isVictory()
{
	if (stepList.empty())
		return false;
	int state = currentBoard->getLastPiece().getState();
	int score = currentBoard->getStepScores(currentBoard->lastStep.uRow, currentBoard->lastStep.uCol,
		currentBoard->getLastPiece().getState(), parameter.ban);
	if (parameter.ban&&state == 1 && score < 0)//�����ж�
	{
		uGameState = GAME_STATE_BLACKBAN;
		return true;
	}
	if (score >= 100000) {
		if (state == 1) {
			uGameState = GAME_STATE_BLACKWIN;
		}
		else if (state == -1) {
			uGameState = GAME_STATE_WHITEWIN;
		}
		return true;
	}
	return false;
}

//AIStepResult Game::getBestStepAI1(ChessBoard currentBoard, int state)
//{
//	ChessBoard tempBoard;
//	AIStepResult stepCurrent;
//	AIStepResult randomStep[225];
//	randomStep[0].score = 0;
//	randomStep[0].x = 0;
//	randomStep[0].y = 0;
//	int randomCount = 0;
//	stepCurrent.score = 0;
//	int HighestScore = -500000;
//	int HighestScoreTemp = -500000;
//	int StepScore = 0;
//	int score = 0;
//	for (int i = 0; i < BOARD_ROW_MAX; ++i) {
//		for (int j = 0; j < BOARD_COL_MAX; ++j) {
//			HighestScoreTemp = -500000;
//			if (!currentBoard.getPiece(i, j).isHot())
//				continue;
//			if (currentBoard.getPiece(i, j).getState() == 0) {
//				tempBoard = currentBoard;
//				tempBoard.doNextStep(i, j, state);
//				StepScore = tempBoard.getStepScores(i, j, state, parameter.ban, false);
//				for (int a = 0; a < BOARD_ROW_MAX; ++a) {
//					for (int b = 0; b < BOARD_COL_MAX; ++b) {
//						if (!tempBoard.getPiece(a, b).isHot())
//							continue;
//						if (tempBoard.getPiece(a, b).getState() == 0) {
//							tempBoard.getPiece(a, b).setState(-state);
//							score = tempBoard.getStepScores(a, b, -state, parameter.ban, true);
//							if (score > HighestScoreTemp) {
//								HighestScoreTemp = score;
//							}
//							tempBoard.getPiece(a, b).setState(0);
//						}
//
//					}
//				}
//				StepScore = StepScore - HighestScoreTemp;
//				if (StepScore > HighestScore) {
//					HighestScore = StepScore;
//					stepCurrent.score = HighestScore;
//					stepCurrent.x = i;
//					stepCurrent.y = j;
//					randomCount = 0;
//					randomStep[randomCount] = stepCurrent;
//				}
//				else if (StepScore == HighestScore) {
//					stepCurrent.score = HighestScore;
//					stepCurrent.x = i;
//					stepCurrent.y = j;
//					randomCount++;
//					randomStep[randomCount] = stepCurrent;
//				}
//			}
//		}
//	}
//
//	int random = rand() % (randomCount + 1);
//	return randomStep[random];
//}
//
//AIStepResult Game::getBestStepAI2(ChessBoard currentBoard, int state)
//{
//	currentBoard.setGlobalThreat(parameter.ban);
//	ChessBoard tempBoard;
//	AIStepResult stepCurrent;
//	AIStepResult randomStep[225];
//	ThreatInfo info = { 0,0 }, tempInfo = { 0,0 };
//	randomStep[0].score = 0;
//	randomStep[0].x = 0;
//	randomStep[0].y = 0;
//	int randomCount = 0;
//	stepCurrent.score = 0;
//	int HighestScore = -500000;
//	int StepScore = 0;
//	for (int i = 0; i < BOARD_ROW_MAX; ++i) {
//		for (int j = 0; j < BOARD_COL_MAX; ++j) {
//			if (currentBoard.getPiece(i, j).isHot() && currentBoard.getPiece(i, j).getState() == 0)
//			{
//				tempBoard = currentBoard;
//				//StepScore = tempBoard.getPiece(i, j).getThreat(state);
//				tempBoard.doNextStep(i, j, state);
//				tempBoard.updateThreat(parameter.ban);
//				StepScore = tempBoard.getStepScores(parameter.ban, false);
//				info = tempBoard.getThreatInfo(-state);
//				//tempInfo = tempBoard.getThreatInfo(state);
//				//����
//				if (StepScore >= 100000) {
//					stepCurrent.score = StepScore;
//					stepCurrent.x = i;
//					stepCurrent.y = j;
//					return stepCurrent;
//				}
//				else if (StepScore >= 10000) {
//					if (info.HighestScore < 100000) {
//						stepCurrent.score = StepScore;
//						stepCurrent.x = i;
//						stepCurrent.y = j;
//						return stepCurrent;
//					}
//				}
//				else if (StepScore >= 8000) {
//					if (info.HighestScore < 10000) {
//						stepCurrent.score = StepScore;
//						stepCurrent.x = i;
//						stepCurrent.y = j;
//						return stepCurrent;
//					}
//				}//��һ��С���⣬����и��õ�ѡ����޷��������ˡ���
//				/*	StepScore = tempInfo.totalScore - info.totalScore;
//					if (info.HighestScore>99999)
//					{
//					StepScore = tempBoard.getPiece(i, j).getThreat(state);
//					}
//					else if (info.HighestScore > 9999 && tempInfo.HighestScore<100000)
//					{
//					StepScore = tempBoard.getPiece(i, j).getThreat(state);
//					}*/
//				StepScore = StepScore - info.totalScore;
//
//				if (StepScore > HighestScore) {
//					HighestScore = StepScore;
//					stepCurrent.score = HighestScore;
//					stepCurrent.x = i;
//					stepCurrent.y = j;
//					randomCount = 0;
//					randomStep[randomCount] = stepCurrent;
//					randomCount++;
//				}
//				else if (StepScore == HighestScore) {
//					stepCurrent.score = HighestScore;
//					stepCurrent.x = i;
//					stepCurrent.y = j;
//					randomStep[randomCount] = stepCurrent;
//					randomCount++;
//				}
//			}
//		}
//	}
//	int random = rand() % randomCount;
//	return randomStep[random];
//}


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
			currentBoard->lastStep = (step);
			currentBoard->resetHotArea();
			if (uGameState != GAME_STATE_RUN)
				uGameState = GAME_STATE_RUN;
		}
	}
	else
	{
		if (stepList.size() > 1)
		{
			if (currentBoard->getLastPiece().getState() == playerSide)
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
			currentBoard->lastStep = (step);
			currentBoard->resetHotArea();
			if (uGameState != GAME_STATE_RUN)
				uGameState = GAME_STATE_RUN;
		}
	}
}

void Game::playerWork(int row, int col)
{
	currentBoard->doNextStep(row, col, playerSide);
	stepList.push_back(STEP(uint8_t(stepList.size()) + 1, row, col, playerSide == 1 ? true : false));
	updateGameState();
	if (playerToPlayer)
		playerSide = -playerSide;
}

void Game::setJoseki(vector<Position> &choose)//��ʽ
{
    int r = rand() % choose.size();
    if (currentBoard->getPiece(choose[r].row, choose[r].col).getState())
    {
        choose[r] = choose[choose.size() - 1];
        choose.pop_back();
        setJoseki(choose);
    }
    else
    {
        currentBoard->getPiece(choose[r].row, choose[r].col).setState(-playerSide);
        if (currentBoard->getStepScores(choose[r].row, choose[r].col, -playerSide, false) != 0)
        {
            currentBoard->getPiece(choose[r].row, choose[r].col).setState(0);
            currentBoard->doNextStep(choose[r].row, choose[r].col, -playerSide);
            stepList.push_back(STEP(uint8_t(stepList.size()) + 1, choose[r].row, choose[r].col, -playerSide == 1 ? true : false));
        }
        else
        {
            currentBoard->getPiece(choose[r].row, choose[r].col).setState(0);
            choose[r] = choose[choose.size() - 1];
            choose.pop_back();
            setJoseki(choose);
        }
    }
}

Position Game::getNextStepByAI(byte level)
{
    ChessAI *ai = NULL;
    ChessBoard *board = new ChessBoard();
    *board = *currentBoard;
    if (level == 1)
    {
        ai = new AIWalker();
        parameter.level = level;
    }
    else if (level == 2)
    {
        ai = new AIWalker();
        parameter.level = level;
    }
    else if (level == 3)
    {
        ai = new AIGameTree();
        parameter.level = level;
    }
    Position pos = ai->getNextStep(board, parameter);
    delete board;
    if (ai)
    {
        delete ai;
    }
    return pos;
}

void Game::AIWork()
{
	if (stepList.empty())
	{
		currentBoard->doNextStep(7, 7, -playerSide);
		stepList.push_back(STEP(uint8_t(stepList.size()) + 1, 7, 7, -playerSide == 1 ? true : false));
	}
	//else if (stepList.size() == 2)
	//{
	//	//����ʽ
	//	vector<Position> choose = { Position(6, 6), Position(8, 8), Position(8, 6), Position(6, 8) };
	//	setJoseki(choose);
	//	updateGameState();
	//}
	else
	{
        Position pos = getNextStepByAI(AIlevel);
		//���Ӳ���
		currentBoard->doNextStep(pos.row, pos.col, -playerSide);
		stepList.push_back(STEP(uint8_t(stepList.size()) + 1, pos.row, pos.col, -playerSide == 1 ? true : false));
		updateGameState();
        
	}
}

void Game::AIHelp()
{
	if (uGameState == GAME_STATE_RUN) {
		//���Ӳ���
		if (stepList.empty()) {
			currentBoard->doNextStep(7, 7, playerSide);
			stepList.push_back(STEP(uint8_t(stepList.size()) + 1, 7, 7, playerSide == 1 ? true : false));
		}
		else 
        {
            Position pos = getNextStepByAI(HelpLevel);
			currentBoard->doNextStep(pos.row, pos.col, playerSide);
			stepList.push_back(STEP(uint8_t(stepList.size()) + 1, pos.row, pos.col, playerSide == 1 ? true : false));
		}
		updateGameState();

		if (uGameState == GAME_STATE_RUN)
		{
			AIWork();
			updateGameState();
		}
	}
}

#pragma comment (lib, "Version.lib")
BOOL GetMyProcessVer(CString& strver)   //����ȡ���Լ��İ汾��   
{
	TCHAR strfile[MAX_PATH];
	GetModuleFileName(NULL, strfile, sizeof(strfile));  //����ȡ���Լ����ļ���   

	DWORD dwVersize = 0;
	DWORD dwHandle = 0;

	dwVersize = GetFileVersionInfoSize(strfile, &dwHandle);
	if (dwVersize == 0)
	{
		return FALSE;
	}

	TCHAR szVerBuf[8192] = _T("");
	if (GetFileVersionInfo(strfile, 0, dwVersize, szVerBuf))
	{
		VS_FIXEDFILEINFO* pInfo;
		UINT nInfoLen;

		if (VerQueryValue(szVerBuf, _T("\\"), (LPVOID*)&pInfo, &nInfoLen))
		{
			strver.Format(_T("%d.%d.%d.%d"), HIWORD(pInfo->dwFileVersionMS),
				LOWORD(pInfo->dwFileVersionMS), HIWORD(pInfo->dwFileVersionLS),
				LOWORD(pInfo->dwFileVersionLS));

			return TRUE;
		}
	}
	return FALSE;
}

bool Game::saveBoard(CString path)
{
	CFile oFile(path, CFile::
		modeCreate | CFile::modeWrite);
	CArchive oar(&oFile, CArchive::store);
	//д��汾��
	CString version;
	if (!GetMyProcessVer(version))
	{
		version = _T("0.0.0.0");
	}
	oar << version;
	//д��stepList
	for (UINT i = 0; i < stepList.size(); ++i)
	{
		oar << stepList[i].step << stepList[i].uRow << stepList[i].uCol << stepList[i].isBlack;
	}
	oar.Close();
	oFile.Close();
	return true;
}

bool Game::loadBoard(CString path)
{
	/*CFile oFile(path, CFile::modeRead);*/
	CFile oFile;
	CFileException fileException;
	if (!oFile.Open(path, CFile::modeRead, &fileException))
	{
		return false;
	}

	CArchive oar(&oFile, CArchive::load);

	//����汾��
	CString version;
	oar >> version;
	//��ʼ������
	if (currentBoard)
	{
		delete currentBoard;
	}
	currentBoard = new ChessBoard();
	stepList.clear();
	//����stepList
	byte step, uRow, uCol; bool black;
	while (!oar.IsBufferEmpty())
	{
		oar >> step >> uRow >> uCol >> black;
		stepList.push_back(STEP(step, uRow, uCol, black));
	}

	for (UINT i = 0; i < stepList.size(); ++i)
	{
		currentBoard->getPiece(stepList[i]).setState(stepList[i].isBlack ? 1 : -1);
	}
	if (!stepList.empty())
	{
		currentBoard->lastStep = (stepList.back());
		currentBoard->resetHotArea();
	}

	updateGameState();

	if (stepList.empty())
		playerSide = 1;
	else if (uGameState == GAME_STATE_RUN)
		playerSide = -currentBoard->getLastPiece().getState();

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
		temp.setGlobalThreat(parameter.ban);
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
			ss << "\n\n";
		}
		ss << temp.getThreatInfo(1).HighestScore << "," << temp.getThreatInfo(1).totalScore
			<< "|" << temp.getThreatInfo(-1).HighestScore << "," << temp.getThreatInfo(-1).totalScore;


	}
	else if (mode == 3)
	{
		ss << sizeof(TreeNode) << "\n" << sizeof(ChessBoard) << "\n" << sizeof(Piece);
	}
	of << ss.str().c_str();
	of.close();
	return CString(ss.str().c_str());
}

