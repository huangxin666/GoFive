#include "ChessBoard.h"
#include "TrieTree.h"
#include <random>
#include <algorithm>
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
    initPositionWeightTable();
}

uint8_t* ChessBoard::layer2_table[BOARD_SIZE_MAX + 1] = { NULL };
uint8_t* ChessBoard::layer2_table_ban[BOARD_SIZE_MAX + 1] = { NULL };
uint8_t ChessBoard::layer2_to_layer3_table[CHESSTYPE_COUNT][CHESSTYPE_COUNT][CHESSTYPE_COUNT][CHESSTYPE_COUNT][3];
uint8_t ChessBoard::pattern_to_layer2_table[UINT8_MAX + 1][UINT8_MAX + 1];//2^8
uint8_t ChessBoard::pattern_to_layer2_table_ban[UINT8_MAX + 1][UINT8_MAX + 1];//2^8
double ChessBoard::position_weight[UINT8_MAX + 1];
bool ChessBoard::relatedsituation[5][5][5][5];

ChessBoard::ChessBoard()
{
    lastStep.step = 0;
    lastStep.state = PIECE_WHITE;//��һ�����Ǻ���
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

static uint8_t around_init_help[4] = { 0x51,0xA2,0x94,0x68 };
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

        //pieces[pos.row][pos.col].around = around_max_offset * 8;
        ////�˸�����
        //int breaked[DIRECTION8_COUNT] = { 0 };
        //breaked[DIRECTION8::DIRECTION8_L] = std::max(0, around_max_offset - pos.col);//ֻ��colӰ��
        //breaked[DIRECTION8::DIRECTION8_R] = std::max(0, around_max_offset - (Util::BoardSize - 1 - pos.col));//ֻ��colӰ��
        //breaked[DIRECTION8::DIRECTION8_U] = std::max(0, around_max_offset - pos.row);//ֻ��rowӰ��
        //breaked[DIRECTION8::DIRECTION8_D] = std::max(0, around_max_offset - (Util::BoardSize - 1 - pos.row));//ֻ��rowӰ��
        //breaked[DIRECTION8::DIRECTION8_LU] = std::max(breaked[DIRECTION8::DIRECTION8_L], breaked[DIRECTION8::DIRECTION8_U]);
        //breaked[DIRECTION8::DIRECTION8_RD] = std::max(breaked[DIRECTION8::DIRECTION8_R], breaked[DIRECTION8::DIRECTION8_D]);
        //breaked[DIRECTION8::DIRECTION8_LD] = std::max(breaked[DIRECTION8::DIRECTION8_L], breaked[DIRECTION8::DIRECTION8_D]);
        //breaked[DIRECTION8::DIRECTION8_RU] = std::max(breaked[DIRECTION8::DIRECTION8_R], breaked[DIRECTION8::DIRECTION8_U]);

        //for (uint8_t d = 0; d < DIRECTION8_COUNT; ++d)
        //{
        //    pieces[pos.row][pos.col].around -= breaked[d];
        //}
        pieces[pos.row][pos.col].around[PIECE_BLACK] = 0;
        pieces[pos.row][pos.col].around[PIECE_WHITE] = 0;
        if (pos.col < 2)
        {
            pieces[pos.row][pos.col].around[PIECE_BLACK] |= around_init_help[0];
            pieces[pos.row][pos.col].around[PIECE_WHITE] |= around_init_help[0];
        }

        if (pos.col > Util::BoardSize - 3)
        {
            pieces[pos.row][pos.col].around[PIECE_BLACK] |= around_init_help[1];
            pieces[pos.row][pos.col].around[PIECE_WHITE] |= around_init_help[1];
        }

        if (pos.row < 2)
        {
            pieces[pos.row][pos.col].around[PIECE_BLACK] |= around_init_help[2];
            pieces[pos.row][pos.col].around[PIECE_WHITE] |= around_init_help[2];
        }

        if (pos.row > Util::BoardSize - 3)
        {
            pieces[pos.row][pos.col].around[PIECE_BLACK] |= around_init_help[3];
            pieces[pos.row][pos.col].around[PIECE_WHITE] |= around_init_help[3];
        }
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

            if (p > 2)//  < 2
            {
                pieces[temp.row][temp.col].around[side] |= 1 >> (d8 + 1);//�ұ�
            }

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

            if (p < 64)//  < 3
            {
                pieces[temp.row][temp.col].around[side] |= 1 >> (d8 - 1);//���
            }

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

            if (p > 2)//  < 3
            {
                pieces[temp.row][temp.col].around[side] ^= 1 >> (d8 + 1);
            }

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

            if (p < 64)//  < 3
            {
                pieces[temp.row][temp.col].around[side] ^= 1 >> (d8 - 1);//���
            }

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

void ChessBoard::update_layer_old(int8_t row, int8_t col, uint8_t side, GAME_RULE ban)//���Ӵ�
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
        while (temp.displace4(-1, d))//����
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
        while (temp.displace4(1, d))//����
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

    if (count[CHESSTYPE_5] > 0) return CHESSTYPE_5;//��5�������ӽ���
    if (count[CHESSTYPE_BAN] > 0) return STANDARD ? CHESSTYPE_0 : CHESSTYPE_BAN; //����
    if (count[CHESSTYPE_44] > 0) return rule == RENJU ? CHESSTYPE_BAN : CHESSTYPE_44;
    if (count[CHESSTYPE_D4] + count[CHESSTYPE_D4P] + count[CHESSTYPE_4] > 1) return rule == RENJU ? CHESSTYPE_BAN : CHESSTYPE_44;//44���ȼ���4�ߣ������ǽ���

    //���⴦��
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

    for (uint8_t type = CHESSTYPE_D4P; type > CHESSTYPE_0; --type)
    {
        if (count[type] > 0) return type;
    }
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
        return false;//��������
    }
    lastStep.step++;
    lastStep.setState(side);
    lastStep.pos.set(row, col);
    lastStep.chessType = getChessType(row, col, side);

    pieces[row][col].layer1 = side;

    //update_layer_old(row, col, rule);
    update_layer(row, col, side, ban);

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

    for (size_t i = 0; i < moves.size(); ++i)
    {
        pieces[moves[i].row][moves[i].col].layer1 = lastStep.state;

        //update_layer_old(moves[i].row, moves[i].col, rule);
        update_layer(moves[i].row, moves[i].col, lastStep.state, ban);

        //updateHashPair(moves[i].row, moves[i].col, lastStep.state, true);
    }
    return true;
}

bool ChessBoard::unmove(Position xy, ChessStep last, GAME_RULE ban)
{
    uint8_t side = pieces[xy.row][xy.col].layer1;
    if (side == PIECE_BLANK || lastStep.step < 1)
    {
        return false;//û������
    }
    lastStep = last;

    pieces[xy.row][xy.col].layer1 = PIECE_BLANK;

    //update_layer_old(pos.row, pos.col, rule);
    update_layer_undo(xy.row, xy.col, side, ban);

    updateHashPair(xy.row, xy.col, side, false);

    return true;
}


void ChessBoard::getDependentThreatCandidates(Position pos, int level, vector<StepCandidateItem>& moves, bool extend)
{
    uint8_t side = lastStep.getOtherSide();
    if (hasChessType(side, CHESSTYPE_5))
    {
        ForEachPosition
        {
            if (!canMove(pos.row,pos.col)) continue;
        if (getChessType(pos,side) == CHESSTYPE_5)
        {
            moves.emplace_back(pos, 1000, getChessDirection(pos, side));
            return;
        }
        }
    }

    if (level == 0)
    {
        return;
    }

    for (int d = 0; d < DIRECTION4_COUNT; ++d)
    {
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//����
        {
            Position temppos = pos;
            for (int8_t offset = 1; offset < 5; ++offset)
            {
                if (!temppos.displace4(symbol, d))//equal otherside
                {
                    break;
                }
                if (getState(temppos) == PIECE_BLANK)
                {
                    if (getChessType(temppos, side) == CHESSTYPE_BAN)
                    {
                        continue;
                    }

                    if (Util::isthreat(getLayer2(temppos.row, temppos.col, side, d)))
                    {
                        if (level < 2 && Util::isalive3or33(getChessType(temppos, side)))
                        {
                            continue;
                        }

                        moves.emplace_back(temppos, getLayer2(temppos.row, temppos.col, side, d), d);
                    }
                    else if (extend)
                    {
                        bool dead4 = Util::isdead4(getChessType(temppos, side));
                        bool alive3 = Util::isalive3(getChessType(temppos, side));
                        if (dead4 || alive3)
                        {
                            if (level == 1)
                            {
                                if (dead4) moves.emplace_back(temppos, 2, getChessDirection(temppos, side));
                                continue;
                            }

                            moves.emplace_back(temppos, 2, getChessDirection(temppos, side));
                            //find ex44 or ex34 or ex33

                            //int leftoffset = 5;//-symbol
                            //int rightoffset = 5 - offset;//+symbol
                            //Position testPos = temppos;

                            //for (int8_t off = 1; off < rightoffset; ++off)
                            //{
                            //    if (!testPos.displace4(symbol, d))//equal otherside
                            //    {
                            //        break;
                            //    }
                            //    if (getState(testPos) == PIECE_BLANK)
                            //    {
                            //        if (getLayer2(testPos, side, d) > CHESSTYPE_0 && getLayer2(testPos, side, d) < CHESSTYPE_D3)
                            //        {
                            //            if (alive3 && level < 3) continue;

                            //            moves.emplace_back(testPos, 1, d);
                            //        }
                            //        else if (getLayer2(testPos, side, d) == CHESSTYPE_D3)
                            //        {
                            //            moves.emplace_back(testPos, 1, d);
                            //        }
                            //    }
                            //    else if (getState(testPos) == side)
                            //    {
                            //        continue;
                            //    }
                            //    else
                            //    {
                            //        break;
                            //    }
                            //}
                            //testPos = temppos;
                            //for (int8_t off = 1; off < leftoffset; ++off)
                            //{
                            //    if (!testPos.displace4(-symbol, d))//equal otherside
                            //    {
                            //        break;
                            //    }
                            //    if (getState(testPos) == PIECE_BLANK)
                            //    {
                            //        if (getLayer2(testPos, side, d) > CHESSTYPE_0 && getLayer2(testPos, side, d) < CHESSTYPE_D3)
                            //        {
                            //            if (alive3 && level < 3) continue;

                            //            moves.emplace_back(testPos, 1, d);
                            //        }
                            //        else if (getLayer2(testPos, side, d) == CHESSTYPE_D3)
                            //        {
                            //            moves.emplace_back(testPos, 1, d);
                            //        }
                            //    }
                            //    else if (getState(testPos) == side)
                            //    {
                            //        continue;
                            //    }
                            //    else
                            //    {
                            //        break;
                            //    }
                            //}
                        }
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
    std::sort(moves.begin(), moves.end(), CandidateItemCmp);
}


void ChessBoard::getThreatReplies(Position pos, uint8_t type, uint8_t direction, Position* reply, uint8_t &num, GAME_RULE ban)
{
    vector<Position> replies;
    getThreatReplies(pos, type, direction, replies, ban);
    size_t len = replies.size();
    for (num = 0; num < len && num < 3; ++num)
    {
        reply[num] = replies[num];
    }
}


void ChessBoard::getThreatReplies(Position pos, uint8_t type, uint8_t direction, vector<Position>& reply, GAME_RULE rule)
{
    if (type == CHESSTYPE_5) return;

    uint8_t side = lastStep.state;
    if (type == CHESSTYPE_4)
    {
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//����
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
                        if (getChessType(temppos, lastStep.getOtherSide()) != CHESSTYPE_BAN)
                        {
                            reply.emplace_back(temppos);
                        }
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
    else if (Util::isdead4(type))
    {
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//����
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
                        if (getChessType(temppos, lastStep.getOtherSide()) != CHESSTYPE_BAN)
                        {
                            reply.emplace_back(temppos);
                        }
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
        int count = 0;
        Position center;
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//����
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
                        count++;
                        center = temppos;
                        if (getChessType(temppos, lastStep.getOtherSide()) != CHESSTYPE_BAN)
                        {
                            reply.emplace_back(temppos);
                        }
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
        //�����
        if (count > 1) return;
        //�ٻ���
        if (count == 0) return;
        //����
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//����
        {
            Position temppos = center;
            for (int8_t offset = 1; offset < 5; ++offset)
            {
                if (!temppos.displace4(symbol, direction))//equal otherside
                {
                    break;
                }
                if (getState(temppos.row, temppos.col) == PIECE_BLANK)
                {
                    if (Util::isdead4(getLayer2(temppos.row, temppos.col, side, direction)) ||
                        getLayer2(temppos.row, temppos.col, side, direction) == CHESSTYPE_44)//special
                    {
                        if (getChessType(temppos, lastStep.getOtherSide()) != CHESSTYPE_BAN)
                        {
                            reply.emplace_back(temppos);
                        }
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
    else // 33 34 44
    {
        if (hasChessType(side, CHESSTYPE_44) || hasChessType(side, CHESSTYPE_43) || hasChessType(side, CHESSTYPE_33))
        {
            for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//����
            {
                Position temppos = pos;
                for (int8_t offset = 1; offset < 5; ++offset)
                {
                    if (!temppos.displace4(symbol, direction))//equal otherside
                    {
                        break;
                    }
                    if (getState(temppos.row, temppos.col) == PIECE_BLANK && Util::isSpecialType(getChessType(temppos, side)))
                    {
                        if (getChessType(temppos, lastStep.getOtherSide()) != CHESSTYPE_BAN)
                        {
                            reply.emplace_back(temppos);
                        }
                        ChessBoard tempboard = *this;
                        tempboard.move(temppos.row, temppos.col, side, rule);
                        for (uint8_t d = 0; d < DIRECTION4::DIRECTION4_COUNT; ++d)
                        {
                            if (Util::isthreat(getLayer2(temppos, side, d)))
                            {
                                tempboard.getThreatReplies(temppos, getLayer2(temppos, side, d), d, reply, rule);
                            }
                        }
                        return;
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

void ChessBoard::getFourkillDefendCandidates(Position pos, vector<Position>& moves, GAME_RULE rule)
{
    //���ڸ÷��ط�����
    uint8_t defendside = getLastStep().getOtherSide();//���ط�
    uint8_t atackside = getLastStep().state;//������
    uint8_t atackType = getChessType(pos, atackside);

    vector<uint8_t> directions;

    if (atackType == CHESSTYPE_5)
    {
        moves.emplace_back(pos);
        return;
    }
    else if (atackType == CHESSTYPE_4)//����������__ooo__����������/һ��������x_ooo__����һ�߱��£�����������
    {
        moves.emplace_back(pos);
        uint8_t d = 0;
        for (; d < DIRECTION4_COUNT; ++d)
        {
            if (getLayer2(pos.row, pos.col, atackside, d) == CHESSTYPE_4) break;
        }
        //�ж�����������
        int defend_point_count = 1;

        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//����
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
    else if (atackType == CHESSTYPE_44)//һ�����㣬��������
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
    else if (atackType == CHESSTYPE_43)//һ�����㣬�ĸ�����
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
    else if (atackType == CHESSTYPE_33)//һ�����㣬�������
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
        while (temppos.displace8(1, n)) //����������߽�
        {
            if (getState(temppos.row, temppos.col) == PIECE_BLANK)
            {
                blankCount++;
                uint8_t tempType = getLayer2(temppos.row, temppos.col, atackside, n / 2);
                if (tempType > CHESSTYPE_0)
                {
                    if (getChessType(temppos.row, temppos.col, defendside) != CHESSTYPE_BAN)//��������
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

void ChessBoard::getThreatCandidates(int level, vector<StepCandidateItem>& moves, bool extend)
{
    uint8_t side = getLastStep().getOtherSide();
    ForEachPosition
    {
        uint8_t type = getChessType(pos, side);
        if (!(canMove(pos)))
        {
            continue;
        }

        if (type > CHESSTYPE_D3 && type < CHESSTYPE_BAN)
        {
            if (level < 1)
            {
                if (type != CHESSTYPE_5) continue;
            }
            else if (level < 2)
            {
                if (Util::isalive3or33(type)) continue;
            }
            moves.emplace_back(pos, type, getChessDirection(pos, side));
        }
    }
    std::sort(moves.begin(), moves.end(), CandidateItemCmp);
}

void ChessBoard::getBanReletedPos(set<Position>& releted, Position center, uint8_t side)
{
    //�ҳ��Ƿ��н��ֵ�
    Position temppos;

    vector<Position> banset;
    for (int d = 0; d < DIRECTION4_COUNT; ++d)
    {
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//����
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
            for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//����
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


size_t ChessBoard::getPNCandidates(vector<StepCandidateItem>& moves, bool isatack)
{
    uint8_t side = getLastStep().getOtherSide();

    if (hasChessType(side, CHESSTYPE_5))
    {
        ForEachPosition
        {
            if (!canMove(pos.row, pos.col)) continue;
            if (getChessType(pos, side) == CHESSTYPE_5)
            {
                moves.emplace_back(pos, 10);
                return 1;
            }
        }
        return 0;
    }
    else if (hasChessType(Util::otherside(side), CHESSTYPE_5))
    {
        ForEachPosition
        {
            if (!canMove(pos.row, pos.col)) continue;
            if (getChessType(pos, Util::otherside(side)) == CHESSTYPE_5)
            {
                moves.emplace_back(pos, 10);
                return 1;
            }
        }
        return 0;
    }


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

        if (isatack && Util::isdead4(selftype))
        {
            continue;
        }

        uint8_t otherp = getChessType(pos, Util::otherside(side));



        if (isatack)
        {
            moves.emplace_back(pos, getChessTypeInfo(selftype).atackBaseFactor);
        }
        else // defend
        {
            int atack = getRelatedFactor(pos, side);
            int defend = getRelatedFactor(pos, Util::otherside(side), true);

            if (lastStep.step < 10)
            {
                if (otherp == CHESSTYPE_2) defend += 2;
                //if (selftype == CHESSTYPE_J2) defend -= 4;
            }

            if (atack + defend == 0 && (!Util::isdead4(selftype)))
            {
                continue;
            }
            moves.emplace_back(pos, atack + defend);
        }
    }
    std::sort(moves.begin(), moves.end(), CandidateItemCmp);

    if (isatack)
    {
        for (auto i = 0; i < moves.size(); ++i)
        {
            if (moves[i].value == 0)
            {
                return i;
            }
        }

    }
    return moves.size();
}

size_t ChessBoard::getUsefulCandidates(vector<StepCandidateItem>& moves, bool atack)
{
    uint8_t side = getLastStep().getOtherSide();
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

        if (atack == 0 && otherp < CHESSTYPE_J3)
        {
            moves.emplace_back(pos, 0);
            continue;
        }

        int defend = getRelatedFactor(pos, Util::otherside(side), true);

        if (lastStep.step < 10)
        {
            if (otherp == CHESSTYPE_2) defend += 2;
        }

        if (Util::isdead4(selftype) && atack == 0) defend = 0;

        if (atack == 0 && otherp == CHESSTYPE_0) moves.emplace_back(pos, 0);
        else moves.emplace_back(pos, atack + defend, 0, selftype);
    }

    std::sort(moves.begin(), moves.end(), CandidateItemCmp);

    for (auto i = 0; i < moves.size(); ++i)
    {
        if (moves[i].value <= moves[0].value / 3)
        {
            return i;
        }
    }

    return moves.size();
}

size_t ChessBoard::getNormalCandidates(vector<StepCandidateItem>& moves, bool isAtacker)
{
    uint8_t side = getLastStep().getOtherSide();
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

        /*if (atack == 0 && otherp < CHESSTYPE_J3)
        {
            if (!(otherp > CHESSTYPE_0 && Util::isdead4(selftype))) moves.emplace_back(pos, -1);
        }*/

        if (selftype <CHESSTYPE_J2 && otherp < CHESSTYPE_J3)
        {
            continue;
        }

        int defend = getRelatedFactor(pos, Util::otherside(side), true);

        //if (lastStep.step < 10)
        //{
        //    if (otherp == CHESSTYPE_2) defend += 2;
        //}

        //if (isAtacker)
        //{
        //    if (Util::isdead4(selftype) && atack == 0) defend = 0;
        //}
        //else
        //{
        //    if (atack == 0 && otherp == CHESSTYPE_0) moves.emplace_back(pos, -1);
        //    if (Util::isdead4(selftype) && atack == 0) defend = 0;
        //}
        moves.emplace_back(pos, atack + defend, 0, selftype);

    }

        //std::sort(moves.begin(), moves.end(), CandidateItemCmp);

        //for (auto i = 0; i < moves.size(); ++i)
        //{
        //    if (moves[i].value == 0)
        //    {
        //        return i;
        //    }
        //}

    return moves.size();
}


const ChessTypeInfo chesstypes[CHESSTYPE_COUNT] = {
    { 0    ,   0,   0 },           //CHESSTYPE_0,  +CHESSTYPE_2*2 +CHESSTYPE_J2*2 (0)
    { 0   ,    0,   0 },           //CHESSTYPE_dj2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
    { 10   ,   4,   0 },           //CHESSTYPE_j2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
    { 10   ,   6,   0 },           //CHESSTYPE_2,  -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*2 +CHESSTYPE_J3*2 (0)
    { 10   ,   2,   0 },           //CHESSTYPE_d3, -CHESSTYPE_D3*2 +CHESSTYPE_D4*2 (0)
    { 80   ,   6,   4 },           //CHESSTYPE_J3  -CHESSTYPE_3*1 -CHESSTYPE_J3*2 +CHESSTYPE_4*1 +CHESSTYPE_D4*2 (0)
    { 100  ,   8,   6 },           //CHESSTYPE_3,  -CHESSTYPE_3*2 -CHESSTYPE_J3*2 +CHESSTYPE_4*2 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
    { 120  ,   0,   6 },           //CHESSTYPE_d4, -CHESSTYPE_D4*2 +CHESSTYPE_5 (0) ���ȼ�����
    { 150  ,   0,   6 },           //CHESSTYPE_d4p -CHESSTYPE_D4P*1 -CHESSTYPE_D4 +CHESSTYPE_5 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
    { 250  ,  15,  10 },           //CHESSTYPE_33, -CHESSTYPE_33*1 -CHESSTYPE_3*0-2 -CHESSTYPE_J3*2-4 +CHESSTYPE_4*2-4 +CHESSTYPE_D4*2-4 (CHESSTYPE_4*2)
    { 450  ,  50,  12 },           //CHESSTYPE_43, -CHESSTYPE_43*1 -CHESSTYPE_D4*1 -CHESSTYPE_J3*2 -CHESSTYPE_3*1 +CHESSTYPE_5*1 +CHESSTYPE_4*2 (CHESSTYPE_4*2)
    { 500  , 100,  12 },           //CHESSTYPE_44, -CHESSTYPE_44 -CHESSTYPE_D4*2 +2��CHESSTYPE_5    (CHESSTYPE_5)
    { 500  , 100,  15 },           //CHESSTYPE_4,  -CHESSTYPE_4*1-2 -CHESSTYPE_D4*1-2 +CHESSTYPE_5*2 (CHESSTYPE_5)
    { 10000, 100, 150 },           //CHESSTYPE_5,
    { -100 ,-100, -10 },           //CHESSTYPE_BAN,
};

//const ChessTypeInfo chesstypes[CHESSTYPE_COUNT] = {
//    { 0    ,   0,   0 },           //CHESSTYPE_0,  +CHESSTYPE_2*2 +CHESSTYPE_J2*2 (0)
//      { 0, 0, 0 },           //CHESSTYPE_dj2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
//    { 10   ,   4,   0 },           //CHESSTYPE_j2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
//    { 10   ,   8,   2 },           //CHESSTYPE_2,  -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*2 +CHESSTYPE_J3*2 (0)
//    { 10   ,   6,   2 },           //CHESSTYPE_d3, -CHESSTYPE_D3*2 +CHESSTYPE_D4*2 (0)
//    { 80   ,   8,   6 },           //CHESSTYPE_J3  -CHESSTYPE_3*1 -CHESSTYPE_J3*2 +CHESSTYPE_4*1 +CHESSTYPE_D4*2 (0)
//    { 100  ,  10,   8 },           //CHESSTYPE_3,  -CHESSTYPE_3*2 -CHESSTYPE_J3*2 +CHESSTYPE_4*2 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
//    { 120  ,  12,   8 },           //CHESSTYPE_d4, -CHESSTYPE_D4*2 +CHESSTYPE_5 (0) ���ȼ�����
//    { 150  ,  12,  10 },           //CHESSTYPE_d4p -CHESSTYPE_D4P*1 -CHESSTYPE_D4 +CHESSTYPE_5 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
//    { 250  ,  20,  20 },           //CHESSTYPE_33, -CHESSTYPE_33*1 -CHESSTYPE_3*0-2 -CHESSTYPE_J3*2-4 +CHESSTYPE_4*2-4 +CHESSTYPE_D4*2-4 (CHESSTYPE_4*2)
//    { 450  ,  50,  20 },           //CHESSTYPE_43, -CHESSTYPE_43*1 -CHESSTYPE_D4*1 -CHESSTYPE_J3*2 -CHESSTYPE_3*1 +CHESSTYPE_5*1 +CHESSTYPE_4*2 (CHESSTYPE_4*2)
//    { 500  , 100,  20 },           //CHESSTYPE_44, -CHESSTYPE_44 -CHESSTYPE_D4*2 +2��CHESSTYPE_5    (CHESSTYPE_5)
//    { 500  , 100,  20 },           //CHESSTYPE_4,  -CHESSTYPE_4*1-2 -CHESSTYPE_D4*1-2 +CHESSTYPE_5*2 (CHESSTYPE_5)
//    { 10000, 100, 150 },           //CHESSTYPE_5,
//    { -100 ,-100, -10 },           //CHESSTYPE_BAN,
//};

//�����龰ϸ�֣���Ҫ�۽��ڶԾ��Ƶ�Ӱ�����ƣ����Ǿ��Ʊ���
//���Ծ��㱾��ΪCHESSTYPE_0��Ҳ�п��ܶԾ����кܴ�Ӱ��
int ChessBoard::getRelatedFactor(Position pos, uint8_t side, bool defend)
{
    int base_factor = 0;//��ʼֵ
    int related_factor = 0;
    if (pieces[pos.row][pos.col].layer3[side] > CHESSTYPE_D4P)
    {
        return defend ? chesstypes[pieces[pos.row][pos.col].layer3[side]].defendBaseFactor : chesstypes[pieces[pos.row][pos.col].layer3[side]].atackBaseFactor;
    }
    Position temppos;
    for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
    {
        base_factor += defend ? chesstypes[pieces[pos.row][pos.col].layer2[side][d]].defendBaseFactor : chesstypes[pieces[pos.row][pos.col].layer2[side][d]].atackBaseFactor;
        continue;
        int iterbase = pieces[pos.row][pos.col].layer2[side][d] > CHESSTYPE_0 ? 2 : 1;
        //related factor, except base 
        int related_count_3[2] = { 0,0 };
        int related_count_d4[2] = { 0,0 };
        int blank[2] = { 0,0 }; // 0 left 1 right
        int chess[2] = { 0,0 };

        for (int i = 0; i < 2; ++i)//����
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
                                if (blank[i] == 3)//����ͬ������һ��related������blank[i] == 3����̫Զ�ˣ����ӵ�
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
                                if (blank[i] == 3)//�п��� �������7 ������
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
        if (relatedsituation[blank[0]][chess[0]][blank[1]][chess[1]])//����Ҫ5��������в ��������
        {
            if (defend)
            {
                if (related_count_3[0] + related_count_3[1] + related_count_d4[0] + related_count_d4[1] > 1)//�����ӷ�Χ�ڴ�������related
                {
                    related_factor += (related_count_d4[0] + related_count_d4[1]) * 1 + (related_count_3[0] + related_count_3[1]) * 1;
                }
            }
            else
            {
                if (related_count_3[0] + related_count_3[1] + related_count_d4[0] + related_count_d4[1] > 1)
                {
                    related_factor += (related_count_d4[0] + related_count_d4[1]) * 2 + (related_count_3[0] + related_count_3[1]) * 1;
                }
                else
                {
                    related_factor += (related_count_d4[0] + related_count_d4[1]) * 2 + (related_count_3[0] + related_count_3[1]) * 1;
                }
            }

        }
    }
    base_factor += related_factor;
    return base_factor;
}



struct StaticEvaluate
{
    int atack;
    int defend;
};

//const StaticEvaluate staticEvaluate[CHESSTYPE_COUNT] = {
//    {    0,  0 },           //CHESSTYPE_0,  +CHESSTYPE_2*2 +CHESSTYPE_J2*2 (0)
//    { 0, 0 },           //CHESSTYPE_dj2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
//    {    0,  0 },           //CHESSTYPE_j2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
//    {    0,  0 },           //CHESSTYPE_2,  -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*2 +CHESSTYPE_J3*2 (0)
//    {    0,  0 },           //CHESSTYPE_d3, -CHESSTYPE_D3*2 +CHESSTYPE_D4*2 (0)
//    {    8,  4 },           //CHESSTYPE_J3  -CHESSTYPE_3*1 -CHESSTYPE_J3*2 +CHESSTYPE_4*1 +CHESSTYPE_D4*2 (0)
//    {   12,  6 },           //CHESSTYPE_3,  -CHESSTYPE_3*2 -CHESSTYPE_J3*2 +CHESSTYPE_4*2 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
//    {   16,  8 },           //CHESSTYPE_d4, -CHESSTYPE_D4*2 +CHESSTYPE_5 (0) ���ȼ�����
//    {   16,  8 },           //CHESSTYPE_d4p -CHESSTYPE_D4P*1 -CHESSTYPE_D4 +CHESSTYPE_5 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
//    {   60, 20 },           //CHESSTYPE_33, -CHESSTYPE_33*1 -CHESSTYPE_3*0-2 -CHESSTYPE_J3*2-4 +CHESSTYPE_4*2-4 +CHESSTYPE_D4*2-4 (CHESSTYPE_4*2)
//    {  200, 20 },           //CHESSTYPE_43, -CHESSTYPE_43*1 -CHESSTYPE_D4*1 -CHESSTYPE_J3*2 -CHESSTYPE_3*1 +CHESSTYPE_5*1 +CHESSTYPE_4*2 (CHESSTYPE_4*2)
//    {  400, 20 },           //CHESSTYPE_44, -CHESSTYPE_44 -CHESSTYPE_D4*2 +2��CHESSTYPE_5    (CHESSTYPE_5)
//    {  400, 20 },           //CHESSTYPE_4,  -CHESSTYPE_4*1-2 -CHESSTYPE_D4*1-2 +CHESSTYPE_5*2 (CHESSTYPE_5)
//    { 1000, 25 },           //CHESSTYPE_5,
//    {  -10,-10 },           //CHESSTYPE_BAN,
//};

//const StaticEvaluate staticEvaluate[CHESSTYPE_COUNT] = {
//    { 0,   0 },           //CHESSTYPE_0,  +CHESSTYPE_2*2 +CHESSTYPE_J2*2 (0)
//    { 0,   0 },           //CHESSTYPE_dj2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
//    { 1,   1 },           //CHESSTYPE_j2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
//    { 2,   1 },           //CHESSTYPE_2,  -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*2 +CHESSTYPE_J3*2 (0)
//    { 1,   1 },           //CHESSTYPE_d3, -CHESSTYPE_D3*2 +CHESSTYPE_D4*2 (0)
//    { 8,   6 },           //CHESSTYPE_J3  -CHESSTYPE_3*1 -CHESSTYPE_J3*2 +CHESSTYPE_4*1 +CHESSTYPE_D4*2 (0)
//    { 10,  6 },           //CHESSTYPE_3,  -CHESSTYPE_3*2 -CHESSTYPE_J3*2 +CHESSTYPE_4*2 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
//    { 12,  8 },           //CHESSTYPE_d4, -CHESSTYPE_D4*2 +CHESSTYPE_5 (0) ���ȼ�����
//    { 12,  8 },           //CHESSTYPE_d4p -CHESSTYPE_D4P*1 -CHESSTYPE_D4 +CHESSTYPE_5 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
//    { 30,  10 },           //CHESSTYPE_33, -CHESSTYPE_33*1 -CHESSTYPE_3*0-2 -CHESSTYPE_J3*2-4 +CHESSTYPE_4*2-4 +CHESSTYPE_D4*2-4 (CHESSTYPE_4*2)
//    { 50,  20 },           //CHESSTYPE_43, -CHESSTYPE_43*1 -CHESSTYPE_D4*1 -CHESSTYPE_J3*2 -CHESSTYPE_3*1 +CHESSTYPE_5*1 +CHESSTYPE_4*2 (CHESSTYPE_4*2)
//    { 100, 20 },           //CHESSTYPE_44, -CHESSTYPE_44 -CHESSTYPE_D4*2 +2��CHESSTYPE_5    (CHESSTYPE_5)
//    { 100, 40 },           //CHESSTYPE_4,  -CHESSTYPE_4*1-2 -CHESSTYPE_D4*1-2 +CHESSTYPE_5*2 (CHESSTYPE_5)
//    { 1000,50 },           //CHESSTYPE_5,
//    { -10,-10 },           //CHESSTYPE_BAN,
//};

const StaticEvaluate staticEvaluate[CHESSTYPE_COUNT] = {
    { 0,    0 },           //CHESSTYPE_0,  +CHESSTYPE_2*2 +CHESSTYPE_J2*2 (0)
    { 0,    0 },           //CHESSTYPE_dj2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
    { 1,    1 },           //CHESSTYPE_j2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
    { 2,    2 },           //CHESSTYPE_2,  -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*2 +CHESSTYPE_J3*2 (0)
    { 3,    3 },           //CHESSTYPE_d3, -CHESSTYPE_D3*2 +CHESSTYPE_D4*2 (0)
    { 8,    8 },           //CHESSTYPE_J3  -CHESSTYPE_3*1 -CHESSTYPE_J3*2 +CHESSTYPE_4*1 +CHESSTYPE_D4*2 (0)
    { 10,  10 },           //CHESSTYPE_3,  -CHESSTYPE_3*2 -CHESSTYPE_J3*2 +CHESSTYPE_4*2 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
    { 12,  12 },           //CHESSTYPE_d4, -CHESSTYPE_D4*2 +CHESSTYPE_5 (0) ���ȼ�����
    { 12,  12 },           //CHESSTYPE_d4p -CHESSTYPE_D4P*1 -CHESSTYPE_D4 +CHESSTYPE_5 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
    { 20,  20 },           //CHESSTYPE_33, -CHESSTYPE_33*1 -CHESSTYPE_3*0-2 -CHESSTYPE_J3*2-4 +CHESSTYPE_4*2-4 +CHESSTYPE_D4*2-4 (CHESSTYPE_4*2)
    { 30,  30 },           //CHESSTYPE_43, -CHESSTYPE_43*1 -CHESSTYPE_D4*1 -CHESSTYPE_J3*2 -CHESSTYPE_3*1 +CHESSTYPE_5*1 +CHESSTYPE_4*2 (CHESSTYPE_4*2)
    { 40,  40 },           //CHESSTYPE_44, -CHESSTYPE_44 -CHESSTYPE_D4*2 +2��CHESSTYPE_5    (CHESSTYPE_5)
    { 20,  20 },           //CHESSTYPE_4,  -CHESSTYPE_4*1-2 -CHESSTYPE_D4*1-2 +CHESSTYPE_5*2 (CHESSTYPE_5)
    { 10000,50 },           //CHESSTYPE_5,
    { -10,-10 },           //CHESSTYPE_BAN,
};


//const StaticEvaluate staticEvaluate[CHESSTYPE_COUNT] = {
//    { 0,   0 },           //CHESSTYPE_0,  +CHESSTYPE_2*2 +CHESSTYPE_J2*2 (0)
//    { 0,   0 },           //CHESSTYPE_dj2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
//    { 0,   0 },           //CHESSTYPE_j2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
//    { 0,   0 },           //CHESSTYPE_2,  -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*2 +CHESSTYPE_J3*2 (0)
//    { 0,   0 },           //CHESSTYPE_d3, -CHESSTYPE_D3*2 +CHESSTYPE_D4*2 (0)
//    { 12,  6 },           //CHESSTYPE_J3  -CHESSTYPE_3*1 -CHESSTYPE_J3*2 +CHESSTYPE_4*1 +CHESSTYPE_D4*2 (0)
//    { 14,  8 },           //CHESSTYPE_3,  -CHESSTYPE_3*2 -CHESSTYPE_J3*2 +CHESSTYPE_4*2 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
//    { 12,  6 },           //CHESSTYPE_d4, -CHESSTYPE_D4*2 +CHESSTYPE_5 (0) ���ȼ�����
//    { 16,  8 },           //CHESSTYPE_d4p -CHESSTYPE_D4P*1 -CHESSTYPE_D4 +CHESSTYPE_5 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
//    { 40, 30 },           //CHESSTYPE_33, -CHESSTYPE_33*1 -CHESSTYPE_3*0-2 -CHESSTYPE_J3*2-4 +CHESSTYPE_4*2-4 +CHESSTYPE_D4*2-4 (CHESSTYPE_4*2)
//    { 50, 35 },           //CHESSTYPE_43, -CHESSTYPE_43*1 -CHESSTYPE_D4*1 -CHESSTYPE_J3*2 -CHESSTYPE_3*1 +CHESSTYPE_5*1 +CHESSTYPE_4*2 (CHESSTYPE_4*2)
//    { 60, 40 },           //CHESSTYPE_44, -CHESSTYPE_44 -CHESSTYPE_D4*2 +2��CHESSTYPE_5    (CHESSTYPE_5)
//    { 60, 40 },           //CHESSTYPE_4,  -CHESSTYPE_4*1-2 -CHESSTYPE_D4*1-2 +CHESSTYPE_5*2 (CHESSTYPE_5)
//    { 10000, 60 },           //CHESSTYPE_5,
//    { -20,-20 },           //CHESSTYPE_BAN,
//};



//weight�Ƕ���side����ƫ��Ĭ��100
//int ChessBoard::getGlobalEvaluate(uint8_t side, int weight)
//{
//    //ʼ�����Խ�����(atackside)Ϊ��
//    uint8_t defendside = lastStep.state;
//    uint8_t atackside = Util::otherside(defendside);
//
//    int atack_evaluate = 0;
//    int defend_evaluate = 0;
//    //������������
//    ForEachPosition
//    {
//        //�������ӵĲ�������
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
//    return side == atackside ? atack_evaluate - defend_evaluate : -(atack_evaluate - defend_evaluate);
//}

int ChessBoard::getGlobalEvaluate(uint8_t side, int weight)
{
    //ʼ�����Խ�����(atackside)Ϊ��
    uint8_t defendside = lastStep.state;
    uint8_t atackside = Util::otherside(defendside);

    int atack_evaluate = 0;
    int defend_evaluate = 0;
    double factor = 1.0;
    //������������
    ForEachPosition
    {
        //�������ӵĲ�������
        if (!(canMove(pos) && useful(pos)))
        {
            continue;
        }
        factor = position_weight[pieces[pos.row][pos.col].around[defendside]];
        if (pieces[pos.row][pos.col].layer3[atackside] < CHESSTYPE_33)
        {
            for (uint8_t d = 0; d < 4; ++d)
            {
                atack_evaluate += (int)(staticEvaluate[pieces[pos.row][pos.col].layer2[atackside][d]].atack * factor);
                //atack_evaluate += (staticEvaluate[pieces[pos.row][pos.col].layer2[atackside][d]].atack);
            }
        }
        else
        {
            //atack_evaluate += (int)(staticEvaluate[pieces[pos.row][pos.col].layer3[atackside]].atack * factor);
            atack_evaluate += staticEvaluate[pieces[pos.row][pos.col].layer3[atackside]].atack; // �����Ͳ���Ҫ��factor��
        }

        factor = position_weight[pieces[pos.row][pos.col].around[atackside]];
        if (pieces[pos.row][pos.col].layer3[defendside] < CHESSTYPE_33)
        {
            for (uint8_t d = 0; d < 4; ++d)
            {
                defend_evaluate += (int)(staticEvaluate[pieces[pos.row][pos.col].layer2[defendside][d]].defend * factor);
                //defend_evaluate += (staticEvaluate[pieces[pos.row][pos.col].layer2[defendside][d]].defend);
            }
        }
        else
        {
            defend_evaluate += (int)(staticEvaluate[pieces[pos.row][pos.col].layer3[defendside]].defend * factor);
            //defend_evaluate += staticEvaluate[pieces[pos.row][pos.col].layer3[defendside]].defend;
        }

    }
    return side == atackside ? atack_evaluate * weight / 100 - defend_evaluate + 30 : -(atack_evaluate - defend_evaluate * weight / 100 + 30);
}



double ChessBoard::getStaticFactor(Position pos, uint8_t side, bool defend)
{
    uint8_t layer3type = pieces[pos.row][pos.col].layer3[side];
    if (layer3type == CHESSTYPE_0 || layer3type > CHESSTYPE_D4P)
    {
        return 1.0;
    }

    double base_factor = 1.0;//��ʼֵ

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
        continue;
        //related factor, except base 
        int releted_count_2 = 0;
        int releted_count_3 = 0;
        int releted_count_d4 = 0;
        int blank[2] = { 0,0 }; // 0 left 1 right
        int chess[2] = { 0,0 };

        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//����
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
            base_factor += releted_count_2 * 0.2 + releted_count_3 * 0.4 + releted_count_d4 * 0.6;
        }
    }
    return base_factor;
}

void ChessBoard::printGlobalEvaluate(string &s)
{
    //ʼ�����Խ�����(atackside)Ϊ��
    uint8_t defendside = lastStep.state;
    uint8_t atackside = Util::otherside(defendside);
    stringstream ss;
    int atack = 0, defend = 0;
    ss << "/" << "\t";
    for (int index = 0; index < Util::BoardSize; index++)
    {
        ss << index << "\t";
    }
    //������������
    ForEachPosition
    {
        if (pos.col == 0)
        {
            ss << "\r\n\r\n\r\n";
            ss << (int)pos.row << "\t";
        }
    //�������ӵĲ�������
    if (!canMove(pos) || !useful(pos))
    {
        ss << 0 << "|" << 0 << "\t";
        continue;
    }
    double factor = position_weight[pieces[pos.row][pos.col].around[defendside]];
    int atack_evaluate = 0;
    if (pieces[pos.row][pos.col].layer3[atackside] < CHESSTYPE_33)
    {
        for (uint8_t d = 0; d < 4; ++d)
        {
            atack_evaluate += (int)(staticEvaluate[pieces[pos.row][pos.col].layer2[atackside][d]].atack * factor);
            //atack_evaluate += (staticEvaluate[pieces[pos.row][pos.col].layer2[atackside][d]].atack);
        }
    }
    else
    {
        //atack_evaluate += (int)(staticEvaluate[pieces[pos.row][pos.col].layer3[atackside]].atack * factor);
        atack_evaluate += staticEvaluate[pieces[pos.row][pos.col].layer3[atackside]].atack; // �����Ͳ���Ҫ��factor��
    }
    atack += atack_evaluate;

    factor = position_weight[pieces[pos.row][pos.col].around[atackside]];
    int defend_evaluate = 0;
    if (pieces[pos.row][pos.col].layer3[defendside] < CHESSTYPE_33)
    {
        for (uint8_t d = 0; d < 4; ++d)
        {
            defend_evaluate += (int)(staticEvaluate[pieces[pos.row][pos.col].layer2[defendside][d]].defend * factor);
            //defend_evaluate += (staticEvaluate[pieces[pos.row][pos.col].layer2[defendside][d]].defend);
        }
    }
    else
    {
        defend_evaluate += (int)(staticEvaluate[pieces[pos.row][pos.col].layer3[defendside]].defend * factor);
        //defend_evaluate += staticEvaluate[pieces[pos.row][pos.col].layer3[defendside]].defend;
    }

    defend += defend_evaluate;

    ss << (atackside == PIECE_BLACK ? atack_evaluate : defend_evaluate) << "|" << (atackside == PIECE_WHITE ? atack_evaluate : defend_evaluate) << "\t";
    }
    ss << "\r\n\r\n\r\n";
    ss << "black:" << (atackside == PIECE_BLACK ? atack : defend) << "|" << "white:" << (atackside == PIECE_WHITE ? atack : defend);
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
    if (add) //�������
    {
        hash.check_key ^= zcheck[row][col][PIECE_BLANK];//ԭ���ǿյ�
        hash.check_key ^= zcheck[row][col][side];
        hash.hash_key ^= zkey[row][col][PIECE_BLANK];
        hash.hash_key ^= zkey[row][col][side];
    }
    else //��������
    {
        hash.check_key ^= zcheck[row][col][side];       //ԭ�������ӵ�
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
