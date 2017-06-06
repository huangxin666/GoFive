#include "ChessBoard.h"
#include <random>
using namespace std;

bool ChessBoard::ban = false;
int8_t ChessBoard::level = AILEVEL_UNLIMITED;
TrieTreeNode* ChessBoard::searchTrieTree = NULL;
string ChessBoard::debugInfo = "";
uint32_t ChessBoard::z32[BOARD_ROW_MAX][BOARD_COL_MAX][3] = { 0 };
uint64_t ChessBoard::z64[BOARD_ROW_MAX][BOARD_COL_MAX][3] = { 0 };
uint8_t* ChessBoard::chessModeHashTable[16] = { 0 };
uint8_t* ChessBoard::chessModeHashTableBan[16] = { 0 };

ChessBoard::ChessBoard()
{
    lastStep.step = 0;
    lastStep.black = false;
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

void ChessBoard::initBoard()
{
    init_layer1();
    initHash();
    initTotalRatings();
    initHighestRatings();
}

void ChessBoard::init_layer1()
{
    for (uint8_t i = 0; i < 225; ++i)
    {
        pieces_layer1[i] = PIECE_BLANK;
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
        update_layer2(i, PIECE_BLACK);
        update_layer2(i, PIECE_WHITE);
    }
}

void ChessBoard::update_layer2(uint8_t index, int side)//落子处
{
    Position pos = { util::getRow(index),util::getCol(index) };
    Position temp;
    for (int d = 0; d < DIRECTION4_COUNT; ++d)
    {
        uint16_t l_hash_index = 0, r_hash_index = 0;
        int l_offset = 0, r_offset = 0;
        while (1)//向左
        {
            temp = pos.getNextPosition(d, -(l_offset + 1));
            if (!temp.valid())// out of range
            {
                break;
            }
            if (pieces_layer1[temp.toIndex()] == util::otherside(side))
            {
                break;
            }

            if (pieces_layer1[temp.toIndex()] == side)
            {
                l_hash_index |= 1 << l_offset;
            }
            l_offset++;
        }
        temp = pos;
        while (1)//向右
        {
            temp = pos.getNextPosition(d, (r_offset + 1));

            if (!temp.valid())// out of range
            {
                break;
            }

            if (pieces_layer1[temp.toIndex()] == util::otherside(side))
            {
                break;
            }

            if (pieces_layer1[temp.toIndex()] == side)
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

        if (pieces_layer1[index] == util::otherside(side))
        {
            int len = l_offset;
            int index_offset = l_hash_index * len;
            uint8_t index_fix = index - direction_offset_index[d] * l_offset;
            //update
            for (int i = 0; i < len; ++i, index_fix += direction_offset_index[d])
            {
                if (len > 4)
                {
                    pieces_layer2[index_fix][d][side] = (ban && side == PIECE_BLACK) ? chessModeHashTableBan[len][index_offset + i] : chessModeHashTable[len][index_offset + i];
                }
                else
                {
                    pieces_layer2[index_fix][d][side] = 0;
                }
            }

            len = r_offset;
            index_offset = r_hash_index * len;
            index_fix = index + direction_offset_index[d];
            //update
            for (int i = 0; i < len; ++i, index_fix += direction_offset_index[d])
            {
                if (len > 4)
                {
                    pieces_layer2[index_fix][d][side] = (ban && side == PIECE_BLACK) ? chessModeHashTableBan[len][index_offset + i] : chessModeHashTable[len][index_offset + i];
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
            int index_offset = ((((l_hash_index << 1) + (pieces_layer1[index] == side ? 1 : 0)) << r_offset) + r_hash_index) * len;
            int index_fix = index - direction_offset_index[d] * l_offset;
            //update
            for (int i = 0; i < len; ++i, index_fix += direction_offset_index[d])
            {
                if (len > 4)
                {
                    pieces_layer2[index_fix][d][side] = (ban && side == PIECE_BLACK) ? chessModeHashTableBan[len][index_offset + i] : chessModeHashTable[len][index_offset + i];
                }
                else
                {
                    pieces_layer2[index_fix][d][side] = 0;
                }
            }
        }
    }
}

void ChessBoard::init_layer3()
{
    for (uint8_t index = 0; util::valid(index); ++index)
    {
        updatePoint_layer3(index, PIECE_BLACK);
        updatePoint_layer3(index, PIECE_WHITE);
    }
}

void ChessBoard::updatePoint_layer3(uint8_t index, int side)//空白处
{
    if (pieces_layer1[index] != PIECE_BLANK)
    {
        return;
    }
    uint8_t count[CHESSTYPE_COUNT] = { 0 };//BASEMODE
    uint8_t result = 0;
    int deadfour = 0, alivethree = 0;
    for (int d = 0; d < 4; ++d)
    {
        if (pieces_layer2[index][d][side] < CHESSTYPE_5)
        {
            if (pieces_layer2[index][d][side] > result)
            {
                result = pieces_layer2[index][d][side];
            }
            if (pieces_layer2[index][d][side] == CHESSTYPE_3)
            {
                alivethree++;
            }
            if (pieces_layer2[index][d][side] == CHESSTYPE_D4
                || pieces_layer2[index][d][side] == CHESSTYPE_D4P)
            {
                deadfour++;
            }
        }
        else if (pieces_layer2[index][d][side] == CHESSTYPE_5)//5连最大
        {
            pieces_layer3[index][side] = CHESSTYPE_5;
            return;
        }
        else if (pieces_layer2[index][d][side] == CHESSTYPE_BAN)
        {
            pieces_layer3[index][side] = CHESSTYPE_BAN;
            return;
        }
        else if (pieces_layer2[index][d][side] == CHESSTYPE_44)//双四相当于活四，非禁手条件
        {
            pieces_layer3[index][side] = CHESSTYPE_44;
            return;
        }
    }
    pieces_layer3[index][side] = result;
    //组合棋型
    if (ban && side == PIECE_BLACK)
    {
        if (result == CHESSTYPE_4)
        {
            deadfour++;
        }

        if (deadfour > 1)
        {
            pieces_layer3[index][side] = CHESSTYPE_BAN;
        }
        else if (alivethree > 1)
        {
            pieces_layer3[index][side] = CHESSTYPE_BAN;
        }
        else if (deadfour == 1 && alivethree == 1 && result != CHESSTYPE_4)
        {
            pieces_layer3[index][side] = CHESSTYPE_43;
        }
    }
    else
    {
        if (result == CHESSTYPE_4)
        {
            return;
        }

        if (deadfour > 1)
        {
            pieces_layer3[index][side] = CHESSTYPE_44;
        }
        else if (deadfour == 1 && alivethree > 0)
        {
            pieces_layer3[index][side] = CHESSTYPE_43;
        }
        else if (alivethree > 1)
        {
            pieces_layer3[index][side] = CHESSTYPE_33;
        }
    }

}

void ChessBoard::updateArea_layer3(uint8_t index, uint8_t side)//落子处
{
    int blankCount, chessCount, r, c;
    int8_t row = util::getRow(index);
    int8_t col = util::getCol(index);
    if (pieces_layer1[index] == PIECE_BLANK)
    {
        //ratings[side] -= chess_ratings[pieces_layer3[index][side]];
        updatePoint_layer3(index, side);
        totalRatings[side] += chesstype2rating[pieces_layer3[index][side]];
    }
    else
    {
        totalRatings[side] -= chesstype2rating[pieces_layer3[index][side]];
    }

    for (int i = 0; i < DIRECTION8_COUNT; ++i)//8个方向
    {
        r = row, c = col;
        blankCount = 0;
        chessCount = 0;
        while (nextPosition(r, c, 1, i)) //如果不超出边界
        {
            if (pieces_layer1[util::xy2index(r, c)] == PIECE_BLANK)
            {
                blankCount++;
                if (pieces_hot[util::xy2index(r, c)] || pieces_layer1[index] == PIECE_BLANK)//unmove的时候无视hot flag
                {
                    totalRatings[side] -= chesstype2rating[pieces_layer3[util::xy2index(r, c)][side]];
                    updatePoint_layer3(util::xy2index(r, c), side);
                    totalRatings[side] += chesstype2rating[pieces_layer3[util::xy2index(r, c)][side]];
                }
            }
            else if (pieces_layer1[util::xy2index(r, c)] == util::otherside(side))
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
}

int ChessBoard::getUpdateThreat(uint8_t index, uint8_t side)
{
    int result = 0;
    int blankCount, chessCount, r, c;
    int8_t row = util::getRow(index);
    int8_t col = util::getCol(index);
    for (int i = 0; i < DIRECTION8_COUNT; ++i)//8个方向
    {
        r = row, c = col;
        blankCount = 0;
        chessCount = 0;
        while (nextPosition(r, c, 1, i)) //如果不超出边界
        {
            if (pieces_layer1[util::xy2index(r, c)] == PIECE_BLANK)
            {
                blankCount++;
                if (pieces_hot[util::xy2index(r, c)])
                {
                    result += chesstype2rating[pieces_layer3[util::xy2index(r, c)][side]];
                }
            }
            else if (pieces_layer1[util::xy2index(r, c)] == util::otherside(side))
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

void ChessBoard::updateHotArea(uint8_t index)
{
    int row = util::getRow(index);
    int col = util::getCol(index);
    int range = 5;//2 * 2 + 1
    int tempcol, temprow;
    for (int i = 0; i < range; ++i)
    {
        //横向
        tempcol = col - 2 + i;
        if (tempcol > -1 && tempcol < BOARD_COL_MAX && pieces_layer1[util::xy2index(row, tempcol)] == PIECE_BLANK)
        {
            setHot(row, tempcol, true);
        }
        //纵向
        temprow = row - 2 + i;
        if (temprow > -1 && temprow < BOARD_ROW_MAX && pieces_layer1[util::xy2index(temprow, col)] == PIECE_BLANK)
        {
            setHot(temprow, col, true);
        }
        //右下
        if (temprow > -1 && temprow < BOARD_ROW_MAX && tempcol> -1 && tempcol < BOARD_COL_MAX &&pieces_layer1[util::xy2index(temprow, tempcol)] == PIECE_BLANK)
        {
            setHot(temprow, tempcol, true);
        }
        //右上
        temprow = row + 2 - i;
        if (temprow > -1 && temprow < BOARD_ROW_MAX && tempcol> -1 && tempcol < BOARD_COL_MAX && pieces_layer1[util::xy2index(temprow, tempcol)] == PIECE_BLANK)
        {
            setHot(temprow, tempcol, true);
        }
    }
}

void ChessBoard::initHotArea() {
    for (uint8_t index = 0; util::valid(index); ++index)
    {
        pieces_hot[index] = false;
    }
    for (uint8_t index = 0; util::valid(index); ++index)
    {
        if (pieces_layer1[index] != PIECE_BLANK)
        {
            updateHotArea(index);
        }
    }
}

bool ChessBoard::move(uint8_t index)
{
    if (pieces_layer1[index] != PIECE_BLANK || lastStep.step > 224)
    {
        return false;//已有棋子
    }
    lastStep.step++;
    lastStep.black = !lastStep.black;
    lastStep.index = index;

    pieces_layer1[index] = lastStep.getColor();
    updateHotArea(index);
    update_layer2(index);
    updateArea_layer3(index);//and update highest ratings
    initHighestRatings();
    updateHashPair(util::getRow(index), util::getCol(index), lastStep.getColor());

    return true;
}

bool ChessBoard::unmove(uint8_t index, uint8_t lastIndex)
{
    uint8_t side = pieces_layer1[index];
    if (side == PIECE_BLANK || lastStep.step < 1)
    {
        return false;//没有棋子
    }
    lastStep.step--;
    lastStep.black = !lastStep.black;
    lastStep.index = lastIndex;

    pieces_layer1[index] = PIECE_BLANK;
    initHotArea();
    update_layer2(index);
    updateArea_layer3(index);
    initHighestRatings();
    updateHashPair(util::getRow(index), util::getCol(index), side, false);
    return true;
}

void ChessBoard::initTotalRatings()
{
    totalRatings[PIECE_BLACK] = 0;
    totalRatings[PIECE_WHITE] = 0;
    for (uint8_t index = 0; util::valid(index); ++index)
    {
        if (pieces_hot[index] && pieces_layer1[index] == PIECE_BLANK)
        {
            totalRatings[PIECE_BLACK] += chesstype2rating[pieces_layer3[index][PIECE_BLACK]];
            totalRatings[PIECE_WHITE] += chesstype2rating[pieces_layer3[index][PIECE_WHITE]];
        }
    }
}

void ChessBoard::initHighestRatings()
{
    highestRatings[PIECE_BLACK] = { 0,0 };
    highestRatings[PIECE_WHITE] = { 0,0 };
    for (uint8_t index = 0; util::valid(index); ++index)
    {
        if (pieces_hot[index] && pieces_layer1[index] == PIECE_BLANK)
        {
            for (uint8_t side = 0; side < 2; ++side)
            {
                if (chesstype2rating[pieces_layer3[index][side]] > chesstype2rating[highestRatings[side].chessmode])
                {
                    highestRatings[side].chessmode = pieces_layer3[index][side];
                    highestRatings[side].index = index;
                }
            }
        }
    }
}

void ChessBoard::formatChessInt(uint32_t chessInt, char chessStr[FORMAT_LENGTH])
{
    for (int i = 0; i < FORMAT_LENGTH; ++i)
    {
        chessStr[i] = (chessInt >> i * 2) & 3 + '/'; //取最后两位
    }
}

void ChessBoard::formatChess2Int(uint32_t chessInt[DIRECTION4_COUNT], int row, int col, int side)
{
    //chessInt需要初始化为0
    int rowstart = row - SEARCH_LENGTH, colstart = col - SEARCH_LENGTH, rowend = row + SEARCH_LENGTH;

    for (int i = 0; i < FORMAT_LENGTH; ++i, ++rowstart, ++colstart, --rowend)
    {
        //横向
        if (colstart < 0 || colstart > 14)
        {
            chessInt[DIRECTION4_R] |= PIECE_WHITE << i * 2;
        }
        else
        {
            if (pieces_layer1[util::xy2index(row, colstart)] == PIECE_BLANK)
            {
                chessInt[DIRECTION4_R] |= (PIECE_BLANK) << i * 2;
            }
            else
            {
                chessInt[DIRECTION4_R] |= (pieces_layer1[util::xy2index(row, colstart)] == side ? PIECE_BLACK : PIECE_WHITE) << i * 2;
            }

        }

        //纵向
        if (rowstart < 0 || rowstart > 14)
        {
            chessInt[DIRECTION4_D] |= PIECE_WHITE << i * 2;
        }
        else
        {
            if (pieces_layer1[util::xy2index(rowstart, col)] == PIECE_BLANK)
            {
                chessInt[DIRECTION4_D] |= (PIECE_BLANK) << i * 2;
            }
            else
            {
                chessInt[DIRECTION4_D] |= (pieces_layer1[util::xy2index(rowstart, col)] == side ? 0 : 1) << i * 2;
            }
        }
        //右下向
        if (colstart < 0 || rowstart < 0 || colstart > 14 || rowstart > 14)
        {
            chessInt[DIRECTION4_RD] |= PIECE_WHITE << i * 2;
        }
        else
        {
            if (pieces_layer1[util::xy2index(rowstart, colstart)] == PIECE_BLANK)
            {
                chessInt[DIRECTION4_RD] |= (PIECE_BLANK) << i * 2;
            }
            else
            {
                chessInt[DIRECTION4_RD] |= (pieces_layer1[util::xy2index(rowstart, colstart)] == side ? 0 : 1) << i * 2;
            }

        }

        //右上向
        if (colstart < 0 || rowend > 14 || colstart > 14 || rowend < 0)
        {
            chessInt[DIRECTION4_RU] |= PIECE_WHITE << i * 2;
        }
        else
        {
            if (pieces_layer1[util::xy2index(rowend, colstart)] == PIECE_BLANK)
            {
                chessInt[DIRECTION4_RU] |= (PIECE_BLANK) << i * 2;
            }
            else
            {
                chessInt[DIRECTION4_RU] |= (pieces_layer1[util::xy2index(rowend, colstart)] == side ? 0 : 1) << i * 2;
            }
        }
    }
}

//int ChessBoard::getStepScores(const int& row, const int& col, const int& state, const bool& isdefend)
//{
//    int stepScore = 0;
//    uint32_t direction[DIRECTION4_COUNT] = { 0 };//四个方向棋面（0表示空，-1表示断，1表示连）
//    formatChess2Int(direction, row, col, state);
//    uint8_t chessModeCount[TRIE_COUNT] = { 0 };
//    SearchResult result;
//    for (int i = 0; i < DIRECTION4_COUNT; ++i)
//    {
//
//        result = searchTrieTree->searchAC(direction[i]);
//
//        if (result.chessMode < 0)
//        {
//            continue;
//        }
//        //handle result
//        if (result.chessMode == TRIE_5_CONTINUE)
//        {
//            return chessMode[TRIE_5_CONTINUE].evaluation;//没必要算了
//        }
//        else if (result.chessMode >= TRIE_4_CONTINUE)//不需要特殊处理
//        {
//            chessModeCount[result.chessMode]++;
//            continue;
//        }
//
//        //处理特殊棋型
//        result.pos = chessMode[result.chessMode].pat_len - (result.pos - SEARCH_LENGTH);
//        stepScore = handleSpecial(result, state, chessModeCount);
//        if (stepScore)
//        {
//            return stepScore;
//        }
//    }
//
//    //组合棋型
//    int deadFour =
//        chessModeCount[TRIE_4_CONTINUE_DEAD] +
//        chessModeCount[TRIE_4_CONTINUE_DEAD_R] +
//        chessModeCount[TRIE_4_BLANK] +
//        chessModeCount[TRIE_4_BLANK_R] +
//        chessModeCount[TRIE_4_BLANK_DEAD] +
//        chessModeCount[TRIE_4_BLANK_DEAD_R] +
//        chessModeCount[TRIE_4_BLANK_M];
//
//    if (deadFour + chessModeCount[TRIE_4_CONTINUE] > 1)//双四
//    {
//        if (ban&&state == STATE_CHESS_BLACK)//有禁手
//        {
//            return BAN_DOUBLEFOUR;
//        }
//    }
//
//    if (deadFour > 1 && level >= AILEVEL_INTERMEDIATE) //双死四且无禁手
//    {
//        stepScore += 10001;
//        deadFour = 0;
//        chessModeCount[TRIE_4_CONTINUE_DEAD] = 0;
//        chessModeCount[TRIE_4_CONTINUE_DEAD_R] = 0;
//        chessModeCount[TRIE_4_BLANK] = 0;
//        chessModeCount[TRIE_4_BLANK_R] = 0;
//        chessModeCount[TRIE_4_BLANK_DEAD] = 0;
//        chessModeCount[TRIE_4_BLANK_DEAD_R] = 0;
//        chessModeCount[TRIE_4_BLANK_M] = 0;
//    }
//
//
//    int aliveThree =
//        chessModeCount[TRIE_3_CONTINUE] +
//        chessModeCount[TRIE_3_CONTINUE_R] +
//        chessModeCount[TRIE_3_BLANK] +
//        chessModeCount[TRIE_3_BLANK_R];
//
//    if (aliveThree == 1 && deadFour == 1 && (level >= AILEVEL_HIGH))//死四活三
//    {
//        stepScore += 10001;
//        deadFour = 0;
//        aliveThree = 0;
//        chessModeCount[TRIE_4_CONTINUE_DEAD] = 0;
//        chessModeCount[TRIE_4_CONTINUE_DEAD_R] = 0;
//        chessModeCount[TRIE_4_BLANK] = 0;
//        chessModeCount[TRIE_4_BLANK_R] = 0;
//        chessModeCount[TRIE_4_BLANK_DEAD] = 0;
//        chessModeCount[TRIE_4_BLANK_DEAD_R] = 0;
//        chessModeCount[TRIE_4_BLANK_M] = 0;
//        chessModeCount[TRIE_3_CONTINUE] = 0;
//        chessModeCount[TRIE_3_CONTINUE_R] = 0;
//        chessModeCount[TRIE_3_BLANK] = 0;
//        chessModeCount[TRIE_3_BLANK_R] = 0;
//    }
//
//    if (aliveThree > 1)//双活三
//    {
//        if (ban&&state == STATE_CHESS_BLACK)//有禁手
//        {
//            return BAN_DOUBLETHREE;
//        }
//        if (level >= AILEVEL_INTERMEDIATE)
//        {
//            stepScore += 8000;
//            chessModeCount[TRIE_3_CONTINUE] = 0;
//            chessModeCount[TRIE_3_CONTINUE_R] = 0;
//            chessModeCount[TRIE_3_BLANK] = 0;
//            chessModeCount[TRIE_3_BLANK_R] = 0;
//        }
//    }
//
//    if (isdefend)
//    {
//        for (int i = TRIE_4_CONTINUE; i < TRIE_COUNT; i++)
//        {
//            if (chessModeCount[i])
//            {
//                stepScore += chessModeCount[i] * chessMode[i].evaluation_defend;
//            }
//        }
//    }
//    else
//    {
//        for (int i = TRIE_4_CONTINUE; i < TRIE_COUNT; i++)
//        {
//            if (chessModeCount[i])
//            {
//                stepScore += chessModeCount[i] * chessMode[i].evaluation;
//            }
//        }
//    }
//
//    return stepScore;
//}
//
//int ChessBoard::handleSpecial(const SearchResult &result, const int &state, uint8_t chessModeCount[TRIE_COUNT])
//{
//    switch (result.chessMode)
//    {
//    case TRIE_6_CONTINUE:
//        if (ban&&state == STATE_CHESS_BLACK)
//        {
//            return BAN_LONGCONTINUITY;
//        }
//        else
//        {
//            return chessMode[TRIE_5_CONTINUE].evaluation;//没必要算了
//        }
//        break;
//    case TRIE_4_DOUBLE_BAN1:
//        if (result.pos < 6 && result.pos > 2)
//        {
//            if (ban&&state == STATE_CHESS_BLACK)
//            {
//                return BAN_DOUBLEFOUR;
//            }
//            else
//            {
//                chessModeCount[TRIE_4_BLANK_DEAD]++;
//                chessModeCount[TRIE_4_BLANK_DEAD_R]++;
//            }
//        }
//        else
//        {
//            chessModeCount[TRIE_4_BLANK_DEAD]++;
//        }
//        break;
//    case TRIE_4_DOUBLE_BAN2:
//        if (result.pos < 6 && result.pos > 3)
//        {
//            if (ban&&state == STATE_CHESS_BLACK)
//            {
//                return BAN_DOUBLEFOUR;
//            }
//            else
//            {
//                chessModeCount[TRIE_4_BLANK_M] += 2;
//            }
//        }
//        else
//        {
//            chessModeCount[TRIE_4_BLANK_M]++;
//        }
//        break;
//    case TRIE_4_DOUBLE_BAN3:
//        if (result.pos == 5)
//        {
//            if (ban&&state == STATE_CHESS_BLACK)
//            {
//                return BAN_DOUBLEFOUR;
//            }
//            else
//            {
//                chessModeCount[TRIE_4_BLANK_DEAD]++;
//                chessModeCount[TRIE_4_BLANK_DEAD_R]++;
//            }
//        }
//        else
//        {
//            chessModeCount[TRIE_4_BLANK_DEAD]++;
//        }
//        break;
//    case TRIE_4_CONTINUE_BAN:
//        if (result.pos > 2)
//        {
//            if (ban&&state == STATE_CHESS_BLACK)
//            {
//                chessModeCount[TRIE_4_CONTINUE_DEAD]++;
//            }
//            else
//            {
//                chessModeCount[TRIE_4_CONTINUE]++;
//            }
//        }
//        else
//        {
//            if (ban&&state == STATE_CHESS_BLACK)
//            {
//                //无效步，已经可以赢了，不能走这里
//            }
//            else
//            {
//                chessModeCount[TRIE_4_BLANK_DEAD]++;
//            }
//        }
//        break;
//    case TRIE_4_CONTINUE_BAN_R:
//        if (result.pos < 6)
//        {
//            if (ban&&state == STATE_CHESS_BLACK)
//            {
//                chessModeCount[TRIE_4_CONTINUE_DEAD]++;
//            }
//            else
//            {
//                chessModeCount[TRIE_4_CONTINUE]++;
//            }
//        }
//        else
//        {
//            if (ban&&state == STATE_CHESS_BLACK)
//            {
//                //无效步，已经可以赢了，不能走这里
//            }
//            else
//            {
//                chessModeCount[TRIE_4_BLANK_DEAD]++;
//            }
//        }
//        break;
//    case TRIE_4_CONTINUE_DEAD_BAN:
//        if (result.pos > 2)
//        {
//            if (ban&&state == STATE_CHESS_BLACK)
//            {
//                //无效步
//            }
//            else
//            {
//                chessModeCount[TRIE_4_CONTINUE_DEAD]++;
//            }
//        }
//        else
//        {
//            if (ban&&state == STATE_CHESS_BLACK)
//            {
//                //无效步，已经可以赢了，不能走这里
//            }
//            else
//            {
//                chessModeCount[TRIE_4_BLANK_DEAD]++;
//            }
//        }
//        break;
//    case TRIE_4_CONTINUE_DEAD_BAN_R:
//        if (result.pos < 6)
//        {
//            if (ban&&state == STATE_CHESS_BLACK)
//            {
//                //无效步
//            }
//            else
//            {
//                chessModeCount[TRIE_4_CONTINUE_DEAD]++;
//            }
//        }
//        else
//        {
//            if (ban&&state == STATE_CHESS_BLACK)
//            {
//                //无效步，已经可以赢了，不能走这里
//            }
//            else
//            {
//                chessModeCount[TRIE_4_BLANK_DEAD]++;
//            }
//        }
//        break;
//    case TRIE_4_BLANK_BAN:
//        if (result.pos > 3)
//        {
//            if (ban&&state == STATE_CHESS_BLACK)
//            {
//                chessModeCount[TRIE_3_CONTINUE_DEAD]++;
//            }
//            else
//            {
//                chessModeCount[TRIE_4_BLANK]++;
//            }
//        }
//        else
//        {
//            if (ban&&state == STATE_CHESS_BLACK)
//            {
//                //无效步
//            }
//            else
//            {
//                chessModeCount[TRIE_4_BLANK_M]++;
//            }
//        }
//        break;
//    case TRIE_4_BLANK_BAN_R:
//        if (result.pos < 6)
//        {
//            if (ban&&state == STATE_CHESS_BLACK)
//            {
//                chessModeCount[TRIE_3_CONTINUE_DEAD]++;
//            }
//            else
//            {
//                chessModeCount[TRIE_4_BLANK]++;
//            }
//        }
//        else
//        {
//            if (ban&&state == STATE_CHESS_BLACK)
//            {
//                //无效步
//            }
//            else
//            {
//                chessModeCount[TRIE_4_BLANK_M]++;
//            }
//        }
//        break;
//    case TRIE_4_BLANK_DEAD_BAN:
//        if (result.pos < 4)
//        {
//            if (ban&&state == STATE_CHESS_BLACK)
//            {
//                //无效步
//            }
//            else
//            {
//                chessModeCount[TRIE_4_BLANK_DEAD]++;
//            }
//        }
//        else
//        {
//            if (ban&&state == STATE_CHESS_BLACK)
//            {
//                //无效步
//            }
//            else
//            {
//                chessModeCount[TRIE_4_BLANK_M]++;
//            }
//        }
//        break;
//    case TRIE_4_BLANK_DEAD_BAN_R:
//        if (result.pos > 3)
//        {
//            if (ban&&state == STATE_CHESS_BLACK)
//            {
//                //无效步
//            }
//            else
//            {
//                chessModeCount[TRIE_4_BLANK_DEAD]++;
//            }
//        }
//        else
//        {
//            if (ban&&state == STATE_CHESS_BLACK)
//            {
//                //无效步
//            }
//            else
//            {
//                chessModeCount[TRIE_4_BLANK_M]++;
//            }
//        }
//        break;
//    default:
//        break;
//    }
//    return 0;
//}

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

void ChessBoard::initHash()
{
    for (uint8_t index = 0; index < 225; ++index)
    {
        hash.z32key ^= z32[util::getRow(index)][util::getCol(index)][pieces_layer1[index]];
        hash.z64key ^= z64[util::getRow(index)][util::getCol(index)][pieces_layer1[index]];
    }
}
void ChessBoard::updateHashPair(uint8_t row, uint8_t col, uint8_t side, bool add)
{
    if (add) //添加棋子
    {
        hash.z32key ^= z32[row][col][PIECE_BLANK];//原来是空的
        hash.z32key ^= z32[row][col][side];
        hash.z64key ^= z64[row][col][PIECE_BLANK];
        hash.z64key ^= z64[row][col][side];
    }
    else //拿走棋子
    {
        hash.z32key ^= z32[row][col][side];       //原来是有子的
        hash.z32key ^= z32[row][col][PIECE_BLANK];
        hash.z64key ^= z64[row][col][side];
        hash.z64key ^= z64[row][col][PIECE_BLANK];
    }
}
