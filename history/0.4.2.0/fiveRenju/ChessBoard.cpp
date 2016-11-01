#include "stdafx.h"
#include "ChessBoard.h"

char pats[STR_COUNT][10] = { ("oooooo"), ("ooooo"), ("?oooo?"), ("?oooox"),
("o?ooo??"), ("ooo?o"), ("oo?oo"), ("?ooo??"), ("?ooo?"),
("?o?oo?"), ("??ooox"), ("?o?oox"), ("?oo?ox"),
("?oo?"), ("?o?o?") };

int fail[STR_COUNT][10] = { { -1, 0, 1, 2, 3, 4 }, { -1, 0, 1, 2, 3 }, { -1, -1, -1, -1, -1, 0 }, { -1, -1, -1, -1, -1, -1 },
{ -1, -1, 0, 0, 0, 1, -1 }, { -1, 0, 1, -1, 0 }, { -1, 0, -1, 0, 1 }, { -1, -1, -1, -1, 0, 0 }, { -1, -1, -1, -1, 0 },
{ -1, -1, 0, 1, -1, 0 }, { -1, 0, -1, -1, -1, -1 }, { -1, -1, 0, 1, -1, -1 }, { -1, -1, -1, 0, 1, -1 },
{ -1, -1, -1, 0 }, { -1, -1, 0, 1, 2 } };

int size_p[STR_COUNT] = { 6, 5, 6, 6,
7, 5, 5, 6,5,
6, 6, 6, 6,
4, 5 };

int range[STR_COUNT] = { 5, 4, 4, 4,
6, 4, 4, 4,3,
4, 4, 4, 4,
2, 3 };

bool isReverse[STR_COUNT] = { false, false, false, true,
true, true, false, true,false,
true, true, true, true,
false, false };

int evaluate[STR_COUNT] = { -1, 100000, 12000, 1000,
1230, 960, 960, 1200, 30,
800, 20, 5, 10,
50, 45 };
int evaluate_defend[STR_COUNT] = {
	-1, 100000, 12000, 1000,
	1030, 999, 999, 1200, 30,
	100, 20, 5, 10,
	10, 5
};

int fastfind(int f[], int size_p, char p[], int size_o, char o[], int range)
{
	int i = 6 - range, j = 0, sum = 0;
	size_o = size_o - 6 + range;
	if (size_p == 0)
		return sum;
	while (i < size_o){
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
		}
	}
	return sum;
}

ChessBoard::ChessBoard()
{
	for (int i = 0; i < 15; ++i)
	{
		for (int j = 0; j < 15; ++j)
		{
			m_pFive[i][j].setXY(i, j);
		}
	}
	lastStep.step = 0;
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
	return m_pFive[lastStep.uRow][lastStep.uCol];
}

void ChessBoard::setLastStep(STEP step)
{
	lastStep = step;
}

STEP ChessBoard::getLastStep()
{
	return lastStep;
}

void ChessBoard::setThreat(int row, int col, int side, bool ban)
{
	int score = 0;
	m_pFive[row][col].setState(side);
	score = getStepScores(row, col, side, ban,true);
	m_pFive[row][col].setState(0);
	m_pFive[row][col].setThreat(score, side);
}

void ChessBoard::setGlobalThreat(bool ban)
{
	for (int a = 0; a < BOARD_ROW_MAX; ++a){
		for (int b = 0; b < BOARD_COL_MAX; ++b){
			getPiece(a, b).clearThreat();
			if (m_pFive[a][b].isHot() && m_pFive[a][b].getState() == 0)
			{
				setThreat(a, b, 1, ban);
				setThreat(a, b, -1, ban);
			}
		}
	}
}

void ChessBoard::updateThreat(bool ban,int side)
{
	if (side == 0)
	{
		updateThreat(getPiece().getRow(), getPiece().getCol(), 1, ban);
		updateThreat(getPiece().getRow(), getPiece().getCol(), -1, ban);
	}
	else
		updateThreat(getPiece().getRow(), getPiece().getCol(),side, ban);
}

void ChessBoard::updateThreat(int row, int col, int side, bool ban)
{
	int range = UPDATETHREAT_SEARCH_MAX * 2 + 1;
	int tempcol, temprow;
	for (int i = 0; i < range; ++i)
	{
		//横向
		tempcol = col - UPDATETHREAT_SEARCH_MAX + i;
		if (tempcol>-1 && tempcol<15 && m_pFive[row][tempcol].getState() == 0 && m_pFive[row][tempcol].isHot())
		{
			setThreat(row, tempcol, side, ban);
		}
		//纵向
		temprow = row - UPDATETHREAT_SEARCH_MAX + i;
		if (temprow>-1 && temprow<15 && m_pFive[temprow][col].getState() == 0 && m_pFive[temprow][col].isHot())
		{
			setThreat(temprow, col, side, ban);
		}
		//右下
		tempcol = col - UPDATETHREAT_SEARCH_MAX + i;
		temprow = row - UPDATETHREAT_SEARCH_MAX + i;
		if (temprow>-1 && temprow<15 && tempcol>-1 && tempcol<15 &&
			m_pFive[temprow][tempcol].getState() == 0 && m_pFive[temprow][tempcol].isHot())
		{
			setThreat(temprow, tempcol, side, ban);
		}
		//右上
		tempcol = col - UPDATETHREAT_SEARCH_MAX + i;
		temprow = row + UPDATETHREAT_SEARCH_MAX - i;
		if (temprow>-1 && temprow<15 && tempcol>-1 && tempcol < 15 &&
			m_pFive[temprow][tempcol].getState() == 0 && m_pFive[temprow][tempcol].isHot())
		{
			setThreat(temprow, tempcol, side, ban);
		}
	}
}

void ChessBoard::updateHotArea(int row, int col)
{
	int range = 2 * 2 + 1;
	int tempcol, temprow;
	for (int i = 0; i < range; ++i)
	{
		//横向
		tempcol = col - 2 + i;
		if (tempcol>-1 && tempcol<15 && m_pFive[row][tempcol].getState() == 0)
		{
			m_pFive[row][tempcol].setHot(true);
		}
		//纵向
		temprow = row - 2 + i;
		if (temprow>-1 && temprow<15 && m_pFive[temprow][col].getState() == 0)
		{
			m_pFive[temprow][col].setHot(true);
		}
		//右下
		if (temprow>-1 && temprow<15 && tempcol>-1 && tempcol<15 &&
			m_pFive[temprow][tempcol].getState() == 0)
		{
			m_pFive[temprow][tempcol].setHot(true);
		}
		//右上
		temprow = row + 2 - i;
		if (temprow>-1 && temprow<15 && tempcol>-1 && tempcol < 15 &&
			m_pFive[temprow][tempcol].getState() == 0)
		{
			m_pFive[temprow][tempcol].setHot(true);
		}
	}
}

void ChessBoard::resetHotArea(){
	for (int i = 0; i < 15; ++i){
		for (int j = 0; j < 15; ++j){
			m_pFive[i][j].setHot(false);
		}
	}
	for (int row = 0; row < BOARD_ROW_MAX; ++row)
	{
		for (int col = 0; col < BOARD_COL_MAX; ++col)
		{
			if (m_pFive[row][col].getState() != 0)
			{
				updateHotArea(row, col);
			}
		}
	}
}

bool ChessBoard::doNextStep(int row, int col, int side){
	if (m_pFive[row][col].getState() != 0)
	{
		return false;//已有棋子
	}
	else
	{
		lastStep.uCol = col;
		lastStep.uRow = row;
		lastStep.step = lastStep.step + 1;
		m_pFive[row][col].setState(side);
		updateHotArea(row, col);
		return true;
	}
}

//new
THREATINFO ChessBoard::getThreatInfo(int side)
{
	THREATINFO result(0, 0);
	for (int i = 0; i < BOARD_ROW_MAX; ++i)
	{
		for (int j = 0; j < BOARD_COL_MAX; ++j)
		{
			if (getPiece(i, j).isHot() && getPiece(i, j).getState() == 0)
			{
				result.totalScore += getPiece(i, j).getThreat(side);
				if (getPiece(i, j).getThreat(side)>result.HighestScore)
					result.HighestScore = getPiece(i, j).getThreat(side);
			}
		}
	}
	return result;
}

int ChessBoard::getStepScores(int row, int col, int state, bool ban)
{
	return getStepScores(row, col, state, ban, true);
}

int ChessBoard::getStepScores(int row, int col, int state, bool ban,bool isdefend){
	int stepScore = 0;
	char direction[2][4][13];//四个方向棋面（0表示空，-1表示断，1表示连）
	int temprow, tempcol, reverse;
	for (int i = 0; i < 13; ++i)
	{
		reverse = 12 - i;
		//横向
		tempcol = col - 6 + i;
		if (tempcol<0 || tempcol>14 || m_pFive[row][tempcol].getState() == -state)
		{
			direction[0][0][i] = 'x'; direction[1][0][reverse] = 'x';
		}
		else if (m_pFive[row][tempcol].getState() == state)
		{
			direction[0][0][i] = 'o'; direction[1][0][reverse] = 'o';
		}
		else if (m_pFive[row][tempcol].getState() == 0)
		{
			direction[0][0][i] = '?'; direction[1][0][reverse] = '?';
		}
		//纵向
		temprow = row - 6 + i;
		if (temprow<0 || temprow>14 || m_pFive[temprow][col].getState() == -state)
		{
			direction[0][1][i] = 'x'; direction[1][1][reverse] = 'x';
		}
		else if (m_pFive[temprow][col].getState() == state)
		{
			direction[0][1][i] = 'o'; direction[1][1][reverse] = 'o';
		}
		else if (m_pFive[temprow][col].getState() == 0)
		{
			direction[0][1][i] = '?'; direction[1][1][reverse] = '?';
		}
		//右下向
		if (tempcol<0 || temprow<0 || tempcol>14 || temprow>14 || m_pFive[temprow][tempcol].getState() == -state)
		{
			direction[0][2][i] = 'x'; direction[1][2][reverse] = 'x';
		}
		else if (m_pFive[temprow][tempcol].getState() == state)
		{
			direction[0][2][i] = 'o'; direction[1][2][reverse] = 'o';
		}
		else if (m_pFive[temprow][tempcol].getState() == 0)
		{
			direction[0][2][i] = '?'; direction[1][2][reverse] = '?';
		}
		//右上向
		temprow = row + 6 - i;
		if (tempcol<0 || temprow>14 || tempcol > 14 || temprow < 0 || m_pFive[temprow][tempcol].getState() == -state)
		{
			direction[0][3][i] = 'x'; direction[1][3][reverse] = 'x';
		}
		else if (m_pFive[temprow][tempcol].getState() == state)
		{
			direction[0][3][i] = 'o'; direction[1][3][reverse] = 'o';
		}
		else if (m_pFive[temprow][tempcol].getState() == 0)
		{
			direction[0][3][i] = '?'; direction[1][3][reverse] = '?';
		}
	}

	int situationCount[STR_COUNT] = { 0 };
	int flag = 0;
	for (int i = 0; i < 4; ++i){
		for (int j = 0; j < STR_COUNT; ++j){
			flag = fastfind(fail[j], size_p[j], pats[j], 13, direction[0][i], range[j]);
			if ((flag == 0 && isReverse[j]) || j == STR_FOUR_BLANK_DEAD)
				flag += fastfind(fail[j], size_p[j], pats[j], 13, direction[1][i], range[j]);
			if (flag>0){
				situationCount[j] += flag;
				break;//只统计一次
			}
		}
	}
	
	if (0 == situationCount[STR_FIVE_CONTINUE])
	{
		if (situationCount[STR_SIX_CONTINUE]>0)//长连
		{
			if (ban&&state == STATE_CHESS_BLACK)//有禁手
				return -3;
			else
			{
				stepScore += situationCount[STR_SIX_CONTINUE] * 100000;
			}
		}

		int deadFour = situationCount[STR_FOUR_CONTINUE_DEAD] + situationCount[STR_FOUR_BLANK] + situationCount[STR_FOUR_BLANK_DEAD] + situationCount[STR_FOUR_BLANK_M];

		if (deadFour + situationCount[STR_FOUR_CONTINUE] > 1)//双四
		{
			if (ban&&state == STATE_CHESS_BLACK)//有禁手
				return -2;
		}

		if (deadFour > 1){//双死四
			stepScore += 10000;
			deadFour = 0;
			situationCount[STR_FOUR_CONTINUE_DEAD] = 0;
			situationCount[STR_FOUR_BLANK] = 0;
			situationCount[STR_FOUR_BLANK_DEAD] = 0;
			situationCount[STR_FOUR_BLANK_M] = 0;
		}

		int aliveThree = situationCount[STR_THREE_CONTINUE] + situationCount[STR_THREE_BLANK];

		if (aliveThree == 1 && deadFour == 1){ //死四活三
			stepScore += 10000;
			deadFour = 0;
			aliveThree = 0;
			situationCount[STR_FOUR_CONTINUE_DEAD] = 0;
			situationCount[STR_FOUR_BLANK] = 0;
			situationCount[STR_FOUR_BLANK_DEAD] = 0;
			situationCount[STR_FOUR_BLANK_M] = 0;
			situationCount[STR_THREE_CONTINUE] = 0;
			situationCount[STR_THREE_BLANK] = 0;
		}

		if (aliveThree > 1){//双活三
			if (ban&&state == STATE_CHESS_BLACK)//有禁手
				return -1;
			else
				stepScore += 8000;
			aliveThree = 0;
			situationCount[STR_THREE_CONTINUE] = 0;
			situationCount[STR_THREE_BLANK] = 0;
		}
	}

	if (isdefend)
		for (int j = 1; j < STR_COUNT; ++j){//从1开始 长连珠特殊处理
			stepScore += situationCount[j] * evaluate_defend[j];
		}
	else
		for (int j = 1; j < STR_COUNT; ++j){//从1开始 长连珠特殊处理
			stepScore += situationCount[j] * evaluate[j];
		}

	return stepScore;
}

int ChessBoard::getStepScores(bool ban,bool isdefend)
{
	return getStepScores(lastStep.uRow, lastStep.uCol, m_pFive[lastStep.uRow][lastStep.uCol].getState(), ban, isdefend);
}

//AI算法

// 00000


//end AI算法
