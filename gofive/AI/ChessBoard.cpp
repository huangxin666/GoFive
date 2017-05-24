#include "ChessBoard.h"
#include <random>
using namespace std;

bool ChessBoard::ban = false;
int8_t ChessBoard::level = AILEVEL_UNLIMITED;
TrieTreeNode* ChessBoard::searchTrieTree = NULL;
string ChessBoard::debugInfo = "";
uint32_t ChessBoard::z32[BOARD_ROW_MAX][BOARD_COL_MAX][3] = { 0 };
uint64_t ChessBoard::z64[BOARD_ROW_MAX][BOARD_COL_MAX][3] = { 0 };
uint8_t* ChessBoard::chessModeHashTable[16];

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

void ChessBoard::init_layer2()
{
    for (uint8_t i = 0; i < 225; ++i)
    {
        if (pieces_layer1[i] == PIECE_BLANK)
        {
            continue;
        }
        update_layer2(Position2(i), PIECE_BLACK);
        update_layer2(Position2(i), PIECE_WHITE);
    }
}

void ChessBoard::update_layer2(Position2 index, int side)
{

    for (int d = 0; d < DIRECTION4_COUNT; ++d)
    {
        uint16_t l_hash_index = 0, r_hash_index = 0;
        int l_offset = 0, r_offset = 0;
        while (1)//向左
        {
            Position2 temp = Position2(index.index - direction_offset_index[d] * (l_offset + 1));
            if (temp.index > index.index)// out of range
            {
                break;
            }
            if (pieces_layer1[temp.index] == REVERSESTATE(side))
            {
                break;
            }

            if (pieces_layer1[temp.index] == side)
            {
                l_hash_index |= 1 << l_offset;
            }
            l_offset++;
        }
        while (1)//向右
        {
            Position2 temp = Position2(index.index + direction_offset_index[d] * (r_offset + 1));

            if (temp.index < index.index || !temp.valid())// out of range
            {
                break;
            }

            if (pieces_layer1[temp.index] == REVERSESTATE(side))
            {
                break;
            }

            if (pieces_layer1[temp.index] == side)
            {
                r_hash_index <<= 1;
                r_hash_index += 1;
            }
            else
            {
                r_hash_index <<= 1;
            }
            r_offset++;
        }

        if (pieces_layer1[index.index] == REVERSESTATE(side))
        {
            int len = l_offset;
            int index_offset = l_hash_index * len;
            int index_fix = index.index - direction_offset_index[d] * l_offset;
            //update
            for (int i = 0; i < len; ++i, index_fix += direction_offset_index[d])
            {
                if (len > 4)
                {
                    pieces_layer2[index_fix][d][side] = chessModeHashTable[len][index_offset + i];
                }
                else
                {
                    pieces_layer2[index_fix][d][side] = 0;
                }
            }

            len = r_offset;
            index_offset = r_hash_index * len;
            index_fix = index.index + direction_offset_index[d];
            //update
            for (int i = 0; i < len; ++i, index_fix += direction_offset_index[d])
            {
                if (len > 4)
                {
                    pieces_layer2[index_fix][d][side] = chessModeHashTable[len][index_offset + i];
                }
                else
                {
                    pieces_layer2[index_fix][d][side] = 0;
                }
            }
        }
        else
        {
            int len = l_offset + r_offset + 1;
            int index_offset = ((((l_hash_index << 1) + (pieces_layer1[index.index] == side ? 1 : 0)) << r_offset) + r_hash_index) *len;
            int index_fix = index.index - direction_offset_index[d] * l_offset;
            //update
            for (int i = 0; i < len; ++i, index_fix += direction_offset_index[d])
            {
                if (len > 4)
                {
                    pieces_layer2[index_fix][d][side] = chessModeHashTable[len][index_offset + i];
                }
                else
                {
                    pieces_layer2[index_fix][d][side] = 0;
                }
            }
        }
    }
}



void ChessBoard::updatePoint_layer3(Position2 index, int side)
{
    if (pieces_layer1[index.index] != PIECE_BLANK)
    {
        return;
    }
    uint8_t chessModeCount[MODE_COUNT] = { 0 };//BASEMODE
    for (int d = 0; d < 4; ++d)
    {
        chessModeCount[pieces_layer2[index.index][d][side]]++;
    }
    int alive_three = chessModeCount[MODE_BASE_3];
    int dead_four = chessModeCount[MODE_BASE_d4] + chessModeCount[MODE_BASE_d4p] + 2 * chessModeCount[MODE_BASE_t4];

    //禁手检测
    if (ban && side == PIECE_BLACK)
    {
        dead_four += chessModeCount[MODE_BASE_4_b];
        if (chessModeCount[MODE_BASE_6_b] > 0 ||
            (chessModeCount[MODE_BASE_5] == 0 &&
                (alive_three > 1 || dead_four > 1)
            ))
        {
            pieces_layer3[index.index][side] = MODE_ADV_BAN;
        }
    }
    else
    {

    }
}

void ChessBoard::updateArea_layer3(Position2 index, int side)
{

}


bool ChessBoard::move(Position2 index, int side)
{

}

bool ChessBoard::unmove()
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
