#include "ChessBoard.h"
#include "TrieTree.h"
#include <random>
using namespace std;


int8_t Util::BoardSize = 15;
int Util::SizeUpper = 14;

void ChessBoard::initStaticHelper()
{
    initZobrist();
    initLayer2Table();
    init2to3table();
    initRelatedSituation();
}

uint8_t* ChessBoard::layer2_table[BOARD_SIZE_MAX + 1] = { NULL };
uint8_t* ChessBoard::layer2_table_ban[BOARD_SIZE_MAX + 1] = { NULL };
uint8_t ChessBoard::layer2_to_layer3_table[CHESSTYPE_COUNT][CHESSTYPE_COUNT][CHESSTYPE_COUNT][CHESSTYPE_COUNT][3];
bool ChessBoard::relatedsituation[5][5][5][5];

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
        pieces[pos.row][pos.col].layer1 = PIECE_BLANK;
    }
}

void ChessBoard::init_layer2()
{
    ForEachPosition
    {
        for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
        {
            pieces[pos.row][pos.col].layer2[d][PIECE_BLACK] = 0;
            pieces[pos.row][pos.col].layer2[d][PIECE_WHITE] = 0;
        }

    }
}


void ChessBoard::init_layer3()
{
    ForEachPosition
    {
        pieces[pos.row][pos.col].layer3[PIECE_BLACK] = 0;
        pieces[pos.row][pos.col].layer3[PIECE_WHITE] = 0;
    }
}

void ChessBoard::init_pattern()
{
    //ForEachPosition
    //{
    //    for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
    //    {
    //        pieces[pos.row][pos.col].pattern[d][PIECE_BLACK] = 0;
    //        pieces[pos.row][pos.col].pattern[d][PIECE_WHITE] = 0;


    //        pieces[pos.row][pos.col].pattern_offset[d][PIECE_BLACK][0] = 0;
    //        pieces[pos.row][pos.col].pattern_offset[d][PIECE_WHITE][0] = 0;
    //        pieces[pos.row][pos.col].pattern_offset[d][PIECE_BLACK][1] = 0;
    //        pieces[pos.row][pos.col].pattern_offset[d][PIECE_WHITE][1] = 0;
    //        for (int i = 5; i > 0; --i)
    //        {
    //            Position temp = pos;
    //            if (temp.displace4(-1 * i, d))
    //            {
    //                pieces[pos.row][pos.col].pattern_offset[d][PIECE_BLACK][0] = i;
    //                pieces[pos.row][pos.col].pattern_offset[d][PIECE_WHITE][0] = i;
    //                break;
    //            }
    //        }

    //        for (int i = 5; i > 0; --i)
    //        {
    //            Position temp = pos;
    //            if (temp.displace4(i, d))
    //            {
    //                pieces[pos.row][pos.col].pattern_offset[d][PIECE_BLACK][1] = i;
    //                pieces[pos.row][pos.col].pattern_offset[d][PIECE_WHITE][1] = i;
    //                break;
    //            }
    //        }
    //    }
    //}
}

void ChessBoard::update_pattern(int8_t row, int8_t col)
{
    //for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
    //{
    //    pieces[row][col].pattern[d][PIECE_BLACK] = 0;
    //    pieces[row][col].pattern[d][PIECE_WHITE] = 0;

    //    pieces[row][col].pattern_offset[d][PIECE_BLACK][0] = 5;
    //    pieces[row][col].pattern_offset[d][PIECE_WHITE][0] = 5;
    //    Position temp(row, col);
    //    for (int i = 0; i < 5; ++i)
    //    {
    //        if (!temp.displace4(-1, d))
    //        {
    //            pieces[row][col].pattern_offset[d][PIECE_BLACK][0] = i > pieces[row][col].pattern_offset[d][PIECE_BLACK][0] ? pieces[row][col].pattern_offset[d][PIECE_BLACK][0] : i;
    //            pieces[row][col].pattern_offset[d][PIECE_WHITE][0] = i > pieces[row][col].pattern_offset[d][PIECE_WHITE][0] ? pieces[row][col].pattern_offset[d][PIECE_WHITE][0] : i;
    //            break;
    //        }
    //        if (pieces[temp.row][temp.col].layer1 == PIECE_BLANK)
    //        {
    //            pieces[row][col].pattern[d][0] &= ~(1 << (4 - i));
    //            pieces[row][col].pattern[d][1] &= ~(1 << (4 - i));
    //        }
    //        else if (pieces[temp.row][temp.col].layer1 == PIECE_BLACK)
    //        {
    //            pieces[row][col].pattern[d][PIECE_BLACK] |= (1 << (4 - i));
    //            pieces[row][col].pattern_offset[d][PIECE_WHITE][0] = i > pieces[row][col].pattern_offset[d][PIECE_WHITE][0] ? pieces[row][col].pattern_offset[d][PIECE_WHITE][0] : i;
    //        }
    //        else
    //        {
    //            pieces[row][col].pattern[d][PIECE_WHITE] |= (1 << (4 - i));
    //            pieces[row][col].pattern_offset[d][PIECE_BLACK][0] = i > pieces[row][col].pattern_offset[d][PIECE_BLACK][0] ? pieces[row][col].pattern_offset[d][PIECE_BLACK][0] : i;
    //        }

    //    }

    //    pieces[row][col].pattern_offset[d][PIECE_BLACK][1] = 5;
    //    pieces[row][col].pattern_offset[d][PIECE_WHITE][1] = 5;
    //    temp.set(row, col);
    //    for (int i = 0; i < 5; ++i)
    //    {
    //        if (!temp.displace4(1, d))
    //        {
    //            pieces[row][col].pattern_offset[d][PIECE_BLACK][1] = i > pieces[row][col].pattern_offset[d][PIECE_BLACK][1] ? pieces[row][col].pattern_offset[d][PIECE_BLACK][1] : i;
    //            pieces[row][col].pattern_offset[d][PIECE_WHITE][1] = i > pieces[row][col].pattern_offset[d][PIECE_WHITE][1] ? pieces[row][col].pattern_offset[d][PIECE_WHITE][1] : i;
    //            break;
    //        }
    //        if (pieces[temp.row][temp.col].layer1 == PIECE_BLANK)
    //        {
    //            pieces[row][col].pattern[d][0] &= ~(1 << (6 + i));
    //            pieces[row][col].pattern[d][1] &= ~(1 << (6 + i));
    //        }
    //        else if (pieces[temp.row][temp.col].layer1 == PIECE_BLACK)
    //        {
    //            pieces[row][col].pattern[d][PIECE_BLACK] |= (1 << (6 + i));
    //            pieces[row][col].pattern_offset[d][PIECE_WHITE][1] = i > pieces[row][col].pattern_offset[d][PIECE_WHITE][1] ? pieces[row][col].pattern_offset[d][PIECE_WHITE][1] : i;
    //        }
    //        else
    //        {
    //            pieces[row][col].pattern[d][PIECE_WHITE] |= (1 << (6 + i));
    //            pieces[row][col].pattern_offset[d][PIECE_BLACK][1] = i > pieces[row][col].pattern_offset[d][PIECE_BLACK][1] ? pieces[row][col].pattern_offset[d][PIECE_BLACK][1] : i;
    //        }
    //    }
    //}
}


void ChessBoard::update_layer2_new(int8_t row, int8_t col, uint8_t side)
{
    ////unmove
    //if (side == PIECE_BLANK)
    //{
    //    update_pattern(row, col);

    //    for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
    //    {
    //        update_layer3_with_layer2_new(row, col, 0, d);
    //        update_layer3_with_layer2_new(row, col, 1, d);
    //        //往左 在temp右边
    //        Position temp(row, col);
    //        for (int i = 0; i < 5; ++i)
    //        {
    //            if (!temp.displace4(-1, d)) break;
    //            //update pattern and offset
    //            if (pieces[temp.row][temp.col].layer1 != PIECE_BLANK)
    //            {
    //                continue;
    //            }

    //            update_pattern(temp.row, temp.col);

    //            //update layer2 and layer3
    //            update_layer3_with_layer2_new(temp.row, temp.col, PIECE_BLACK, d);
    //            update_layer3_with_layer2_new(temp.row, temp.col, 1, d);
    //        }

    //        //往右 在temp左边
    //        temp.set(row, col);
    //        for (int i = 0; i < 5; ++i)
    //        {
    //            if (!temp.displace4(1, d)) break;

    //            if (pieces[temp.row][temp.col].layer1 != PIECE_BLANK)
    //            {
    //                continue;
    //            }

    //            update_pattern(temp.row, temp.col);

    //            //update layer2 and layer3
    //            update_layer3_with_layer2_new(temp.row, temp.col, PIECE_BLACK, d);
    //            update_layer3_with_layer2_new(temp.row, temp.col, 1, d);
    //        }
    //    }
    //    return;
    //}

    //bool breakblack = false, breakwhite = false;
    //for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
    //{
    //    //往左 在temp右边
    //    Position temp(row, col);
    //    breakblack = false; breakwhite = false;
    //    for (int i = 0; i < 5; ++i)
    //    {
    //        if (!temp.displace4(-1, d)) break;
    //        //update pattern and offset
    //        if (pieces[temp.row][temp.col].layer1 == PIECE_BLACK)
    //        {
    //            breakwhite = true;
    //            continue;
    //        }
    //        else if (pieces[temp.row][temp.col].layer1 == PIECE_WHITE)
    //        {
    //            breakblack = true;
    //            continue;
    //        }
    //        //right pattern
    //        pieces[temp.row][temp.col].pattern[d][side] |= 1 << (6 + i);
    //        //right offset
    //        if (pieces[temp.row][temp.col].pattern_offset[d][Util::otherside(side)][1] > i)
    //        {
    //            pieces[temp.row][temp.col].pattern_offset[d][Util::otherside(side)][1] = i;
    //        }

    //        //update layer2 and layer3
    //        if (!breakblack) update_layer3_with_layer2_new(temp.row, temp.col, PIECE_BLACK, d);
    //        if (!breakwhite) update_layer3_with_layer2_new(temp.row, temp.col, PIECE_WHITE, d);
    //    }

    //    //往右 在temp左边
    //    temp.set(row, col);
    //    breakblack = false; breakwhite = false;
    //    for (int i = 0; i < 5; ++i)
    //    {
    //        if (!temp.displace4(1, d)) break;

    //        if (pieces[temp.row][temp.col].layer1 == PIECE_BLACK)
    //        {
    //            breakwhite = true;
    //            continue;
    //        }
    //        else if (pieces[temp.row][temp.col].layer1 == PIECE_WHITE)
    //        {
    //            breakblack = true;
    //            continue;
    //        }
    //        //update left pattern
    //        pieces[temp.row][temp.col].pattern[d][side] |= 1 << (4 - i);
    //        //update left offset
    //        if (pieces[temp.row][temp.col].pattern_offset[d][Util::otherside(side)][0] > i)
    //        {
    //            pieces[temp.row][temp.col].pattern_offset[d][Util::otherside(side)][0] = i;
    //        }

    //        //update layer2 and layer3
    //        if (!breakblack) update_layer3_with_layer2_new(temp.row, temp.col, PIECE_BLACK, d);
    //        if (!breakwhite) update_layer3_with_layer2_new(temp.row, temp.col, PIECE_WHITE, d);
    //    }
    //}
}

void ChessBoard::update_layer3_with_layer2_new(int8_t row, int8_t col, uint8_t side, uint8_t d)
{
    //int len, index;
    //len = pieces[row][col].pattern_offset[d][side][0] + pieces[row][col].pattern_offset[d][side][1] + 1;
    //index = pieces[row][col].pattern[d][side] & (0x07ff >> (5 - pieces[row][col].pattern_offset[d][side][1]));//len只有11
    //index = index >> (5 - pieces[row][col].pattern_offset[d][side][0]);
    //pieces[row][col].layer2[d][side] = (ban && side == PIECE_BLACK) ? layer2_table_ban[len][index * len + pieces[row][col].pattern_offset[d][side][0]] : layer2_table[len][index * len + pieces[row][col].pattern_offset[d][side][0]];

    //pieces[row][col].layer3[side] = layer2_to_layer3_table[pieces[row][col].layer2[0][side]][pieces[row][col].layer2[1][side]][pieces[row][col].layer2[2][side]][pieces[row][col].layer2[3][side]][(ban && side == PIECE_BLACK) ? 1 : 0];

    ////update highest
    //if (update_info_flag[side] != NEED)//本来就需要遍历的就不需要增量更新了
    //{
    //    if (highestRatings[side].pos.equel(row, col))
    //    {
    //        if (pieces[row][col].layer3[side] == CHESSTYPE_BAN || pieces[row][col].layer3[side] < highestRatings[side].chesstype)
    //        {
    //            update_info_flag[side] = UNSURE;
    //        }
    //    }

    //    if (pieces[row][col].layer3[side] != CHESSTYPE_BAN && pieces[row][col].layer3[side] > highestRatings[side].chesstype)
    //    {
    //        highestRatings[side].chesstype = pieces[row][col].layer3[side];
    //        highestRatings[side].pos.set(row, col);
    //        update_info_flag[side] = NONEED;
    //    }
    //}
}

void ChessBoard::update_layer2(int8_t row, int8_t col, uint8_t side, GAME_RULE ban)//落子处
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
            if (pieces[temp.row][temp.col].layer1 == side)
            {
                chessCount++;
                l_hash_index <<= 1;
                l_hash_index += 1;
            }
            else if (pieces[temp.row][temp.col].layer1 == PIECE_BLANK)
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
            if (pieces[temp.row][temp.col].layer1 == side)
            {
                chessCount++;
                r_hash_index |= 1 << r_offset;
            }
            else if (pieces[temp.row][temp.col].layer1 == PIECE_BLANK)
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

        if (pieces[row][col].layer1 == Util::otherside(side))
        {
            int len = l_offset;
            int index_offset = l_hash_index * len;
            Position pos_fix(row, col);
            pos_fix.displace4(-l_changeable_offset, d);
            //update
            for (int i = l_offset - l_changeable_offset; i < len; ++i, pos_fix.displace4(1, d))
            {
                if (pieces[pos_fix.row][pos_fix.col].layer1 == PIECE_BLANK)
                {
                    update_layer3_with_layer2(pos_fix.row, pos_fix.col, side, ban, d, len, index_offset + i);
                }
            }

            len = r_offset;
            index_offset = r_hash_index * len;
            pos_fix.set(row, col);
            pos_fix.displace4(1, d);
            //update
            for (int i = 0; i < r_changeable_offset; ++i, pos_fix.displace4(1, d))
            {
                if (pieces[pos_fix.row][pos_fix.col].layer1 == PIECE_BLANK)
                {
                    update_layer3_with_layer2(pos_fix.row, pos_fix.col, side, ban, d, len, index_offset + i);
                }
            }
        }
        else
        {
            int len = l_offset + r_offset + 1;
            int index_offset = ((((r_hash_index << 1) + (pieces[row][col].layer1 == side ? 1 : 0)) << l_offset) + l_hash_index) * len;
            Position pos_fix(row, col);
            pos_fix.displace4(-l_changeable_offset, d);
            //update
            for (int i = l_offset - l_changeable_offset; i < l_offset + 1 + r_changeable_offset; ++i, pos_fix.displace4(1, d))
            {
                if (pieces[pos_fix.row][pos_fix.col].layer1 == PIECE_BLANK)
                {
                    update_layer3_with_layer2(pos_fix.row, pos_fix.col, side, ban, d, len, index_offset + i);
                }
            }
        }
    }
}


void ChessBoard::update_layer3_with_layer2(int8_t row, int8_t col, uint8_t side, GAME_RULE rule, uint8_t d, int len, int chessHashIndex)
{
    uint8_t oldchesstype = pieces[row][col].layer3[side];
    uint8_t oldtype = pieces[row][col].layer2[d][side];

    pieces[row][col].layer2[d][side] = (rule == FREESTYLE || side == PIECE_WHITE) ? layer2_table[len][chessHashIndex] : layer2_table_ban[len][chessHashIndex];

    pieces[row][col].layer3[side] = layer2_to_layer3_table[pieces[row][col].layer2[0][side]][pieces[row][col].layer2[1][side]][pieces[row][col].layer2[2][side]][pieces[row][col].layer2[3][side]][(side == PIECE_BLACK) ? rule : FREESTYLE];

    //update highest
    if (update_info_flag[side] != NEED)//本来就需要遍历的就不需要增量更新了
    {
        if (highestRatings[side].pos.equel(row, col))
        {
            if (pieces[row][col].layer3[side] == CHESSTYPE_BAN || pieces[row][col].layer3[side] < highestRatings[side].chesstype)
            {
                update_info_flag[side] = UNSURE;
            }
        }

        if (pieces[row][col].layer3[side] != CHESSTYPE_BAN && pieces[row][col].layer3[side] > highestRatings[side].chesstype)
        {
            highestRatings[side].chesstype = pieces[row][col].layer3[side];
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
                    layer2_to_layer3_table[d1][d2][d3][d4][FREESTYLE] = layer2_to_layer3(d1, d2, d3, d4, FREESTYLE);
                    layer2_to_layer3_table[d1][d2][d3][d4][STANDARD] = layer2_to_layer3(d1, d2, d3, d4, STANDARD);
                    layer2_to_layer3_table[d1][d2][d3][d4][RENJU] = layer2_to_layer3(d1, d2, d3, d4, RENJU);
                }
            }
        }
    }
}

uint8_t ChessBoard::layer2_to_layer3(uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4, GAME_RULE rule)
{
    int count[CHESSTYPE_COUNT] = { 0 };
    ++count[d1]; ++count[d2]; ++count[d3]; ++count[d4];

    if (count[CHESSTYPE_5] > 0) return CHESSTYPE_5;//有5连可无视禁手
    if (count[CHESSTYPE_BAN] > 0) return CHESSTYPE_BAN;
    if (count[CHESSTYPE_44] > 0) return rule == RENJU ? CHESSTYPE_BAN : CHESSTYPE_44;
    if (count[CHESSTYPE_D4] + count[CHESSTYPE_D4P] + count[CHESSTYPE_4] > 1) return rule == RENJU ? CHESSTYPE_BAN : CHESSTYPE_44;//44优先级比4高，可能是禁手

    //特殊处理
    if (rule == RENJU)
    {
        if (count[CHESSTYPE_3] + count[CHESSTYPE_J3] > 1) return CHESSTYPE_BAN;
        if (count[CHESSTYPE_4] > 0) return CHESSTYPE_4;
        if (count[CHESSTYPE_D4] + count[CHESSTYPE_D4P] == 1 && count[CHESSTYPE_3] + count[CHESSTYPE_J3] > 0) return CHESSTYPE_43;
    }
    else
    {
        if (count[CHESSTYPE_4] > 0) return CHESSTYPE_4;
        if (count[CHESSTYPE_D4] + count[CHESSTYPE_D4P] == 1 && count[CHESSTYPE_3] + count[CHESSTYPE_J3] > 0) return CHESSTYPE_43;
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

int ChessBoard::getSimpleTotalScore(uint8_t side)
{
    int result = 0;
    ForEachPosition
    {
        if (pieces[pos.row][pos.col].layer1 == PIECE_BLANK)
        {
            result += getChessTypeInfo(pieces[pos.row][pos.col].layer3[side]).rating;
        }
    }
    return result;
}


void ChessBoard::updateChessInfo(uint8_t side)
{
    highestRatings[side].chesstype = CHESSTYPE_BAN;
    ForEachPosition
    {
        if (pieces[pos.row][pos.col].layer1 == PIECE_BLANK)
        {
            if (highestRatings[side].chesstype == CHESSTYPE_BAN)
            {
                highestRatings[side].chesstype = pieces[pos.row][pos.col].layer3[side];
                highestRatings[side].pos = pos;
            }
            else if (pieces[pos.row][pos.col].layer3[side] != CHESSTYPE_BAN && pieces[pos.row][pos.col].layer3[side] > highestRatings[side].chesstype)
            {
                highestRatings[side].chesstype = pieces[pos.row][pos.col].layer3[side];
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

bool ChessBoard::moveOnlyHash(Position pos)
{
    if (pieces[pos.row][pos.col].layer1 != PIECE_BLANK)
    {
        return false;//已有棋子
    }
    lastStep.step++;
    lastStep.changeSide();
    lastStep.pos.set(pos.row, pos.col);
    pieces[pos.row][pos.col].layer1 = lastStep.getState();
    updateHashPair(pos.row, pos.col, lastStep.getState(), true);
    return true;
}

bool ChessBoard::move(int8_t row, int8_t col, uint8_t side, GAME_RULE ban)
{
    if (pieces[row][col].layer1 != PIECE_BLANK)
    {
        return false;//已有棋子
    }
    lastStep.step++;
    lastStep.setState(side);
    lastStep.pos.set(row, col);
    lastStep.chessType = getChessType(row, col, side);

    pieces[row][col].layer1 = side;

    for (int i = 0; i < 2; ++i)
    {
        if (highestRatings[i].pos.equel(row, col))
        {
            update_info_flag[i] = UNSURE;
        }
    }

    update_layer2(row, col, ban);
    //update_layer2_new(row, col, side);

    updateHashPair(row, col, side, true);

    return true;
}

bool ChessBoard::unmove(Position pos, ChessStep last, GAME_RULE ban)
{
    uint8_t side = pieces[pos.row][pos.col].layer1;
    if (side == PIECE_BLANK || lastStep.step < 1)
    {
        return false;//没有棋子
    }
    lastStep = last;

    pieces[pos.row][pos.col].layer1 = PIECE_BLANK;

    update_layer2(pos.row, pos.col, ban);
    //update_layer2_new(row, col, PIECE_BLANK);

    updateHashPair(pos.row, pos.col, side, false);

    return true;
}

void ChessBoard::getDefendReletedPos(set<Position>& releted, Position center, uint8_t side)
{
    for (int d = 0; d < DIRECTION4_COUNT; ++d)
    {
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
        {
            int blankcount = 0;
            Position temppos = center;
            for (int8_t offset = 1; offset < 6; ++offset)
            {
                if (!temppos.displace4(symbol, d) || pieces[temppos.row][temppos.col].layer1 == Util::otherside(side))//equal otherside
                {
                    break;
                }

                if (pieces[temppos.row][temppos.col].layer1 == side)
                {
                    continue;
                }
                else if (pieces[temppos.row][temppos.col].layer1 == PIECE_BLANK)
                {
                    blankcount++;
                    if (pieces[temppos.row][temppos.col].layer2[d][side] > CHESSTYPE_0)
                    {
                        releted.insert(temppos);
                        getDefendReletedPos2(releted, temppos, side);//因为没有求交集，暂时去掉
                    }
                    else
                    {
                        releted.insert(temppos);
                    }
                }
                if (blankcount == 3)
                {
                    break;
                }
            }
        }
    }
}

void ChessBoard::getDefendReletedPos2(set<Position>& releted, Position center, uint8_t side)
{
    Position temppos;
    for (int d = 0; d < DIRECTION4_COUNT; ++d)
    {
        if (pieces[center.row][center.col].layer2[d][side] == CHESSTYPE_0 && pieces[center.row][center.col].layer2[d][Util::otherside(side)] == CHESSTYPE_0)
        {
            continue;
        }
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
        {
            int blankcount = 0;
            temppos = center;
            for (int8_t offset = 1; offset < 5; ++offset)
            {
                if (!temppos.displace4(symbol, d))//equal otherside
                {
                    break;
                }

                if (pieces[temppos.row][temppos.col].layer1 == PIECE_BLANK)
                {
                    blankcount++;
                    if (pieces[temppos.row][temppos.col].layer2[d][Util::otherside(side)] > CHESSTYPE_D3)
                    {
                        releted.insert(temppos);
                    }
                    else if (pieces[temppos.row][temppos.col].layer2[d][Util::otherside(side)] > CHESSTYPE_0 && pieces[temppos.row][temppos.col].layer3[Util::otherside(side)] > CHESSTYPE_D3)
                    {
                        releted.insert(temppos);
                    }
                }
                else
                {
                    continue;
                }

                if (blankcount == 2)
                {
                    break;
                }
            }
        }
    }
}

void ChessBoard::getAtackReletedPos(set<Position>& releted, Position center, uint8_t side)
{
    for (int d = 0; d < DIRECTION4_COUNT; ++d)
    {
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
        {
            int blankcount = 0;
            Position temppos = center;
            for (int8_t offset = 1; offset < 6; ++offset)
            {
                if (!temppos.displace4(symbol, d) || pieces[temppos.row][temppos.col].layer1 == Util::otherside(side))//equal otherside
                {
                    break;
                }

                if (pieces[temppos.row][temppos.col].layer1 == side)
                {
                    continue;
                }
                else if (pieces[temppos.row][temppos.col].layer1 == PIECE_BLANK)
                {
                    blankcount++;
                    if (pieces[temppos.row][temppos.col].layer2[d][side] > CHESSTYPE_0)
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
                    //    if (pieces[temppos.row][temppos.col].layer3[side] > CHESSTYPE_2)
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
        if (pieces[center.row][center.col].layer2[d][side] == CHESSTYPE_0)
        {
            continue;
        }
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
        {
            int blankcount = 0;
            temppos = center;
            for (int8_t offset = 1; offset < 5; ++offset)
            {
                if (!temppos.displace4(symbol, d) || pieces[temppos.row][temppos.col].layer1 == Util::otherside(side))//equal otherside
                {
                    break;
                }

                if (pieces[temppos.row][temppos.col].layer1 == side)
                {
                    continue;
                }
                else if (pieces[temppos.row][temppos.col].layer1 == PIECE_BLANK)
                {
                    blankcount++;
                    if (pieces[temppos.row][temppos.col].layer2[d][side] > CHESSTYPE_0)
                    {
                        releted.insert(temppos);
                    }
                    else if (pieces[temppos.row][temppos.col].layer3[side] > CHESSTYPE_D3)
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
                if (!temppos.valid() || pieces[temppos.row][temppos.col].layer1 == Util::otherside(side))//equal otherside
                {
                    break;
                }

                if (pieces[temppos.row][temppos.col].layer1 == side)
                {
                    continue;
                }
                else if (pieces[temppos.row][temppos.col].layer1 == PIECE_BLANK)
                {
                    if (pieces[temppos.row][temppos.col].layer3[side] == CHESSTYPE_BAN)
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
                    if (!temppos.valid() || pieces[temppos.row][temppos.col].layer1 == side)//equal otherside
                    {
                        break;
                    }

                    if (pieces[temppos.row][temppos.col].layer1 == PIECE_BLANK)
                    {
                        if (pieces[temppos.row][temppos.col].layer3[Util::otherside(side)] >= CHESSTYPE_J3)
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

const ChessTypeInfo chesstypes[CHESSTYPE_COUNT] = {
    { 0    ,   0,   0 },           //CHESSTYPE_0,  +CHESSTYPE_2*2 +CHESSTYPE_J2*2 (0)
    { 10   ,   8,   0 },           //CHESSTYPE_j2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
    { 10   ,  10,   2 },           //CHESSTYPE_2,  -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*2 +CHESSTYPE_J3*2 (0)
    { 10   ,  10,   2 },           //CHESSTYPE_d3, -CHESSTYPE_D3*2 +CHESSTYPE_D4*2 (0)
    { 80   ,   6,   6 },           //CHESSTYPE_J3  -CHESSTYPE_3*1 -CHESSTYPE_J3*2 +CHESSTYPE_4*1 +CHESSTYPE_D4*2 (0)
    { 100  ,   8,  10 },           //CHESSTYPE_3,  -CHESSTYPE_3*2 -CHESSTYPE_J3*2 +CHESSTYPE_4*2 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
    { 120  ,   8,  10 },           //CHESSTYPE_d4, -CHESSTYPE_D4*2 +CHESSTYPE_5 (0) 优先级降低
    { 150  ,  10,  10 },           //CHESSTYPE_d4p -CHESSTYPE_D4P*1 -CHESSTYPE_D4 +CHESSTYPE_5 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
    { 250  ,  20,  20 },           //CHESSTYPE_33, -CHESSTYPE_33*1 -CHESSTYPE_3*0-2 -CHESSTYPE_J3*2-4 +CHESSTYPE_4*2-4 +CHESSTYPE_D4*2-4 (CHESSTYPE_4*2)
    { 450  ,  50,  20 },           //CHESSTYPE_43, -CHESSTYPE_43*1 -CHESSTYPE_D4*1 -CHESSTYPE_J3*2 -CHESSTYPE_3*1 +CHESSTYPE_5*1 +CHESSTYPE_4*2 (CHESSTYPE_4*2)
    { 500  , 100,  20 },           //CHESSTYPE_44, -CHESSTYPE_44 -CHESSTYPE_D4*2 +2个CHESSTYPE_5    (CHESSTYPE_5)
    { 500  , 100,  20 },           //CHESSTYPE_4,  -CHESSTYPE_4*1-2 -CHESSTYPE_D4*1-2 +CHESSTYPE_5*2 (CHESSTYPE_5)
    { 10000, 100, 150 },           //CHESSTYPE_5,
    { -100 ,-100, -10 },           //CHESSTYPE_BAN,
};

//进行情景细分，主要聚焦于对局势的影响趋势，而非局势本身
//所以就算本身为CHESSTYPE_0，也有可能对局势有很大影响
int ChessBoard::getRelatedFactor(Position pos, uint8_t side, bool defend)
{
    int base_factor = 0;//初始值

    if (pieces[pos.row][pos.col].layer3[side] > CHESSTYPE_D4P)
    {
        return defend ? chesstypes[pieces[pos.row][pos.col].layer3[side]].defendBaseFactor : chesstypes[pieces[pos.row][pos.col].layer3[side]].atackBaseFactor;
    }
    Position temppos;
    for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
    {
        base_factor += defend ? chesstypes[pieces[pos.row][pos.col].layer3[side]].defendBaseFactor : chesstypes[pieces[pos.row][pos.col].layer2[d][side]].atackBaseFactor;

        //related factor, except base 
        int releted_count_3 = 0;
        int releted_count_d4 = 0;
        int blank[2] = { 0,0 }; // 0 left 1 right
        int chess[2] = { 0,0 };

        for (int i = 0; i < 2; ++i)//正反
        {
            temppos = pos;
            while (temppos.displace8(1, d * 2 + i))
            {
                if (pieces[temppos.row][temppos.col].layer1 == side)
                {
                    chess[i]++;
                }
                else if (pieces[temppos.row][temppos.col].layer1 == PIECE_BLANK)
                {
                    blank[i]++;
                    if (pieces[temppos.row][temppos.col].layer3[side] != CHESSTYPE_BAN)
                    {
                        for (uint8_t d2 = 0; d2 < DIRECTION4_COUNT; ++d2)
                        {
                            if (d == d2) continue;

                            if (pieces[temppos.row][temppos.col].layer2[d2][side] > CHESSTYPE_3)
                            {
                                releted_count_d4++;
                            }
                            else if (pieces[temppos.row][temppos.col].layer2[d2][side] > CHESSTYPE_D3)
                            {
                                releted_count_3++;
                            }
                        }
                    }
                }
                else
                {
                    break;
                }

                if (blank[i] == 3 || blank[i] + chess[i] == 5)
                {
                    break;
                }
            }
        }
        if (relatedsituation[blank[0]][chess[0]][blank[1]][chess[1]])//至少要5才能有威胁 加上自身
        {
            base_factor += defend ? (releted_count_d4 * 5 + releted_count_3 * 5) : (releted_count_d4 * 6 + releted_count_3 * 4);
        }
    }
    return base_factor;
}

//const ChessTypeInfo chesstypes[CHESSTYPE_COUNT] = {
//    { 0    ,   0,   0 },           //CHESSTYPE_0,  +CHESSTYPE_2*2 +CHESSTYPE_J2*2 (0)
//    { 10   ,   5,   0 },           //CHESSTYPE_j2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
//    { 10   ,   7,   2 },           //CHESSTYPE_2,  -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*2 +CHESSTYPE_J3*2 (0)
//    { 10   ,   7,   5 },           //CHESSTYPE_d3, -CHESSTYPE_D3*2 +CHESSTYPE_D4*2 (0)
//    { 80   ,  15,  10 },           //CHESSTYPE_J3  -CHESSTYPE_3*1 -CHESSTYPE_J3*2 +CHESSTYPE_4*1 +CHESSTYPE_D4*2 (0)
//    { 100  ,  20,  15 },           //CHESSTYPE_3,  -CHESSTYPE_3*2 -CHESSTYPE_J3*2 +CHESSTYPE_4*2 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
//    { 120  ,   0,  15 },           //CHESSTYPE_d4, -CHESSTYPE_D4*2 +CHESSTYPE_5 (0) 优先级降低
//    { 150  ,  20,  20 },           //CHESSTYPE_d4p -CHESSTYPE_D4P*1 -CHESSTYPE_D4 +CHESSTYPE_5 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
//    { 250  ,  30,  25 },           //CHESSTYPE_33, -CHESSTYPE_33*1 -CHESSTYPE_3*0-2 -CHESSTYPE_J3*2-4 +CHESSTYPE_4*2-4 +CHESSTYPE_D4*2-4 (CHESSTYPE_4*2)
//    { 450  ,  35,  30 },           //CHESSTYPE_43, -CHESSTYPE_43*1 -CHESSTYPE_D4*1 -CHESSTYPE_J3*2 -CHESSTYPE_3*1 +CHESSTYPE_5*1 +CHESSTYPE_4*2 (CHESSTYPE_4*2)
//    { 500  , 100,  30 },           //CHESSTYPE_44, -CHESSTYPE_44 -CHESSTYPE_D4*2 +2个CHESSTYPE_5    (CHESSTYPE_5)
//    { 500  , 100,  30 },           //CHESSTYPE_4,  -CHESSTYPE_4*1-2 -CHESSTYPE_D4*1-2 +CHESSTYPE_5*2 (CHESSTYPE_5)
//    { 10000, 100, 150 },           //CHESSTYPE_5,
//    { -100 , -90,  30 },           //CHESSTYPE_BAN,
//};
//
//int ChessBoard::getRelatedFactor(Position pos, uint8_t side, bool defend)
//{
//    int base_factor = 0;//初始值
//    if (defend)
//    {
//        int count = 0;
//        if (pieces[pos.row][pos.col].layer3[side] > CHESSTYPE_D4P)
//        {
//            return chesstypes[pieces[pos.row][pos.col].layer3[side]].defendBaseFactor;
//        }
//
//        for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
//        {
//            if (pieces[pos.row][pos.col].layer2[d][side] > CHESSTYPE_D3)
//            {
//                if (pieces[pos.row][pos.col].layer2[d][side] == CHESSTYPE_J3)//特殊处理
//                {
//                    bool no_use = false;
//                    Position temppos;
//                    for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
//                    {
//                        temppos = pos.getNextPosition(d, symbol);
//
//                        if (!temppos.valid() || pieces[temppos.row][temppos.col].layer2[d][side] != CHESSTYPE_3)
//                        {
//                            continue;
//                        }
//                        temppos = pos.getNextPosition(d, 4 * symbol);
//                        if (temppos.valid() && pieces[temppos.row][temppos.col].layer2[d][side] == CHESSTYPE_3)//?!?oo??? 无用
//                        {
//                            base_factor += 5;
//                            no_use = true;
//                            break;
//                        }
//                    }
//                    if (!no_use)
//                    {
//                        base_factor += chesstypes[pieces[pos.row][pos.col].layer2[d][side]].defendBaseFactor;
//                    }
//                }
//                else
//                {
//                    base_factor += chesstypes[pieces[pos.row][pos.col].layer2[d][side]].defendBaseFactor;
//                }
//            }
//            else if (pieces[pos.row][pos.col].layer2[d][side] > CHESSTYPE_0)
//            {
//                count += 1;
//            }
//
//        }
//        if (count > 1)
//        {
//            if (pieces[pos.row][pos.col].layer3[side] < CHESSTYPE_J3)
//            {
//                base_factor += count * 7;
//            }
//            else
//            {
//                base_factor += 10;
//            }
//
//        }
//        else if (count == 1)
//        {
//            if (pieces[pos.row][pos.col].layer3[side] < CHESSTYPE_J3)
//            {
//                base_factor = chesstypes[pieces[pos.row][pos.col].layer3[side]].defendBaseFactor;
//            }
//            else
//            {
//                base_factor += 5;
//            }
//        }
//        return base_factor;
//    }
//
//    if (pieces[pos.row][pos.col].layer3[side] > CHESSTYPE_D4P)
//    {
//        return chesstypes[pieces[pos.row][pos.col].layer3[side]].atackBaseFactor;
//    }
//    Position temppos;
//    for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
//    {
//        base_factor += chesstypes[pieces[pos.row][pos.col].layer2[d][side]].atackBaseFactor;
//
//        //related factor, except base 
//        int releted_count_3 = 0;
//        int releted_count_d4 = 0;
//        int availi_count = 0;
//
//        for (int i = 0; i < 2; ++i)//正反
//        {
//            int blank = 0;
//            temppos = pos;
//            while (temppos.displace8(1, d * 2 + i))
//            {
//                if (pieces[temppos.row][temppos.col].layer1 == side)
//                {
//                    availi_count++;
//                    continue;
//                }
//                else if (pieces[temppos.row][temppos.col].layer1 == PIECE_BLANK)
//                {
//                    availi_count++;
//                    blank++;
//
//                    for (uint8_t d2 = 0; d2 < DIRECTION4_COUNT; ++d2)
//                    {
//                        if (d == d2) continue;
//
//                        if (pieces[temppos.row][temppos.col].layer2[d2][side] > CHESSTYPE_3)
//                        {
//                            releted_count_d4++;
//                        }
//                        else if (pieces[temppos.row][temppos.col].layer2[d2][side] > CHESSTYPE_D3)
//                        {
//                            releted_count_3++;
//                        }
//                    }
//                }
//                else
//                {
//                    break;
//                }
//
//                if (blank == 3)
//                {
//                    break;
//                }
//            }
//        }
//        if (availi_count > 3)//至少要5才能有威胁
//        {
//            if (pieces[pos.row][pos.col].layer2[d][side] == CHESSTYPE_0)
//            {
//                if (releted_count_d4 > 1)
//                {
//                    base_factor += 30;
//                }
//                else if (releted_count_3 > 1)
//                {
//                    base_factor += 20;
//                }
//                else if (releted_count_d4 + releted_count_3 > 1)
//                {
//                    base_factor += 25;
//                }
//            }
//            else//> CHESSTYPE_0
//            {
//                base_factor += releted_count_d4 * 15 + releted_count_3 * 10;
//            }
//        }
//    }
//    return base_factor;
//}

double ChessBoard::getStaticFactor(Position pos, uint8_t side, bool defend)
{
    uint8_t layer3type = pieces[pos.row][pos.col].layer3[side];
    if (layer3type < CHESSTYPE_J3 || layer3type > CHESSTYPE_D4P)
    {
        return 1.0;
    }

    double base_factor = 1.0;//初始值

    Position temppos;
    bool findself = false;
    for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
    {
        if (!findself && pieces[pos.row][pos.col].layer2[d][side] == layer3type)
        {
            findself = true;
            continue;
        }
        else
        {
            if (pieces[pos.row][pos.col].layer2[d][side] > CHESSTYPE_0)
            {
                base_factor += 0.5;
            }
        }

        //related factor, except base 
        int releted_count_2 = 0;
        int releted_count_3 = 0;
        int releted_count_d4 = 0;
        int blank[2] = { 0,0 }; // 0 left 1 right
        int chess[2] = { 0,0 };

        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
        {
            temppos = pos;
            while (temppos.displace4(symbol, d))
            {
                if (pieces[temppos.row][temppos.col].layer1 == Util::otherside(side))//equal otherside
                {
                    break;
                }
                else if (pieces[temppos.row][temppos.col].layer1 == side)
                {
                    chess[i]++;
                }
                else//blank
                {
                    blank[i]++;
                    if (pieces[temppos.row][temppos.col].layer3[side] != CHESSTYPE_BAN)
                    {
                        for (uint8_t d2 = 0; d2 < DIRECTION4_COUNT; ++d2)
                        {
                            if (d == d2) continue;

                            if (pieces[temppos.row][temppos.col].layer2[d2][side] > CHESSTYPE_3)
                            {
                                releted_count_d4++;
                            }
                            else if (pieces[temppos.row][temppos.col].layer2[d2][side] > CHESSTYPE_D3)
                            {
                                releted_count_3++;
                            }
                            else if (pieces[pos.row][pos.col].layer2[d][side] > CHESSTYPE_0 && pieces[temppos.row][temppos.col].layer2[d2][side] > CHESSTYPE_J2)
                            {
                                releted_count_2++;
                            }
                        }
                    }
                }
                if (blank[i] == 3 || blank[i] + chess[i] == 5)
                {
                    break;
                }
            }
        }

        if (relatedsituation[blank[0]][chess[0]][blank[1]][chess[1]])
        {
            base_factor += releted_count_2*0.2 + releted_count_3*0.4 + releted_count_d4*0.6;
        }
    }
    return base_factor;
}

struct StaticEvaluate
{
    int atack;
    int defend;
};

//const StaticEvaluate staticEvaluate[CHESSTYPE_COUNT] = {
//    {    0,  0 },           //CHESSTYPE_0,  +CHESSTYPE_2*2 +CHESSTYPE_J2*2 (0)
//    {    2,  2 },           //CHESSTYPE_j2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
//    {    4,  4 },           //CHESSTYPE_2,  -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*2 +CHESSTYPE_J3*2 (0)
//    {    4,  4 },           //CHESSTYPE_d3, -CHESSTYPE_D3*2 +CHESSTYPE_D4*2 (0)
//    {   12,  8 },           //CHESSTYPE_J3  -CHESSTYPE_3*1 -CHESSTYPE_J3*2 +CHESSTYPE_4*1 +CHESSTYPE_D4*2 (0)
//    {   16, 12 },           //CHESSTYPE_3,  -CHESSTYPE_3*2 -CHESSTYPE_J3*2 +CHESSTYPE_4*2 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
//    {   18, 14 },           //CHESSTYPE_d4, -CHESSTYPE_D4*2 +CHESSTYPE_5 (0) 优先级降低
//    {   20, 16 },           //CHESSTYPE_d4p -CHESSTYPE_D4P*1 -CHESSTYPE_D4 +CHESSTYPE_5 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
//    {   40, 30 },           //CHESSTYPE_33, -CHESSTYPE_33*1 -CHESSTYPE_3*0-2 -CHESSTYPE_J3*2-4 +CHESSTYPE_4*2-4 +CHESSTYPE_D4*2-4 (CHESSTYPE_4*2)
//    {   50, 35 },           //CHESSTYPE_43, -CHESSTYPE_43*1 -CHESSTYPE_D4*1 -CHESSTYPE_J3*2 -CHESSTYPE_3*1 +CHESSTYPE_5*1 +CHESSTYPE_4*2 (CHESSTYPE_4*2)
//    {   60, 40 },           //CHESSTYPE_44, -CHESSTYPE_44 -CHESSTYPE_D4*2 +2个CHESSTYPE_5    (CHESSTYPE_5)
//    {   60, 40 },           //CHESSTYPE_4,  -CHESSTYPE_4*1-2 -CHESSTYPE_D4*1-2 +CHESSTYPE_5*2 (CHESSTYPE_5)
//    {10000,100 },           //CHESSTYPE_5,
//    {  -30,-30 },           //CHESSTYPE_BAN,
//};

const StaticEvaluate staticEvaluate[CHESSTYPE_COUNT] = {
    {    0,  0 },           //CHESSTYPE_0,  +CHESSTYPE_2*2 +CHESSTYPE_J2*2 (0)
    {    2,  1 },           //CHESSTYPE_j2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
    {    4,  3 },           //CHESSTYPE_2,  -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*2 +CHESSTYPE_J3*2 (0)
    {    4,  3 },           //CHESSTYPE_d3, -CHESSTYPE_D3*2 +CHESSTYPE_D4*2 (0)
    {   12,  8 },           //CHESSTYPE_J3  -CHESSTYPE_3*1 -CHESSTYPE_J3*2 +CHESSTYPE_4*1 +CHESSTYPE_D4*2 (0)
    {   16, 12 },           //CHESSTYPE_3,  -CHESSTYPE_3*2 -CHESSTYPE_J3*2 +CHESSTYPE_4*2 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
    {   14, 10 },           //CHESSTYPE_d4, -CHESSTYPE_D4*2 +CHESSTYPE_5 (0) 优先级降低
    {   16, 12 },           //CHESSTYPE_d4p -CHESSTYPE_D4P*1 -CHESSTYPE_D4 +CHESSTYPE_5 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
    {   40, 30 },           //CHESSTYPE_33, -CHESSTYPE_33*1 -CHESSTYPE_3*0-2 -CHESSTYPE_J3*2-4 +CHESSTYPE_4*2-4 +CHESSTYPE_D4*2-4 (CHESSTYPE_4*2)
    {   50, 35 },           //CHESSTYPE_43, -CHESSTYPE_43*1 -CHESSTYPE_D4*1 -CHESSTYPE_J3*2 -CHESSTYPE_3*1 +CHESSTYPE_5*1 +CHESSTYPE_4*2 (CHESSTYPE_4*2)
    {   60, 40 },           //CHESSTYPE_44, -CHESSTYPE_44 -CHESSTYPE_D4*2 +2个CHESSTYPE_5    (CHESSTYPE_5)
    {   60, 40 },           //CHESSTYPE_4,  -CHESSTYPE_4*1-2 -CHESSTYPE_D4*1-2 +CHESSTYPE_5*2 (CHESSTYPE_5)
    {10000,100 },           //CHESSTYPE_5,
    {  -20,-20 },           //CHESSTYPE_BAN,
};



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

        atack_evaluate += (int)(staticEvaluate[pieces[pos.row][pos.col].layer3[atackside]].atack*getStaticFactor(pos, atackside));

        defend_evaluate += (int)(staticEvaluate[pieces[pos.row][pos.col].layer3[defendside]].defend*getStaticFactor(pos, defendside));
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


    atack += (int)(staticEvaluate[pieces[pos.row][pos.col].layer3[atackside]].atack*getStaticFactor(pos, atackside));
    ss << (int)(staticEvaluate[pieces[pos.row][pos.col].layer3[atackside]].atack*getStaticFactor(pos, atackside));


    ss << "|";


    defend += (int)(staticEvaluate[pieces[pos.row][pos.col].layer3[defendside]].defend*getStaticFactor(pos, defendside));
    ss << (int)(staticEvaluate[pieces[pos.row][pos.col].layer3[defendside]].defend*getStaticFactor(pos, defendside));

    ss << "\t";
    }
    ss << "\r\n\r\n\r\n";
    ss << "atack:" << atack << "|" << "defend:" << defend;
    s = ss.str();

}



ChessTypeInfo ChessBoard::getChessTypeInfo(uint8_t type)
{
    return chesstypes[type];
}

void ChessBoard::initHash()
{
    ForEachPosition
    {
        hash.check_key ^= zcheck[pos.row][pos.col][pieces[pos.row][pos.col].layer1];
        hash.hash_key ^= zkey[pos.row][pos.col][pieces[pos.row][pos.col].layer1];
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
    default_random_engine e(6847);//fixed seed

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

void ChessBoard::initRelatedSituation()
{
    for (int left_blank = 0; left_blank < 5; ++left_blank)
    {
        for (int left_chess = 0; left_chess < 5; ++left_chess)
        {
            for (int right_blank = 0; right_blank < 5; ++right_blank)
            {
                for (int right_chess = 0; right_chess < 5; ++right_chess)
                {
                    if (left_blank + left_chess + right_blank + right_chess < 4)
                    {
                        relatedsituation[left_blank][left_chess][right_blank][right_chess] = false;
                        continue;
                    }

                    if (left_blank + left_chess == 0)// right_blank + right_chess >=4
                    {
                        if (right_chess < 2)
                        {
                            relatedsituation[left_blank][left_chess][right_blank][right_chess] = false;
                            continue;
                        }
                        else
                        {
                            relatedsituation[left_blank][left_chess][right_blank][right_chess] = true;
                            continue;
                        }
                    }

                    if (right_blank + right_chess == 0)
                    {
                        if (left_chess < 2)
                        {
                            relatedsituation[left_blank][left_chess][right_blank][right_chess] = false;
                            continue;
                        }
                        else
                        {
                            relatedsituation[left_blank][left_chess][right_blank][right_chess] = true;
                            continue;
                        }
                    }

                    if (left_blank == 1 && left_chess == 0)
                    {
                        if (right_blank + right_chess > 3)
                        {
                            relatedsituation[left_blank][left_chess][right_blank][right_chess] = true;
                            continue;
                        }
                        else
                        {
                            relatedsituation[left_blank][left_chess][right_blank][right_chess] = false;
                            continue;
                        }
                    }

                    if (right_blank == 1 && right_chess == 0)
                    {
                        if (left_blank + left_chess > 3)
                        {
                            relatedsituation[left_blank][left_chess][right_blank][right_chess] = true;
                            continue;
                        }
                        else
                        {
                            relatedsituation[left_blank][left_chess][right_blank][right_chess] = false;
                            continue;
                        }
                    }

                    relatedsituation[left_blank][left_chess][right_blank][right_chess] = true;
                }
            }
        }
    }
}
