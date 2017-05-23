#include "ChessBoard.h"
#include <random>
using namespace std;

bool ChessBoard::ban = false;
int8_t ChessBoard::level = AILEVEL_UNLIMITED;
TrieTreeNode* ChessBoard::searchTrieTree = NULL;
string ChessBoard::debugInfo = "";
uint32_t ChessBoard::z32[BOARD_ROW_MAX][BOARD_COL_MAX][3] = { 0 };
uint64_t ChessBoard::z64[BOARD_ROW_MAX][BOARD_COL_MAX][3] = { 0 };
uint8_t ChessBoard::chessModeTable[CHESSMODE_TABLE_SIZE][CHESSMODE_LEN] = { 0 };

ChessBoard::ChessBoard()
{
    lastStep.step = 0;
}

ChessBoard::~ChessBoard()
{

}


void ChessBoard::setBan(bool b)
{
    ban = b;
}

void ChessBoard::setLevel(int8_t l)
{
    level = l;
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
    default_random_engine e(4768);//fixed seed
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

void ChessBoard::initChessModeTable()
{
    uint32_t searchMode = 0;
    uint32_t searchModeTemp = 0;
    //棋型 2^5
    for (uint8_t chess = 0; chess < 32; ++chess)
    {
        //两端类型 2^3 大于4的都是保留字段
        for (uint8_t type = 0; type < 16; ++type)
        {
            searchMode = 0;
            searchModeTemp = 0;
            for (int i = 0; i < 5; ++i)
            {
                if ((chess >> i) & 0x1)
                {
                    searchModeTemp |= 2 << i * 2;
                }
                else
                {
                    searchModeTemp |= 1 << i * 2;
                }
            }
            //left
            switch (type >> 2)//searchModeTemp length = 7 
            {
            case 0://不堵
                searchModeTemp = (searchModeTemp << 2) + 1;
                break;
            case 1://直接堵
                searchModeTemp = searchModeTemp << 2;
                break;
            case 2://延长
                searchModeTemp = (searchModeTemp << 2) + 2;
                break;
            default:
                break;
            }
            //right
            switch (type & 3)//searchModeTemp length = 7 
            {
            case 0://不堵
                searchModeTemp |= 1 << 5 * 2;
                break;
            case 1://直接堵
                break;
            case 2://延长
                searchModeTemp |= 2 << 5 * 2;
                break;
            default:
                break;
            }

            searchModeTemp = searchModeTemp << 4 * 2;
            uint16_t index = type;
            index = (index << 5) + chess;
            for (int i = 0; i < 5; ++i)
            {
                if ((chess >> i) & 0x1)
                {
                    chessModeTable[index][i] = MODE_BASE_0;
                }
                else
                {
                    SearchResult ret = searchTrieTree->search((searchModeTemp >> i * 2) | (2 << 5 * 2));
                    if (ret.chessMode == TRIE_6_CONTINUE)
                    {
                        chessModeTable[index][i] = MODE_BASE_6_b;
                    }
                    else if (ret.chessMode == TRIE_5_CONTINUE)
                    {
                        chessModeTable[index][i] = MODE_BASE_5;
                    }
                    else if (ret.chessMode == TRIE_4_CONTINUE_BAN || ret.chessMode == TRIE_4_CONTINUE_BAN_R)
                    {
                        chessModeTable[index][i] = MODE_BASE_4_b;
                    }
                    else if (ret.chessMode == TRIE_4_BLANK_BAN || ret.chessMode == TRIE_4_BLANK_BAN_R)
                    {
                        chessModeTable[index][i] = MODE_BASE_d4p_b;
                    }
                    else if (ret.chessMode >= TRIE_4_CONTINUE_DEAD_BAN && ret.chessMode <= TRIE_4_BLANK_DEAD_BAN_R)
                    {
                        chessModeTable[index][i] = MODE_BASE_d4_b;
                    }
                    else if (ret.chessMode == TRIE_4_CONTINUE)
                    {
                        chessModeTable[index][i] = MODE_BASE_4;
                    }
                    else if (ret.chessMode == TRIE_4_BLANK || ret.chessMode == TRIE_4_BLANK_R)
                    {
                        chessModeTable[index][i] = MODE_BASE_d4p;
                    }
                    else if (ret.chessMode >= TRIE_4_CONTINUE_DEAD && ret.chessMode <= TRIE_4_BLANK_M)
                    {
                        chessModeTable[index][i] = MODE_BASE_d4;
                    }
                    else if (ret.chessMode >= TRIE_3_CONTINUE && ret.chessMode <= TRIE_3_BLANK_R)
                    {
                        chessModeTable[index][i] = MODE_BASE_3;
                    }
                    else if (ret.chessMode == TRIE_3_BLANK_DEAD2 || ret.chessMode == TRIE_3_BLANK_DEAD2_R)
                    {
                        chessModeTable[index][i] = MODE_BASE_d3p;
                    }
                    else if (ret.chessMode >= TRIE_3_CONTINUE_F && ret.chessMode <= TRIE_3_BLANK_DEAD1_R)
                    {
                        chessModeTable[index][i] = MODE_BASE_d3;
                    }
                    else if (ret.chessMode == TRIE_2_CONTINUE)
                    {
                        chessModeTable[index][i] = MODE_BASE_2;
                    }
                    else if (ret.chessMode == TRIE_2_BLANK)
                    {
                        chessModeTable[index][i] = MODE_BASE_j2;
                    }
                    else
                    {
                        chessModeTable[index][i] = MODE_BASE_0;
                    }
                }
            }

        }
    }
}

void ChessBoard::updatePoint_layer2(Position2 index)
{
    for (int d = 0; d < DIRECTION4_COUNT; ++d)
    {
        uint16_t chessMode = 0;
        uint16_t type = 0;
        Position2 temp(0);
        temp = Position2(index.index - direction_offset_index[d] * 5);
        if (temp.index < index.index && temp.valid())
        {
            if (pieces_layer1[temp.index] == 0)
            {
                type |= 2 >> 2;
            }
            else if (pieces_layer1[temp.index] == 1)
            {
                type |= 1 >> 2;
            }
            else // 2 empty
            {
                type |= 0 >> 2;
            }
        }
        else
        {
            //左边为x
            type |= 1 >> 2;
        }

        temp = Position2(index.index + direction_offset_index[d] * 5);
        if (temp.index > index.index && temp.valid())
        {
            if (pieces_layer1[temp.index] == 0)
            {
                type |= 2;
            }
            else if (pieces_layer1[temp.index] == 1)
            {
                type |= 1;
            }
            else // 2 empty
            {
                type |= 0;
            }
        }
        else
        {
            //右边为x
            type |= 1;
        }
    }
}
void ChessBoard::updateArea_layer2(Position2 index)
{

}
void ChessBoard::updatePoint_layer3(Position2 index)
{

}
void ChessBoard::updateArea_layer3(Position2 index)
{

}


int ChessBoard::setThreat(const int& row, const int& col, const int& side, bool defend)
{
    int score = 0;
    pieces[row][col].state = side;
    score = getStepScores(row, col, side, defend);
    pieces[row][col].state = (0);
    pieces[row][col].setThreat(score, side);
    return score;
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

int ChessBoard::updateThreat(const int& row, const int& col, const int& side, bool defend)
{
    int result = 0;
    int blankCount, chessCount, r, c;
    for (int i = 0; i < DIRECTION8_COUNT; ++i)//8个方向
    {
        r = row, c = col;
        blankCount = 0;
        chessCount = 0;
        while (nextPosition(r, c, 1, i)) //如果不超出边界
        {
            if (pieces[r][c].state == 0)
            {
                blankCount++;
                if (pieces[r][c].hot)
                {
                    result += setThreat(r, c, side, defend);
                }
            }
            else if (pieces[r][c].state == -side)
            {
                break;//遇到敌方棋子，停止搜索
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
    return result;
}

void ChessBoard::updateHotArea(int row, int col)
{
    int range = 5;//2 * 2 + 1
    int tempcol, temprow;
    for (int i = 0; i < range; ++i)
    {
        //横向
        tempcol = col - 2 + i;
        if (tempcol > -1 && tempcol < BOARD_COL_MAX && pieces[row][tempcol].state == 0)
        {
            pieces[row][tempcol].hot = (true);
        }
        //纵向
        temprow = row - 2 + i;
        if (temprow > -1 && temprow < BOARD_ROW_MAX && pieces[temprow][col].state == 0)
        {
            pieces[temprow][col].hot = (true);
        }
        //右下
        if (temprow > -1 && temprow < BOARD_ROW_MAX && tempcol> -1 && tempcol < BOARD_COL_MAX && pieces[temprow][tempcol].state == 0)
        {
            pieces[temprow][tempcol].hot = (true);
        }
        //右上
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

void ChessBoard::formatChessInt(uint32_t chessInt, char chessStr[FORMAT_LENGTH])
{
    for (int i = 0; i < FORMAT_LENGTH; ++i)
    {
        chessStr[i] = (chessInt >> i * 2) & 3 + '/'; //取最后两位
    }
}

void ChessBoard::formatChess2Int(uint32_t chessInt[DIRECTION4_COUNT], const int& row, const int& col, const int& state)
{
    //chessInt需要初始化为0
    int rowstart = row - SEARCH_LENGTH, colstart = col - SEARCH_LENGTH, rowend = row + SEARCH_LENGTH;

    for (int i = 0; i < FORMAT_LENGTH; ++i, ++rowstart, ++colstart, --rowend)
    {
        //横向
        if (colstart < 0 || colstart > 14)
        {
            //chessInt[DIRECTION4_R] |= 0 << i * 2;
        }
        else
        {
            chessInt[DIRECTION4_R] |= (pieces[row][colstart].state*state + 1) << i * 2;
        }

        //纵向
        if (rowstart < 0 || rowstart > 14)
        {
            //chessInt[DIRECTION4_D] |= 0 << i * 2;
        }
        else
        {
            chessInt[DIRECTION4_D] |= (pieces[rowstart][col].state*state + 1) << i * 2;
        }
        //右下向
        if (colstart < 0 || rowstart < 0 || colstart > 14 || rowstart > 14)
        {
            //chessInt[DIRECTION4_RD] |= 0 << i * 2;
        }
        else
        {
            chessInt[DIRECTION4_RD] |= (pieces[rowstart][colstart].state*state + 1) << i * 2;
        }

        //右上向
        if (colstart < 0 || rowend > 14 || colstart > 14 || rowend < 0)
        {
            //chessInt[DIRECTION4_RU] |= 0 << i * 2;
        }
        else
        {
            chessInt[DIRECTION4_RU] |= (pieces[rowend][colstart].state*state + 1) << i * 2;
        }
    }
}

int ChessBoard::getStepScores(const int& row, const int& col, const int& state, const bool& isdefend)
{
    int stepScore = 0;
    uint32_t direction[DIRECTION4_COUNT] = { 0 };//四个方向棋面（0表示空，-1表示断，1表示连）
    formatChess2Int(direction, row, col, state);
    uint8_t chessModeCount[TRIE_COUNT] = { 0 };
    SearchResult result;
    for (int i = 0; i < DIRECTION4_COUNT; ++i)
    {

        result = searchTrieTree->searchAC(direction[i]);

        if (result.chessMode < 0)
        {
            continue;
        }
        //handle result
        if (result.chessMode == TRIE_5_CONTINUE)
        {
            return chessMode[TRIE_5_CONTINUE].evaluation;//没必要算了
        }
        else if (result.chessMode >= TRIE_4_CONTINUE)//不需要特殊处理
        {
            chessModeCount[result.chessMode]++;
            continue;
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

bool ChessBoard::nextPosition(int& row, int& col, int i, int direction)
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
    pair.z32key ^= z32[row][col][1];//原来是空的
    pair.z32key ^= z32[row][col][side + 1];

    pair.z64key ^= z64[row][col][1];
    pair.z64key ^= z64[row][col][side + 1];
}
