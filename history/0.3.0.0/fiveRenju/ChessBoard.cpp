#include "stdafx.h"
#include "ChessBoard.h"

CString pats[STR_COUNT] = { _T("ooooo"), _T("?oooo?"),
							_T("?oooox"), _T("xoooo?"),
							_T("??ooo?o"), _T("o?ooo??"),
							_T("oo?oo"), _T("?ooo?"),
							_T("?oo?o?"), _T("?o?oo?"),
							_T("??ooox"), _T("xooo??"),
							_T("?o?oox"), _T("xo?oo?"),
							_T("?oo?ox"), _T("xoo?o?"),
							_T("?oo?"), _T("?o?o?"),
							_T("xooo?o"), _T("xo?ooo")};
int evaluate[STR_COUNT] = { 100000, 10000,
							1000, 1000,
							1030, 1030,
							1000, 1200,
							100, 100,
							30, 30,
							10, 10,
							10, 10,
							20, 20,
							999,999};

ChessBoard::ChessBoard()
{	
	nextStep = 0;
	stepList.reserve(225);
	for (int i = 0; i<15; ++i){
		for (int j = 0; j<15; ++j){
			m_pFive[i][j].uCol = j;
			m_pFive[i][j].uRow = i;
		}
	}
}


ChessBoard::~ChessBoard()
{

}

Piece * ChessBoard::getPiece(int row, int col){
	return &m_pFive[row][col];
}
Piece * ChessBoard::getPiece(STEP step){
	return &m_pFive[step.uRow][step.uCol];
}
Piece * ChessBoard::getPiece(){//无参数，返回当前棋子
	return &m_pFive[stepList.back().uRow][stepList.back().uCol];
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
			m_pFive[i][j].isHot = false;
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
				m_pFive[i][j].isHot = true;
			}
		}
	}
}

void ChessBoard::stepBack(int playerSide){
	if (stepList.size()>1){
		if (getPiece()->uState == playerSide){
			getPiece()->isFocus = false;
			getPiece()->isFlag = false;
			getPiece()->uState = 0;
			stepList.pop_back();
			nextStep--;
		}else{
			getPiece()->isFocus = false;
			getPiece()->isFlag = false;
			getPiece()->uState = 0;
			stepList.pop_back();
			nextStep--;
			getPiece()->isFocus = false;
			getPiece()->isFlag = false;
			getPiece()->uState = 0;
			stepList.pop_back();
			nextStep--;
		}
		if (!stepList.empty())
			getPiece()->isFlag = true;
	}
	resetHotArea();
}

bool ChessBoard::doNextStep(int row, int col, int side){
	if (m_pFive[row][col].uState != 0){
		return false;//已有棋子
	}else{
		if (!stepListIsEmpty())
			getPiece()->isFlag = false;
		STEP step;
		step.step = stepListGetCount() + 1;
		step.uCol = col;
		step.uRow = row;
		stepList.push_back(step);
		nextStep++;
		m_pFive[row][col].uState = side;
		m_pFive[row][col].isFlag = true;
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
				m_pFive[i][j].isHot = true;
			}
		}
		return true;
	}
}

int ChessBoard::getGlobalScore(int color){

	//横向
	int count[STR_COUNT] = { 0 };
	CString cols[BOARD_ROW_MAX];
	for (int i = 0; i < BOARD_ROW_MAX; ++i){
		for (int j = 0; j < BOARD_COL_MAX; ++j){
			if (m_pFive[i][j].uState == color)
				cols[i] += _T("o");
			else if (m_pFive[i][j].uState == -color)
				cols[i] += _T("x");
			else
				cols[i] += _T("?");
		}
	}
	//纵向
	CString rows[BOARD_COL_MAX];
	for (int j = 0; j < BOARD_ROW_MAX; ++j){
		for (int i = 0; i < BOARD_COL_MAX; ++i){
			if (m_pFive[i][j].uState == color)
				rows[j] += _T("o");
			else if (m_pFive[i][j].uState == -color)
				rows[j] += _T("x");
			else
				rows[j] += _T("?");
		}
	}
	for (int i = 0; i < BOARD_ROW_MAX; ++i){
		for (int j = 0; j < STR_COUNT; ++j){
			if (cols[i].Find(pats[j])>-1)
				count[j]++;
			if (rows[i].Find(pats[j])>-1)
				count[j]++;
		}
	}
	//右下
	CString rds[21];
	for (int k = 0; k < 11; k++){
		for (int i = k, j = 0; i < 15 && j < 15; i++, j++){
			if (m_pFive[i][j].uState == color)
				rds[k] += _T("o");
			else if (m_pFive[i][j].uState == -color)
				rds[k] += _T("x");
			else
				rds[k] += _T("?");
		}
	}
	for (int k = 1; k < 11; k++){
		for (int i = 0, j = k; i < 15 && j < 15; i++, j++){
			if (m_pFive[i][j].uState == color)
				rds[k + 10] += _T("o");
			else if (m_pFive[i][j].uState == -color)
				rds[k + 10] += _T("x");
			else
				rds[k + 10] += _T("?");
		}
	}
	//右上
	CString rus[21];
	for (int k = 0; k < 11; k++){
		for (int i = 14-k, j = 0; i >=0 && j < 15; i--, j++){
			if (m_pFive[i][j].uState == color)
				rus[k] += _T("o");
			else if (m_pFive[i][j].uState == -color)
				rus[k] += _T("x");
			else
				rus[k] += _T("?");
		}
	}
	for (int k = 1; k < 11; k++){
		for (int i = 14, j = k; i >=0 && j < 15; i--, j++){
			if (m_pFive[i][j].uState == color)
				rus[k + 10] += _T("o");
			else if (m_pFive[i][j].uState == -color)
				rus[k + 10] += _T("x");
			else
				rus[k + 10] += _T("?");
		}
	}
	for (int i = 0; i < 21; ++i){
		for (int j = 0; j < STR_COUNT; ++j){
			if (rds[i].Find(pats[j])>-1)
				count[j]++;
			if (rus[i].Find(pats[j])>-1)
				count[j]++;
		}
	}
	int score = 0;
	for (int j = 0; j < 18; ++j){
		score += count[j] * evaluate[j];
	}
	return score;
}

//int ChessBoard::getStepScores(int row, int col, int state){//row和col相反
//	int stepScore = 0;
//	int direction[4][9];//四个方向棋面（0表示空，-1表示断，1表示连）
//
//	for (int i = 0; i<9; ++i){//横向
//		if (col - 4 + i<0)
//			direction[0][i] = -1;
//		else if (col - 4 + i>14)
//			direction[0][i] = -1;
//		else if (m_pFive[row][col - 4 + i].uState == -state)
//			direction[0][i] = -1;
//		else if (m_pFive[row][col - 4 + i].uState == state)
//			direction[0][i] = 1;
//		else if (m_pFive[row][col - 4 + i].uState == 0)
//			direction[0][i] = 0;
//	}
//	for (int i = 0; i<9; ++i){//纵向
//		if (row - 4 + i<0)
//			direction[1][i] = -1;
//		else if (row - 4 + i>14)
//			direction[1][i] = -1;
//		else if (m_pFive[row - 4 + i][col].uState == -state)
//			direction[1][i] = -1;
//		else if (m_pFive[row - 4 + i][col].uState == state)
//			direction[1][i] = 1;
//		else if (m_pFive[row - 4 + i][col].uState == 0)
//			direction[1][i] = 0;
//	}
//	for (int i = 0; i<9; ++i){//右下向
//		if (col - 4 + i<0 || row - 4 + i<0)
//			direction[2][i] = -1;
//		else if (col - 4 + i>14 || row - 4 + i>14)
//			direction[2][i] = -1;
//		else if (m_pFive[row - 4 + i][col - 4 + i].uState == -state)
//			direction[2][i] = -1;
//		else if (m_pFive[row - 4 + i][col - 4 + i].uState == state)
//			direction[2][i] = 1;
//		else if (m_pFive[row - 4 + i][col - 4 + i].uState == 0)
//			direction[2][i] = 0;
//	}
//	for (int i = 0; i<9; ++i){//左上向
//		if (col - 4 + i<0 || row + 4 - i>14)
//			direction[3][i] = -1;
//		else if (col - 4 + i>14 || row + 4 - i<0)
//			direction[3][i] = -1;
//		else if (m_pFive[row + 4 - i][col - 4 + i].uState == -state)
//			direction[3][i] = -1;
//		else if (m_pFive[row + 4 - i][col - 4 + i].uState == state)
//			direction[3][i] = 1;
//		else if (m_pFive[row + 4 - i][col - 4 + i].uState == 0)
//			direction[3][i] = 0;
//	}
//
//	int situationCount[12] = { 0 };
//	for (int i = 0; i<4; ++i){//四个方向检测,分数代表AI的逻辑优先度
//		if (isFiveContinue(direction[i])){
//			situationCount[0]++;
//			//stepScore+=100000;
//		}
//		else if (isFourContinue(direction[i])){
//			situationCount[1]++;
//			//stepScore+=10000;
//		}
//		else if (isFourContinueSide(direction[i])){
//			situationCount[2]++;
//			//stepScore+=1001;
//		}
//		else if (isFourContinueVar1(direction[i])){
//			situationCount[3]++;
//			//stepScore+=1000;
//		}
//		else if (isFourContinueVar2(direction[i])){
//			situationCount[4]++;
//			//stepScore+=1000;
//		}
//		else if (isThreeContinue(direction[i])){
//			situationCount[5]++;
//			//stepScore+=400;
//		}
//		else if (isThreeContinueVar(direction[i])){
//			situationCount[6]++;
//			//stepScore+=350;
//		}
//		else if (isThreeContinueSide(direction[i])){
//			situationCount[7]++;
//			//stepScore+=100;
//		}
//		else if (isThreeContinueVarSide(direction[i])){
//			situationCount[8]++;
//			//stepScore+=99;
//		}
//		else if (isTwoContinue(direction[i])){
//			situationCount[9]++;
//			//stepScore+=60;
//		}
//		else if (isTwoContinueVar(direction[i])){
//			situationCount[10]++;
//			//stepScore+=59;
//		}
//		else if (isDeadTwo(direction[i])){
//			situationCount[11]++;
//			//stepScore+=10;
//		}
//	}
//	if ((situationCount[2] + situationCount[3] + situationCount[4])>1){
//		return 9999;
//	}
//	if ((situationCount[2] + situationCount[3] + situationCount[4]) == 1){
//		if ((situationCount[5] + situationCount[6])>0)
//			return 9999;
//	}
//	if ((situationCount[5] + situationCount[6])>1){
//		return 3500;
//	}
//	if ((situationCount[5] + situationCount[6]) == 1){
//		if ((situationCount[7] + situationCount[8]) == 1)
//			return 1100;
//	}
//
//	stepScore = situationCount[0] * 100000 + situationCount[1] * 10000 + situationCount[2] * 1001
//		+ situationCount[3] * 1000 + situationCount[4] * 999 + situationCount[5] * 400 + situationCount[6] * 350
//		+ situationCount[7] * 100 + situationCount[8] * 99 + situationCount[9] * 60 + situationCount[10] * 59 + situationCount[11] * 10;
//
//	return stepScore;
//
//}
int ChessBoard::getStepScores(int row, int col, int state){//row和col相反
	int stepScore = 0;
	CString direction[4];//四个方向棋面（0表示空，-1表示断，1表示连）

	for (int i = 0; i<9; ++i){//横向
		if (col - 4 + i<0 )
			direction[0] += _T("x");
		else if (col - 4 + i>14)
			direction[0] += _T("x");
		else if (m_pFive[row][col - 4 + i].uState == -state)
			direction[0] += _T("x");
		else if (m_pFive[row][col - 4 + i].uState == state)
			direction[0] += _T("o");
		else if (m_pFive[row][col - 4 + i].uState == 0)
			direction[0] += _T("?");
	}
	for (int i = 0; i<9; ++i){//纵向
		if (row - 4 + i<0)
			direction[1] += _T("x");
		else if (row - 4 + i>14)
			direction[1] += _T("x");
		else if (m_pFive[row - 4 + i][col].uState == -state)
			direction[1] += _T("x");
		else if (m_pFive[row - 4 + i][col].uState == state)
			direction[1] += _T("o");
		else if (m_pFive[row - 4 + i][col].uState == 0)
			direction[1] += _T("?");
	}
	for (int i = 0; i<9; ++i){//右下向
		if (col - 4 + i<0 || row - 4 + i<0)
			direction[2] += _T("x");
		else if (col - 4 + i>14 || row - 4 + i>14)
			direction[2] += _T("x");
		else if (m_pFive[row - 4 + i][col - 4 + i].uState == -state)
			direction[2] += _T("x");
		else if (m_pFive[row - 4 + i][col - 4 + i].uState == state)
			direction[2] += _T("o");
		else if (m_pFive[row - 4 + i][col - 4 + i].uState == 0)
			direction[2] += _T("?");
	}
	for (int i = 0; i<9; ++i){//左上向
		if (col - 4 + i<0 || row + 4 - i>14)
			direction[3]+= _T("x");
		else if (col - 4 + i>14 || row + 4 - i<0)
			direction[3]+= _T("x");
		else if (m_pFive[row + 4 - i][col - 4 + i].uState == -state)
			direction[3]+= _T("x");
		else if (m_pFive[row + 4 - i][col - 4 + i].uState == state)
			direction[3]+= _T("o");
		else if (m_pFive[row + 4 - i][col - 4 + i].uState == 0)
			direction[3]+= _T("?");
	}

	int situationCount[STR_COUNT] = { 0 };
	int flag = 0;
	for (int i = 0; i < 4; ++i){
		for (int j = 0; j < STR_COUNT; ++j){
			if (direction[i].Find(pats[j])>-1){
				/*if (j == STR_FOUR_BLANK_L || j == STR_FOUR_BLANK_R)
					flag = 1;
				if (flag == 1 && j == STR_THREE_CONTINUE)
					continue;*/
				situationCount[j]++;
				break;//只统计一次，已知特殊情况0 000 0无法统计两次
			}
		}
		//flag = 0;
	}
	for (int j = 0; j < 18; ++j){
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
	return getStepScores(thisStep.uRow, thisStep.uCol, thisStep.uState);
}


//AI算法

// 00000
bool ChessBoard::isFiveContinue(int direction[])
{
	int flag = 0;
	for (int i = 0; i<9; ++i){
		if (direction[i] == 1){
			flag += 1;
			if (flag == 5)
				return true;
		}
		else
			flag = 0;
	}
	return false;
}

// 0000 
bool ChessBoard::isFourContinue(int direction[])
{
	int flag = 0;
	for (int i = 0; i<9; ++i){
		if (direction[i] == 1){
			flag += 1;
			if (flag == 4){
				if (direction[i + 1] == 0 && direction[i - 4] == 0)
					return true;
			}
		}
		else{
			flag = 0;
		}
	}
	return false;
}

// 0000X||X0000
bool ChessBoard::isFourContinueSide(int direction[])
{
	int flag = 0;
	for (int i = 0; i<9; ++i){
		if (direction[i] == 1){
			flag += 1;
			if (flag == 4){
				if (direction[i + 1] == -1 && direction[i - 4] == 0)
					return true;
				else if (direction[i + 1] == 0 && direction[i - 4] == -1)
					return true;
			}
		}
		else{
			flag = 0;
		}
	}
	return false;
}

//  000 0||0 000
bool ChessBoard::isFourContinueVar1(int direction[])
{
	int flag = 0;
	for (int i = 0; i<9; ++i){
		if (direction[i] == 1){
			flag += 1;
			if (flag == 4){
				if (direction[i - 1] == 0 && direction[i - 2] == 1 && direction[i - 3] == 1 && direction[i - 4] == 1)
					return true;
				else if (direction[i - 1] == 1 && direction[i - 2] == 1 && direction[i - 3] == 0 && direction[i - 4] == 1)
					return true;
			}
		}
		else if (direction[i] == 0){

		}
		else if (direction[i] == -1){
			flag = 0;
		}
	}
	return false;
}

// 00 00
bool ChessBoard::isFourContinueVar2(int direction[])//有可能有问题
{
	int flag = 0;
	for (int i = 0; i<9; ++i){
		if (direction[i] == 1){
			flag += 1;
			if (flag == 4){
				if (direction[i - 1] == 1 && direction[i - 2] == 0 && direction[i - 3] == 1 && direction[i - 4] == 1)
					return true;
			}
		}
		else if (direction[i] == 0){

		}
		else if (direction[i] == -1){
			flag = 0;
		}
	}
	return false;
}

// 000
bool ChessBoard::isThreeContinue(int direction[])
{
	if (direction[3] == 1)//0o0
	if (direction[5] == 1)
	if (direction[2] == 0)
	if (direction[6] == 0)
		return true;
	if (direction[3] == 0)//o00
	if (direction[5] == 1)
	if (direction[6] == 1)
	if (direction[7] == 0)
		return true;
	if (direction[3] == 1)//00o
	if (direction[5] == 0)
	if (direction[2] == 1)
	if (direction[1] == 0)
		return true;
	return false;
}

// 000X||X000
bool ChessBoard::isThreeContinueSide(int direction[])
{
	if (direction[3] == 1){//0o0
		if (direction[5] == 1){
			if (direction[2] == 0){
				if (direction[6] == 1){
					if (direction[1] == 0)
						return true;
				}
			}
			else if (direction[2] == 1){
				if (direction[6] == 0){
					if (direction[7] == 0)
						return true;
				}
			}
		}
	}
	if (direction[5] == 1){//o00
		if (direction[6] == 1){
			if (direction[3] == 0){
				if (direction[7] == 1){
					if (direction[2] == 0)
						return true;
				}
			}
			else if (direction[3] == 1){
				if (direction[7] == 0){
					if (direction[8] == 0)
						return true;
				}
			}
		}
	}
	if (direction[3] == 1){//00o
		if (direction[2] == 1){
			if (direction[1] == 0){
				if (direction[5] == 1){
					if (direction[0] == 0)
						return true;
				}
			}
			else if (direction[1] == 1){
				if (direction[5] == 0){
					if (direction[6] == 0)
						return true;
				}
			}
		}
	}
	return false;
}

// 00 0||0 00
bool ChessBoard::isThreeContinueVar(int direction[])//有可能有问题
{
	int flag = 0;
	for (int i = 0; i<9; ++i){
		if (direction[i] == 1){
			flag += 1;
			if (flag == 3){
				if (direction[i + 1] == 0 && direction[i - 4] == 0)//有可能溢出
					return true;
				else
					return false;
			}
		}
		else if (direction[i] == 0){
			if (flag){
				if (direction[i - 1] == 1 && direction[i + 1] == 1){//有可能溢出
				}
				else{
					flag = 0;
				}
			}
		}
		else if (direction[i] == -1){
			flag = 0;
		}
	}
	return false;
}

// 0 00X || X0 00 || 00 0X || X00 0
bool ChessBoard::isThreeContinueVarSide(int direction[])
{
	int flag = 0;
	int startFlag = 0;
	for (int i = 0; i<9; ++i){
		if (direction[i] == 1){
			flag += 1;
		}
		else if (direction[i] == 0){
			if (flag == 1 && !startFlag)
				startFlag = 1;//0 00
			else if (flag == 2 && !startFlag)
				startFlag = 2;//00 0
			else{
				flag = 0;
				startFlag = 0;
			}
		}
		else if (direction[i] == -1){
			if (startFlag&&flag == 3)
				return true;
			flag = -1;
			startFlag = 0;
		}
	}
	flag = 0;
	startFlag = 0;
	for (int i = 8; i >= 0; --i){
		if (direction[i] == 1){
			flag += 1;
		}
		else if (direction[i] == 0){
			if (flag == 1 && !startFlag)
				startFlag = 1;//0 00
			else if (flag == 2 && !startFlag)
				startFlag = 2;//00 0
			else{
				flag = 0;
				startFlag = 0;
			}
		}
		else if (direction[i] == -1){
			if (startFlag&&flag == 3)
				return true;
			flag = -1;
			startFlag = 0;
		}
	}
	return false;
}

// 00
bool ChessBoard::isTwoContinue(int direction[])
{
	if (direction[5] == 1){//o0
		if (direction[6] == 0){
			if (direction[3] == 0){
				if (direction[2] == 0 || direction[7] == 0)
					return true;
			}
		}
	}
	if (direction[3] == 1){//0o
		if (direction[2] == 0){
			if (direction[5] == 0){
				if (direction[1] == 0 || direction[6] == 0)
					return true;
			}
		}
	}
	return false;
}

// 0 0
bool ChessBoard::isTwoContinueVar(int direction[])
{
	if (direction[5] == 0){//o 0
		if (direction[6] == 1){
			if (direction[3] == 0){
				if (direction[7] == 0)
					return true;
			}
		}
	}
	if (direction[3] == 0){//0 o
		if (direction[2] == 1){
			if (direction[5] == 0){
				if (direction[1] == 0)
					return true;
			}
		}
	}
	return false;
}

//00X || X00)
bool ChessBoard::isDeadTwo(int direction[])
{
	if (direction[3] == -1 && direction[5] == 1){
		if (direction[6] == 0 && direction[7] == 0 && direction[8] == 0)
			return true;
	}
	if (direction[2] == -1 && direction[3] == 1){
		if (direction[5] == 0 && direction[6] == 0 && direction[7] == 0)
			return true;
	}
	if (direction[5] == -1 && direction[3] == 1){
		if (direction[2] == 0 && direction[1] == 0 && direction[0] == 0)
			return true;
	}
	if (direction[5] == 1 && direction[6] == -1){
		if (direction[3] == 0 && direction[2] == 0 && direction[1] == 0)
			return true;
	}
	return false;
}

//end AI算法