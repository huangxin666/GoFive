#include "ChessBoard.h"
#include "TrieTree.h"
#include <random>
using namespace std;


int8_t Util::BoardSize = 15;
int Util::BoardIndexUpper = 15 * 15;
set<Position> Util::board_range;
void Util::initBoardRange()
{
    board_range.clear();
    for (int row = 0; row < BoardSize; ++row)
    {
        for (int col = 0; col < BoardSize; ++col)
        {
            board_range.insert(Position(row, col));
        }
    }
}

void ChessBoard::initStaticHelper()
{
    initZobrist();
    initLayer2Table();
    init2to3table();
}

bool ChessBoard::ban = false;

uint8_t* ChessBoard::layer2_table[BOARD_SIZE_MAX + 1] = { NULL };
uint8_t* ChessBoard::layer2_table_ban[BOARD_SIZE_MAX + 1] = { NULL };
uint8_t ChessBoard::layer2_to_layer3_table[CHESSTYPE_COUNT][CHESSTYPE_COUNT][CHESSTYPE_COUNT][CHESSTYPE_COUNT][2];

ChessBoard::ChessBoard()
{
    lastStep.step = 0;
    lastStep.state = PIECE_WHITE;//下一个就是黑了
    highestRatings[0].chesstype = CHESSTYPE_0;
    highestRatings[1].chesstype = CHESSTYPE_0;
}

ChessBoard::~ChessBoard()
{

}


void ChessBoard::setBan(bool b)
{
    ban = b;
}

void ChessBoard::initBoard()
{
    init_layer1();
    init_layer2();
    init_layer3();
    init_pattern();
    initHash();
}

void ChessBoard::init_layer1()
{
    ForEachPosition
    {
        pieces_layer1[pos.row][pos.col] = PIECE_BLANK;
    }
}

void ChessBoard::init_layer2()
{
    ForEachPosition
    {
        for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
        {
            pieces_layer2[pos.row][pos.col][d][PIECE_BLACK] = 0;
            pieces_layer2[pos.row][pos.col][d][PIECE_WHITE] = 0;
        }

    }
}


void ChessBoard::init_layer3()
{
    ForEachPosition
    {
        pieces_layer3[pos.row][pos.col][PIECE_BLACK] = 0;
        pieces_layer3[pos.row][pos.col][PIECE_WHITE] = 0;
    }
}

void ChessBoard::init_pattern()
{
    ForEachPosition
    {
        for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
        {
            pieces_pattern[pos.row][pos.col][d][PIECE_BLACK] = 0;
            pieces_pattern[pos.row][pos.col][d][PIECE_WHITE] = 0;


            pieces_pattern_offset[pos.row][pos.col][d][PIECE_BLACK][0] = 0;
            pieces_pattern_offset[pos.row][pos.col][d][PIECE_WHITE][0] = 0;
            pieces_pattern_offset[pos.row][pos.col][d][PIECE_BLACK][1] = 0;
            pieces_pattern_offset[pos.row][pos.col][d][PIECE_WHITE][1] = 0;
            for (int i = 5; i > 0; --i)
            {
                Position temp = pos;
                if (temp.displace4(-1 * i, d))
                {
                    pieces_pattern_offset[pos.row][pos.col][d][PIECE_BLACK][0] = i;
                    pieces_pattern_offset[pos.row][pos.col][d][PIECE_WHITE][0] = i;
                    break;
                }
            }

            for (int i = 5; i > 0; --i)
            {
                Position temp = pos;
                if (temp.displace4(i, d))
                {
                    pieces_pattern_offset[pos.row][pos.col][d][PIECE_BLACK][1] = i;
                    pieces_pattern_offset[pos.row][pos.col][d][PIECE_WHITE][1] = i;
                    break;
                }
            }
        }
    }
}

void ChessBoard::update_pattern(int8_t row, int8_t col)
{
    for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
    {
        pieces_pattern[row][col][d][PIECE_BLACK] = 0;
        pieces_pattern[row][col][d][PIECE_WHITE] = 0;

        pieces_pattern_offset[row][col][d][PIECE_BLACK][0] = 5;
        pieces_pattern_offset[row][col][d][PIECE_WHITE][0] = 5;
        Position temp(row, col);
        for (int i = 0; i < 5; ++i)
        {
            if (!temp.displace4(-1, d))
            {
                pieces_pattern_offset[row][col][d][PIECE_BLACK][0] = i;
                pieces_pattern_offset[row][col][d][PIECE_WHITE][0] = i;
                break;
            }
            if (pieces_layer1[temp.row][temp.col])
            {

            }

        }

        pieces_pattern_offset[row][col][d][PIECE_BLACK][1] = 5;
        pieces_pattern_offset[row][col][d][PIECE_WHITE][1] = 5;
        temp.set(row, col);
        for (int i = 0; i < 5; ++i)
        {
            if (!temp.displace4(d))
            {
                pieces_pattern_offset[row][col][d][PIECE_BLACK][1] = i;
                pieces_pattern_offset[row][col][d][PIECE_WHITE][1] = i;
                break;
            }
        }
    }
}


void ChessBoard::update_layer2_new(int8_t row, int8_t col, uint8_t side)
{
    //unmove
    if (side == PIECE_BLANK)
    {
        update_pattern(row, col);
        for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
        {
            //往左 在temp右边
            Position temp(row, col);
            for (int i = 0; i < 5; ++i)
            {
                if (!temp.displace4(-1, d)) break;
                //update pattern and offset
                if (pieces_layer1[temp.row][temp.col] != PIECE_BLANK)
                {
                    continue;
                }

                update_pattern(temp.row, temp.col);
                //right pattern
                //pieces_pattern[temp.row][temp.col][d][0] &= ~(1 << (6 + i));
                //pieces_pattern[temp.row][temp.col][d][1] &= ~(1 << (6 + i));
                ////right offset
                //if (pieces_pattern_offset[temp.row][temp.col][d][Util::otherside(side)][1] > i)
                //{
                //    pieces_pattern_offset[temp.row][temp.col][d][Util::otherside(side)][1] = i;
                //}

                //update layer2 and layer3
                update_layer3_with_layer2_new(temp.row, temp.col, 0, d);
                update_layer3_with_layer2_new(temp.row, temp.col, 1, d);
            }

            //往右 在temp左边
            temp.set(row, col);
            for (int i = 0; i < 5; ++i)
            {
                if (!temp.displace4(1, d)) break;

                if (pieces_layer1[temp.row][temp.col] != PIECE_BLANK)
                {
                    continue;
                }

                update_pattern(temp.row, temp.col);
                //update left pattern
                //pieces_pattern[temp.row][temp.col][d][0] &= ~(1 << (4 - i));
                //pieces_pattern[temp.row][temp.col][d][1] &= ~(1 << (4 - i));
                ////update left offset
                //if (pieces_pattern_offset[temp.row][temp.col][d][Util::otherside(side)][1] > i)
                //{
                //    pieces_pattern_offset[temp.row][temp.col][d][Util::otherside(side)][1] = i;
                //}

                //update layer2 and layer3
                update_layer3_with_layer2_new(temp.row, temp.col, 0, d);
                update_layer3_with_layer2_new(temp.row, temp.col, 1, d);
            }
        }
        return;
    }

    for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
    {
        //往左 在temp右边
        Position temp(row, col);
        for (int i = 0; i < 5; ++i)
        {
            if (!temp.displace4(-1, d)) break;
            //update pattern and offset
            if (pieces_layer1[temp.row][temp.col] != PIECE_BLANK)
            {
                continue;
            }
            //right pattern
            pieces_pattern[temp.row][temp.col][d][side] |= 1 << (6 + i);
            //right offset
            if (pieces_pattern_offset[temp.row][temp.col][d][Util::otherside(side)][1] > i)
            {
                pieces_pattern_offset[temp.row][temp.col][d][Util::otherside(side)][1] = i;
            }

            //update layer2 and layer3
            update_layer3_with_layer2_new(temp.row, temp.col, 0, d);
            update_layer3_with_layer2_new(temp.row, temp.col, 1, d);
        }

        //往右 在temp左边
        temp.set(row, col);
        for (int i = 0; i < 5; ++i)
        {
            if (!temp.displace4(1, d)) break;

            if (pieces_layer1[temp.row][temp.col] != PIECE_BLANK)
            {
                continue;
            }
            //update left pattern
            pieces_pattern[temp.row][temp.col][d][side] |= 1 << (4 - i);
            //update left offset
            if (pieces_pattern_offset[temp.row][temp.col][d][Util::otherside(side)][0] > i)
            {
                pieces_pattern_offset[temp.row][temp.col][d][Util::otherside(side)][0] = i;
            }

            //update layer2 and layer3
            update_layer3_with_layer2_new(temp.row, temp.col, 0, d);
            update_layer3_with_layer2_new(temp.row, temp.col, 1, d);
        }
    }
}

void ChessBoard::update_layer3_with_layer2_new(int8_t row, int8_t col, uint8_t side, uint8_t d)
{
    int len, index;
    len = pieces_pattern_offset[row][col][d][side][0] + pieces_pattern_offset[row][col][d][side][1] + 1;
    index = pieces_pattern[row][col][d][side] & (0xffff >> (5 - pieces_pattern_offset[row][col][d][side][1]));
    index = index >> (5 - pieces_pattern_offset[row][col][d][side][0]);
    pieces_layer2[row][col][d][side] = (ban && side == PIECE_BLACK) ? layer2_table_ban[len][index * len + pieces_pattern_offset[row][col][d][side][0]] : layer2_table[len][index * len + pieces_pattern_offset[row][col][d][side][0]];

    pieces_layer3[row][col][side] = layer2_to_layer3_table[pieces_layer2[row][col][0][side]][pieces_layer2[row][col][1][side]][pieces_layer2[row][col][2][side]][pieces_layer2[row][col][3][side]][(ban && side == PIECE_BLACK) ? 1 : 0];

    //update highest
    if (update_info_flag[side] != NEED)//本来就需要遍历的就不需要增量更新了
    {
        if (highestRatings[side].pos.equel(row, col))
        {
            if (pieces_layer3[row][col][side] == CHESSTYPE_BAN || pieces_layer3[row][col][side] < highestRatings[side].chesstype)
            {
                update_info_flag[side] = UNSURE;
            }
        }

        if (pieces_layer3[row][col][side] != CHESSTYPE_BAN && pieces_layer3[row][col][side] > highestRatings[side].chesstype)
        {
            highestRatings[side].chesstype = pieces_layer3[row][col][side];
            highestRatings[side].pos.set(row, col);
            update_info_flag[side] = NONEED;
        }
    }
}

void ChessBoard::update_layer2(int8_t row, int8_t col, uint8_t side)//落子处
{
    Position temp;
    int blankCount, chessCount;

    for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
    {
        int l_changeable_offset = 0, r_changeable_offset = 0;

        uint32_t l_hash_index = 0, r_hash_index = 0;
        int l_offset = 0, r_offset = 0;
        temp.set(row, col);
        blankCount = 0, chessCount = 0;
        while (temp.displace4(-1, d))//向左
        {
            if (pieces_layer1[temp.row][temp.col] == side)
            {
                chessCount++;
                l_hash_index <<= 1;
                l_hash_index += 1;
            }
            else if (pieces_layer1[temp.row][temp.col] == PIECE_BLANK)
            {
                blankCount++;
                l_hash_index <<= 1;
            }
            else
            {
                break;
            }
            if (blankCount < 4 && chessCount < 5 && l_changeable_offset < 5)
            {
                l_changeable_offset++;
            }

            if (++l_offset == 7)
            {
                break;
            }
        }
        temp.set(row, col);
        blankCount = 0, chessCount = 0;
        while (temp.displace4(1, d))//向右
        {
            if (pieces_layer1[temp.row][temp.col] == side)
            {
                chessCount++;
                r_hash_index |= 1 << r_offset;
            }
            else if (pieces_layer1[temp.row][temp.col] == PIECE_BLANK)
            {
                blankCount++;
                r_hash_index |= 0 << r_offset;
            }
            else
            {
                break;
            }

            if (blankCount < 4 && chessCount < 5 && r_changeable_offset < 5)
            {
                r_changeable_offset++;
            }

            if (++r_offset == 7)
            {
                break;
            }
        }

        if (pieces_layer1[row][col] == Util::otherside(side))
        {
            int len = l_offset;
            int index_offset = l_hash_index * len;
            Position pos_fix(row, col);
            pos_fix.displace4(-l_changeable_offset, d);
            //update
            for (int i = l_offset - l_changeable_offset; i < len; ++i, pos_fix.displace4(1, d))
            {
                if (pieces_layer1[pos_fix.row][pos_fix.col] == PIECE_BLANK)
                {
                    update_layer3_with_layer2(pos_fix.row, pos_fix.col, side, d, len, index_offset + i);
                }
            }

            len = r_offset;
            index_offset = r_hash_index * len;
            pos_fix.set(row, col);
            pos_fix.displace4(1, d);
            //update
            for (int i = 0; i < r_changeable_offset; ++i, pos_fix.displace4(1, d))
            {
                if (pieces_layer1[pos_fix.row][pos_fix.col] == PIECE_BLANK)
                {
                    update_layer3_with_layer2(pos_fix.row, pos_fix.col, side, d, len, index_offset + i);
                }
            }
        }
        else
        {
            int len = l_offset + r_offset + 1;
            int index_offset = ((((r_hash_index << 1) + (pieces_layer1[row][col] == side ? 1 : 0)) << l_offset) + l_hash_index) * len;
            Position pos_fix(row, col);
            pos_fix.displace4(-l_changeable_offset, d);
            //update
            for (int i = l_offset - l_changeable_offset; i < l_offset + 1 + r_changeable_offset; ++i, pos_fix.displace4(1, d))
            {
                if (pieces_layer1[pos_fix.row][pos_fix.col] == PIECE_BLANK)
                {
                    update_layer3_with_layer2(pos_fix.row, pos_fix.col, side, d, len, index_offset + i);
                }
            }
        }
    }
}


void ChessBoard::update_layer3_with_layer2(int8_t row, int8_t col, uint8_t side, uint8_t d, int len, int chessHashIndex)
{
    uint8_t oldchesstype = pieces_layer3[row][col][side];
    uint8_t oldtype = pieces_layer2[row][col][d][side];

    pieces_layer2[row][col][d][side] = (ban && side == PIECE_BLACK) ? layer2_table_ban[len][chessHashIndex] : layer2_table[len][chessHashIndex];

    pieces_layer3[row][col][side] = layer2_to_layer3_table[pieces_layer2[row][col][0][side]][pieces_layer2[row][col][1][side]][pieces_layer2[row][col][2][side]][pieces_layer2[row][col][3][side]][(ban && side == PIECE_BLACK) ? 1 : 0];

    //update highest
    if (update_info_flag[side] != NEED)//本来就需要遍历的就不需要增量更新了
    {
        if (highestRatings[side].pos.equel(row, col))
        {
            if (pieces_layer3[row][col][side] == CHESSTYPE_BAN || pieces_layer3[row][col][side] < highestRatings[side].chesstype)
            {
                update_info_flag[side] = UNSURE;
            }
        }

        if (pieces_layer3[row][col][side] != CHESSTYPE_BAN && pieces_layer3[row][col][side] > highestRatings[side].chesstype)
        {
            highestRatings[side].chesstype = pieces_layer3[row][col][side];
            highestRatings[side].pos.set(row, col);
            update_info_flag[side] = NONEED;
        }
    }
}

void ChessBoard::init2to3table()
{
    for (uint8_t d1 = 0; d1 < CHESSTYPE_COUNT; ++d1)
    {
        for (uint8_t d2 = 0; d2 < CHESSTYPE_COUNT; ++d2)
        {
            for (uint8_t d3 = 0; d3 < CHESSTYPE_COUNT; ++d3)
            {
                for (uint8_t d4 = 0; d4 < CHESSTYPE_COUNT; ++d4)
                {
                    layer2_to_layer3_table[d1][d2][d3][d4][0] = layer2_to_layer3(d1, d2, d3, d4, false);
                    layer2_to_layer3_table[d1][d2][d3][d4][1] = layer2_to_layer3(d1, d2, d3, d4, true);
                }
            }
        }
    }
}

uint8_t ChessBoard::layer2_to_layer3(uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4, bool ban)
{
    int count[CHESSTYPE_COUNT] = { 0 };
    ++count[d1]; ++count[d2]; ++count[d3]; ++count[d4];

    if (count[CHESSTYPE_5] > 0) return CHESSTYPE_5;//有5连可无视禁手
    if (count[CHESSTYPE_BAN] > 0) return CHESSTYPE_BAN;
    if (count[CHESSTYPE_44] > 0) return ban ? CHESSTYPE_BAN : CHESSTYPE_44;
    if (count[CHESSTYPE_D4] + count[CHESSTYPE_D4P] + count[CHESSTYPE_4] > 1) return ban ? CHESSTYPE_BAN : CHESSTYPE_44;//44优先级比4高，可能是禁手

    //特殊处理
    if (ban)
    {
        if (count[CHESSTYPE_3] + count[CHESSTYPE_J3] > 1) return CHESSTYPE_BAN;
        if (count[CHESSTYPE_4] > 0) return CHESSTYPE_4;
        if (count[CHESSTYPE_D4] + count[CHESSTYPE_D4P] + count[CHESSTYPE_3] + count[CHESSTYPE_J3] > 1) return CHESSTYPE_43;
    }
    else
    {
        if (count[CHESSTYPE_4] > 0) return CHESSTYPE_4;
        if (count[CHESSTYPE_D4] + count[CHESSTYPE_D4P] + count[CHESSTYPE_3] + count[CHESSTYPE_J3] > 1) return CHESSTYPE_43;
        if (count[CHESSTYPE_3] + count[CHESSTYPE_J3] > 1) return CHESSTYPE_33;
    }


    if (count[CHESSTYPE_D4P] > 0) return CHESSTYPE_D4P;
    if (count[CHESSTYPE_D4] > 0) return CHESSTYPE_D4;
    if (count[CHESSTYPE_3] > 0) return CHESSTYPE_3;
    if (count[CHESSTYPE_J3] > 0) return CHESSTYPE_J3;
    if (count[CHESSTYPE_D3] > 0) return CHESSTYPE_D3;
    if (count[CHESSTYPE_2] > 0) return CHESSTYPE_2;
    if (count[CHESSTYPE_J2] > 0) return CHESSTYPE_J2;
    return CHESSTYPE_0;
}

uint8_t ChessBoard::layer2_to_layer3_old(uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4, bool ban)
{
    uint8_t d[4] = { d1,d2,d3,d4 };
    uint8_t result = 0;
    int deadfour = 0, alivethree = 0;
    for (int i = 0; i < 4; ++i)
    {
        if (d[i] < CHESSTYPE_5)
        {
            if (d[i] > result)
            {
                result = d[i];
            }
            if (Util::isalive3(d[i]))
            {
                alivethree++;
            }
            if (d[i] == CHESSTYPE_D4
                || d[i] == CHESSTYPE_D4P)
            {
                deadfour++;
            }
        }
        else if (d[i] == CHESSTYPE_5)//5连最大
        {
            return CHESSTYPE_5;
        }
        else if (d[i] == CHESSTYPE_BAN)
        {
            return CHESSTYPE_BAN;
        }
        else if (d[i] == CHESSTYPE_44)//双四相当于活四，非禁手条件
        {
            return CHESSTYPE_44;
        }
    }
    //组合棋型
    if (ban)
    {
        if (result == CHESSTYPE_4)
        {
            deadfour++;
        }

        if (deadfour > 1)
        {
            result = CHESSTYPE_BAN;
        }
        else if (alivethree > 1)
        {
            result = CHESSTYPE_BAN;
        }
        else if (deadfour == 1 && alivethree == 1 && result != CHESSTYPE_4)
        {
            result = CHESSTYPE_43;
        }
    }
    else
    {
        if (result == CHESSTYPE_4)
        {
            return result;
        }

        if (deadfour > 1)
        {
            result = CHESSTYPE_44;
        }
        else if (deadfour == 1 && alivethree > 0)
        {
            result = CHESSTYPE_43;
        }
        else if (alivethree > 1)
        {
            result = CHESSTYPE_33;
        }
    }
    return result;
}

void ChessBoard::updatePoint_layer3(int8_t row, int8_t col, int side)//空白处
{
    uint8_t result = 0;
    int deadfour = 0, alivethree = 0;
    for (int d = 0; d < 4; ++d)
    {
        if (pieces_layer2[row][col][d][side] < CHESSTYPE_5)
        {
            if (pieces_layer2[row][col][d][side] > result)
            {
                result = pieces_layer2[row][col][d][side];
            }
            if (Util::isalive3(pieces_layer2[row][col][d][side]))
            {
                alivethree++;
            }
            if (pieces_layer2[row][col][d][side] == CHESSTYPE_D4
                || pieces_layer2[row][col][d][side] == CHESSTYPE_D4P)
            {
                deadfour++;
            }
        }
        else if (pieces_layer2[row][col][d][side] == CHESSTYPE_5)//5连最大
        {
            pieces_layer3[row][col][side] = CHESSTYPE_5;
            return;
        }
        else if (pieces_layer2[row][col][d][side] == CHESSTYPE_BAN)
        {
            pieces_layer3[row][col][side] = CHESSTYPE_BAN;
            return;
        }
        else if (pieces_layer2[row][col][d][side] == CHESSTYPE_44)//双四相当于活四，非禁手条件
        {
            pieces_layer3[row][col][side] = CHESSTYPE_44;
            return;
        }
    }
    pieces_layer3[row][col][side] = result;
    //组合棋型
    if (ban && side == PIECE_BLACK)
    {
        if (result == CHESSTYPE_4)
        {
            deadfour++;
        }

        if (deadfour > 1)
        {
            pieces_layer3[row][col][side] = CHESSTYPE_BAN;
        }
        else if (alivethree > 1)
        {
            pieces_layer3[row][col][side] = CHESSTYPE_BAN;
        }
        else if (deadfour == 1 && alivethree == 1 && result != CHESSTYPE_4)
        {
            pieces_layer3[row][col][side] = CHESSTYPE_43;
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
            pieces_layer3[row][col][side] = CHESSTYPE_44;
        }
        else if (deadfour == 1 && alivethree > 0)
        {
            pieces_layer3[row][col][side] = CHESSTYPE_43;
        }
        else if (alivethree > 1)
        {
            pieces_layer3[row][col][side] = CHESSTYPE_33;
        }
    }

}

int ChessBoard::getSimpleTotalScore(uint8_t side)
{
    int result = 0;
    ForEachPosition
    {
        if (pieces_layer1[pos.row][pos.col] == PIECE_BLANK)
        {
            result += getChessTypeInfo(pieces_layer3[pos.row][pos.col][side]).rating;
        }
    }
    return result;
}


void ChessBoard::updateChessInfo(uint8_t side)
{
    highestRatings[side].chesstype = CHESSTYPE_BAN;
    ForEachPosition
    {
        if (pieces_layer1[pos.row][pos.col] == PIECE_BLANK)
        {
            if (highestRatings[side].chesstype == CHESSTYPE_BAN)
            {
                highestRatings[side].chesstype = pieces_layer3[pos.row][pos.col][side];
                highestRatings[side].pos = pos;
            }
            else if (pieces_layer3[pos.row][pos.col][side] != CHESSTYPE_BAN && pieces_layer3[pos.row][pos.col][side] > highestRatings[side].chesstype)
            {
                highestRatings[side].chesstype = pieces_layer3[pos.row][pos.col][side];
                highestRatings[side].pos = pos;
            }
        }
    }
    update_info_flag[side] = NONEED;
}

bool ChessBoard::moveNull()
{
    lastStep.changeSide();
    return true;
}

bool ChessBoard::move(int8_t row, int8_t col, uint8_t side)
{
    if (pieces_layer1[row][col] != PIECE_BLANK)
    {
        return false;//已有棋子
    }
    lastStep.step++;
    lastStep.setState(side);
    lastStep.pos.set(row, col);
    lastStep.chessType = getChessType(row, col, side);

    pieces_layer1[row][col] = side;

    for (int i = 0; i < 2; ++i)
    {
        if (highestRatings[i].pos.equel(row, col))
        {
            update_info_flag[i] = UNSURE;
        }
    }

    //update_layer2(row, col);
    update_layer2_new(row, col, side);
    updateHashPair(row, col, side, true);

    return true;
}

bool ChessBoard::unmove(int8_t row, int8_t col, ChessStep last)
{
    uint8_t side = pieces_layer1[row][col];
    if (side == PIECE_BLANK || lastStep.step < 1)
    {
        return false;//没有棋子
    }
    lastStep = last;

    pieces_layer1[row][col] = PIECE_BLANK;

    //update_layer2(row, col);
    update_layer2_new(row, col, PIECE_BLANK);

    updateHashPair(row, col, side, false);

    return true;
}

void ChessBoard::formatChess2Int(char chessInt[DIRECTION4_COUNT][11], int row, int col, int side)
{
    //chessInt需要初始化为0
    int rowstart = row - SEARCH_LENGTH, colstart = col - SEARCH_LENGTH, rowend = row + SEARCH_LENGTH;

    for (int i = 0; i < FORMAT_MAX_LENGTH; ++i, ++rowstart, ++colstart, --rowend)
    {
        //横向
        if (colstart < 0 || colstart > 14)
        {
            chessInt[DIRECTION4_LR][i] = 'x';
        }
        else
        {
            if (pieces_layer1[row][colstart] == PIECE_BLANK)
            {
                chessInt[DIRECTION4_LR][i] = '?';
            }
            else
            {
                chessInt[DIRECTION4_LR][i] = (pieces_layer1[row][colstart] == side ? 'o' : 'x');
            }

        }

        //纵向
        if (rowstart < 0 || rowstart > 14)
        {
            chessInt[DIRECTION4_UD][i] = 'x';
        }
        else
        {
            if (pieces_layer1[rowstart][col] == PIECE_BLANK)
            {
                chessInt[DIRECTION4_UD][i] = '?';
            }
            else
            {
                chessInt[DIRECTION4_UD][i] = (pieces_layer1[rowstart][col] == side ? 'o' : 'x');
            }
        }
        //右下向
        if (colstart < 0 || rowstart < 0 || colstart > 14 || rowstart > 14)
        {
            chessInt[DIRECTION4_RD][i] = 'x';
        }
        else
        {
            if (pieces_layer1[rowstart][colstart] == PIECE_BLANK)
            {
                chessInt[DIRECTION4_RD][i] = '?';
            }
            else
            {
                chessInt[DIRECTION4_RD][i] = (pieces_layer1[rowstart][colstart] == side ? 'o' : 'x');
            }

        }

        //右上向
        if (colstart < 0 || rowend > 14 || colstart > 14 || rowend < 0)
        {
            chessInt[DIRECTION4_RU][i] = 'x';
        }
        else
        {
            if (pieces_layer1[rowend][colstart] == PIECE_BLANK)
            {
                chessInt[DIRECTION4_RU][i] = '?';
            }
            else
            {
                chessInt[DIRECTION4_RU][i] = (pieces_layer1[rowend][colstart] == side ? 'o' : 'x');
            }
        }
    }
}

void ChessBoard::getAtackReletedPos(set<Position>& releted, Position center, uint8_t side)
{
    Position temppos;
    for (int d = 0; d < DIRECTION4_COUNT; ++d)
    {
        int continus = 0;
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
        {
            int blankcount = 0;
            for (int8_t offset = 1; offset < 6; ++offset)
            {
                temppos = center.getNextPosition(d, offset*symbol);

                if (!temppos.valid() || pieces_layer1[temppos.row][temppos.col] == Util::otherside(side))//equal otherside
                {
                    break;
                }

                if (pieces_layer1[temppos.row][temppos.col] == side)
                {
                    continue;
                }
                else if (pieces_layer1[temppos.row][temppos.col] == PIECE_BLANK)
                {
                    blankcount++;
                    if (pieces_layer2[temppos.row][temppos.col][d][side] > CHESSTYPE_2)
                    {
                        releted.insert(temppos);
                        getAtackReletedPos2(releted, temppos, side);//因为没有求交集，暂时去掉
                    }
                    else
                    {
                        releted.insert(temppos);
                        //getAtackReletedPos2(releted, temppos, side);
                    }
                    //else
                    //{
                    //    if (pieces_layer3[temppos.row][temppos.col][side] > CHESSTYPE_2)
                    //    {
                    //        releted.insert(temppos);
                    //        getAtackReletedPos2(releted, temppos, side);
                    //    }
                    //}
                }
                if (blankcount == 3)
                {
                    break;
                }
            }
        }
    }

    getBanReletedPos(releted, lastStep.pos, Util::otherside(side));
}


void ChessBoard::getAtackReletedPos2(set<Position>& releted, Position center, uint8_t side)
{
    Position temppos;
    for (int d = 0; d < DIRECTION4_COUNT; ++d)
    {
        if (pieces_layer2[center.row][center.col][d][side] == CHESSTYPE_0)
        {
            continue;
        }
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
        {
            int blankcount = 0;
            for (int8_t offset = 1; offset < 5; ++offset)
            {
                temppos = center.getNextPosition(d, offset*symbol);
                if (!temppos.valid() || pieces_layer1[temppos.row][temppos.col] == Util::otherside(side))//equal otherside
                {
                    break;
                }

                if (pieces_layer1[temppos.row][temppos.col] == side)
                {
                    continue;
                }
                else if (pieces_layer1[temppos.row][temppos.col] == PIECE_BLANK)
                {
                    blankcount++;
                    if (pieces_layer2[temppos.row][temppos.col][d][side] > CHESSTYPE_0)
                    {
                        releted.insert(temppos);
                    }
                    else if (pieces_layer3[temppos.row][temppos.col][side] > CHESSTYPE_D3)
                    {
                        releted.insert(temppos);
                    }
                }

                if (blankcount == 2)
                {
                    break;
                }
            }
        }
    }
}

void ChessBoard::getBanReletedPos(set<Position>& releted, Position center, uint8_t side)
{
    //找出是否有禁手点
    Position temppos;

    vector<Position> banset;
    for (int d = 0; d < DIRECTION4_COUNT; ++d)
    {
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
        {
            for (int8_t offset = 1; offset < 5; ++offset)
            {
                temppos = center.getNextPosition(d, offset*symbol);
                if (!temppos.valid() || pieces_layer1[temppos.row][temppos.col] == Util::otherside(side))//equal otherside
                {
                    break;
                }

                if (pieces_layer1[temppos.row][temppos.col] == side)
                {
                    continue;
                }
                else if (pieces_layer1[temppos.row][temppos.col] == PIECE_BLANK)
                {
                    if (pieces_layer3[temppos.row][temppos.col][side] == CHESSTYPE_BAN)
                    {
                        banset.emplace_back(temppos);
                    }
                }
            }
        }
    }

    for (auto banpos : banset)
    {
        Position temppos;
        for (int d = 0; d < DIRECTION4_COUNT; ++d)
        {
            for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
            {
                for (int8_t offset = 1; offset < 5; ++offset)
                {
                    temppos = banpos.getNextPosition(d, offset*symbol);
                    if (!temppos.valid() || pieces_layer1[temppos.row][temppos.col] == side)//equal otherside
                    {
                        break;
                    }

                    if (pieces_layer1[temppos.row][temppos.col] == PIECE_BLANK)
                    {
                        if (pieces_layer3[temppos.row][temppos.col][Util::otherside(side)] >= CHESSTYPE_J3)
                        {
                            releted.insert(temppos);
                        }
                    }
                    else
                    {
                        continue;
                    }
                }
            }
        }
    }
}

//必须想办法解决奇偶层效应
//const ChessTypeInfo chesstypes[CHESSTYPE_COUNT] = {
//    { 0    ,   0,   0,     0,  0 },           //CHESSTYPE_0,  +CHESSTYPE_2*2 +CHESSTYPE_J2*2 (0)
//    { 10   ,   8,   0,     0,  0 },           //CHESSTYPE_j2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
//    { 10   ,  10,   2,     0,  0 },           //CHESSTYPE_2,  -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*2 +CHESSTYPE_J3*2 (0)
//    { 10   ,  10,   5,     0,  0 },           //CHESSTYPE_d3, -CHESSTYPE_D3*2 +CHESSTYPE_D4*2 (0)
//    { 80   ,  10,  15,    12,  6 },           //CHESSTYPE_J3  -CHESSTYPE_3*1 -CHESSTYPE_J3*2 +CHESSTYPE_4*1 +CHESSTYPE_D4*2 (0)
//    { 100  ,  20,  20,    25, 16 },           //CHESSTYPE_3,  -CHESSTYPE_3*2 -CHESSTYPE_J3*2 +CHESSTYPE_4*2 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
//    { 120  ,   0,  20,    20, 12 },           //CHESSTYPE_d4, -CHESSTYPE_D4*2 +CHESSTYPE_5 (0) 优先级降低
//    { 150  ,  20,  25,    30, 20 },           //CHESSTYPE_d4p -CHESSTYPE_D4P*1 -CHESSTYPE_D4 +CHESSTYPE_5 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
//    { 250  ,  30,  30,   100, 40 },           //CHESSTYPE_33, -CHESSTYPE_33*1 -CHESSTYPE_3*0-2 -CHESSTYPE_J3*2-4 +CHESSTYPE_4*2-4 +CHESSTYPE_D4*2-4 (CHESSTYPE_4*2)
//    { 450  ,  35,  30,   200, 50 },           //CHESSTYPE_43, -CHESSTYPE_43*1 -CHESSTYPE_D4*1 -CHESSTYPE_J3*2 -CHESSTYPE_3*1 +CHESSTYPE_5*1 +CHESSTYPE_4*2 (CHESSTYPE_4*2)
//    { 500  , 100,  40,   250, 60 },           //CHESSTYPE_44, -CHESSTYPE_44 -CHESSTYPE_D4*2 +2个CHESSTYPE_5    (CHESSTYPE_5)
//    { 500  , 100,  50,   250, 60 },           //CHESSTYPE_4,  -CHESSTYPE_4*1-2 -CHESSTYPE_D4*1-2 +CHESSTYPE_5*2 (CHESSTYPE_5)
//    { 10000, 100, 150, 10000,100 },           //CHESSTYPE_5,
//    { -100 , -90,  30,   -10,-5 },           //CHESSTYPE_BAN,
//};

const ChessTypeInfo chesstypes[CHESSTYPE_COUNT] = {
    { 0    ,   0,   0,     0,  0 },           //CHESSTYPE_0,  +CHESSTYPE_2*2 +CHESSTYPE_J2*2 (0)
    { 10   ,   5,   0,     0,  0 },           //CHESSTYPE_j2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
    { 10   ,   7,   2,     1,  0 },           //CHESSTYPE_2,  -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*2 +CHESSTYPE_J3*2 (0)
    { 10   ,   7,   5,     1,  0 },           //CHESSTYPE_d3, -CHESSTYPE_D3*2 +CHESSTYPE_D4*2 (0)
    { 80   ,  15,  10,     8,  4 },           //CHESSTYPE_J3  -CHESSTYPE_3*1 -CHESSTYPE_J3*2 +CHESSTYPE_4*1 +CHESSTYPE_D4*2 (0)
    { 100  ,  20,  15,    16,  8 },           //CHESSTYPE_3,  -CHESSTYPE_3*2 -CHESSTYPE_J3*2 +CHESSTYPE_4*2 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
    { 120  ,   0,  15,    12,  6 },           //CHESSTYPE_d4, -CHESSTYPE_D4*2 +CHESSTYPE_5 (0) 优先级降低
    { 150  ,  20,  20,    20, 10 },           //CHESSTYPE_d4p -CHESSTYPE_D4P*1 -CHESSTYPE_D4 +CHESSTYPE_5 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
    { 250  ,  30,  25,   100, 40 },           //CHESSTYPE_33, -CHESSTYPE_33*1 -CHESSTYPE_3*0-2 -CHESSTYPE_J3*2-4 +CHESSTYPE_4*2-4 +CHESSTYPE_D4*2-4 (CHESSTYPE_4*2)
    { 450  ,  35,  30,   200, 50 },           //CHESSTYPE_43, -CHESSTYPE_43*1 -CHESSTYPE_D4*1 -CHESSTYPE_J3*2 -CHESSTYPE_3*1 +CHESSTYPE_5*1 +CHESSTYPE_4*2 (CHESSTYPE_4*2)
    { 500  , 100,  30,   250, 60 },           //CHESSTYPE_44, -CHESSTYPE_44 -CHESSTYPE_D4*2 +2个CHESSTYPE_5    (CHESSTYPE_5)
    { 500  , 100,  30,   250, 60 },           //CHESSTYPE_4,  -CHESSTYPE_4*1-2 -CHESSTYPE_D4*1-2 +CHESSTYPE_5*2 (CHESSTYPE_5)
    { 10000, 100, 150, 10000,100 },           //CHESSTYPE_5,
    { -100 , -90,  30,   -10,-5 },           //CHESSTYPE_BAN,
};


ChessTypeInfo ChessBoard::getChessTypeInfo(uint8_t type)
{
    return chesstypes[type];
}

//进行情景细分，主要聚焦于对局势的影响趋势，而非局势本身
//所以就算本身为CHESSTYPE_0，也有可能对局势有很大影响
int ChessBoard::getRelatedFactor(Position pos, uint8_t side, bool defend)
{
    int base_factor = 0;//初始值
    if (defend)
    {
        int count = 0;
        if (pieces_layer3[pos.row][pos.col][side] > CHESSTYPE_D4P)
        {
            return chesstypes[pieces_layer3[pos.row][pos.col][side]].defendBaseFactor;
        }

        for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
        {
            if (pieces_layer2[pos.row][pos.col][d][side] > CHESSTYPE_D3)
            {
                if (pieces_layer2[pos.row][pos.col][d][side] == CHESSTYPE_J3)//特殊处理
                {
                    bool no_use = false;
                    Position temppos;
                    for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
                    {
                        temppos = pos.getNextPosition(d, symbol);

                        if (!temppos.valid() || pieces_layer2[temppos.row][temppos.col][d][side] != CHESSTYPE_3)
                        {
                            continue;
                        }
                        temppos = pos.getNextPosition(d, 4 * symbol);
                        if (temppos.valid() && pieces_layer2[temppos.row][temppos.col][d][side] == CHESSTYPE_3)//?!?oo??? 无用
                        {
                            base_factor += 5;
                            no_use = true;
                            break;
                        }
                    }
                    if (!no_use)
                    {
                        base_factor += chesstypes[pieces_layer2[pos.row][pos.col][d][side]].defendBaseFactor;
                    }
                }
                else
                {
                    base_factor += chesstypes[pieces_layer2[pos.row][pos.col][d][side]].defendBaseFactor;
                }
            }
            else if (pieces_layer2[pos.row][pos.col][d][side] > CHESSTYPE_0)
            {
                count += 1;
            }

        }
        if (count > 1)
        {
            if (pieces_layer3[pos.row][pos.col][side] < CHESSTYPE_J3)
            {
                base_factor += count * 7;
            }
            else
            {
                base_factor += 10;
            }

        }
        else if (count == 1)
        {
            if (pieces_layer3[pos.row][pos.col][side] < CHESSTYPE_J3)
            {
                base_factor = chesstypes[pieces_layer3[pos.row][pos.col][side]].defendBaseFactor;
            }
            else
            {
                base_factor += 5;
            }
        }
        return base_factor;
    }

    if (pieces_layer3[pos.row][pos.col][side] > CHESSTYPE_D4P)
    {
        return chesstypes[pieces_layer3[pos.row][pos.col][side]].atackBaseFactor;
    }
    Position temppos;
    for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
    {
        base_factor += chesstypes[pieces_layer2[pos.row][pos.col][d][side]].atackBaseFactor;

        //related factor, except base 
        int releted_count_3 = 0;
        int releted_count_d4 = 0;
        int availi_count = 0;

        for (int i = 0; i < 2; ++i)//正反
        {
            int blank = 0;
            temppos = pos;
            while (temppos.displace8(1, d * 2 + i))
            {
                if (pieces_layer1[temppos.row][temppos.col] == side)
                {
                    availi_count++;
                    continue;
                }
                else if (pieces_layer1[temppos.row][temppos.col] == PIECE_BLANK)
                {
                    availi_count++;
                    blank++;

                    for (uint8_t d2 = 0; d2 < DIRECTION4_COUNT; ++d2)
                    {
                        if (d == d2) continue;

                        if (pieces_layer2[temppos.row][temppos.col][d2][side] > CHESSTYPE_3)
                        {
                            releted_count_d4++;
                        }
                        else if (pieces_layer2[temppos.row][temppos.col][d2][side] > CHESSTYPE_D3)
                        {
                            releted_count_3++;
                        }
                    }
                }
                else
                {
                    break;
                }

                if (blank == 3)
                {
                    break;
                }
            }
        }
        if (availi_count > 4)//至少要5才能有威胁
        {
            if (pieces_layer2[pos.row][pos.col][d][side] == CHESSTYPE_0)
            {
                if (releted_count_d4 > 1)
                {
                    base_factor += 30;
                }
                else if (releted_count_3 > 1)
                {
                    base_factor += 20;
                }
                else if (releted_count_d4 + releted_count_3 > 1)
                {
                    base_factor += 25;
                }
            }
            else//> CHESSTYPE_0
            {
                base_factor += releted_count_d4 * 15 + releted_count_3 * 10;
            }
        }
    }
    return base_factor;
}

//进行情景细分，主要聚焦于局势本身
//double ChessBoard::getStaticFactor(Position pos, uint8_t side, bool defend)
//{
//    uint8_t layer3type = pieces_layer3[pos.row][pos.col][side];
//    if (layer3type > CHESSTYPE_D4P)
//    {
//        return 1.0;
//    }
//    else if (layer3type < CHESSTYPE_J3)
//    {
//        return 1.0;
//    }
//
//    double base_factor = 1.0;//初始值
//
//    Position temppos;
//    double related_factor = 1.0;
//    bool findself = false;
//    for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
//    {
//        if (!findself && pieces_layer2[pos.row][pos.col][d][side] == layer3type)
//        {
//            findself = true;
//            continue;//过滤自身那条线
//        }
//        else
//        {
//            if (pieces_layer2[pos.row][pos.col][d][side] > CHESSTYPE_0)
//            {
//                base_factor += 0.5;
//            }
//        }
//
//        //related factor, except base 
//        double related_factor = 0.0;
//        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
//        {
//            int blank = 0;
//            for (int8_t offset = 1; offset < 5; ++offset)
//            {
//                temppos = pos.getNextPosition(d, offset*symbol);
//                if (!temppos.valid() || pieces_layer1[temppos.row][temppos.col] == Util::otherside(side))//equal otherside
//                {
//                    break;
//                }
//                else if (pieces_layer1[temppos.row][temppos.col] == side)
//                {
//                    continue;
//                }
//                else//blank
//                {
//                    blank++;
//                    //pieces_layer2[index][d][side]一定是 < CHESSTYPE_J3
//                    if (pieces_layer2[pos.row][pos.col][d][side] > CHESSTYPE_J2)//CHESSTYPE_2 || CHESSTYPE_D3
//                    {
//                        //o??!o
//
//                        if (pieces_layer3[temppos.row][temppos.col][side] > CHESSTYPE_D3)
//                        {
//                            if (pieces_layer2[temppos.row][temppos.col][d][side] < CHESSTYPE_2)
//                            {
//                                temppos = temppos.getNextPosition(d, offset*symbol);
//                                if (temppos.valid() && pieces_layer1[temppos.row][temppos.col] != Util::otherside(side))//equal otherside
//                                {
//                                    if (pieces_layer2[temppos.row][temppos.col][d][side] == CHESSTYPE_0)
//                                    {
//                                        related_factor += 0.25;
//                                    }
//                                    else//CHESSTYPE_J2
//                                    {
//                                        if (pieces_layer3[pos.row][pos.col][side] > CHESSTYPE_D3)
//                                        {
//                                            related_factor += 2.0;
//                                        }
//                                        else
//                                        {
//                                            related_factor += 0.5;
//                                        }
//                                    }
//                                }
//                            }
//                            else
//                            {
//                                if (pieces_layer3[pos.row][pos.col][side] > CHESSTYPE_D3)
//                                {
//                                    related_factor += 2.0;
//                                }
//                                else
//                                {
//                                    related_factor += 0.5;
//                                }
//                            }
//                        }
//                        else if (pieces_layer3[temppos.row][temppos.col][side] > CHESSTYPE_2)
//                        {
//                            if (pieces_layer2[temppos.row][temppos.col][d][side] < CHESSTYPE_2)
//                            {
//                                temppos = temppos.getNextPosition(d, offset*symbol);
//                                if (temppos.valid() && pieces_layer1[temppos.row][temppos.col] != Util::otherside(side))//equal otherside
//                                {
//                                    related_factor += 0.2;
//                                }
//                            }
//                            else
//                            {
//                                related_factor += 0.2;
//                            }
//                        }
//                    }
//                    else//pieces_layer2[index][d][side] == CHESSTYPE_0 || CHESSTYPE_J2
//                    {
//
//                        if (pieces_layer3[temppos.row][temppos.col][side] > CHESSTYPE_D3)
//                        {
//                            if (pieces_layer2[temppos.row][temppos.col][d][side] < CHESSTYPE_2)
//                            {
//                                temppos = temppos.getNextPosition(d, offset*symbol);
//                                if (temppos.valid() && pieces_layer1[temppos.row][temppos.col] != Util::otherside(side))//equal otherside
//                                {
//                                    related_factor += 0.25;
//                                }
//                            }
//                            else
//                            {
//                                related_factor += 0.25;
//                            }
//                        }
//                    }
//                }
//                if (blank == 3)
//                {
//                    break;
//                }
//            }
//        }
//
//        base_factor += related_factor;
//    }
//    return base_factor;
//}

double ChessBoard::getStaticFactor(Position pos, uint8_t side, bool defend)
{
    uint8_t layer3type = pieces_layer3[pos.row][pos.col][side];
    if (layer3type > CHESSTYPE_D4P)
    {
        return 1.0;
    }
    else if (layer3type < CHESSTYPE_J3)
    {
        return 1.0;
    }

    double base_factor = 1.0;//初始值

    Position temppos;
    double related_factor = 1.0;
    bool findself = false;
    for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
    {
        if (!findself && pieces_layer2[pos.row][pos.col][d][side] == layer3type)
        {
            findself = true;
            continue;//过滤自身那条线
        }
        else
        {
            if (pieces_layer2[pos.row][pos.col][d][side] > CHESSTYPE_0)
            {
                base_factor += 0.5;
            }
        }

        //related factor, except base 
        double related_factor = 0.0;
        int releted_count_3 = 0;
        int releted_count_d4 = 0;
        int availi_count = 0;
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
        {
            int blank = 0;
            for (int8_t offset = 1; offset < 5; ++offset)
            {
                temppos = pos.getNextPosition(d, offset*symbol);
                if (!temppos.valid() || pieces_layer1[temppos.row][temppos.col] == Util::otherside(side))//equal otherside
                {
                    break;
                }
                else if (pieces_layer1[temppos.row][temppos.col] == side)
                {
                    availi_count++;
                    continue;
                }
                else//blank
                {
                    availi_count++;
                    blank++;
                    //pieces_layer2[index][d][side]一定是 < CHESSTYPE_J3
                    if (pieces_layer2[pos.row][pos.col][d][side] > CHESSTYPE_0)
                    {
                        if (pieces_layer3[temppos.row][temppos.col][side] > CHESSTYPE_D3)
                        {
                            related_factor += 1.0;
                        }
                        else if (pieces_layer2[temppos.row][temppos.col][d][side] > CHESSTYPE_0)
                        {
                            for (uint8_t d1 = 0; d1 < DIRECTION4_COUNT; ++d1)
                            {
                                if (d1 == d)
                                {
                                    continue;
                                }
                                if (pieces_layer2[temppos.row][temppos.col][d1][side] > CHESSTYPE_0)
                                {
                                    related_factor += 0.2;
                                }
                            }

                        }
                    }
                    else//pieces_layer2[index][d][side] == CHESSTYPE_0 || CHESSTYPE_J2
                    {

                        if (pieces_layer3[temppos.row][temppos.col][side] > CHESSTYPE_D3)
                        {
                            related_factor += 0.5;
                        }
                        else if (pieces_layer2[temppos.row][temppos.col][d][side] > CHESSTYPE_0)
                        {
                            for (uint8_t d1 = 0; d1 < DIRECTION4_COUNT; ++d1)
                            {
                                if (d1 == d)
                                {
                                    continue;
                                }
                                if (pieces_layer2[temppos.row][temppos.col][d1][side] > CHESSTYPE_0)
                                {
                                    related_factor += 0.2;
                                }
                            }

                        }
                    }
                }
                if (blank == 3)
                {
                    break;
                }
            }
        }
        if (availi_count > 4)
        {
            base_factor += related_factor;
        }
    }
    return base_factor;
}

//weight是对于side方的偏向，默认100
int ChessBoard::getGlobalEvaluate(uint8_t side, int weight)
{
    //始终是以进攻方(atackside)为正
    uint8_t defendside = lastStep.getState();
    uint8_t atackside = Util::otherside(defendside);

    int atack_evaluate = 0;
    int defend_evaluate = 0;
    //遍历所有棋子
    ForEachPosition
    {
        //已有棋子的不做计算
        if (!canMove(pos) || !useful(pos))
        {
            continue;
        }

        atack_evaluate += (int)(chesstypes[pieces_layer3[pos.row][pos.col][atackside]].atackPriority*getStaticFactor(pos, atackside));

        defend_evaluate += (int)(chesstypes[pieces_layer3[pos.row][pos.col][defendside]].defendPriority*getStaticFactor(pos, defendside));
    }

    return side == atackside ? atack_evaluate * weight / 100 - defend_evaluate : -(atack_evaluate - defend_evaluate * weight / 100);
}

void ChessBoard::printGlobalEvaluate(string &s)
{
    //始终是以进攻方(atackside)为正
    uint8_t defendside = lastStep.getState();
    uint8_t atackside = Util::otherside(defendside);
    stringstream ss;
    int atack = 0, defend = 0;
    ss << "/" << "\t";
    for (int index = 0; index < Util::BoardSize; index++)
    {
        ss << index << "\t";
    }
    //遍历所有棋子
    ForEachPosition
    {
        if (pos.col == 0)
        {
            ss << "\r\n\r\n\r\n";
            ss << (int)pos.row << "\t";
        }
    //已有棋子的不做计算
    if (!canMove(pos) || !useful(pos))
    {
        ss << 0 << "|" << 0 << "\t";
        continue;
    }


    atack += (int)(chesstypes[pieces_layer3[pos.row][pos.col][atackside]].atackPriority*getStaticFactor(pos, atackside));
    ss << (int)(chesstypes[pieces_layer3[pos.row][pos.col][atackside]].atackPriority*getStaticFactor(pos, atackside));


    ss << "|";


    defend += (int)(chesstypes[pieces_layer3[pos.row][pos.col][defendside]].defendPriority*getStaticFactor(pos, defendside));
    ss << (int)(chesstypes[pieces_layer3[pos.row][pos.col][defendside]].defendPriority*getStaticFactor(pos, defendside));

    ss << "\t";
    }
    ss << "\r\n\r\n\r\n";
    ss << "atack:" << atack << "|" << "defend:" << defend;
    s = ss.str();

}

void ChessBoard::initHash()
{
    ForEachPosition
    {
        hash.check_key ^= zcheck[pos.row][pos.col][pieces_layer1[pos.row][pos.col]];
        hash.hash_key ^= zkey[pos.row][pos.col][pieces_layer1[pos.row][pos.col]];
    }
}

void ChessBoard::updateHashPair(int8_t row, int8_t col, uint8_t side, bool add)
{
    if (add) //添加棋子
    {
        hash.check_key ^= zcheck[row][col][PIECE_BLANK];//原来是空的
        hash.check_key ^= zcheck[row][col][side];
        hash.hash_key ^= zkey[row][col][PIECE_BLANK];
        hash.hash_key ^= zkey[row][col][side];
    }
    else //拿走棋子
    {
        hash.check_key ^= zcheck[row][col][side];       //原来是有子的
        hash.check_key ^= zcheck[row][col][PIECE_BLANK];
        hash.hash_key ^= zkey[row][col][side];
        hash.hash_key ^= zkey[row][col][PIECE_BLANK];
    }
}


uint32_t ChessBoard::zkey[BOARD_SIZE_MAX][BOARD_SIZE_MAX][3] = { 0 };
uint32_t ChessBoard::zcheck[BOARD_SIZE_MAX][BOARD_SIZE_MAX][3] = { 0 };

void ChessBoard::initZobrist()
{
    default_random_engine e(407618);//fixed seed
                                    //uniform_int_distribution<uint64_t> rd64;
    uniform_int_distribution<uint32_t> rd32;

    for (int row = 0; row < BOARD_SIZE_MAX; ++row)
    {
        for (int col = 0; col < BOARD_SIZE_MAX; ++col)
        {
            for (int k = 0; k < 3; ++k)
            {
                zkey[row][col][k] = rd32(e);
                zcheck[row][col][k] = rd32(e);
            }
        }
    }
}
