#include "ChessBoard.h"
#include <string>
using namespace std;

bool ChessBoard::ban = false;
int8_t ChessBoard::level = AILEVEL_UNLIMITED;
TrieTreeNode* ChessBoard::searchTrieTree = NULL;
int8_t ChessBoard::algType = 1;
string ChessBoard::debugInfo = "";

ChessBoard::ChessBoard()
{
    lastStep.step = 0;
}

ChessBoard::~ChessBoard()
{

}

bool ChessBoard::buildTrieTree()
{
    if (searchTrieTree == NULL)
    {
        searchTrieTree = new TrieTreeNode();
        if (!searchTrieTree->buildTrieTree())
        {
            return false;
        }
        return true;
    }
    return false;
}

void ChessBoard::setBan(bool b)
{
    ban = b;
}

void ChessBoard::setLevel(int8_t l)
{
    level = l;
}

void ChessBoard::setThreat(int row, int col, int side, bool defend)
{
    int score = 0;
    pieces[row][col].state = (side);
    score = getStepScores(row, col, side, defend);
    pieces[row][col].state = (0);
    pieces[row][col].setThreat(score, side);
}

void ChessBoard::setGlobalThreat(bool defend)
{
    for (int a = 0; a < BOARD_ROW_MAX; ++a) {
        for (int b = 0; b < BOARD_COL_MAX; ++b) {
            getPiece(a, b).clearThreat();
            if (pieces[a][b].hot && pieces[a][b].state == 0)
            {
                setThreat(a, b, 1, defend);
                setThreat(a, b, -1, defend);
            }
        }
    }
}

void ChessBoard::updateThreat(int side, bool defend)
{
    if (side == 0)
    {
        updateThreat(lastStep.row, lastStep.col, 1, defend);
        updateThreat(lastStep.row, lastStep.col, -1, defend);
    }
    else
    {
        updateThreat(lastStep.row, lastStep.col, side, defend);
    }
}

void ChessBoard::updateThreat(int row, int col, int side, bool defend)
{
    int range = UPDATETHREAT_SEARCH_MAX * 2 + 1;
    int tempcol, temprow;
    for (int i = 0; i < range; ++i)
    {
        //横向
        tempcol = col - UPDATETHREAT_SEARCH_MAX + i;
        if (tempcol > -1 && tempcol < 15 && pieces[row][tempcol].state == 0 && pieces[row][tempcol].hot)
        {
            setThreat(row, tempcol, side, defend);
        }
        //纵向
        temprow = row - UPDATETHREAT_SEARCH_MAX + i;
        if (temprow > -1 && temprow < 15 && pieces[temprow][col].state == 0 && pieces[temprow][col].hot)
        {
            setThreat(temprow, col, side, defend);
        }
        //右下
        tempcol = col - UPDATETHREAT_SEARCH_MAX + i;
        temprow = row - UPDATETHREAT_SEARCH_MAX + i;
        if (temprow > -1 && temprow<15 && tempcol>-1 && tempcol < 15 &&
            pieces[temprow][tempcol].state == 0 && pieces[temprow][tempcol].hot)
        {
            setThreat(temprow, tempcol, side, defend);
        }
        //右上
        tempcol = col - UPDATETHREAT_SEARCH_MAX + i;
        temprow = row + UPDATETHREAT_SEARCH_MAX - i;
        if (temprow > -1 && temprow<15 && tempcol>-1 && tempcol < 15 &&
            pieces[temprow][tempcol].state == 0 && pieces[temprow][tempcol].hot)
        {
            setThreat(temprow, tempcol, side, defend);
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
        if (tempcol > -1 && tempcol < 15 && pieces[row][tempcol].state == 0)
        {
            pieces[row][tempcol].hot = (true);
        }
        //纵向
        temprow = row - 2 + i;
        if (temprow > -1 && temprow < 15 && pieces[temprow][col].state == 0)
        {
            pieces[temprow][col].hot = (true);
        }
        //右下
        if (temprow > -1 && temprow<15 && tempcol>-1 && tempcol < 15 &&
            pieces[temprow][tempcol].state == 0)
        {
            pieces[temprow][tempcol].hot = (true);
        }
        //右上
        temprow = row + 2 - i;
        if (temprow > -1 && temprow<15 && tempcol>-1 && tempcol < 15 &&
            pieces[temprow][tempcol].state == 0)
        {
            pieces[temprow][tempcol].hot = (true);
        }
    }
}

void ChessBoard::resetHotArea() {
    for (int i = 0; i < 15; ++i) {
        for (int j = 0; j < 15; ++j) {
            pieces[i][j].hot = (false);
        }
    }
    for (int row = 0; row < BOARD_ROW_MAX; ++row)
    {
        for (int col = 0; col < BOARD_COL_MAX; ++col)
        {
            if (pieces[row][col].state != 0)
            {
                updateHotArea(row, col);
            }
        }
    }
}

bool ChessBoard::doNextStep(int row, int col, int side) {
    if (pieces[row][col].state != 0)
    {
        return false;//已有棋子
    }
    else
    {
        lastStep.col = col;
        lastStep.row = row;
        lastStep.step = lastStep.step + 1;
        lastStep.setColor(side);
        pieces[row][col].state = (side);
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
            if (getPiece(i, j).hot && getPiece(i, j).state == 0)
            {
                result.totalScore += getPiece(i, j).getThreat(side);
                if (getPiece(i, j).getThreat(side) > result.HighestScore)
                    result.HighestScore = getPiece(i, j).getThreat(side);
            }
        }
    }
    return result;
}

void ChessBoard::formatChess2String(char chessStr[][FORMAT_LENGTH], int row, int col, int state, bool reverse)
{
    int tempstate;
    int rowstart = row - SEARCH_LENGTH, colstart = col - SEARCH_LENGTH, rowend = row + SEARCH_LENGTH;
    int index, step;
    if (reverse)
    {
        index = FORMAT_LENGTH - 1;
        step = -1;
    }
    else
    {
        index = 0;
        step = 1;
    }
    for (int i = 0; i < FORMAT_LENGTH; ++i, index += step, ++rowstart, ++colstart, --rowend)
    {
        //横向
        if (colstart < 0 || colstart > 14)
        {
            chessStr[DIRECTION4_R][index] = 'x';
        }
        else if ((tempstate = pieces[row][colstart].state) == -state)
        {
            chessStr[DIRECTION4_R][index] = 'x';
        }
        else if (tempstate == state)
        {
            chessStr[DIRECTION4_R][index] = 'o';
        }
        else if (tempstate == 0)
        {
            chessStr[DIRECTION4_R][index] = '?';
        }
        //纵向
        if (rowstart < 0 || rowstart > 14)
        {
            chessStr[DIRECTION4_D][index] = 'x';
        }
        else if ((tempstate = pieces[rowstart][col].state) == -state)
        {
            chessStr[DIRECTION4_D][index] = 'x';
        }
        else if (tempstate == state)
        {
            chessStr[DIRECTION4_D][index] = 'o';
        }
        else if (tempstate == 0)
        {
            chessStr[DIRECTION4_D][index] = '?';
        }
        //右下向
        if (colstart < 0 || rowstart < 0 || colstart > 14 || rowstart > 14)
        {
            chessStr[DIRECTION4_RD][index] = 'x';
        }
        else if ((tempstate = pieces[rowstart][colstart].state) == -state)
        {
            chessStr[DIRECTION4_RD][index] = 'x';
        }
        else if (tempstate == state)
        {
            chessStr[DIRECTION4_RD][index] = 'o';
        }
        else if (tempstate == 0)
        {
            chessStr[DIRECTION4_RD][index] = '?';
        }
        //右上向
        if (colstart < 0 || rowend > 14 || colstart > 14 || rowend < 0)
        {
            chessStr[DIRECTION4_RU][index] = 'x';
        }
        else if ((tempstate = pieces[rowend][colstart].state) == -state)
        {
            chessStr[DIRECTION4_RU][index] = 'x';
        }
        else if (tempstate == state)
        {
            chessStr[DIRECTION4_RU][index] = 'o';
        }
        else if (tempstate == 0)
        {
            chessStr[DIRECTION4_RU][index] = '?';
        }
    }
}

extern ChessModeData chessMode[TRIE_COUNT];
int ChessBoard::handleSpecial(SearchResult &result, int &state, uint8_t chessModeCount[TRIE_COUNT])
{
    switch (result.chessMode)
    {
    case TRIE_6_CONTINUE:
        if (ban&&state == STATE_CHESS_BLACK)
        {
            return BAN_LONGCONTINUITY;
        }
        else
        {
            return chessMode[TRIE_5_CONTINUE].evaluation;//没必要算了
        }
        break;
    case TRIE_4_DOUBLE_BAN1:
        if (result.pos < 6 && result.pos > 2)
        {
            if (ban&&state == STATE_CHESS_BLACK)
            {
                return BAN_DOUBLEFOUR;
            }
            else
            {
                chessModeCount[TRIE_4_BLANK_DEAD]++;
                chessModeCount[TRIE_4_BLANK_DEAD_R]++;
            }
        }
        else
        {
            chessModeCount[TRIE_4_BLANK_DEAD]++;
        }
        break;
    case TRIE_4_DOUBLE_BAN2:
        if (result.pos < 6 && result.pos > 3)
        {
            if (ban&&state == STATE_CHESS_BLACK)
            {
                return BAN_DOUBLEFOUR;
            }
            else
            {
                chessModeCount[TRIE_4_BLANK_M] += 2;
            }
        }
        else
        {
            chessModeCount[TRIE_4_BLANK_M]++;
        }
        break;
    case TRIE_4_DOUBLE_BAN3:
        if (result.pos == 5)
        {
            if (ban&&state == STATE_CHESS_BLACK)
            {
                return BAN_DOUBLEFOUR;
            }
            else
            {
                chessModeCount[TRIE_4_BLANK_DEAD]++;
                chessModeCount[TRIE_4_BLANK_DEAD_R]++;
            }
        }
        else
        {
            chessModeCount[TRIE_4_BLANK_DEAD]++;
        }
        break;
    case TRIE_4_CONTINUE_BAN:
        if (result.pos > 2)
        {
            if (ban&&state == STATE_CHESS_BLACK)
            {
                chessModeCount[TRIE_4_CONTINUE_DEAD]++;
            }
            else
            {
                chessModeCount[TRIE_4_CONTINUE]++;
            }
        }
        else
        {
            if (ban&&state == STATE_CHESS_BLACK)
            {
                //无效步，已经可以赢了，不能走这里
            }
            else
            {
                chessModeCount[TRIE_4_BLANK_DEAD]++;
            }
        }
        break;
    case TRIE_4_CONTINUE_BAN_R:
        if (result.pos < 6)
        {
            if (ban&&state == STATE_CHESS_BLACK)
            {
                chessModeCount[TRIE_4_CONTINUE_DEAD]++;
            }
            else
            {
                chessModeCount[TRIE_4_CONTINUE]++;
            }
        }
        else
        {
            if (ban&&state == STATE_CHESS_BLACK)
            {
                //无效步，已经可以赢了，不能走这里
            }
            else
            {
                chessModeCount[TRIE_4_BLANK_DEAD]++;
            }
        }
        break;
    case TRIE_4_CONTINUE_DEAD_BAN:
        if (result.pos > 2)
        {
            if (ban&&state == STATE_CHESS_BLACK)
            {
                //无效步
            }
            else
            {
                chessModeCount[TRIE_4_CONTINUE_DEAD]++;
            }
        }
        else
        {
            if (ban&&state == STATE_CHESS_BLACK)
            {
                //无效步，已经可以赢了，不能走这里
            }
            else
            {
                chessModeCount[TRIE_4_BLANK_DEAD]++;
            }
        }
        break;
    case TRIE_4_CONTINUE_DEAD_BAN_R:
        if (result.pos < 6)
        {
            if (ban&&state == STATE_CHESS_BLACK)
            {
                //无效步
            }
            else
            {
                chessModeCount[TRIE_4_CONTINUE_DEAD]++;
            }
        }
        else
        {
            if (ban&&state == STATE_CHESS_BLACK)
            {
                //无效步，已经可以赢了，不能走这里
            }
            else
            {
                chessModeCount[TRIE_4_BLANK_DEAD]++;
            }
        }
        break;
    case TRIE_4_BLANK_BAN:
        if (result.pos > 3)
        {
            if (ban&&state == STATE_CHESS_BLACK)
            {
                chessModeCount[TRIE_3_CONTINUE_DEAD]++;
            }
            else
            {
                chessModeCount[TRIE_4_BLANK]++;
            }
        }
        else
        {
            if (ban&&state == STATE_CHESS_BLACK)
            {
                //无效步
            }
            else
            {
                chessModeCount[TRIE_4_BLANK_M]++;
            }
        }
        break;
    case TRIE_4_BLANK_BAN_R:
        if (result.pos < 6)
        {
            if (ban&&state == STATE_CHESS_BLACK)
            {
                chessModeCount[TRIE_3_CONTINUE_DEAD]++;
            }
            else
            {
                chessModeCount[TRIE_4_BLANK]++;
            }
        }
        else
        {
            if (ban&&state == STATE_CHESS_BLACK)
            {
                //无效步
            }
            else
            {
                chessModeCount[TRIE_4_BLANK_M]++;
            }
        }
        break;
    case TRIE_4_BLANK_DEAD_BAN:
        if (result.pos < 4)
        {
            if (ban&&state == STATE_CHESS_BLACK)
            {
                //无效步
            }
            else
            {
                chessModeCount[TRIE_4_BLANK_DEAD]++;
            }
        }
        else
        {
            if (ban&&state == STATE_CHESS_BLACK)
            {
                //无效步
            }
            else
            {
                chessModeCount[TRIE_4_BLANK_M]++;
            }
        }
        break;
    case TRIE_4_BLANK_DEAD_BAN_R:
        if (result.pos > 3)
        {
            if (ban&&state == STATE_CHESS_BLACK)
            {
                //无效步
            }
            else
            {
                chessModeCount[TRIE_4_BLANK_DEAD]++;
            }
        }
        else
        {
            if (ban&&state == STATE_CHESS_BLACK)
            {
                //无效步
            }
            else
            {
                chessModeCount[TRIE_4_BLANK_M]++;
            }
        }
        break;
    default:
        break;
    }
    return 0;
}
int ChessBoard::getStepScores(int row, int col, int state, bool isdefend)
{
    int stepScore = 0;
    char direction[4][FORMAT_LENGTH];//四个方向棋面（0表示空，-1表示断，1表示连）
    formatChess2String(direction, row, col, state);
    uint8_t chessModeCount[TRIE_COUNT] = { 0 };
    SearchResult result;
    for (int i = 0; i < 4; ++i)
    {
        result = searchTrieTree->search(direction[i]);
        if (result.chessMode < 0)
        {
            continue;
        }
        //handle result
        if (result.chessMode > TRIE_5_CONTINUE)//不需要特殊处理
        {
            chessModeCount[result.chessMode]++;
            continue;
        }
        else if (result.chessMode == TRIE_5_CONTINUE)
        {
            return chessMode[TRIE_5_CONTINUE].evaluation;//没必要算了
        }
        //处理特殊棋型
        result.pos = chessMode[result.chessMode].pat_len - (result.pos - SEARCH_LENGTH);
        stepScore = handleSpecial(result, state, chessModeCount);
        if (stepScore)
        {
            return stepScore;
        }
    }

    //组合棋型
    int deadFour =
        chessModeCount[TRIE_4_CONTINUE_DEAD] +
        chessModeCount[TRIE_4_CONTINUE_DEAD_R] +
        chessModeCount[TRIE_4_BLANK] +
        chessModeCount[TRIE_4_BLANK_R] +
        chessModeCount[TRIE_4_BLANK_DEAD] +
        chessModeCount[TRIE_4_BLANK_DEAD_R] +
        chessModeCount[TRIE_4_BLANK_M];

    if (deadFour + chessModeCount[TRIE_4_CONTINUE] > 1)//双四
    {
        if (ban&&state == STATE_CHESS_BLACK)//有禁手
        {
            return BAN_DOUBLEFOUR;
        }
    }

    if (deadFour > 1 && level >= AILEVEL_INTERMEDIATE) //双死四且无禁手
    {
        stepScore += 10001;
        deadFour = 0;
        chessModeCount[TRIE_4_CONTINUE_DEAD] = 0;
        chessModeCount[TRIE_4_CONTINUE_DEAD_R] = 0;
        chessModeCount[TRIE_4_BLANK] = 0;
        chessModeCount[TRIE_4_BLANK_R] = 0;
        chessModeCount[TRIE_4_BLANK_DEAD] = 0;
        chessModeCount[TRIE_4_BLANK_DEAD_R] = 0;
        chessModeCount[TRIE_4_BLANK_M] = 0;
    }


    int aliveThree =
        chessModeCount[TRIE_3_CONTINUE] +
        chessModeCount[TRIE_3_CONTINUE_R] +
        chessModeCount[TRIE_3_BLANK] +
        chessModeCount[TRIE_3_BLANK_R];

    if (aliveThree == 1 && deadFour == 1 && (level >= AILEVEL_HIGH))//死四活三
    {
        stepScore += 10001;
        deadFour = 0;
        aliveThree = 0;
        chessModeCount[TRIE_4_CONTINUE_DEAD] = 0;
        chessModeCount[TRIE_4_CONTINUE_DEAD_R] = 0;
        chessModeCount[TRIE_4_BLANK] = 0;
        chessModeCount[TRIE_4_BLANK_R] = 0;
        chessModeCount[TRIE_4_BLANK_DEAD] = 0;
        chessModeCount[TRIE_4_BLANK_DEAD_R] = 0;
        chessModeCount[TRIE_4_BLANK_M] = 0;
        chessModeCount[TRIE_3_CONTINUE] = 0;
        chessModeCount[TRIE_3_CONTINUE_R] = 0;
        chessModeCount[TRIE_3_BLANK] = 0;
        chessModeCount[TRIE_3_BLANK_R] = 0;
    }

    if (aliveThree > 1)//双活三
    {
        if (ban&&state == STATE_CHESS_BLACK)//有禁手
        {
            return BAN_DOUBLETHREE;
        }
        if (level >= AILEVEL_INTERMEDIATE)
        {
            stepScore += 8000;
            chessModeCount[TRIE_3_CONTINUE] = 0;
            chessModeCount[TRIE_3_CONTINUE_R] = 0;
            chessModeCount[TRIE_3_BLANK] = 0;
            chessModeCount[TRIE_3_BLANK_R] = 0;
        }
    }

    if (isdefend)
    {
        for (int i = TRIE_4_CONTINUE; i < TRIE_COUNT; i++)
        {
            if (chessModeCount[i])
            {
                stepScore += chessModeCount[i] * chessMode[i].evaluation_defend;
            }
        }
    }
    else
    {
        for (int i = TRIE_4_CONTINUE; i < TRIE_COUNT; i++)
        {
            if (chessModeCount[i])
            {
                stepScore += chessModeCount[i] * chessMode[i].evaluation;
            }
        }
    }

    return stepScore;
}

bool ChessBoard::applyDirection(int& row, int& col, int i, int direction)
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

int ChessBoard::getAtackScore(int currentScore, int threat)
{
    int row = lastStep.row, col = lastStep.col, color = lastStep.getColor();
    int resultScore = 0, flagalivethree = false;
    if (currentScore >= 1210 && currentScore < 2000)//是否冲四
    {
        int temprow, tempcol;
        for (int diretion = 0; diretion < 8; ++diretion)//先堵冲四
        {
            for (int i = 1; i < 5; ++i)
            {
                temprow = row, tempcol = col;
                if (applyDirection(temprow, tempcol, i, diretion) && pieces[temprow][tempcol].state == 0)
                {
                    if (pieces[temprow][tempcol].getThreat(color) >= 100000)
                    {
                        if (pieces[temprow][tempcol].getThreat(-color) >= 12000)//活四
                        {
                            return -1;
                        }
                        else if (pieces[temprow][tempcol].getThreat(-color) >= 10000)//双四、三四
                        {
                            return -1;//add at 17.3.23
                        }
                        else if (pieces[temprow][tempcol].getThreat(-color) > 990)//活三、双三
                        {
                            flagalivethree = true;
                        }
                        doNextStep(temprow, tempcol, -color);
                        updateThreat(0, false);//启用进攻权重 add at 17.3.23
                        //updateThreat();
                        goto breakflag;
                    }
                }
            }
        }

    breakflag:
        int count;
        //八个方向
        //↑+↓
        count = getAtackScoreHelp(row, col, color, resultScore, DIRECTION8_U) + getAtackScoreHelp(row, col, color, resultScore, DIRECTION8_D);
        if (count > 1)
            resultScore += 10000;
        //←+→
        count = getAtackScoreHelp(row, col, color, resultScore, DIRECTION8_L) + getAtackScoreHelp(row, col, color, resultScore, DIRECTION8_R);
        if (count > 1)
            resultScore += 10000;
        //↖+↘
        count = getAtackScoreHelp(row, col, color, resultScore, DIRECTION8_LU) + getAtackScoreHelp(row, col, color, resultScore, DIRECTION8_RD);
        if (count > 1)
            resultScore += 10000;
        //↙+↗
        count = getAtackScoreHelp(row, col, color, resultScore, DIRECTION8_LD) + getAtackScoreHelp(row, col, color, resultScore, DIRECTION8_RU);
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

int ChessBoard::getAtackScoreHelp(int row, int col, int color, int &resultScore, int direction8)
{
    int blankcount = 0, count = 0;
    int maxSearch = 4;
    for (int i = 1; i <= maxSearch; ++i)
    {
        if (!applyDirection(row, col, 1, direction8) || pieces[row][col].state == -color)
        {
            if (blankcount == 0)
                return count - 1;
            else
                break;
        }
        else if (pieces[row][col].state == 0)
        {
            blankcount++;
            if (blankcount > 2)
                break;
            else
            {
                if (pieces[row][col].getThreat(color) >= 100 && pieces[row][col].getThreat(color) < 500)
                {
                    pieces[row][col].state = (color);
                    if (getChessModeDirection(row, col, color) == direction8 / 2)
                        resultScore += 1000;
                    else
                        resultScore += 100;
                    pieces[row][col].state = (0);
                    blankcount = 0;
                    //count++;
                }
                else if (pieces[row][col].getThreat(color) >= 998 && pieces[row][col].getThreat(color) < 1200)
                {
                    pieces[row][col].state = (color);
                    if (getChessModeDirection(row, col, color) == direction8 / 2)
                        resultScore += pieces[row][col].getThreat(color);
                    else
                    {
                        resultScore += pieces[row][col].getThreat(color) / 10;
                        count++;
                    }
                    pieces[row][col].state = (0);
                    blankcount = 0;

                }
                else if (pieces[row][col].getThreat(color) >= 1200 && pieces[row][col].getThreat(color) < 2000)
                {
                    resultScore += pieces[row][col].getThreat(color);
                    blankcount = 0;
                    //count++;
                }
                else if (pieces[row][col].getThreat(color) >= 8000)
                {
                    resultScore += 1200;
                }
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

//查找三连四连的方向
int ChessBoard::getChessModeDirection(int row, int col, int state)
{
    char direction[4][FORMAT_LENGTH];//四个方向棋面（0表示空，-1表示断，1表示连）
    formatChess2String(direction, row, col, state);
    int situationCount[TRIE_COUNT] = { 0 };
    SearchResult result;
    for (int i = 0; i < 4; ++i)
    {
        result = searchTrieTree->search(direction[i]);
        if (result.chessMode >= TRIE_4_CONTINUE_DEAD && result.chessMode <= TRIE_3_BLANK_R)
        {
            return i;
        }
        else if (result.chessMode < TRIE_6_CONTINUE)
        {
            result.pos = chessMode[result.chessMode].pat_len - (result.pos - SEARCH_LENGTH);
            uint8_t chessModeCount[TRIE_COUNT] = { 0 };
            handleSpecial(result, state, chessModeCount);
            for (int index = TRIE_4_CONTINUE_DEAD; index <= TRIE_3_BLANK_R; ++index)
            {
                if (chessModeCount[index] > 0)
                {
                    return i;
                }
            }
        }
    }
    return -1;//错误
}
