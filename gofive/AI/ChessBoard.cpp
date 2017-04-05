#include "ChessBoard.h"
#include <string>
#include <random>
using namespace std;

bool ChessBoard::ban = false;
int8_t ChessBoard::level = AILEVEL_UNLIMITED;
TrieTreeNode* ChessBoard::searchTrieTree = NULL;
string ChessBoard::debugInfo = "";
uint32_t ChessBoard::z32[BOARD_ROW_MAX][BOARD_COL_MAX][3] = {0};
uint64_t ChessBoard::z64[BOARD_ROW_MAX][BOARD_COL_MAX][3] = {0};

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

void ChessBoard::initZobrist()
{
    default_random_engine e;
    uniform_int_distribution<uint64_t> rd64;
    uniform_int_distribution<uint32_t> rd32;
    for (int i = 0; i < BOARD_ROW_MAX; ++i)
    {
        for (int j = 0; j < BOARD_COL_MAX; ++j)
        {
            for (int k = 0; k < 3; ++k)
            {
                z32[i][j][k] = rd32(e);
                z64[i][j][k] = rd64(e);
            }
            
        }
    }
}

void ChessBoard::setBan(bool b)
{
    ban = b;
}

void ChessBoard::setLevel(int8_t l)
{
    level = l;
}

void ChessBoard::setThreat(const int& row, const int& col, const int& side, bool defend)
{
    int score = 0;
    pieces[row][col].state = side;
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

void ChessBoard::updateThreat(const int& row, const int& col, const int& side, bool defend)
{
    int tempcol, temprow;
    for (int i = 0; i < UPDATETHREAT_SEARCH_RANGE; ++i)
    {
        //����
        tempcol = col - UPDATETHREAT_SEARCH_MAX + i;
        if (tempcol > -1 && tempcol < 15 && pieces[row][tempcol].state == 0 && pieces[row][tempcol].hot)
        {
            setThreat(row, tempcol, side, defend);
        }
        //����
        temprow = row - UPDATETHREAT_SEARCH_MAX + i;
        if (temprow > -1 && temprow < 15 && pieces[temprow][col].state == 0 && pieces[temprow][col].hot)
        {
            setThreat(temprow, col, side, defend);
        }
        //����
        tempcol = col - UPDATETHREAT_SEARCH_MAX + i;
        temprow = row - UPDATETHREAT_SEARCH_MAX + i;
        if (temprow > -1 && temprow<15 && tempcol>-1 && tempcol < 15 &&
            pieces[temprow][tempcol].state == 0 && pieces[temprow][tempcol].hot)
        {
            setThreat(temprow, tempcol, side, defend);
        }
        //����
        tempcol = col - UPDATETHREAT_SEARCH_MAX + i;
        temprow = row + UPDATETHREAT_SEARCH_MAX - i;
        if (temprow > -1 && temprow<15 && tempcol>-1 && tempcol < 15 &&
            pieces[temprow][tempcol].state == 0 && pieces[temprow][tempcol].hot)
        {
            setThreat(temprow, tempcol, side, defend);
        }
    }
}

void ChessBoard::updateThreat2(const int& row, const int& col, const int& side, bool defend)
{
    int blankCount, chessCount, r, c;
    for (int i = 0; i < DIRECTION8_COUNT; ++i)//8������
    {
        r = row, c = col;
        blankCount = 0;
        chessCount = 0;
        while (applyDirection(r, c, 1, i)) //����������߽�
        {
            if (pieces[r][c].state == 0)
            {
                blankCount++;
                if (pieces[r][c].hot)
                {
                    setThreat(r, c, side, defend);
                }
            }
            else if (pieces[r][c].state == side)
            {
                break;//�����з����ӣ�ֹͣ����
            }
            else
            {
                chessCount++;
            }

            if (blankCount == 2 || chessCount == 5)
            {
                break; 
            }
        }
    }
}

void ChessBoard::updateHotArea(int row, int col)
{
    int range = 5;//2 * 2 + 1
    int tempcol, temprow;
    for (int i = 0; i < range; ++i)
    {
        //����
        tempcol = col - 2 + i;
        if (tempcol > -1 && tempcol < BOARD_COL_MAX && pieces[row][tempcol].state == 0)
        {
            pieces[row][tempcol].hot = (true);
        }
        //����
        temprow = row - 2 + i;
        if (temprow > -1 && temprow < BOARD_ROW_MAX && pieces[temprow][col].state == 0)
        {
            pieces[temprow][col].hot = (true);
        }
        //����
        if (temprow > -1 && temprow < BOARD_ROW_MAX && tempcol> -1 && tempcol < BOARD_COL_MAX && pieces[temprow][tempcol].state == 0)
        {
            pieces[temprow][tempcol].hot = (true);
        }
        //����
        temprow = row + 2 - i;
        if (temprow > -1 && temprow < BOARD_ROW_MAX && tempcol> -1 && tempcol < BOARD_COL_MAX && pieces[temprow][tempcol].state == 0)
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

bool ChessBoard::doNextStep(const int& row, const int& col, const int& side) {
    if (pieces[row][col].state != 0)
    {
        return false;//��������
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
RatingInfo ChessBoard::getRatingInfo(int side)
{
    RatingInfo result = { 0, 0 };
    for (int i = 0; i < BOARD_ROW_MAX; ++i)
    {
        for (int j = 0; j < BOARD_COL_MAX; ++j)
        {
            if (getPiece(i, j).hot && getPiece(i, j).state == 0)
            {
                result.totalScore += getPiece(i, j).getThreat(side);
                if (getPiece(i, j).getThreat(side) > result.highestScore)
                    result.highestScore = getPiece(i, j).getThreat(side);
            }
        }
    }
    return result;
}

void ChessBoard::formatChess2String(char chessStr[][FORMAT_LENGTH], const int& row, const int& col, const int& state, bool reverse)
{
    int rowstart = row - SEARCH_LENGTH, colstart = col - SEARCH_LENGTH, rowend = row + SEARCH_LENGTH;
    int index, step;
    if (reverse)
    {
        index = FORMAT_LAST_INDEX;
        step = -1;
    }
    else
    {
        index = 0;
        step = 1;
    }
    for (int i = 0; i < FORMAT_LENGTH; ++i, index += step, ++rowstart, ++colstart, --rowend)
    {
        //����
        if (colstart < 0 || colstart > 14)
        {
            chessStr[DIRECTION4_R][index] = 'x';
        }
        else if (pieces[row][colstart].state == -state)
        {
            chessStr[DIRECTION4_R][index] = 'x';
        }
        else if (pieces[row][colstart].state == state)
        {
            chessStr[DIRECTION4_R][index] = 'o';
        }
        else
        {
            chessStr[DIRECTION4_R][index] = '?';
        }
        //����
        if (rowstart < 0 || rowstart > 14)
        {
            chessStr[DIRECTION4_D][index] = 'x';
        }
        else if (pieces[rowstart][col].state == -state)
        {
            chessStr[DIRECTION4_D][index] = 'x';
        }
        else if (pieces[rowstart][col].state == state)
        {
            chessStr[DIRECTION4_D][index] = 'o';
        }
        else
        {
            chessStr[DIRECTION4_D][index] = '?';
        }
        //������
        if (colstart < 0 || rowstart < 0 || colstart > 14 || rowstart > 14)
        {
            chessStr[DIRECTION4_RD][index] = 'x';
        }
        else if (pieces[rowstart][colstart].state == -state)
        {
            chessStr[DIRECTION4_RD][index] = 'x';
        }
        else if (pieces[rowstart][colstart].state == state)
        {
            chessStr[DIRECTION4_RD][index] = 'o';
        }
        else
        {
            chessStr[DIRECTION4_RD][index] = '?';
        }
        //������
        if (colstart < 0 || rowend > 14 || colstart > 14 || rowend < 0)
        {
            chessStr[DIRECTION4_RU][index] = 'x';
        }
        else if (pieces[rowend][colstart].state == -state)
        {
            chessStr[DIRECTION4_RU][index] = 'x';
        }
        else if (pieces[rowend][colstart].state == state)
        {
            chessStr[DIRECTION4_RU][index] = 'o';
        }
        else
        {
            chessStr[DIRECTION4_RU][index] = '?';
        }
    }
}

extern ChessModeData chessMode[TRIE_COUNT];
int ChessBoard::handleSpecial(const SearchResult &result, const int &state, uint8_t chessModeCount[TRIE_COUNT])
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
            return chessMode[TRIE_5_CONTINUE].evaluation;//û��Ҫ����
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
                //��Ч�����Ѿ�����Ӯ�ˣ�����������
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
                //��Ч�����Ѿ�����Ӯ�ˣ�����������
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
                //��Ч��
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
                //��Ч�����Ѿ�����Ӯ�ˣ�����������
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
                //��Ч��
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
                //��Ч�����Ѿ�����Ӯ�ˣ�����������
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
                //��Ч��
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
                //��Ч��
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
                //��Ч��
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
                //��Ч��
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
                //��Ч��
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
                //��Ч��
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
int ChessBoard::getStepScores(const int& row, const int& col, const int& state, const bool& isdefend)
{
    int stepScore = 0;
    char direction[4][FORMAT_LENGTH];//�ĸ��������棨0��ʾ�գ�-1��ʾ�ϣ�1��ʾ����
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
        if (result.chessMode > TRIE_5_CONTINUE)//����Ҫ���⴦��
        {
            chessModeCount[result.chessMode]++;
            continue;
        }
        else if (result.chessMode == TRIE_5_CONTINUE)
        {
            return chessMode[TRIE_5_CONTINUE].evaluation;//û��Ҫ����
        }
        //������������
        result.pos = chessMode[result.chessMode].pat_len - (result.pos - SEARCH_LENGTH);
        stepScore = handleSpecial(result, state, chessModeCount);
        if (stepScore)
        {
            return stepScore;
        }
    }

    //�������
    int deadFour =
        chessModeCount[TRIE_4_CONTINUE_DEAD] +
        chessModeCount[TRIE_4_CONTINUE_DEAD_R] +
        chessModeCount[TRIE_4_BLANK] +
        chessModeCount[TRIE_4_BLANK_R] +
        chessModeCount[TRIE_4_BLANK_DEAD] +
        chessModeCount[TRIE_4_BLANK_DEAD_R] +
        chessModeCount[TRIE_4_BLANK_M];

    if (deadFour + chessModeCount[TRIE_4_CONTINUE] > 1)//˫��
    {
        if (ban&&state == STATE_CHESS_BLACK)//�н���
        {
            return BAN_DOUBLEFOUR;
        }
    }

    if (deadFour > 1 && level >= AILEVEL_INTERMEDIATE) //˫�������޽���
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

    if (aliveThree == 1 && deadFour == 1 && (level >= AILEVEL_HIGH))//���Ļ���
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

    if (aliveThree > 1)//˫����
    {
        if (ban&&state == STATE_CHESS_BLACK)//�н���
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
        if (col < 0) return false;
        break;
    case DIRECTION8_R:
        col += i;
        if (col > 14) return false;
        break;
    case DIRECTION8_U:
        row -= i;
        if (row < 0) return false;
        break;
    case DIRECTION8_D:
        row += i;
        if (row > 14) return false;
        break;
    case DIRECTION8_LU:
        row -= i; col -= i;
        if (row < 0 || col < 0) return false;
        break;
    case DIRECTION8_RD:
        col += i; row += i;
        if (row > 14 || col > 14) return false;
        break;
    case DIRECTION8_LD:
        col -= i; row += i;
        if (row > 14 || col < 0) return false;
        break;
    case DIRECTION8_RU:
        col += i; row -= i;
        if (row < 0 || col > 14) return false;
        break;
    default:
        return false;
        break;
    }
    return true;
}

int ChessBoard::getAtackScore(int currentScore, int threat)
{
    int row = lastStep.row, col = lastStep.col, color = lastStep.getColor();
    int resultScore = 0, flagalivethree = false;
    if (currentScore >= 1210 && currentScore < 2000)//�Ƿ����
    {
        int temprow, tempcol;
        for (int diretion = 0; diretion < 8; ++diretion)//�ȶ³���
        {
            for (int i = 1; i < 5; ++i)
            {
                temprow = row, tempcol = col;
                if (applyDirection(temprow, tempcol, i, diretion) && pieces[temprow][tempcol].state == 0)
                {
                    if (pieces[temprow][tempcol].getThreat(color) >= SCORE_5_CONTINUE)
                    {
                        if (pieces[temprow][tempcol].getThreat(-color) >= SCORE_4_CONTINUE)//����
                        {
                            return -1;
                        }
                        else if (pieces[temprow][tempcol].getThreat(-color) >= SCORE_4_DOUBLE)//˫�ġ�����
                        {
                            return -1;//add at 17.3.23
                        }
                        else if (pieces[temprow][tempcol].getThreat(-color) > 990)//������˫��
                        {
                            flagalivethree = true;
                        }
                        doNextStep(temprow, tempcol, -color);
                        updateThreat(0, false);//���ý���Ȩ�� add at 17.3.23
                        //updateThreat();
                        goto breakflag;
                    }
                }
            }
        }

    breakflag:
        int count;
        //�˸�����
        //��+��
        count = getAtackScoreHelp(row, col, color, resultScore, DIRECTION8_U) + getAtackScoreHelp(row, col, color, resultScore, DIRECTION8_D);
        if (count > 1)
            resultScore += 10000;
        //��+��
        count = getAtackScoreHelp(row, col, color, resultScore, DIRECTION8_L) + getAtackScoreHelp(row, col, color, resultScore, DIRECTION8_R);
        if (count > 1)
            resultScore += 10000;
        //�I+�K
        count = getAtackScoreHelp(row, col, color, resultScore, DIRECTION8_LU) + getAtackScoreHelp(row, col, color, resultScore, DIRECTION8_RD);
        if (count > 1)
            resultScore += 10000;
        //�L+�J
        count = getAtackScoreHelp(row, col, color, resultScore, DIRECTION8_LD) + getAtackScoreHelp(row, col, color, resultScore, DIRECTION8_RU);
        if (count > 1)
            resultScore += 10000;


    }
    else if (currentScore < 1210 && currentScore>1000)//����
    {
        int resultScore1 = 0, resultScore2 = 0;
    }
    if (resultScore < 10000 && resultScore > 3000 && getRatingInfo(-color).totalScore > threat + resultScore)
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

//�������������ķ���
int ChessBoard::getChessModeDirection(int row, int col, int state)
{
    char direction[4][FORMAT_LENGTH];//�ĸ��������棨0��ʾ�գ�-1��ʾ�ϣ�1��ʾ����
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
    return -1;//����
}


string ChessBoard::toString()
{
    stringbuf buf;
    int stateflag = 2, statecount = 0;
    for (int i = 0; i < BOARD_ROW_MAX; ++i)
    {
        for (int j = 0; j < BOARD_COL_MAX; ++j)
        {
            if (stateflag != pieces[i][j].state)
            {
                if (stateflag == STATE_EMPTY)
                {
                    if (statecount > 1)
                    {
                        buf.sputc(statecount + 'A');
                    }
                    buf.sputc('?');
                }
                else if (stateflag == STATE_CHESS_BLACK)
                {
                    if (statecount > 1)
                    {
                        buf.sputc(statecount + 'A');
                    }
                    buf.sputc('o');
                }
                else if (stateflag == STATE_CHESS_WHITE)
                {
                    if (statecount > 1)
                    {
                        buf.sputc(statecount + 'A');
                    }
                    buf.sputc('x');
                }
                stateflag = pieces[i][j].state;
                statecount = 1;
            }
            else
            {
                statecount++;
            }
        }
        if (stateflag == STATE_EMPTY)
        {
            if (statecount > 1)
            {
                buf.sputc(statecount + 'A');
            }
            buf.sputc('?');
        }
        else if (stateflag == STATE_CHESS_BLACK)
        {
            if (statecount > 1)
            {
                buf.sputc(statecount + 'A');
            }
            buf.sputc('o');
        }
        else if (stateflag == STATE_CHESS_WHITE)
        {
            if (statecount > 1)
            {
                buf.sputc(statecount + 'A');
            }
            buf.sputc('x');
        }
        buf.sputc('\n');
        stateflag = 2, statecount = 0;
    }
    return buf.str();
}

HashPair ChessBoard::toHash()
{
    HashPair pair = { 0,0 };
    for (int i = 0; i < BOARD_ROW_MAX; ++i)
    {
        for (int j = 0; j < BOARD_COL_MAX; ++j)
        {
            pair.z32key ^= z32[i][j][pieces[i][j].state + 1];
            pair.z64key ^= z64[i][j][pieces[i][j].state + 1];
        }
    }
    return pair;
}
void ChessBoard::updateHashPair(HashPair &pair, const int& row, const int& col, const int& side)
{
    pair.z32key ^= z32[row][col][1];//ԭ���ǿյ�
    pair.z32key ^= z32[row][col][side + 1];
                 
    pair.z64key ^= z64[row][col][1];
    pair.z64key ^= z64[row][col][side + 1];
}
