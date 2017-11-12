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
    initPatternToLayer2Table();
    initRelatedSituation();
}

uint8_t* ChessBoard::layer2_table[BOARD_SIZE_MAX + 1] = { NULL };
uint8_t* ChessBoard::layer2_table_ban[BOARD_SIZE_MAX + 1] = { NULL };
uint8_t ChessBoard::layer2_to_layer3_table[CHESSTYPE_COUNT][CHESSTYPE_COUNT][CHESSTYPE_COUNT][CHESSTYPE_COUNT][3];
uint8_t ChessBoard::pattern_to_layer2_table[256][256];//2^8
uint8_t ChessBoard::pattern_to_layer2_table_ban[256][256];//2^8
bool ChessBoard::relatedsituation[5][5][5][5];

ChessBoard::ChessBoard()
{
    lastStep.step = 0;
    lastStep.state = PIECE_WHITE;//下一个就是黑了
    //highestRatings[0].chesstype = CHESSTYPE_0;
    //highestRatings[1].chesstype = CHESSTYPE_0;
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
            pieces[pos.row][pos.col].layer2[PIECE_BLACK][d] = 0;
            pieces[pos.row][pos.col].layer2[PIECE_WHITE][d] = 0;
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

static uint8_t pattern_init_help[8] = { 0xF0,0xE0,0xC0,0x80, 0x01,0x03,0x07,0x0F };
void ChessBoard::init_pattern()
{
    ForEachPosition
    {
        for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
        {
            pieces[pos.row][pos.col].pattern[PIECE_BLACK][d] = 0;
            pieces[pos.row][pos.col].pattern[PIECE_WHITE][d] = 0;
        }

        if (pos.col < 4)
        {
            pieces[pos.row][pos.col].pattern[PIECE_BLACK][DIRECTION4_LR] |= pattern_init_help[pos.col];
            pieces[pos.row][pos.col].pattern[PIECE_WHITE][DIRECTION4_LR] |= pattern_init_help[pos.col];
            pieces[pos.row][pos.col].pattern[PIECE_BLACK][DIRECTION4_RD] |= pattern_init_help[pos.col];
            pieces[pos.row][pos.col].pattern[PIECE_WHITE][DIRECTION4_RD] |= pattern_init_help[pos.col];
            pieces[pos.row][pos.col].pattern[PIECE_BLACK][DIRECTION4_RU] |= pattern_init_help[pos.col];
            pieces[pos.row][pos.col].pattern[PIECE_WHITE][DIRECTION4_RU] |= pattern_init_help[pos.col];
        }

        if (pos.col > Util::BoardSize - 5)
        {
            pieces[pos.row][pos.col].pattern[PIECE_BLACK][DIRECTION4_LR] |= pattern_init_help[7 - (Util::BoardSize - 1 - pos.col)];
            pieces[pos.row][pos.col].pattern[PIECE_WHITE][DIRECTION4_LR] |= pattern_init_help[7 - (Util::BoardSize - 1 - pos.col)];
            pieces[pos.row][pos.col].pattern[PIECE_BLACK][DIRECTION4_RD] |= pattern_init_help[7 - (Util::BoardSize - 1 - pos.col)];
            pieces[pos.row][pos.col].pattern[PIECE_WHITE][DIRECTION4_RD] |= pattern_init_help[7 - (Util::BoardSize - 1 - pos.col)];
            pieces[pos.row][pos.col].pattern[PIECE_BLACK][DIRECTION4_RU] |= pattern_init_help[7 - (Util::BoardSize - 1 - pos.col)];
            pieces[pos.row][pos.col].pattern[PIECE_WHITE][DIRECTION4_RU] |= pattern_init_help[7 - (Util::BoardSize - 1 - pos.col)];
        }

        if (pos.row < 4)
        {
            pieces[pos.row][pos.col].pattern[PIECE_BLACK][DIRECTION4_UD] |= pattern_init_help[pos.row];
            pieces[pos.row][pos.col].pattern[PIECE_WHITE][DIRECTION4_UD] |= pattern_init_help[pos.row];
            pieces[pos.row][pos.col].pattern[PIECE_BLACK][DIRECTION4_RD] |= pattern_init_help[pos.row];
            pieces[pos.row][pos.col].pattern[PIECE_WHITE][DIRECTION4_RD] |= pattern_init_help[pos.row];
            pieces[pos.row][pos.col].pattern[PIECE_BLACK][DIRECTION4_RU] |= pattern_init_help[7 - pos.row];
            pieces[pos.row][pos.col].pattern[PIECE_WHITE][DIRECTION4_RU] |= pattern_init_help[7 - pos.row];
        }

        if (pos.row > Util::BoardSize - 5)
        {
            pieces[pos.row][pos.col].pattern[PIECE_BLACK][DIRECTION4_UD] |= pattern_init_help[7 - (Util::BoardSize - 1 - pos.row)];
            pieces[pos.row][pos.col].pattern[PIECE_WHITE][DIRECTION4_UD] |= pattern_init_help[7 - (Util::BoardSize - 1 - pos.row)];
            pieces[pos.row][pos.col].pattern[PIECE_BLACK][DIRECTION4_RD] |= pattern_init_help[7 - (Util::BoardSize - 1 - pos.row)];
            pieces[pos.row][pos.col].pattern[PIECE_WHITE][DIRECTION4_RD] |= pattern_init_help[7 - (Util::BoardSize - 1 - pos.row)];
            pieces[pos.row][pos.col].pattern[PIECE_BLACK][DIRECTION4_RU] |= pattern_init_help[Util::BoardSize - 1 - pos.row];
            pieces[pos.row][pos.col].pattern[PIECE_WHITE][DIRECTION4_RU] |= pattern_init_help[Util::BoardSize - 1 - pos.row];
        }

        pieces[pos.row][pos.col].around[PIECE_BLACK] = 0;
        pieces[pos.row][pos.col].around[PIECE_WHITE] = 0;
    }
}


void ChessBoard::update_layer(int8_t row, int8_t col, uint8_t side, GAME_RULE rule)
{
    global_chesstype_count[PIECE_BLACK][pieces[row][col].layer3[PIECE_BLACK]]--;
    global_chesstype_count[PIECE_WHITE][pieces[row][col].layer3[PIECE_WHITE]]--;
    for (uint8_t d4 = 0; d4 < DIRECTION4_COUNT; ++d4)
    {
        //left
        Position temp(row, col);
        uint8_t d8 = d4 * 2;
        for (uint8_t p = 8; p != 0; p >>= 1)
        {
            if (!temp.displace8(d8)) break;
            pieces[temp.row][temp.col].pattern[side][d4] |= p;
            if (pieces[temp.row][temp.col].layer1 == PIECE_BLANK)
            {
                if (rule == STANDARD)
                {
                    pieces[temp.row][temp.col].layer2[PIECE_BLACK][d4] = pattern_to_layer2_table_ban[pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]][pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]];
                    pieces[temp.row][temp.col].layer2[PIECE_WHITE][d4] = pattern_to_layer2_table_ban[pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]][pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]];
                }
                else
                {
                    pieces[temp.row][temp.col].layer2[PIECE_BLACK][d4] = (rule == FREESTYLE) ? pattern_to_layer2_table[pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]][pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]] : pattern_to_layer2_table_ban[pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]][pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]];
                    pieces[temp.row][temp.col].layer2[PIECE_WHITE][d4] = pattern_to_layer2_table[pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]][pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]];
                }

                global_chesstype_count[PIECE_BLACK][pieces[temp.row][temp.col].layer3[PIECE_BLACK]]--;
                global_chesstype_count[PIECE_WHITE][pieces[temp.row][temp.col].layer3[PIECE_WHITE]]--;

                pieces[temp.row][temp.col].layer3[PIECE_BLACK] = layer2_to_layer3_table[pieces[temp.row][temp.col].layer2[PIECE_BLACK][0]][pieces[temp.row][temp.col].layer2[PIECE_BLACK][1]][pieces[temp.row][temp.col].layer2[PIECE_BLACK][2]][pieces[temp.row][temp.col].layer2[PIECE_BLACK][3]][rule];
                pieces[temp.row][temp.col].layer3[PIECE_WHITE] = layer2_to_layer3_table[pieces[temp.row][temp.col].layer2[PIECE_WHITE][0]][pieces[temp.row][temp.col].layer2[PIECE_WHITE][1]][pieces[temp.row][temp.col].layer2[PIECE_WHITE][2]][pieces[temp.row][temp.col].layer2[PIECE_WHITE][3]][FREESTYLE];

                global_chesstype_count[PIECE_BLACK][pieces[temp.row][temp.col].layer3[PIECE_BLACK]]++;
                global_chesstype_count[PIECE_WHITE][pieces[temp.row][temp.col].layer3[PIECE_WHITE]]++;
            }
        }

        //right
        temp.set(row, col);
        d8 += 1;
        for (uint8_t p = 16; p != 0; p <<= 1)
        {
            if (!temp.displace8(d8)) break;
            pieces[temp.row][temp.col].pattern[side][d4] |= p;
            if (pieces[temp.row][temp.col].layer1 == PIECE_BLANK)
            {
                if (rule == STANDARD)
                {
                    pieces[temp.row][temp.col].layer2[PIECE_BLACK][d4] = pattern_to_layer2_table_ban[pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]][pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]];
                    pieces[temp.row][temp.col].layer2[PIECE_WHITE][d4] = pattern_to_layer2_table_ban[pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]][pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]];
                }
                else
                {
                    pieces[temp.row][temp.col].layer2[PIECE_BLACK][d4] = (rule == FREESTYLE) ? pattern_to_layer2_table[pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]][pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]] : pattern_to_layer2_table_ban[pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]][pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]];
                    pieces[temp.row][temp.col].layer2[PIECE_WHITE][d4] = pattern_to_layer2_table[pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]][pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]];
                }

                global_chesstype_count[PIECE_BLACK][pieces[temp.row][temp.col].layer3[PIECE_BLACK]]--;
                global_chesstype_count[PIECE_WHITE][pieces[temp.row][temp.col].layer3[PIECE_WHITE]]--;

                pieces[temp.row][temp.col].layer3[PIECE_BLACK] = layer2_to_layer3_table[pieces[temp.row][temp.col].layer2[PIECE_BLACK][0]][pieces[temp.row][temp.col].layer2[PIECE_BLACK][1]][pieces[temp.row][temp.col].layer2[PIECE_BLACK][2]][pieces[temp.row][temp.col].layer2[PIECE_BLACK][3]][rule];
                pieces[temp.row][temp.col].layer3[PIECE_WHITE] = layer2_to_layer3_table[pieces[temp.row][temp.col].layer2[PIECE_WHITE][0]][pieces[temp.row][temp.col].layer2[PIECE_WHITE][1]][pieces[temp.row][temp.col].layer2[PIECE_WHITE][2]][pieces[temp.row][temp.col].layer2[PIECE_WHITE][3]][FREESTYLE];

                global_chesstype_count[PIECE_BLACK][pieces[temp.row][temp.col].layer3[PIECE_BLACK]]++;
                global_chesstype_count[PIECE_WHITE][pieces[temp.row][temp.col].layer3[PIECE_WHITE]]++;
            }
        }
    }
}

void ChessBoard::update_layer_undo(int8_t row, int8_t col, uint8_t side, GAME_RULE rule)
{
    for (uint8_t d4 = 0; d4 < DIRECTION4_COUNT; ++d4)
    {
        if (rule == STANDARD)
        {
            pieces[row][col].layer2[PIECE_BLACK][d4] = pattern_to_layer2_table_ban[pieces[row][col].pattern[PIECE_BLACK][d4]][pieces[row][col].pattern[PIECE_WHITE][d4]];
            pieces[row][col].layer2[PIECE_WHITE][d4] = pattern_to_layer2_table_ban[pieces[row][col].pattern[PIECE_WHITE][d4]][pieces[row][col].pattern[PIECE_BLACK][d4]];
        }
        else
        {
            pieces[row][col].layer2[PIECE_BLACK][d4] = (rule == FREESTYLE) ? pattern_to_layer2_table[pieces[row][col].pattern[PIECE_BLACK][d4]][pieces[row][col].pattern[PIECE_WHITE][d4]] : pattern_to_layer2_table_ban[pieces[row][col].pattern[PIECE_BLACK][d4]][pieces[row][col].pattern[PIECE_WHITE][d4]];
            pieces[row][col].layer2[PIECE_WHITE][d4] = pattern_to_layer2_table[pieces[row][col].pattern[PIECE_WHITE][d4]][pieces[row][col].pattern[PIECE_BLACK][d4]];
        }
    }
    pieces[row][col].layer3[PIECE_BLACK] = layer2_to_layer3_table[pieces[row][col].layer2[PIECE_BLACK][0]][pieces[row][col].layer2[PIECE_BLACK][1]][pieces[row][col].layer2[PIECE_BLACK][2]][pieces[row][col].layer2[PIECE_BLACK][3]][rule];
    pieces[row][col].layer3[PIECE_WHITE] = layer2_to_layer3_table[pieces[row][col].layer2[PIECE_WHITE][0]][pieces[row][col].layer2[PIECE_WHITE][1]][pieces[row][col].layer2[PIECE_WHITE][2]][pieces[row][col].layer2[PIECE_WHITE][3]][FREESTYLE];

    global_chesstype_count[PIECE_BLACK][pieces[row][col].layer3[PIECE_BLACK]]++;
    global_chesstype_count[PIECE_WHITE][pieces[row][col].layer3[PIECE_WHITE]]++;
    for (uint8_t d4 = 0; d4 < DIRECTION4_COUNT; ++d4)
    {
        //left
        Position temp(row, col);
        uint8_t d8 = d4 * 2;
        for (uint8_t p = 8; p != 0; p >>= 1)
        {
            if (!temp.displace8(d8)) break;
            pieces[temp.row][temp.col].pattern[side][d4] ^= p;
            if (pieces[temp.row][temp.col].layer1 == PIECE_BLANK)
            {
                if (rule == STANDARD)
                {
                    pieces[temp.row][temp.col].layer2[PIECE_BLACK][d4] = pattern_to_layer2_table_ban[pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]][pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]];
                    pieces[temp.row][temp.col].layer2[PIECE_WHITE][d4] = pattern_to_layer2_table_ban[pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]][pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]];
                }
                else
                {
                    pieces[temp.row][temp.col].layer2[PIECE_BLACK][d4] = (rule == FREESTYLE) ? pattern_to_layer2_table[pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]][pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]] : pattern_to_layer2_table_ban[pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]][pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]];
                    pieces[temp.row][temp.col].layer2[PIECE_WHITE][d4] = pattern_to_layer2_table[pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]][pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]];
                }

                global_chesstype_count[PIECE_BLACK][pieces[temp.row][temp.col].layer3[PIECE_BLACK]]--;
                global_chesstype_count[PIECE_WHITE][pieces[temp.row][temp.col].layer3[PIECE_WHITE]]--;

                pieces[temp.row][temp.col].layer3[PIECE_BLACK] = layer2_to_layer3_table[pieces[temp.row][temp.col].layer2[PIECE_BLACK][0]][pieces[temp.row][temp.col].layer2[PIECE_BLACK][1]][pieces[temp.row][temp.col].layer2[PIECE_BLACK][2]][pieces[temp.row][temp.col].layer2[PIECE_BLACK][3]][rule];
                pieces[temp.row][temp.col].layer3[PIECE_WHITE] = layer2_to_layer3_table[pieces[temp.row][temp.col].layer2[PIECE_WHITE][0]][pieces[temp.row][temp.col].layer2[PIECE_WHITE][1]][pieces[temp.row][temp.col].layer2[PIECE_WHITE][2]][pieces[temp.row][temp.col].layer2[PIECE_WHITE][3]][FREESTYLE];

                global_chesstype_count[PIECE_BLACK][pieces[temp.row][temp.col].layer3[PIECE_BLACK]]++;
                global_chesstype_count[PIECE_WHITE][pieces[temp.row][temp.col].layer3[PIECE_WHITE]]++;
            }
        }

        //right
        temp.set(row, col);
        d8 += 1;
        for (uint8_t p = 16; p != 0; p <<= 1)
        {
            if (!temp.displace8(d8)) break;
            pieces[temp.row][temp.col].pattern[side][d4] ^= p;
            if (pieces[temp.row][temp.col].layer1 == PIECE_BLANK)
            {
                if (rule == STANDARD)
                {
                    pieces[temp.row][temp.col].layer2[PIECE_BLACK][d4] = pattern_to_layer2_table_ban[pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]][pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]];
                    pieces[temp.row][temp.col].layer2[PIECE_WHITE][d4] = pattern_to_layer2_table_ban[pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]][pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]];
                }
                else
                {
                    pieces[temp.row][temp.col].layer2[PIECE_BLACK][d4] = (rule == FREESTYLE) ? pattern_to_layer2_table[pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]][pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]] : pattern_to_layer2_table_ban[pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]][pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]];
                    pieces[temp.row][temp.col].layer2[PIECE_WHITE][d4] = pattern_to_layer2_table[pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]][pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]];
                }

                global_chesstype_count[PIECE_BLACK][pieces[temp.row][temp.col].layer3[PIECE_BLACK]]--;
                global_chesstype_count[PIECE_WHITE][pieces[temp.row][temp.col].layer3[PIECE_WHITE]]--;

                pieces[temp.row][temp.col].layer3[PIECE_BLACK] = layer2_to_layer3_table[pieces[temp.row][temp.col].layer2[PIECE_BLACK][0]][pieces[temp.row][temp.col].layer2[PIECE_BLACK][1]][pieces[temp.row][temp.col].layer2[PIECE_BLACK][2]][pieces[temp.row][temp.col].layer2[PIECE_BLACK][3]][rule];
                pieces[temp.row][temp.col].layer3[PIECE_WHITE] = layer2_to_layer3_table[pieces[temp.row][temp.col].layer2[PIECE_WHITE][0]][pieces[temp.row][temp.col].layer2[PIECE_WHITE][1]][pieces[temp.row][temp.col].layer2[PIECE_WHITE][2]][pieces[temp.row][temp.col].layer2[PIECE_WHITE][3]][FREESTYLE];

                global_chesstype_count[PIECE_BLACK][pieces[temp.row][temp.col].layer3[PIECE_BLACK]]++;
                global_chesstype_count[PIECE_WHITE][pieces[temp.row][temp.col].layer3[PIECE_WHITE]]++;
            }
        }
    }
}

void ChessBoard::update_layer_old(int8_t row, int8_t col, uint8_t side, GAME_RULE ban)//落子处
{
    global_chesstype_count[side][pieces[row][col].layer3[side]]--;
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
                    update_layer3_old(pos_fix.row, pos_fix.col, side, ban, d, len, index_offset + i);
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
                    update_layer3_old(pos_fix.row, pos_fix.col, side, ban, d, len, index_offset + i);
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
                    update_layer3_old(pos_fix.row, pos_fix.col, side, ban, d, len, index_offset + i);
                }
            }
        }
    }
}


void ChessBoard::update_layer3_old(int8_t row, int8_t col, uint8_t side, GAME_RULE rule, uint8_t d, int len, int chessHashIndex)
{
    uint8_t oldchesstype = pieces[row][col].layer3[side];
    uint8_t oldtype = pieces[row][col].layer2[side][d];

    if (rule == STANDARD) pieces[row][col].layer2[side][d] = layer2_table_ban[len][chessHashIndex];
    else pieces[row][col].layer2[side][d] = (rule == FREESTYLE || side == PIECE_WHITE) ? layer2_table[len][chessHashIndex] : layer2_table_ban[len][chessHashIndex];


    global_chesstype_count[side][pieces[row][col].layer3[side]]--;
    pieces[row][col].layer3[side] = layer2_to_layer3_table[pieces[row][col].layer2[side][0]][pieces[row][col].layer2[side][1]][pieces[row][col].layer2[side][2]][pieces[row][col].layer2[side][3]][(side == PIECE_BLACK) ? rule : FREESTYLE];
    global_chesstype_count[side][pieces[row][col].layer3[side]]++;
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
    if (count[CHESSTYPE_BAN] > 0) return STANDARD ? CHESSTYPE_0 : CHESSTYPE_BAN; //长连
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
        if (canMove(pos.row,pos.col))
        {
            result += getChessTypeInfo(pieces[pos.row][pos.col].layer3[side]).rating;
        }
    }
    return result;
}

bool ChessBoard::moveNull()
{
    lastStep.changeSide();
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

    //update_layer_old(row, col, ban);
    update_layer(row, col, side, ban);
    Rect rect = Util::generate_rect(row, col, 3);
    ForRectPosition(rect)
    {
        pieces[pos.row][pos.col].around[side]++;
    }
    updateHashPair(row, col, side, true);
    return true;
}

bool ChessBoard::moveMultiReplies(vector<Position> &moves, GAME_RULE ban)
{
    if (moves.empty())
    {
        return false;
    }
    lastStep.pos.set(moves[0].row, moves[0].col);
    lastStep.step++;
    lastStep.changeSide();

    size_t len = moves.size();
    for (size_t i = 0; i < len; ++i)
    {
        pieces[moves[i].row][moves[i].col].layer1 = lastStep.state;

        //update_layer_old(moves[i].row, moves[i].col, ban);
        update_layer(moves[i].row, moves[i].col, lastStep.getState(), ban);

        updateHashPair(moves[i].row, moves[i].col, lastStep.state, true);
    }
    return true;
}

bool ChessBoard::unmove(Position xy, ChessStep last, GAME_RULE ban)
{
    uint8_t side = pieces[xy.row][xy.col].layer1;
    if (side == PIECE_BLANK || lastStep.step < 1)
    {
        return false;//没有棋子
    }
    lastStep = last;

    pieces[xy.row][xy.col].layer1 = PIECE_BLANK;

    //update_layer_old(pos.row, pos.col, ban);
    update_layer_undo(xy.row, xy.col, side, ban);

    Rect rect = Util::generate_rect(xy.row, xy.col, 3);
    ForRectPosition(rect)
    {
        pieces[pos.row][pos.col].around[side]--;
    }

    updateHashPair(xy.row, xy.col, side, false);

    return true;
}

void ChessBoard::getFourkillDefendCandidates(Position pos, vector<StepCandidateItem>& moves, GAME_RULE rule)
{
    vector<Position> positions;
    getFourkillDefendCandidates(pos, positions, rule);
    size_t len = positions.size();
    for (size_t i = 0; i < len; ++i)
    {
        moves.emplace_back(positions[i], 100);
    }
}

void ChessBoard::getThreatReplies(Position pos, uint8_t type, uint8_t direction, vector<Position>& reply)
{
    if (type == CHESSTYPE_4 || type == CHESSTYPE_5) return;

    uint8_t side = lastStep.getState();
    if (Util::isdead4(type))
    {
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
        {
            Position temppos = pos;
            for (int8_t offset = 1; offset < 5; ++offset)
            {
                if (!temppos.displace4(symbol, direction))//equal otherside
                {
                    break;
                }
                if (getState(temppos.row, temppos.col) == PIECE_BLANK)
                {
                    if (getLayer2(temppos.row, temppos.col, side, direction) == CHESSTYPE_5)
                    {
                        reply.emplace_back(temppos);
                        return;
                    }
                }
                else if (getState(temppos.row, temppos.col) == side)
                {
                    continue;
                }
                else
                {
                    break;
                }
            }
        }
    }
    else if (Util::isalive3(type))
    {
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
        {
            Position temppos = pos;
            for (int8_t offset = 1; offset < 5; ++offset)
            {
                if (!temppos.displace4(symbol, direction))//equal otherside
                {
                    break;
                }
                if (getState(temppos.row, temppos.col) == PIECE_BLANK)
                {
                    if (getLayer2(temppos.row, temppos.col, side, direction) == CHESSTYPE_4)
                    {
                        reply.emplace_back(temppos);
                    }
                }
                else if (getState(temppos.row, temppos.col) == side)
                {
                    continue;
                }
                else
                {
                    break;
                }
            }
        }
        //真活三
        if (reply.size() > 1) return;
        //假活三
        if (reply.empty()) return;
        //跳三
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
        {
            Position temppos = reply[0];
            for (int8_t offset = 1; offset < 5; ++offset)
            {
                if (!temppos.displace4(symbol, direction))//equal otherside
                {
                    break;
                }
                if (getState(temppos.row, temppos.col) == PIECE_BLANK)
                {
                    if (Util::isdead4(getLayer2(temppos.row, temppos.col, side, direction)))
                    {
                        reply.emplace_back(temppos);
                    }
                    break;
                }
                else if (getState(temppos.row, temppos.col) == side)
                {
                    continue;
                }
                else
                {
                    break;
                }
            }
        }
    }

}

void ChessBoard::getFourkillDefendCandidates(Position pos, vector<Position>& moves, GAME_RULE rule)
{
    //现在该防守方落子
    uint8_t defendside = getLastStep().getOtherSide();//防守方
    uint8_t atackside = getLastStep().getState();//进攻方
    uint8_t atackType = getChessType(pos, atackside);

    vector<uint8_t> directions;

    if (atackType == CHESSTYPE_5)
    {
        moves.emplace_back(pos);
        return;
    }
    else if (atackType == CHESSTYPE_4)//两个进攻点__ooo__，两个防点/一个进攻点x_ooo__（有一边被堵），三个防点
    {
        moves.emplace_back(pos);
        uint8_t d = 0;
        for (; d < DIRECTION4_COUNT; ++d)
        {
            if (getLayer2(pos.row, pos.col, atackside, d) == CHESSTYPE_4) break;
        }
        //判断是哪种棋型
        int defend_point_count = 1;

        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
        {
            Position temppos = pos;
            for (int8_t offset = 1; offset < 5; ++offset)
            {
                if (!temppos.displace4(symbol, d))//equal otherside
                {
                    break;
                }
                if (getState(temppos.row, temppos.col) == PIECE_BLANK)
                {

                    if (Util::isdead4(getLayer2(temppos.row, temppos.col, atackside, d)))
                    {
                        moves.emplace_back(temppos);
                        break;
                    }
                    else if (getLayer2(temppos.row, temppos.col, atackside, d) == CHESSTYPE_4)
                    {
                        if (moves.size() > 1) moves.pop_back();
                        moves.emplace_back(temppos);
                        return;
                    }
                }
                else if (getState(temppos.row, temppos.col) == atackside)
                {
                    continue;
                }
                else
                {
                    break;
                }
            }
        }
        return;
    }
    else if (atackType == CHESSTYPE_44)//一个攻点，三个防点
    {
        moves.emplace_back(pos);
        for (int d = 0; d < DIRECTION4_COUNT; ++d)
        {
            if (getLayer2(pos.row, pos.col, atackside, d) == CHESSTYPE_44)
            {
                directions.push_back(d * 2);
                directions.push_back(d * 2 + 1);
                break;
            }
            else if (Util::isdead4(getLayer2(pos.row, pos.col, atackside, d)))
            {
                directions.push_back(d * 2);
                directions.push_back(d * 2 + 1);
            }
        }
    }
    else if (atackType == CHESSTYPE_43)//一个攻点，四个防点
    {
        moves.emplace_back(pos);
        for (int d = 0; d < DIRECTION4_COUNT; ++d)
        {
            if (Util::isdead4(getLayer2(pos.row, pos.col, atackside, d)) || Util::isalive3(getLayer2(pos.row, pos.col, atackside, d)))
            {
                directions.push_back(d * 2);
                directions.push_back(d * 2 + 1);
            }
        }
    }
    else if (atackType == CHESSTYPE_33)//一个攻点，五个防点
    {
        moves.emplace_back(pos);
        for (int d = 0; d < DIRECTION4_COUNT; ++d)
        {
            if (Util::isalive3(getLayer2(pos.row, pos.col, atackside, d)))
            {
                directions.push_back(d * 2);
                directions.push_back(d * 2 + 1);
            }
        }
    }
    else
    {
        return;
    }

    for (auto n : directions)
    {
        Position temppos = pos;
        int blankCount = 0, chessCount = 0;
        while (temppos.displace8(1, n)) //如果不超出边界
        {
            if (getState(temppos.row, temppos.col) == PIECE_BLANK)
            {
                blankCount++;
                uint8_t tempType = getLayer2(temppos.row, temppos.col, atackside, n / 2);
                if (tempType > CHESSTYPE_0)
                {
                    if (getChessType(temppos.row, temppos.col, defendside) != CHESSTYPE_BAN)//被禁手了
                    {
                        ChessBoard tempboard = *this;
                        tempboard.move(temppos, rule);
                        if (tempboard.getChessType(pos, atackside) < atackType)
                        {
                            moves.emplace_back(temppos);
                        }
                    }
                }
            }
            else if (getState(temppos.row, temppos.col) == defendside)
            {
                break;
            }
            else
            {
                chessCount++;
            }

            if (blankCount == 2
                || chessCount > 3)
            {
                break;
            }
        }
    }
}

void ChessBoard::getVCTCandidates(vector<StepCandidateItem>& moves, Position* center)
{
    uint8_t side = getLastStep().getOtherSide();
    size_t begin_index = moves.size();

    if (center == NULL)
    {
        ForEachPosition
        {
            if (!(canMove(pos) && getChessType(pos, side) > CHESSTYPE_D3))
            {
                continue;
            }
            if (getChessType(pos, side) == CHESSTYPE_33)
            {
                moves.emplace_back(pos, 400);
                continue;
            }
            if (Util::isalive3(getChessType(pos, side)))
            {
                moves.emplace_back(pos, getRelatedFactor(pos, side));
            }
        }
    }
    else
    {
        Rect rect = Util::generate_rect(center->row, center->col, 4);
        ForRectPosition(rect)
        {
            if (!(canMove(pos) && getChessType(pos, side) > CHESSTYPE_D3))
            {
                continue;
            }

            if (getChessType(pos, side) == CHESSTYPE_33)
            {
                moves.emplace_back(pos, 400);
                continue;
            }

            if (Util::isalive3(getChessType(pos, side)))
            {
                moves.emplace_back(pos, getRelatedFactor(pos, side));
            }

        }
    }
}

void ChessBoard::getVCFCandidates(vector<StepCandidateItem>& moves, Position* center)
{
    uint8_t side = Util::otherside(getLastStep().getState());

    if (center == NULL)
    {
        ForEachPosition
        {
            if (!(canMove(pos) && getChessType(pos, side) > CHESSTYPE_3))
            {
                continue;
            }
        if (getChessType(pos, side) == CHESSTYPE_4)
        {
            moves.emplace_back(pos, 1000);
        }
        else if (getChessType(pos, side) == CHESSTYPE_44)
        {
            moves.emplace_back(pos, 800);
        }
        else if (getChessType(pos, side) == CHESSTYPE_43)
        {
            moves.emplace_back(pos, 500);
        }
        else if (Util::isdead4(getChessType(pos, side)))
        {
            moves.emplace_back(pos, getRelatedFactor(pos, side));
        }
        }
    }
    else
    {
        Rect rect = Util::generate_rect(center->row, center->col, 4);
        ForRectPosition(rect)
        {
            if (!(canMove(pos) && getChessType(pos, side) > CHESSTYPE_3))
            {
                continue;
            }
            if (getChessType(pos, side) == CHESSTYPE_4)
            {
                moves.emplace_back(pos, 1000);
            }
            else if (getChessType(pos, side) == CHESSTYPE_44)
            {
                moves.emplace_back(pos, 800);
            }
            else if (getChessType(pos, side) == CHESSTYPE_43)
            {
                moves.emplace_back(pos, 500);
            }
            else if (Util::isdead4(getChessType(pos, side)))
            {
                moves.emplace_back(pos, getRelatedFactor(pos, side));
            }
        }
    }
}

void ChessBoard::getVCFCandidates(vector<StepCandidateItem>& moves, set<Position>& reletedset)
{
    uint8_t side = Util::otherside(getLastStep().getState());
    for (auto pos : reletedset)
    {
        if (!canMove(pos))
        {
            continue;
        }
        if (getChessType(pos, side) == CHESSTYPE_4)
        {
            moves.emplace_back(pos, 1000);
        }
        else if (getChessType(pos, side) == CHESSTYPE_44)
        {
            moves.emplace_back(pos, 800);
        }
        else if (getChessType(pos, side) == CHESSTYPE_43)
        {
            moves.emplace_back(pos, 500);
        }
        else if (Util::isdead4(getChessType(pos, side)))
        {
            moves.emplace_back(pos, getRelatedFactor(pos, side));
        }
    }
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
                if (!temppos.displace4(symbol, d))//equal otherside
                {
                    break;
                }

                if (pieces[temppos.row][temppos.col].layer1 == PIECE_BLANK)
                {
                    blankcount++;
                    if (pieces[temppos.row][temppos.col].layer2[side][d] > CHESSTYPE_0)
                    {
                        releted.insert(temppos);
                        getDefendReletedPos2(releted, temppos, side);//因为没有求交集，暂时去掉
                    }
                    else
                    {
                        releted.insert(temppos);
                    }
                }
                else
                {
                    continue;
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
        if (pieces[center.row][center.col].layer2[side][d] == CHESSTYPE_0 && pieces[center.row][center.col].layer2[Util::otherside(side)][d] == CHESSTYPE_0)
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
                    if (pieces[temppos.row][temppos.col].layer2[Util::otherside(side)][d] > CHESSTYPE_D3)
                    {
                        releted.insert(temppos);
                    }
                    else if (pieces[temppos.row][temppos.col].layer2[Util::otherside(side)][d] > CHESSTYPE_0 && pieces[temppos.row][temppos.col].layer3[Util::otherside(side)] > CHESSTYPE_D3)
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
                    releted.insert(temppos);
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
        if (pieces[center.row][center.col].layer2[side][d] == CHESSTYPE_0)
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
                    if (pieces[temppos.row][temppos.col].layer2[side][d] > CHESSTYPE_0)
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
                temppos = center;
                if (!temppos.displace4(offset*symbol, d) || pieces[temppos.row][temppos.col].layer1 == Util::otherside(side))//equal otherside
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
                    temppos = banpos;
                    if (!temppos.displace4(offset*symbol, d) || pieces[temppos.row][temppos.col].layer1 == side)//equal otherside
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


size_t ChessBoard::getNormalCandidates(vector<StepCandidateItem>& moves, bool isatack, bool findwinning)
{
    uint8_t side = getLastStep().getOtherSide();
    Position lastPos = getLastStep().pos;
    ForEachPosition
    {
        if (!(canMove(pos) && useful(pos)))
        {
            continue;
        }

    uint8_t selftype = getChessType(pos, side);

    if (selftype == CHESSTYPE_BAN)
    {
        continue;
    }

    uint8_t otherp = getChessType(pos, Util::otherside(side));

    int atack = getRelatedFactor(pos, side);

    if (findwinning)
    {
        if (!isatack &&  Util::isdead4(selftype) && otherp < CHESSTYPE_J3)
        {
            continue;
        }
    }
    else
    {
        if (Util::isdead4(selftype)) atack -= 12;
        else if (Util::isalive3(selftype)) atack -= 8;

        if (atack == 0 && otherp < CHESSTYPE_J3 && getLastStep().step > 10)
        {
            continue;
        }
    }

    if (isatack)
    {
        moves.emplace_back(pos, atack);
    }
    else // defend
    {
        int defend = getRelatedFactor(pos, Util::otherside(side), true);
        moves.emplace_back(pos, atack + defend);
    }
    }

    std::sort(moves.begin(), moves.end(), CandidateItemCmp);

    if (findwinning)
    {
        if (isatack)
        {
            for (auto i = 0; i < moves.size(); ++i)
            {
                if (moves[i].priority == 0)
                {
                    return i > 20 ? 20 : i;
                }
            }
        }
        else // defend
        {
            for (auto i = 0; i < moves.size(); ++i)
            {
                if (moves[i].priority == 0)
                {
                    return i;
                }
            }
        }
    }
    else
    {
        for (auto i = 0; i < moves.size(); ++i)
        {
            if (moves[i].priority < moves[0].priority / 3)
            {
                return i;
            }
        }
    }

    return moves.size();
}

const ChessTypeInfo chesstypes[CHESSTYPE_COUNT] = {
    { 0    ,   0,   0 },           //CHESSTYPE_0,  +CHESSTYPE_2*2 +CHESSTYPE_J2*2 (0)
    { 10   ,   2,   0 },           //CHESSTYPE_j2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
    { 10   ,   8,   2 },           //CHESSTYPE_2,  -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*2 +CHESSTYPE_J3*2 (0)
    { 10   ,   2,   1 },           //CHESSTYPE_d3, -CHESSTYPE_D3*2 +CHESSTYPE_D4*2 (0)
    { 80   ,   8,   6 },           //CHESSTYPE_J3  -CHESSTYPE_3*1 -CHESSTYPE_J3*2 +CHESSTYPE_4*1 +CHESSTYPE_D4*2 (0)
    { 100  ,  10,   8 },           //CHESSTYPE_3,  -CHESSTYPE_3*2 -CHESSTYPE_J3*2 +CHESSTYPE_4*2 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
    { 120  ,  12,   8 },           //CHESSTYPE_d4, -CHESSTYPE_D4*2 +CHESSTYPE_5 (0) 优先级降低
    { 150  ,  12,   8 },           //CHESSTYPE_d4p -CHESSTYPE_D4P*1 -CHESSTYPE_D4 +CHESSTYPE_5 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
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
    int related_factor = 0;
    if (pieces[pos.row][pos.col].layer3[side] > CHESSTYPE_D4P)
    {
        return defend ? chesstypes[pieces[pos.row][pos.col].layer3[side]].defendBaseFactor : chesstypes[pieces[pos.row][pos.col].layer3[side]].atackBaseFactor;
    }
    Position temppos;
    for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
    {
        base_factor += defend ? chesstypes[pieces[pos.row][pos.col].layer2[side][d]].defendBaseFactor : chesstypes[pieces[pos.row][pos.col].layer2[side][d]].atackBaseFactor;
        int iterbase = pieces[pos.row][pos.col].layer2[side][d] > CHESSTYPE_0 ? 2 : 1;
        //related factor, except base 
        int related_count_3[2] = { 0,0 };
        int related_count_d4[2] = { 0,0 };
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

                            if (pieces[temppos.row][temppos.col].layer2[side][d2] > CHESSTYPE_3)
                            {
                                if (blank[i] == 3)//除非同侧已有一个related，否则blank[i] == 3距离太远了，无视掉
                                {
                                    if (related_count_d4[i] > 0) related_count_d4[i] += iterbase;
                                }
                                else
                                {
                                    related_count_d4[i] += iterbase;
                                }
                            }
                            else if (pieces[temppos.row][temppos.col].layer2[side][d2] > CHESSTYPE_D3)
                            {
                                if (blank[i] == 3)//有可能 两个相距7 无意义
                                {
                                    if (related_count_3[i] > 0) related_count_3[i] += iterbase;
                                }
                                else
                                {
                                    related_count_3[i] += iterbase;
                                }
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
            if (defend)
            {
                if (related_count_3[0] + related_count_3[1] + related_count_d4[0] + related_count_d4[1] > 1)//在五子范围内存在两个related
                {
                    related_factor += (related_count_d4[0] + related_count_d4[1]) * 2 + (related_count_3[0] + related_count_3[1]) * 2;
                }
            }
            else
            {
                if (related_count_3[0] + related_count_3[1] + related_count_d4[0] + related_count_d4[1] > 1)
                {
                    related_factor += (related_count_d4[0] + related_count_d4[1]) * 3 + (related_count_3[0] + related_count_3[1]) * 2;
                }
                else
                {
                    related_factor += (related_count_d4[0] + related_count_d4[1]) * 2 + (related_count_3[0] + related_count_3[1]) * 1;
                }
            }

        }
    }
    base_factor += related_factor;
    if (defend) base_factor = base_factor > 15 ? 15 : base_factor;
    else base_factor = base_factor > 20 ? 20 : base_factor;
    return base_factor;
}

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
        if (!findself && pieces[pos.row][pos.col].layer2[side][d] == layer3type)
        {
            findself = true;
            continue;
        }
        else
        {
            if (pieces[pos.row][pos.col].layer2[side][d] > CHESSTYPE_0)
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

                            if (pieces[temppos.row][temppos.col].layer2[side][d2] > CHESSTYPE_3)
                            {
                                releted_count_d4++;
                            }
                            else if (pieces[temppos.row][temppos.col].layer2[side][d2] > CHESSTYPE_D3)
                            {
                                releted_count_3++;
                            }
                            else if (pieces[pos.row][pos.col].layer2[side][d] > CHESSTYPE_0 && pieces[temppos.row][temppos.col].layer2[side][d2] > CHESSTYPE_J2)
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
    {    1,  0 },           //CHESSTYPE_j2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
    {    2,  1 },           //CHESSTYPE_2,  -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*2 +CHESSTYPE_J3*2 (0)
    {    2,  1 },           //CHESSTYPE_d3, -CHESSTYPE_D3*2 +CHESSTYPE_D4*2 (0)
    {    8,  6 },           //CHESSTYPE_J3  -CHESSTYPE_3*1 -CHESSTYPE_J3*2 +CHESSTYPE_4*1 +CHESSTYPE_D4*2 (0)
    {   16, 12 },           //CHESSTYPE_3,  -CHESSTYPE_3*2 -CHESSTYPE_J3*2 +CHESSTYPE_4*2 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
    {   18, 14 },           //CHESSTYPE_d4, -CHESSTYPE_D4*2 +CHESSTYPE_5 (0) 优先级降低
    {   18, 14 },           //CHESSTYPE_d4p -CHESSTYPE_D4P*1 -CHESSTYPE_D4 +CHESSTYPE_5 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
    {   40, 30 },           //CHESSTYPE_33, -CHESSTYPE_33*1 -CHESSTYPE_3*0-2 -CHESSTYPE_J3*2-4 +CHESSTYPE_4*2-4 +CHESSTYPE_D4*2-4 (CHESSTYPE_4*2)
    {   50, 35 },           //CHESSTYPE_43, -CHESSTYPE_43*1 -CHESSTYPE_D4*1 -CHESSTYPE_J3*2 -CHESSTYPE_3*1 +CHESSTYPE_5*1 +CHESSTYPE_4*2 (CHESSTYPE_4*2)
    {   60, 40 },           //CHESSTYPE_44, -CHESSTYPE_44 -CHESSTYPE_D4*2 +2个CHESSTYPE_5    (CHESSTYPE_5)
    {   80, 40 },           //CHESSTYPE_4,  -CHESSTYPE_4*1-2 -CHESSTYPE_D4*1-2 +CHESSTYPE_5*2 (CHESSTYPE_5)
    { 1000,100 },           //CHESSTYPE_5,
    {  -20,-20 },           //CHESSTYPE_BAN,
};



//weight是对于side方的偏向，默认100
//int ChessBoard::getGlobalEvaluate(uint8_t side, int weight)
//{
//    //始终是以进攻方(atackside)为正
//    uint8_t defendside = lastStep.getState();
//    uint8_t atackside = Util::otherside(defendside);
//
//    int atack_evaluate = 0;
//    int defend_evaluate = 0;
//    //遍历所有棋子
//    ForEachPosition
//    {
//        //已有棋子的不做计算
//        if (!canMove(pos) || !useful(pos))
//        {
//            continue;
//        }
//
//        atack_evaluate += (int)(staticEvaluate[pieces[pos.row][pos.col].layer3[atackside]].atack*getStaticFactor(pos, atackside));
//
//        defend_evaluate += (int)(staticEvaluate[pieces[pos.row][pos.col].layer3[defendside]].defend*getStaticFactor(pos, defendside));
//    }
//
//    return side == atackside ? atack_evaluate * weight / 100 - defend_evaluate : -(atack_evaluate - defend_evaluate * weight / 100);
//}

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
    double atack_factor = 1 + (double)(pieces[pos.row][pos.col].around[atackside] - pieces[pos.row][pos.col].around[defendside]) / 49.0;//7*7

        if (pieces[pos.row][pos.col].layer3[atackside] < CHESSTYPE_33)
        {
            for (uint8_t d = 0; d < 4; ++d)
            {
                atack_evaluate += (int)(staticEvaluate[pieces[pos.row][pos.col].layer2[atackside][d]].atack* atack_factor);
            }
        }
        else
        {
            atack_evaluate += staticEvaluate[pieces[pos.row][pos.col].layer3[atackside]].atack;
        }

        double defend_factor = 1 + (double)(pieces[pos.row][pos.col].around[defendside] - pieces[pos.row][pos.col].around[atackside]) / 49.0;//7*7
        if (pieces[pos.row][pos.col].layer3[defendside] < CHESSTYPE_33)
        {
            for (uint8_t d = 0; d < 4; ++d)
            {
                defend_evaluate += (int)(staticEvaluate[pieces[pos.row][pos.col].layer2[defendside][d]].defend* defend_factor);
            }
        }
        else
        {
            defend_evaluate += staticEvaluate[pieces[pos.row][pos.col].layer3[defendside]].defend;
        }

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
