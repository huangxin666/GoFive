#include "stdafx.h"
#include "ChessBoard.h"


ChessBoard::ChessBoard()
{
	stepList = new CList<STEP, STEP&>;
	searchArea = new CList<STEP, STEP&>;
	STEP temp;
	for (int i = 6; i < 9; i++){
		for (int j = 6; j < 9; j++){
			temp.uCol = j;
			temp.uRow = i;
			searchArea->AddTail(temp);
		}
	}
	for (int i = 0; i<15; ++i){
		for (int j = 0; j<15; ++j){
			m_pFive[i][j].uCol = j;
			m_pFive[i][j].uRow = i;
			m_pFive[i][j].uState = STATE_EMPTY;
			m_pFive[i][j].isFocus = false;
			m_pFive[i][j].isFlag = false;
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
	return getPiece(stepList->GetAt(currentStep));
}

int ChessBoard::stepListGetCount(){
	return stepList->GetCount();
}

void ChessBoard::stepListClear(){
	stepList->RemoveAll();
}

BOOL ChessBoard::stepListIsEmpty(){
	return stepList->IsEmpty();
}
POSITION ChessBoard::stepListAddTail(STEP step){
	return stepList->AddTail(step);
}
void ChessBoard::stepListRemoveTail(){
	stepList->RemoveTail();
}
void ChessBoard::stepListGetPrev(){
	stepList->GetPrev(currentStep);
}

STEP ChessBoard::stepListGetAt(POSITION p){
	return stepList->GetAt(p);
}

int ChessBoard::doNextStep(int row, int col, int side){
	if (m_pFive[row][col].uState != 0){
		return -1;//已有棋子
	}else{
		getPiece()->isFlag = false;
		STEP step;
		step.step = stepListGetCount() + 1;
		step.uCol = col;
		step.uRow = row;
		stepList->AddTail(step);
		m_pFive[row][col].uState = side;
		m_pFive[row][col].isFlag = true;
		return getStepScores(m_pFive[row][col]);
	}
}


int ChessBoard::getStepScores(Piece thisStep)
{
	int stepScore = 0;
	UINT row = thisStep.uCol;
	UINT col = thisStep.uRow;
	int state = thisStep.uState;
	int direction[4][9];//四个方向棋面（0表示空，-1表示断，1表示连）

	for (int i = 0; i<9; ++i){//横向
		if (col - 4 + i<0)
			direction[0][i] = -1;
		else if (col - 4 + i>14)
			direction[0][i] = -1;
		else if (m_pFive[col - 4 + i][row].uState == -state)
			direction[0][i] = -1;
		else if (m_pFive[col - 4 + i][row].uState == state)
			direction[0][i] = 1;
		else if (m_pFive[col - 4 + i][row].uState == 0)
			direction[0][i] = 0;
	}
	for (int i = 0; i<9; ++i){//纵向
		if (row - 4 + i<0)
			direction[1][i] = -1;
		else if (row - 4 + i>14)
			direction[1][i] = -1;
		else if (m_pFive[col][row - 4 + i].uState == -state)
			direction[1][i] = -1;
		else if (m_pFive[col][row - 4 + i].uState == state)
			direction[1][i] = 1;
		else if (m_pFive[col][row - 4 + i].uState == 0)
			direction[1][i] = 0;
	}
	for (int i = 0; i<9; ++i){//右下向
		if (col - 4 + i<0 || row - 4 + i<0)
			direction[2][i] = -1;
		else if (col - 4 + i>14 || row - 4 + i>14)
			direction[2][i] = -1;
		else if (m_pFive[col - 4 + i][row - 4 + i].uState == -state)
			direction[2][i] = -1;
		else if (m_pFive[col - 4 + i][row - 4 + i].uState == state)
			direction[2][i] = 1;
		else if (m_pFive[col - 4 + i][row - 4 + i].uState == 0)
			direction[2][i] = 0;
	}
	for (int i = 0; i<9; ++i){//左上向
		if (col - 4 + i<0 || row + 4 - i>14)
			direction[3][i] = -1;
		else if (col - 4 + i>14 || row + 4 - i<0)
			direction[3][i] = -1;
		else if (m_pFive[col - 4 + i][row + 4 - i].uState == -state)
			direction[3][i] = -1;
		else if (m_pFive[col - 4 + i][row + 4 - i].uState == state)
			direction[3][i] = 1;
		else if (m_pFive[col - 4 + i][row + 4 - i].uState == 0)
			direction[3][i] = 0;
	}

	int situationCount[12] = { 0 };
	for (int i = 0; i<4; ++i){//四个方向检测,分数代表AI的逻辑优先度
		if (isFiveContinue(direction[i])){
			situationCount[0]++;
			//stepScore+=100000;
		}
		else if (isFourContinue(direction[i])){
			situationCount[1]++;
			//stepScore+=10000;
		}
		else if (isFourContinueSide(direction[i])){
			situationCount[2]++;
			//stepScore+=1001;
		}
		else if (isFourContinueVar1(direction[i])){
			situationCount[3]++;
			//stepScore+=1000;
		}
		else if (isFourContinueVar2(direction[i])){
			situationCount[4]++;
			//stepScore+=1000;
		}
		else if (isThreeContinue(direction[i])){
			situationCount[5]++;
			//stepScore+=400;
		}
		else if (isThreeContinueVar(direction[i])){
			situationCount[6]++;
			//stepScore+=350;
		}
		else if (isThreeContinueSide(direction[i])){
			situationCount[7]++;
			//stepScore+=100;
		}
		else if (isThreeContinueVarSide(direction[i])){
			situationCount[8]++;
			//stepScore+=99;
		}
		else if (isTwoContinue(direction[i])){
			situationCount[9]++;
			//stepScore+=60;
		}
		else if (isTwoContinueVar(direction[i])){
			situationCount[10]++;
			//stepScore+=59;
		}
		else if (isDeadTwo(direction[i])){
			situationCount[11]++;
			//stepScore+=10;
		}
	}
	if ((situationCount[2] + situationCount[3] + situationCount[4])>1){
		return 9999;
	}
	if ((situationCount[2] + situationCount[3] + situationCount[4]) == 1){
		if ((situationCount[5] + situationCount[6])>0)
			return 9999;
	}
	if ((situationCount[5] + situationCount[6])>1){
		return 3500;
	}
	if ((situationCount[5] + situationCount[6]) == 1){
		if ((situationCount[7] + situationCount[8]) == 1)
			return 1100;
	}

	stepScore = situationCount[0] * 100000 + situationCount[1] * 10000 + situationCount[2] * 1001
		+ situationCount[3] * 1000 + situationCount[4] * 999 + situationCount[5] * 400 + situationCount[6] * 350
		+ situationCount[7] * 100 + situationCount[8] * 99 + situationCount[9] * 60 + situationCount[10] * 59 + situationCount[11] * 10;

	return stepScore;
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