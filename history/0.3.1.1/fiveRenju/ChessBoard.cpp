#include "stdafx.h"
#include "ChessBoard.h"

char pats[STR_COUNT][10] = { ("ooooo"), ("?oooo?"),("?oooox"), ("xoooo?"),
							("??ooo?o"), ("o?ooo??"),("oo?oo"), ("?ooo?"),
							("?oo?o?"),("?o?oo?"),("??ooox"), ("xooo??"),
							("?o?oox"), ("xo?oo?"),("?oo?ox"), ("xoo?o?"),
							("?oo?"), ("?o?o?"),("xooo?o"), ("o?ooox")};
int fail[STR_COUNT][10] = { { -1, 0, 1, 2, 3 }, { -1, -1, -1, -1, -1, 0 }, { -1, -1, -1, -1, -1, -1 }, { -1, -1, -1, -1, -1, -1 },
							{-1,0,-1,-1,-1,0,-1}, {-1,-1,0,0,0,1,-1},{-1,0,-1,0,1}, {-1,-1,-1,-1,0},
							{-1,-1,-1,0,1,0}, {-1,-1,0,1,-1,0},{-1,0,-1,-1,-1,-1}, {-1,-1,-1,-1,-1,-1},
							{-1,-1,0,1,-1,-1}, {-1,-1,-1,-1,-1,-1},{-1,-1,-1,0,1,-1}, {-1,-1,-1,-1,-1,-1},
							{-1,-1,-1,0}, {-1,-1,0,1,2},{-1,-1,-1,-1,-1,-1}, {-1,-1,0,0,0,-1} };
int size_p[STR_COUNT] = { 5, 6, 6, 6,
						7, 7,5, 5,
						6, 6,6, 6,
						6, 6,6, 6,
						4, 5,6, 6 };
int evaluate[STR_COUNT] = { 100000, 10000,1000, 1000,
							1030, 1030,1000, 1200,
							100, 100,30, 30,
							10, 10,10, 10,
							20, 18,999,999};

int fastfind(int f[], int size_p, char p[], int size_o, char o[]){
	int i = 0, j = 0, sum = 0;
	if (size_p == 0)
		return sum;
	while (i<size_o){
		if (o[i] == p[j]){
			i++; j++;
		}
		else{
			if (j == 0)
				i++;
			else
				j = f[j - 1] + 1;
		}
		if (j == size_p){
			sum++; j = f[j - 1] + 1;
			//i wrote as "j=0;i-=f[j-1]+1" wrongly;which takes more f[j-1]+1 times per count;
		}
	}
	return sum;
}

ChessBoard::ChessBoard()
{	
	stepList.reserve(225);
	for (int i = 0; i<15; ++i){
		for (int j = 0; j<15; ++j){
			m_pFive[i][j].setXY(i, j);
		}
	}
}


ChessBoard::~ChessBoard()
{

}

Piece &ChessBoard::getPiece(int row, int col){
	return m_pFive[row][col];
}
Piece &ChessBoard::getPiece(STEP step){
	return m_pFive[step.uRow][step.uCol];
}
Piece &ChessBoard::getPiece(){//无参数，返回当前棋子
	return m_pFive[stepList.back().uRow][stepList.back().uCol];
}

int ChessBoard::stepListGetCount(){
	return stepList.size();
}

BOOL ChessBoard::stepListIsEmpty(){
	return stepList.empty();
}
void ChessBoard::stepListAddTail(STEP step){
	stepList.push_back(step);
}
void ChessBoard::stepListRemoveTail(){
	stepList.pop_back();
}


STEP ChessBoard::stepListGetAt(int p){
	return stepList[p];
}

void ChessBoard::resetHotArea(){
	for (int i = 0; i<15; ++i){
		for (int j = 0; j<15; ++j){
			m_pFive[i][j].setHot(false);
		}
	}
	int count = stepList.size();
	STEP temp;
	int row, col;
	for (int k = 0; k < count; k++){
		temp = stepList[k];
		row = temp.uRow; col = temp.uCol;
		for (int i = row - 2; i < row + 3; i++){
			if (i<0)
				continue;
			else if (i>14)
				break;
			for (int j = col - 2; j < col + 3; j++){
				if (j<0)
					continue;
				else if (j>14)
					break;
				m_pFive[i][j].setHot(true);
			}
		}
	}
}

void ChessBoard::stepBack(int playerSide){
	if (stepList.size()>1){
		if (getPiece().getState() == playerSide){
			getPiece().setFocus(false);
			getPiece().setFlag(false);
			getPiece().setState(0);
			stepList.pop_back();
		}else{
			getPiece().setFocus(false);
			getPiece().setFlag(false);
			getPiece().setState(0);
			stepList.pop_back();
			getPiece().setFocus(false);
			getPiece().setFlag(false);
			getPiece().setState(0);
			stepList.pop_back();
		}
		if (!stepList.empty())
			getPiece().setFlag(true) ;
	}
	resetHotArea();
}

bool ChessBoard::doNextStep(int row, int col, int side){
	if (m_pFive[row][col].getState() != 0){
		return false;//已有棋子
	}else{
		if (!stepListIsEmpty())
			getPiece().setFlag(false);
		STEP step;
		step.step = stepListGetCount() + 1;
		step.uCol = col;
		step.uRow = row;
		stepList.push_back(step);
		m_pFive[row][col].setState(side);
		m_pFive[row][col].setFlag(true);
		for (int i = row-2; i < row+3; i++){
			if (i<0)
				continue;
			else if (i>14)
				break;
			for (int j = col - 2; j < col+3; j++){
				if (j<0)
					continue;
				else if (j>14)
					break;
				m_pFive[i][j].setHot(true);
			}
		}
		return true;
	}
}

THREATINFO ChessBoard::getThreatInfo(int color,int mode){
	int score = 0;
	int HighestScoreTemp = 0;
	int totalScore = 0;
	if (mode == 1)
	{
		for (int a = 0; a<BOARD_ROW_MAX; ++a){
			for (int b = 0; b<BOARD_COL_MAX; ++b){
				if (!m_pFive[a][b].isHot())
					continue;
				if (m_pFive[a][b].getState() == 0){
					m_pFive[a][b].setState(color);
					score = getStepScores(a, b, color);
					//score = currentBoard.getGlobalScore(state);
					totalScore += score;
					if (score>HighestScoreTemp){
						HighestScoreTemp = score;
					}
					m_pFive[a][b].setState(0);
				}

			}
		}
	}
	else if (mode == 2)
	{
		ChessBoard chessboard_temp;
		for (int a = 0; a<BOARD_ROW_MAX; ++a){
			for (int b = 0; b<BOARD_COL_MAX; ++b){
				if (!m_pFive[a][b].isHot())
					continue;
				if (m_pFive[a][b].getState() == 0){
					m_pFive[a][b].setState(color);
					score = getStepScores(a, b, color);
					if (score<1031 && score>998){
						m_pFive[a][b].setState(0);
						chessboard_temp = *this;
						chessboard_temp.doNextStep(a, b, color);
						score = getDFS(chessboard_temp);
					}
					//score = currentBoard.getGlobalScore(state);
					totalScore += score;
					if (score>HighestScoreTemp){
						HighestScoreTemp = score;
					}
					m_pFive[a][b].setState(0);
				}

			}
		}
	}
	//for (int j = 0; j < 18; ++j){
	//	score += count[j] * evaluate[j];
	//}
	THREATINFO info;
	info.totalScore = totalScore;
	info.HighestScore = HighestScoreTemp;
	return info;
}

int ChessBoard::getDFS(ChessBoard cs){
	int state = -cs.getPiece().getState();
	ChessBoard cb_temp;
	int highestScore = -500000;
	for (int a = 0; a < BOARD_ROW_MAX; ++a){
		for (int b = 0; b < BOARD_COL_MAX; ++b){
			if (!m_pFive[a][b].isHot())
				continue;
			if (m_pFive[a][b].getState() == 0){
				cb_temp = cs;
				cb_temp.doNextStep(a, b, state);
				THREATINFO info = cb_temp.getThreatInfo(-state, 1);
				if (info.HighestScore < 10000){
					info = cb_temp.getThreatInfo(-state, 2);
				}
				if (info.HighestScore>highestScore){
					highestScore = info.HighestScore;
				}
			}
		}
	}
	return highestScore;
}

int ChessBoard::getStepScores(int row, int col, int state){
	int stepScore = 0;
	char direction[4][13];//四个方向棋面（0表示空，-1表示断，1表示连）

	for (int i = 0; i<13; ++i){//横向
		if (col - 6 + i<0 )
			direction[0][i] = 'x';
		else if (col - 6 + i>14)
			direction[0][i] = 'x';
		else if (m_pFive[row][col - 6 + i].getState() == -state)
			direction[0][i] = 'x';
		else if (m_pFive[row][col - 6 + i].getState() == state)
			direction[0][i] = 'o';
		else if (m_pFive[row][col - 6 + i].getState() == 0)
			direction[0][i] = '?';
	}
	for (int i = 0; i<13; ++i){//纵向
		if (row - 6 + i<0)
			direction[1][i] = 'x';
		else if (row - 6 + i>14)
			direction[1][i] = 'x';
		else if (m_pFive[row - 6 + i][col].getState() == -state)
			direction[1][i] = 'x';
		else if (m_pFive[row - 6 + i][col].getState() == state)
			direction[1][i] = 'o';
		else if (m_pFive[row - 6 + i][col].getState() == 0)
			direction[1][i] = '?';
	}
	for (int i = 0; i<13; ++i){//右下向
		if (col - 6 + i<0 || row - 6 + i<0)
			direction[2][i] = 'x';
		else if (col - 6 + i>14 || row - 6 + i>14)
			direction[2][i] = 'x';
		else if (m_pFive[row - 6 + i][col - 6 + i].getState() == -state)
			direction[2][i] = 'x';
		else if (m_pFive[row - 6 + i][col - 6 + i].getState() == state)
			direction[2][i] = 'o';
		else if (m_pFive[row - 6 + i][col - 6 + i].getState() == 0)
			direction[2][i] = '?';
	}
	for (int i = 0; i<13; ++i){//右上向
		if (col - 6 + i<0 || row + 6 - i>14)
			direction[3][i] = 'x';
		else if (col - 6 + i>14 || row + 6 - i<0)
			direction[3][i] = 'x';
		else if (m_pFive[row + 6 - i][col - 6 + i].getState() == -state)
			direction[3][i] = 'x';
		else if (m_pFive[row + 6 - i][col - 6 + i].getState() == state)
			direction[3][i] = 'o';
		else if (m_pFive[row + 6 - i][col - 6 + i].getState() == 0)
			direction[3][i] = '?';
	}

	int situationCount[STR_COUNT] = { 0 };
	int flag = 0;
	for (int i = 0; i < 4; ++i){
		for (int j = 0; j < STR_COUNT; ++j){
			flag = fastfind(fail[j], size_p[j], pats[j], 13, direction[i]);
			if (flag>0){
				situationCount[j] += flag;
				break;//只统计一次，已知特殊情况0 000 0无法统计两次
			}
			
		}
	}
	for (int j = 0; j < STR_COUNT; ++j){
		stepScore += situationCount[j] * evaluate[j];
	}
	if ((situationCount[STR_FOUR_BLANK_L_DEAD] + situationCount[STR_FOUR_BLANK_R_DEAD] +
		situationCount[STR_FOUR_CONTINUE_L] + situationCount[STR_FOUR_CONTINUE_R] + situationCount[STR_FOUR_BLANK_R]
		+ situationCount[STR_FOUR_BLANK_L] + situationCount[STR_FOUR_BLANK_M])>1){//双死四
		return 10000;
	}
	if ((situationCount[STR_THREE_CONTINUE] + situationCount[STR_THREE_BLANK_R] + situationCount[STR_THREE_BLANK_L]) >0){//三四
		if ((situationCount[STR_FOUR_BLANK_L_DEAD] + situationCount[STR_FOUR_BLANK_R_DEAD] + 
			situationCount[STR_FOUR_CONTINUE_L] + situationCount[STR_FOUR_CONTINUE_R] + situationCount[STR_FOUR_BLANK_R]
			+ situationCount[STR_FOUR_BLANK_L] + situationCount[STR_FOUR_BLANK_M])>0)
			return 10000;
	}
	if ((situationCount[STR_THREE_CONTINUE] + situationCount[STR_THREE_BLANK_R] + situationCount[STR_THREE_BLANK_L])>1){//双三
		return 8000;
	}
	return stepScore;

}


int ChessBoard::getStepScores(Piece thisStep)
{
	return getStepScores(thisStep.getRow(), thisStep.getCol(), thisStep.getState());
}

int ChessBoard::getStepScores()
{
	return getStepScores(getPiece());
}

//AI算法

// 00000


//end AI算法




bool ChessBoard::saveBoard(CString path)
{
	CFile oFile(path, CFile::
		modeCreate | CFile::modeWrite);
	CArchive oar(&oFile, CArchive::store);
	//读入m_pFive
	for (int i = 0; i < BOARD_ROW_MAX; i++)
	{
		for (int j = 0; j < BOARD_COL_MAX; j++)
		{
			oar << m_pFive[i][j].getState() << m_pFive[i][j].isFlag() << m_pFive[i][j].isFocus()
				<< m_pFive[i][j].isHot();
		}
	}
	//读入stepList
	int size = stepList.size();
	for (int i = 0; i < size; i++){
		oar<<stepList[i].step << stepList[i].uRow << stepList[i].uCol;
	}
	return true;
}

bool ChessBoard::loadBoard(CString path)
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
	bool Flag, Focus, Hot;
	//读入m_pFive
	for (int i = 0; i < BOARD_ROW_MAX; i++)
	{
		for (int j = 0; j < BOARD_COL_MAX; j++)
		{
			oar >> state >> Flag >> Focus>> Hot;
			m_pFive[i][j].setState(state);
			m_pFive[i][j].setFlag(Flag);
			m_pFive[i][j].setFocus(Focus);
			m_pFive[i][j].setHot(Hot);
		}
	}
	//读入stepList
	UINT step, uRow, uCol;	
	while (!oar.IsBufferEmpty())
	{
		oar >> step >> uRow >> uCol;
		stepList.push_back(STEP(step, uRow, uCol));
	}
	return true;
}
