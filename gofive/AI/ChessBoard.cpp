#include "ChessBoard.h"
#include <string>
using namespace std;

string pats[STR_COUNT] = { ("oooooo"), ("ooooo"), ("?oooo?"), ("?oooox"),
("o?ooo??"), ("ooo?o"), ("oo?oo"), ("?ooo??"),
("?o?oo?"), ("?ooo?"), ("??ooox"), ("?o?oox"), ("?oo?ox"),
("?oo?"), ("?o?o?") };

string pats_check[STR_COUNT] = { ("oooooo"), ("ooooo"), ("o?oooo?"), ("o?oooox"),
("oo?ooo??"), ("oooo?o"), ("ooo?oo"), ("?ooo??"),
("?o?oo?"), ("?ooo?"), ("??ooox"), ("?o?oox"), ("?oo?ox"),
("?oo?"), ("?o?o?") };

int fail[STR_COUNT][10] = { { -1, 0, 1, 2, 3, 4 }, { -1, 0, 1, 2, 3 }, { -1, -1, -1, -1, -1, 0 }, { -1, -1, -1, -1, -1, -1 },
{ -1, -1, 0, 0, 0, 1, -1 }, { -1, 0, 1, -1, 0 }, { -1, 0, -1, 0, 1 }, { -1, -1, -1, -1, 0, 0 },
{ -1, -1, 0, 1, -1, 0 }, { -1, -1, -1, -1, 0 }, { -1, 0, -1, -1, -1, -1 }, { -1, -1, 0, 1, -1, -1 }, { -1, -1, -1, 0, 1, -1 },
{ -1, -1, -1, 0 }, { -1, -1, 0, 1, 2 } };

int fail_check[STR_COUNT][10] = { { -1, 0, 1, 2, 3, 4 }, { -1, 0, 1, 2, 3 }, { -1, -1, 0, 0, 0, 0, 1 }, { -1, -1, 0, 0, 0, 0, -1 },
{ -1, 0, -1, 0, 1, 1, 2,-1 }, { -1, 0, 1, 2, -1,0 }, { -1, 0, 1, -1, 0,1 }, { -1, -1, -1, -1, 0, 0 },
{ -1, -1, 0, 1, -1, 0 }, { -1, -1, -1, -1, 0 }, { -1, 0, -1, -1, -1, -1 }, { -1, -1, 0, 1, -1, -1 }, { -1, -1, -1, 0, 1, -1 },
{ -1, -1, -1, 0 }, { -1, -1, 0, 1, 2 } };

int range[STR_COUNT] = { 5, 4, 4, 4,
6, 4, 4, 4,
4, 3, 4, 4, 4,
2, 3 };

int range_check[STR_COUNT] = { 5, 4, 6, 6,
7, 5, 5, 4,
4, 3, 4, 4, 4,
2, 3 };

bool isReverse[STR_COUNT] = { false, false, false, true,
true, true, true, true,
true, false, true, true, true,
false, false };

int evaluate[STR_COUNT] = {
    -1, 100000, 12000, 1211,
    1300, 1210, 1210, 1100,
    1080, 20, 20, 5, 10,
    35, 30
};

int evaluate_defend[STR_COUNT] = {
    -1, 100000, 12000, 1000,
    1030, 999, 999, 1200,
    100, 20, 20, 5, 10,
    10, 5
};

ChessBoard::ChessBoard()
{
    lastStep.step = 0;
}


ChessBoard::~ChessBoard()
{

}



void ChessBoard::setThreat(int row, int col, int side, bool ban)
{
    int score = 0;
    pieces[row][col].setState(side);
    score = getStepScores(row, col, side, ban, true);
    pieces[row][col].setState(0);
    pieces[row][col].setThreat(score, side);
}

void ChessBoard::setGlobalThreat(bool ban)
{
    for (int a = 0; a < BOARD_ROW_MAX; ++a) {
        for (int b = 0; b < BOARD_COL_MAX; ++b) {
            getPiece(a, b).clearThreat();
            if (pieces[a][b].isHot() && pieces[a][b].getState() == 0)
            {
                setThreat(a, b, 1, ban);
                setThreat(a, b, -1, ban);
            }
        }
    }
}

void ChessBoard::updateThreat(bool ban, int side)
{
    if (side == 0)
    {
        updateThreat(lastStep.uRow, lastStep.uCol, 1, ban);
        updateThreat(lastStep.uRow, lastStep.uCol, -1, ban);
    }
    else
    {
        updateThreat(lastStep.uRow, lastStep.uCol, side, ban);
    }
}

void ChessBoard::updateThreat(int row, int col, int side, bool ban)
{
    int range = UPDATETHREAT_SEARCH_MAX * 2 + 1;
    int tempcol, temprow;
    for (int i = 0; i < range; ++i)
    {
        //横向
        tempcol = col - UPDATETHREAT_SEARCH_MAX + i;
        if (tempcol > -1 && tempcol < 15 && pieces[row][tempcol].getState() == 0 && pieces[row][tempcol].isHot())
        {
            setThreat(row, tempcol, side, ban);
        }
        //纵向
        temprow = row - UPDATETHREAT_SEARCH_MAX + i;
        if (temprow > -1 && temprow < 15 && pieces[temprow][col].getState() == 0 && pieces[temprow][col].isHot())
        {
            setThreat(temprow, col, side, ban);
        }
        //右下
        tempcol = col - UPDATETHREAT_SEARCH_MAX + i;
        temprow = row - UPDATETHREAT_SEARCH_MAX + i;
        if (temprow > -1 && temprow<15 && tempcol>-1 && tempcol < 15 &&
            pieces[temprow][tempcol].getState() == 0 && pieces[temprow][tempcol].isHot())
        {
            setThreat(temprow, tempcol, side, ban);
        }
        //右上
        tempcol = col - UPDATETHREAT_SEARCH_MAX + i;
        temprow = row + UPDATETHREAT_SEARCH_MAX - i;
        if (temprow > -1 && temprow<15 && tempcol>-1 && tempcol < 15 &&
            pieces[temprow][tempcol].getState() == 0 && pieces[temprow][tempcol].isHot())
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
        if (tempcol > -1 && tempcol < 15 && pieces[row][tempcol].getState() == 0)
        {
            pieces[row][tempcol].setHot(true);
        }
        //纵向
        temprow = row - 2 + i;
        if (temprow > -1 && temprow < 15 && pieces[temprow][col].getState() == 0)
        {
            pieces[temprow][col].setHot(true);
        }
        //右下
        if (temprow > -1 && temprow<15 && tempcol>-1 && tempcol < 15 &&
            pieces[temprow][tempcol].getState() == 0)
        {
            pieces[temprow][tempcol].setHot(true);
        }
        //右上
        temprow = row + 2 - i;
        if (temprow > -1 && temprow<15 && tempcol>-1 && tempcol < 15 &&
            pieces[temprow][tempcol].getState() == 0)
        {
            pieces[temprow][tempcol].setHot(true);
        }
    }
}

void ChessBoard::resetHotArea() {
    for (int i = 0; i < 15; ++i) {
        for (int j = 0; j < 15; ++j) {
            pieces[i][j].setHot(false);
        }
    }
    for (int row = 0; row < BOARD_ROW_MAX; ++row)
    {
        for (int col = 0; col < BOARD_COL_MAX; ++col)
        {
            if (pieces[row][col].getState() != 0)
            {
                updateHotArea(row, col);
            }
        }
    }
}

bool ChessBoard::doNextStep(int row, int col, int side) {
    if (pieces[row][col].getState() != 0)
    {
        return false;//已有棋子
    }
    else
    {
        lastStep.uCol = col;
        lastStep.uRow = row;
        lastStep.step = lastStep.step + 1;
        lastStep.setColor(side);
        pieces[row][col].setState(side);
        updateHotArea(row, col);
        return true;
    }
}

//new
ThreatInfo ChessBoard::getThreatInfo(int side)
{
    ThreatInfo result = { 0, 0 };
    for (int i = 0; i < BOARD_ROW_MAX; ++i)
    {
        for (int j = 0; j < BOARD_COL_MAX; ++j)
        {
            if (getPiece(i, j).isHot() && getPiece(i, j).getState() == 0)
            {
                result.totalScore += getPiece(i, j).getThreat(side);
                if (getPiece(i, j).getThreat(side) > result.HighestScore)
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

void ChessBoard::formatChess2String(char chessStr[][FORMAT_LENGTH], int row, int col, int state, bool reverse)
{
    int temprow, tempcol;
    int index;
    for (int i = 0; i < FORMAT_LENGTH; i++)
    {
        if (reverse)
        {
            index = FORMAT_LENGTH - 1 - i;
        }
        else
        {
            index = i;
        }
        //横向
        tempcol = col - SEARCH_LENGTH + i;
        if (tempcol < 0 || tempcol > 14 || pieces[row][tempcol].getState() == -state)
        {
            chessStr[0][index] = 'x';
        }
        else if (pieces[row][tempcol].getState() == state)
        {
            chessStr[0][index] = 'o';
        }
        else if (pieces[row][tempcol].getState() == 0)
        {
            chessStr[0][index] = '?';
        }
        //纵向
        temprow = row - SEARCH_LENGTH + i;
        if (temprow < 0 || temprow > 14 || pieces[temprow][col].getState() == -state)
        {
            chessStr[1][index] = 'x';
        }
        else if (pieces[temprow][col].getState() == state)
        {
            chessStr[1][index] = 'o';
        }
        else if (pieces[temprow][col].getState() == 0)
        {
            chessStr[1][index] = '?';
        }
        //右下向
        if (tempcol < 0 || temprow < 0 || tempcol > 14 || temprow > 14 || pieces[temprow][tempcol].getState() == -state)
        {
            chessStr[2][index] = 'x';
        }
        else if (pieces[temprow][tempcol].getState() == state)
        {
            chessStr[2][index] = 'o';
        }
        else if (pieces[temprow][tempcol].getState() == 0)
        {
            chessStr[2][index] = '?';
        }
        //右上向
        temprow = row + SEARCH_LENGTH - i;
        if (tempcol < 0 || temprow > 14 || tempcol > 14 || temprow < 0 || pieces[temprow][tempcol].getState() == -state)
        {
            chessStr[3][index] = 'x';
        }
        else if (pieces[temprow][tempcol].getState() == state)
        {
            chessStr[3][index] = 'o';
        }
        else if (pieces[temprow][tempcol].getState() == 0)
        {
            chessStr[3][index] = '?';
        }
    }
}

int ChessBoard::getStepScores(int row, int col, int state, bool ban, bool isdefend) {
    int stepScore = 0;
    char direction[2][4][FORMAT_LENGTH];//四个方向棋面（0表示空，-1表示断，1表示连）
    formatChess2String(direction[0], row, col, state);
    formatChess2String(direction[1], row, col, state, true);

    int situationCount[STR_COUNT] = { 0 };
    int flag = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < STR_COUNT; ++j) {
            flag = fastfind(fail[j], pats[j], FORMAT_LENGTH, direction[0][i], range[j]);
            if ((isReverse[j] && flag == 0) || j == STR_4_BLANK_DEAD)
                flag += fastfind(fail[j], pats[j], FORMAT_LENGTH, direction[1][i], range[j]);
            if (flag > 0) {
                situationCount[j] += flag;
                break;//只统计一次
            }
        }
    }

    if (ban&&state == STATE_CHESS_BLACK)//检测禁手棋型
    {
        if (situationCount[STR_4_CONTINUE] > 0)
        {
            for (int i = 0; i < 4; ++i)
            {
                flag = fastfind(fail_check[STR_4_CONTINUE], pats_check[STR_4_CONTINUE], FORMAT_LENGTH, direction[0][i], range_check[STR_4_CONTINUE]);
                if (flag == 0)
                    flag += fastfind(fail_check[STR_4_CONTINUE], pats_check[STR_4_CONTINUE], FORMAT_LENGTH, direction[1][i], range_check[STR_4_CONTINUE]);
                if (flag > 0) {
                    situationCount[STR_4_CONTINUE] -= flag;
                    situationCount[STR_4_CONTINUE_DEAD] += flag;
                }
            }
        }
        else if (situationCount[STR_4_CONTINUE_DEAD] > 0)
        {
            for (int i = 0; i < 4; ++i)
            {
                flag = fastfind(fail_check[STR_4_BLANK_DEAD], pats_check[STR_4_BLANK_DEAD], FORMAT_LENGTH, direction[0][i], range_check[STR_4_BLANK_DEAD]);
                if (flag == 0)
                    flag += fastfind(fail_check[STR_4_BLANK_DEAD], pats_check[STR_4_BLANK_DEAD], FORMAT_LENGTH, direction[1][i], range_check[STR_4_BLANK_DEAD]);
                if (flag > 0) {
                    situationCount[STR_4_CONTINUE_DEAD] -= flag;
                }
            }
        }
        else if (situationCount[STR_4_BLANK] > 0)
        {
            for (int i = 0; i < 4; ++i)
            {
                flag = fastfind(fail_check[STR_4_BLANK_M], pats_check[STR_4_BLANK_M], FORMAT_LENGTH, direction[0][i], range_check[STR_4_BLANK_M]);
                if (flag == 0)
                    flag += fastfind(fail_check[STR_4_BLANK_M], pats_check[STR_4_BLANK_M], FORMAT_LENGTH, direction[1][i], range_check[STR_4_BLANK_M]);
                if (flag > 0) {
                    situationCount[STR_4_BLANK] -= flag;
                }
            }
        }
        else if (situationCount[STR_4_BLANK_DEAD] > 0)
        {
            for (int i = 0; i < 4; ++i)
            {
                flag = fastfind(fail_check[STR_4_BLANK_DEAD], pats_check[STR_4_BLANK_DEAD], FORMAT_LENGTH, direction[0][i], range_check[STR_4_BLANK_DEAD]);
                flag = fastfind(fail_check[STR_4_BLANK_M], pats_check[STR_4_BLANK_M], FORMAT_LENGTH, direction[0][i], range_check[STR_4_BLANK_M]);
                if (flag == 0)
                {
                    flag += fastfind(fail_check[STR_4_BLANK_DEAD], pats_check[STR_4_BLANK_DEAD], FORMAT_LENGTH, direction[1][i], range_check[STR_4_BLANK_DEAD]);
                    flag += fastfind(fail_check[STR_4_BLANK_M], pats_check[STR_4_BLANK_M], FORMAT_LENGTH, direction[1][i], range_check[STR_4_BLANK_M]);
                }
                if (flag > 0) {
                    situationCount[STR_4_BLANK_DEAD] -= flag;
                }
            }
        }
        else if (situationCount[STR_4_BLANK_M] > 0)
        {
            for (int i = 0; i < 4; ++i)
            {
                flag = fastfind(fail_check[STR_4_BLANK_M], pats_check[STR_4_BLANK_M], FORMAT_LENGTH, direction[0][i], range_check[STR_4_BLANK_M]);
                if (flag == 0)
                    flag += fastfind(fail_check[STR_4_BLANK_M], pats_check[STR_4_BLANK_M], FORMAT_LENGTH, direction[1][i], range_check[STR_4_BLANK_M]);
                if (flag > 0) {
                    situationCount[STR_4_BLANK_M] -= flag;
                }
            }
        }
    }

    if (0 == situationCount[STR_5_CONTINUE])
    {
        if (situationCount[STR_6_CONTINUE] > 0)//长连
        {
            if (ban&&state == STATE_CHESS_BLACK)//有禁手
                return -3;
            else
            {
                stepScore += situationCount[STR_6_CONTINUE] * 100000;
            }
        }

        int deadFour = situationCount[STR_4_CONTINUE_DEAD] + situationCount[STR_4_BLANK] + situationCount[STR_4_BLANK_DEAD] + situationCount[STR_4_BLANK_M];

        if (deadFour + situationCount[STR_4_CONTINUE] > 1)//双四
        {
            if (ban&&state == STATE_CHESS_BLACK)//有禁手
                return -2;
        }

        if (deadFour > 1) {//双死四
            stepScore += 10001;
            deadFour = 0;
            situationCount[STR_4_CONTINUE_DEAD] = 0;
            situationCount[STR_4_BLANK] = 0;
            situationCount[STR_4_BLANK_DEAD] = 0;
            situationCount[STR_4_BLANK_M] = 0;
        }

        int aliveThree = situationCount[STR_3_CONTINUE] + situationCount[STR_3_BLANK];

        if (aliveThree == 1 && deadFour == 1) { //死四活三
            stepScore += 10001;
            deadFour = 0;
            aliveThree = 0;
            situationCount[STR_4_CONTINUE_DEAD] = 0;
            situationCount[STR_4_BLANK] = 0;
            situationCount[STR_4_BLANK_DEAD] = 0;
            situationCount[STR_4_BLANK_M] = 0;
            situationCount[STR_3_CONTINUE] = 0;
            situationCount[STR_3_BLANK] = 0;
        }

        if (aliveThree > 1) {//双活三
            if (ban&&state == STATE_CHESS_BLACK)//有禁手
                return -1;
            else
                stepScore += 8000;
            aliveThree = 0;
            situationCount[STR_3_CONTINUE] = 0;
            situationCount[STR_3_BLANK] = 0;
        }
    }

    if (isdefend)
        for (int j = 1; j < STR_COUNT; ++j) {//从1开始 长连珠特殊处理
            stepScore += situationCount[j] * evaluate_defend[j];
        }
    else
        for (int j = 1; j < STR_COUNT; ++j) {//从1开始 长连珠特殊处理
            stepScore += situationCount[j] * evaluate[j];
        }

    return stepScore;
}

int ChessBoard::getStepScores(bool ban, bool isdefend)
{
    return getStepScores(lastStep.uRow, lastStep.uCol, lastStep.getColor(), ban, isdefend);
}

int ChessBoard::getAtackScoreHelp(int row, int col, int color, int &resultScore, char irow, char icol)
{
    int direction;
    if (irow == '0')
    {
        direction = 0;
    }
    else if (icol == '0')
    {
        direction = 1;
    }
    else if (irow == icol)
    {
        direction = 2;
    }
    else
    {
        direction = 3;
    }
    int temprow, tempcol, blankcount = 0, count = 0;
    int maxSearch = 4;
    for (int i = 1; i <= maxSearch; ++i)
    {
        switch (irow)
        {
        case '+':
            temprow = row + i;
            break;
        case '-':
            temprow = row - i;
            break;
        case '0':
            temprow = row;
            break;
        default:
            break;
        }
        switch (icol)
        {
        case '+':
            tempcol = col + i;
            break;
        case '-':
            tempcol = col - i;
            break;
        case '0':
            tempcol = col;
            break;
        default:
            break;
        }
        if (temprow < 0 || temprow>14 || tempcol < 0 || tempcol >14 || pieces[temprow][tempcol].getState() == -color)
        {
            if (blankcount == 0)
                return count - 1;
            else
                break;
        }
        else if (pieces[temprow][tempcol].getState() == 0)
        {
            blankcount++;
            if (blankcount > 2)
                break;
            else
            {
                if (pieces[temprow][tempcol].getThreat(color) >= 100 && pieces[temprow][tempcol].getThreat(color) < 500)
                {
                    pieces[temprow][tempcol].setState(color);
                    if (getStepSituation(temprow, tempcol, color) == direction)
                        resultScore += 1000;
                    else
                        resultScore += 100;
                    pieces[temprow][tempcol].setState(0);
                    blankcount = 0;
                    //count++;
                }
                else if (pieces[temprow][tempcol].getThreat(color) >= 998 && pieces[temprow][tempcol].getThreat(color) < 1200)
                {
                    pieces[temprow][tempcol].setState(color);
                    if (getStepSituation(temprow, tempcol, color) == direction)
                        resultScore += pieces[temprow][tempcol].getThreat(color);
                    else
                    {
                        resultScore += pieces[temprow][tempcol].getThreat(color) / 10;
                        count++;
                    }
                    pieces[temprow][tempcol].setState(0);
                    blankcount = 0;

                }
                else if (pieces[temprow][tempcol].getThreat(color) >= 1200 && pieces[temprow][tempcol].getThreat(color) < 2000)
                {
                    resultScore += pieces[temprow][tempcol].getThreat(color);
                    blankcount = 0;
                    //count++;
                }
                else if (pieces[temprow][tempcol].getThreat(color) >= 8000)
                    resultScore += 1200;
            }

        }
        else
        {
            blankcount = 0;
            count++;
        }
    }//end for
    return count;
}

bool ChessBoard::getDirection(int& row, int& col, int i, int direction)
{
    switch (direction)
    {
    case DIRECTION8_L:
        col -= i;
        break;
    case DIRECTION8_R:
        col += i;
        break;
    case DIRECTION8_U:
        row -= i;
        break;
    case DIRECTION8_D:
        row += i;
        break;
    case DIRECTION8_LU:
        row -= i; col -= i;
        break;
    case DIRECTION8_RD:
        col += i; row += i;
        break;
    case DIRECTION8_LD:
        col -= i; row += i;
        break;
    case DIRECTION8_RU:
        col += i; row -= i;
        break;
    default:
        break;
    }
    if (row < 0 || row > 14 || col < 0 || col > 14)
        return false;
    else
        return true;
}

int ChessBoard::getAtackScore(int currentScore, int threat, bool ban)
{
    int row = lastStep.uRow, col = lastStep.uCol, color = lastStep.getColor();
    int resultScore = 0;
    if (currentScore >= 1210 && currentScore < 2000)//是否死四
    {
        int temprow, tempcol;
        for (int diretion = 0; diretion < 8; ++diretion)
            for (int i = 1; i < 5; ++i)
            {
                temprow = row, tempcol = col;
                if (getDirection(temprow, tempcol, i, diretion) && pieces[temprow][tempcol].getState() == 0)
                {
                    if (pieces[temprow][tempcol].getThreat(color) >= 100000)
                    {
                        if (pieces[temprow][tempcol].getThreat(-color) >= 12000)
                            return -1;
                        else if (pieces[temprow][tempcol].getThreat(-color) >= 10000)
                        {
                        }
                        doNextStep(temprow, tempcol, -color);
                        updateThreat(ban);
                        goto breakflag;
                    }
                }
            }
    breakflag:
        int count;
        //八个方向
        //↑+↓
        count = getAtackScoreHelp(row, col, color, resultScore, '-', '0') + getAtackScoreHelp(row, col, color, resultScore, '+', '0');
        if (count > 1)
            resultScore += 10000;
        //←+→
        count = getAtackScoreHelp(row, col, color, resultScore, '0', '-') + getAtackScoreHelp(row, col, color, resultScore, '0', '+');
        if (count > 1)
            resultScore += 10000;
        //↖+↘
        count = getAtackScoreHelp(row, col, color, resultScore, '-', '-') + getAtackScoreHelp(row, col, color, resultScore, '+', '+');
        if (count > 1)
            resultScore += 10000;
        //↙+↗
        count = getAtackScoreHelp(row, col, color, resultScore, '+', '-') + getAtackScoreHelp(row, col, color, resultScore, '-', '+');
        if (count > 1)
            resultScore += 10000;


    }
    else if (currentScore < 1210 && currentScore>1000)//活三
    {
        int resultScore1 = 0, resultScore2 = 0;
    }
    if (resultScore < 10000 && resultScore > 3000 && getThreatInfo(-color).totalScore > threat + resultScore)
        resultScore = 0;
    return resultScore;
}

int ChessBoard::getStepSituation(int row, int col, int state)
{
    char direction[2][4][FORMAT_LENGTH];//四个方向棋面（0表示空，-1表示断，1表示连）
    formatChess2String(direction[0], row, col, state);
    formatChess2String(direction[1], row, col, state, true);

    int situationCount[STR_COUNT] = { 0 };
    int flag = 0;
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 3; j < 9; ++j)
        {
            flag = fastfind(fail[j], pats[j], FORMAT_LENGTH, direction[0][i], range[j]);
            if ((flag == 0 && isReverse[j]) || j == STR_4_BLANK_DEAD)
                flag += fastfind(fail[j], pats[j], FORMAT_LENGTH, direction[1][i], range[j]);
            if (flag > 0)
            {
                return i;
            }
        }
    }
    return -1;//错误
}