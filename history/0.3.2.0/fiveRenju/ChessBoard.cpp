#include "stdafx.h"
#include "ChessBoard.h"

char pats[STR_COUNT][10] = { ("ooooo"), ("?oooo?"), ("?oooox"), ("xoooo?"),
("??ooo?o"), ("o?ooo??"), ("oo?oo"), ("?ooo?"),
("?oo?o?"), ("?o?oo?"), ("??ooox"), ("xooo??"),
("?o?oox"), ("xo?oo?"), ("?oo?ox"), ("xoo?o?"),
("?oo?"), ("?o?o?"), ("xooo?o"), ("o?ooox") };
int fail[STR_COUNT][10] = { { -1, 0, 1, 2, 3 }, { -1, -1, -1, -1, -1, 0 }, { -1, -1, -1, -1, -1, -1 }, { -1, -1, -1, -1, -1, -1 },
{ -1, 0, -1, -1, -1, 0, -1 }, { -1, -1, 0, 0, 0, 1, -1 }, { -1, 0, -1, 0, 1 }, { -1, -1, -1, -1, 0 },
{ -1, -1, -1, 0, 1, 0 }, { -1, -1, 0, 1, -1, 0 }, { -1, 0, -1, -1, -1, -1 }, { -1, -1, -1, -1, -1, -1 },
{ -1, -1, 0, 1, -1, -1 }, { -1, -1, -1, -1, -1, -1 }, { -1, -1, -1, 0, 1, -1 }, { -1, -1, -1, -1, -1, -1 },
{ -1, -1, -1, 0 }, { -1, -1, 0, 1, 2 }, { -1, -1, -1, -1, -1, -1 }, { -1, -1, 0, 0, 0, -1 } };
int size_p[STR_COUNT] = { 5, 6, 6, 6,
7, 7, 5, 5,
6, 6, 6, 6,
6, 6, 6, 6,
4, 5, 6, 6 };
int range[STR_COUNT] = {4,4,4,4,
6,6,4,3,
4,4,4,4,
4,4,4,4,
2,3,5,5};
int evaluate[STR_COUNT] = { 100000, 10000, 1000, 1000,
1030, 1030, 1000, 1200,
100, 100, 30, 30,
10, 10, 10, 10,
20, 18, 999, 999 };

int fastfind(int f[], int size_p, char p[], int size_o, char o[],int range)
{
	int i = 6-range, j = 0, sum = 0;
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
Piece &ChessBoard::getPiece(){//�޲��������ص�ǰ����
	return m_pFive[lastStep.uRow][lastStep.uCol];
}

void ChessBoard::setLastStep(STEP step)
{
	lastStep = step;
}

void ChessBoard::setThreat(int row, int col, int side,bool ban)
{
	int score = 0;
	m_pFive[row][col].setState(side);
	score = getStepScores(row, col, side,ban);
	m_pFive[row][col].setState(0);
	m_pFive[row][col].setThreat(score, side);
}

void ChessBoard::setGlobalThreat(int mode)
{
	bool ban;
	if (mode == 0)
		ban = false;
	else
		ban = true;
	for (int a = 0; a < BOARD_ROW_MAX; ++a){
		for (int b = 0; b < BOARD_COL_MAX; ++b){
			getPiece(a, b).clearThreat();
			if (m_pFive[a][b].isHot() && m_pFive[a][b].getState() == 0)
			{
				setThreat(a, b, 1,ban);
				setThreat(a, b, -1,ban);
			}
		}
	}
}

void ChessBoard::updateThreat()
{
	updateThreat(false);
}

void ChessBoard::updateThreat(bool ban)
{
	updateThreat(getPiece().getRow(), getPiece().getCol(),1,ban);
	updateThreat(getPiece().getRow(), getPiece().getCol(), -1,ban);
}

void ChessBoard::updateThreat(int side,bool ban)
{
	updateThreat(getPiece().getRow(), getPiece().getCol(), side,ban);
}

void ChessBoard::updateThreat(int row, int col,int side,bool ban)
{
	int range = UPDATETHREAT_SEARCH_MAX * 2 + 1;
	int tempcol, temprow;
	for (int i = 0; i < range; ++i)
	{
		//����
		tempcol = col - UPDATETHREAT_SEARCH_MAX + i;
		if (tempcol>-1 && tempcol<15 && m_pFive[row][tempcol].getState() == 0 && m_pFive[row][tempcol].isHot())
		{
			setThreat(row, tempcol, side,ban);
		}
		//����
		temprow = row - UPDATETHREAT_SEARCH_MAX + i;
		if (temprow>-1 && temprow<15 && m_pFive[temprow][col].getState() == 0 && m_pFive[temprow][col].isHot())
		{
			setThreat(temprow, col, side,ban);
		}
		//����
		tempcol = col - UPDATETHREAT_SEARCH_MAX + i;
		temprow = row - UPDATETHREAT_SEARCH_MAX + i;
		if (temprow>-1 && temprow<15 && tempcol>-1 && tempcol<15 &&
			m_pFive[temprow][tempcol].getState() == 0 && m_pFive[temprow][tempcol].isHot())
		{
			setThreat(temprow, tempcol, side,ban);
		}
		//����
		tempcol = col - UPDATETHREAT_SEARCH_MAX + i;
		temprow = row + UPDATETHREAT_SEARCH_MAX - i;
		if (temprow>-1 && temprow<15 && tempcol>-1 && tempcol < 15 &&
			m_pFive[temprow][tempcol].getState() == 0 && m_pFive[temprow][tempcol].isHot())
		{
			setThreat(temprow, tempcol, side,ban);
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
				for (int i = row - 2; i < row + 3; i++)
				{
					if (i<0)
						continue;
					else if (i>14)
						break;
					for (int j = col - 2; j < col + 3; j++)
					{
						if (j<0)
							continue;
						else if (j>14)
							break;
						m_pFive[i][j].setHot(true);
					}
				}
			}	
		}
	}	
}

bool ChessBoard::doNextStep(int row, int col, int side){
	if (m_pFive[row][col].getState() != 0)
	{
		return false;//��������
	}
	else
	{
		lastStep.uCol = col;
		lastStep.uRow = row;
		lastStep.step = lastStep.step + 1;
		m_pFive[row][col].setState(side);
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
		return true;
	}
}

//new
THREATINFO ChessBoard::getThreatInfo(int side)
{
	THREATINFO result(0,0);
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

//old
//THREATINFO ChessBoard::getThreatInfo(int color, int mode){
//	int score = 0;
//	int HighestScoreTemp = 0;
//	int totalScore = 0;
//	if (mode == 1)
//	{
//		for (int a = 0; a < BOARD_ROW_MAX; ++a){
//			for (int b = 0; b<BOARD_COL_MAX; ++b){
//				if (!m_pFive[a][b].isHot())
//					continue;
//				if (m_pFive[a][b].getState() == 0){
//					m_pFive[a][b].setState(color);
//					score = getStepScores(a, b, color);
//					//score = currentBoard.getGlobalScore(state);
//					totalScore += score;
//					if (score>HighestScoreTemp){
//						HighestScoreTemp = score;
//					}
//					m_pFive[a][b].setState(0);
//				}
//
//			}
//		}
//	}
//	else if (mode == 2)
//	{
//		ChessBoard chessboard_temp;
//		for (int a = 0; a < BOARD_ROW_MAX; ++a){
//			for (int b = 0; b < BOARD_COL_MAX; ++b){
//				if (!m_pFive[a][b].isHot())
//					continue;
//				if (m_pFive[a][b].getState() == 0){
//					m_pFive[a][b].setState(color);
//					score = getStepScores(a, b, color);
//					if (score<1031 && score>998){
//						m_pFive[a][b].setState(0);
//						chessboard_temp = *this;
//						chessboard_temp.doNextStep(a, b, color);
//						score = getDFS(chessboard_temp);
//					}
//					//score = currentBoard.getGlobalScore(state);
//					totalScore += score;
//					if (score > HighestScoreTemp){
//						HighestScoreTemp = score;
//					}
//					m_pFive[a][b].setState(0);
//				}
//
//			}
//		}
//	}
//	//for (int j = 0; j < 18; ++j){
//	//	score += count[j] * evaluate[j];
//	//}
//	THREATINFO info;
//	info.totalScore = totalScore;
//	info.HighestScore = HighestScoreTemp;
//	return info;
//}

//int ChessBoard::getDFS(ChessBoard cs){
//	int state = -cs.getPiece().getState();
//	ChessBoard cb_temp;
//	int highestScore = -500000;
//	for (int a = 0; a < BOARD_ROW_MAX; ++a){
//		for (int b = 0; b < BOARD_COL_MAX; ++b){
//			if (!m_pFive[a][b].isHot())
//				continue;
//			if (m_pFive[a][b].getState() == 0){
//				cb_temp = cs;
//				cb_temp.doNextStep(a, b, state);
//				THREATINFO info = cb_temp.getThreatInfo(-state, 1);
//				if (info.HighestScore < 10000){
//					info = cb_temp.getThreatInfo(-state, 2);
//				}
//				if (info.HighestScore>highestScore){
//					highestScore = info.HighestScore;
//				}
//			}
//		}
//	}
//	return highestScore;
//}

int ChessBoard::getStepScores(int row, int col, int state)
{
	return getStepScores(row, col, state,false);
}

int ChessBoard::getStepScores(int row, int col, int state, bool ban){
	int stepScore = 0;
	char direction[4][13];//�ĸ��������棨0��ʾ�գ�-1��ʾ�ϣ�1��ʾ����
	int temprow, tempcol;
	for (int i = 0; i < 13; ++i)
	{
		//����
		tempcol = col - 6 + i;
		if (tempcol<0)
			direction[0][i] = 'x';
		else if (tempcol>14)
			direction[0][i] = 'x';
		else if (m_pFive[row][tempcol].getState() == -state)
			direction[0][i] = 'x';
		else if (m_pFive[row][tempcol].getState() == state)
			direction[0][i] = 'o';
		else if (m_pFive[row][tempcol].getState() == 0)
			direction[0][i] = '?';
		//����
		temprow = row - 6 + i;
		if (temprow<0)
			direction[1][i] = 'x';
		else if (temprow>14)
			direction[1][i] = 'x';
		else if (m_pFive[temprow][col].getState() == -state)
			direction[1][i] = 'x';
		else if (m_pFive[temprow][col].getState() == state)
			direction[1][i] = 'o';
		else if (m_pFive[temprow][col].getState() == 0)
			direction[1][i] = '?';
		//������
		temprow = row - 6 + i;
		tempcol = col - 6 + i;
		if (tempcol<0 || temprow<0)
			direction[2][i] = 'x';
		else if (tempcol>14 || temprow>14)
			direction[2][i] = 'x';
		else if (m_pFive[temprow][tempcol].getState() == -state)
			direction[2][i] = 'x';
		else if (m_pFive[temprow][tempcol].getState() == state)
			direction[2][i] = 'o';
		else if (m_pFive[temprow][tempcol].getState() == 0)
			direction[2][i] = '?';
		//������
		temprow = row + 6 - i;
		tempcol = col - 6 + i;
		if (tempcol<0 || temprow>14)
			direction[3][i] = 'x';
		else if (tempcol > 14 || temprow < 0)
			direction[3][i] = 'x';
		else if (m_pFive[temprow][tempcol].getState() == -state)
			direction[3][i] = 'x';
		else if (m_pFive[temprow][tempcol].getState() == state)
			direction[3][i] = 'o';
		else if (m_pFive[temprow][tempcol].getState() == 0)
			direction[3][i] = '?';
	}

	int situationCount[STR_COUNT] = { 0 };
	int flag = 0;
	for (int i = 0; i < 4; ++i){
		for (int j = 0; j < STR_COUNT; ++j){
			flag = fastfind(fail[j], size_p[j], pats[j], 13, direction[i],range[j]);
			if (flag>0){
				situationCount[j] += flag;
				break;//ֻͳ��һ�Σ���֪�������0 000 0�޷�ͳ������
			}

		}
	}
	for (int j = 0; j < STR_COUNT; ++j){
		stepScore += situationCount[j] * evaluate[j];
	}
	if ((situationCount[STR_FOUR_BLANK_L_DEAD] + situationCount[STR_FOUR_BLANK_R_DEAD] +
		situationCount[STR_FOUR_CONTINUE_L] + situationCount[STR_FOUR_CONTINUE_R] + situationCount[STR_FOUR_BLANK_R]
		+ situationCount[STR_FOUR_BLANK_L] + situationCount[STR_FOUR_BLANK_M])>1){//˫����
		return 10000;
	}
	if ((situationCount[STR_THREE_CONTINUE] + situationCount[STR_THREE_BLANK_R] + situationCount[STR_THREE_BLANK_L]) > 0){//����
		if ((situationCount[STR_FOUR_BLANK_L_DEAD] + situationCount[STR_FOUR_BLANK_R_DEAD] +
			situationCount[STR_FOUR_CONTINUE_L] + situationCount[STR_FOUR_CONTINUE_R] + situationCount[STR_FOUR_BLANK_R]
			+ situationCount[STR_FOUR_BLANK_L] + situationCount[STR_FOUR_BLANK_M]) > 0)
			return 10000;
	}
	if ((situationCount[STR_THREE_CONTINUE] + situationCount[STR_THREE_BLANK_R] + situationCount[STR_THREE_BLANK_L]) > 1){//˫��
		if (ban&&state == STATE_CHESS_BLACK)//�н���
			return -1;
		else
			return 8000;
	}
	return stepScore;

}


//int ChessBoard::getStepScores(Piece thisStep)
//{
//	return getStepScores(thisStep.getRow(), thisStep.getCol(), thisStep.getState());
//}

int ChessBoard::getStepScores(Piece thisStep)
{
	return getStepScores(thisStep.getRow(), thisStep.getCol(), thisStep.getState());
}

int ChessBoard::getStepScores()
{
	return getStepScores(getPiece());
}

//AI�㷨

// 00000


//end AI�㷨
