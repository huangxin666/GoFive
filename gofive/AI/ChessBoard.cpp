#include "ChessBoard.h"
#include "TrieTree.h"
#include <random>
#include <algorithm>
using namespace std;


int8_t Util::BoardSize = 0;
PositionIter Util::position_iter[BOARD_SIZE_MAX][BOARD_SIZE_MAX];

void ChessBoard::initStaticHelper()
{
    initZobrist();
    initLayer2Table();
    init2to3table();
    initPatternToLayer2Table();
    initPositionWeightTable();
}

uint8_t* ChessBoard::layer2_table[BOARD_SIZE_MAX + 1] = { NULL };
uint8_t* ChessBoard::layer2_table_ban[BOARD_SIZE_MAX + 1] = { NULL };
//uint8_t ChessBoard::layer2_to_layer3_table[CHESSTYPE_COUNT][CHESSTYPE_COUNT][CHESSTYPE_COUNT][CHESSTYPE_COUNT][3];
uint8_t ChessBoard::layer2_to_layer3_table[UINT16_MAX + 1][3];
uint8_t ChessBoard::pattern_to_layer2_table[UINT8_MAX + 1][UINT8_MAX + 1];//2^8
uint8_t ChessBoard::pattern_to_layer2_table_ban[UINT8_MAX + 1][UINT8_MAX + 1];//2^8
double ChessBoard::position_weight[UINT8_MAX + 1];
double ChessBoard::chessboard_weight[BOARD_SIZE_MAX][BOARD_SIZE_MAX];


ChessBoard::ChessBoard()
{
    lastStep.step = 0;
    lastStep.state = PIECE_WHITE;//下一个就是黑了
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
            //pieces[pos.row][pos.col].layer2[PIECE_BLACK][d] = 0;
            //pieces[pos.row][pos.col].layer2[PIECE_WHITE][d] = 0;
            pieces[pos.row][pos.col].layer2[PIECE_BLACK] = 0;
            pieces[pos.row][pos.col].layer2[PIECE_WHITE] = 0;
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

        //pieces[pos.row][pos.col].around[PIECE_BLACK] = 0;
        //pieces[pos.row][pos.col].around[PIECE_WHITE] = 0;
        //if (pos.col < 2)//左边 0 4 6
        //{
        //    pieces[pos.row][pos.col].around[PIECE_BLACK] |= 0x51;
        //    pieces[pos.row][pos.col].around[PIECE_WHITE] |= 0x51;
        //}

        //if (pos.col > Util::BoardSize - 3)//右边 1 5 7
        //{
        //    pieces[pos.row][pos.col].around[PIECE_BLACK] |= 0xA2;
        //    pieces[pos.row][pos.col].around[PIECE_WHITE] |= 0xA2;
        //}

        //if (pos.row < 2)//上边 2 4 7
        //{
        //    pieces[pos.row][pos.col].around[PIECE_BLACK] |= 0x94;
        //    pieces[pos.row][pos.col].around[PIECE_WHITE] |= 0x94;
        //}

        //if (pos.row > Util::BoardSize - 3)//下边 3 5 6
        //{
        //    pieces[pos.row][pos.col].around[PIECE_BLACK] |= 0x68;
        //    pieces[pos.row][pos.col].around[PIECE_WHITE] |= 0x68;
        //}
    }
}

void ChessBoard::update_layer(Position pos, uint8_t side, GAME_RULE rule)
{
    global_chesstype_count[PIECE_BLACK][pieces[pos.row][pos.col].layer3[PIECE_BLACK]]--;
    global_chesstype_count[PIECE_WHITE][pieces[pos.row][pos.col].layer3[PIECE_WHITE]]--;
    // 朝8个方向更新layer
    for (uint8_t d8 = 0; d8 < DIRECTION8_COUNT; ++d8)
    {
        int d4 = d8 >> 1; // d8/2
        Position temp = pos;for (int i = 0; i < 4 && temp.displace8(d8); ++i)
        {
            pieces[temp.row][temp.col].pattern[side][d4] |= pattern_mask[d8][i];
            if (pieces[temp.row][temp.col].layer1 == PIECE_BLANK)
            {
                pieces[temp.row][temp.col].layer2[PIECE_BLACK] &= layer2_mask[d4];
                pieces[temp.row][temp.col].layer2[PIECE_WHITE] &= layer2_mask[d4];

                global_chesstype_count[PIECE_BLACK][pieces[temp.row][temp.col].layer3[PIECE_BLACK]]--;
                global_chesstype_count[PIECE_WHITE][pieces[temp.row][temp.col].layer3[PIECE_WHITE]]--;

                if (rule == STANDARD)
                {
                    pieces[temp.row][temp.col].layer2[PIECE_BLACK] |= (uint16_t)pattern_to_layer2_table_ban[pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]][pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]] << (d4 * 4);
                    pieces[temp.row][temp.col].layer2[PIECE_WHITE] |= (uint16_t)pattern_to_layer2_table_ban[pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]][pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]] << (d4 * 4);

                    pieces[temp.row][temp.col].layer3[PIECE_BLACK] = layer2_to_layer3_table[pieces[temp.row][temp.col].layer2[PIECE_BLACK]][STANDARD];
                    pieces[temp.row][temp.col].layer3[PIECE_WHITE] = layer2_to_layer3_table[pieces[temp.row][temp.col].layer2[PIECE_WHITE]][STANDARD];
                }
                else
                {
                    pieces[temp.row][temp.col].layer2[PIECE_BLACK] |= (rule == FREESTYLE) ? (uint16_t)pattern_to_layer2_table[pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]][pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]] << (d4 * 4) : (uint16_t)pattern_to_layer2_table_ban[pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]][pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]] << (d4 * 4);
                    pieces[temp.row][temp.col].layer2[PIECE_WHITE] |= (uint16_t)pattern_to_layer2_table[pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]][pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]] << (d4 * 4);

                    pieces[temp.row][temp.col].layer3[PIECE_BLACK] = layer2_to_layer3_table[pieces[temp.row][temp.col].layer2[PIECE_BLACK]][rule];
                    pieces[temp.row][temp.col].layer3[PIECE_WHITE] = layer2_to_layer3_table[pieces[temp.row][temp.col].layer2[PIECE_WHITE]][FREESTYLE];
                }

                global_chesstype_count[PIECE_BLACK][pieces[temp.row][temp.col].layer3[PIECE_BLACK]]++;
                global_chesstype_count[PIECE_WHITE][pieces[temp.row][temp.col].layer3[PIECE_WHITE]]++;
            }
        }
    }
}

void ChessBoard::update_layer_undo(Position pos, uint8_t side, GAME_RULE rule)
{
    pieces[pos.row][pos.col].layer2[PIECE_BLACK] = 0;
    pieces[pos.row][pos.col].layer2[PIECE_WHITE] = 0;
    for (uint8_t d4 = 0; d4 < DIRECTION4_COUNT; ++d4)
    {
        if (rule == STANDARD)
        {
            pieces[pos.row][pos.col].layer2[PIECE_BLACK] |= (uint16_t)pattern_to_layer2_table_ban[pieces[pos.row][pos.col].pattern[PIECE_BLACK][d4]][pieces[pos.row][pos.col].pattern[PIECE_WHITE][d4]] << (d4 * 4);
            pieces[pos.row][pos.col].layer2[PIECE_WHITE] |= (uint16_t)pattern_to_layer2_table_ban[pieces[pos.row][pos.col].pattern[PIECE_WHITE][d4]][pieces[pos.row][pos.col].pattern[PIECE_BLACK][d4]] << (d4 * 4);

            pieces[pos.row][pos.col].layer3[PIECE_BLACK] = layer2_to_layer3_table[pieces[pos.row][pos.col].layer2[PIECE_BLACK]][STANDARD];
            pieces[pos.row][pos.col].layer3[PIECE_WHITE] = layer2_to_layer3_table[pieces[pos.row][pos.col].layer2[PIECE_WHITE]][STANDARD];
        }
        else
        {
            pieces[pos.row][pos.col].layer2[PIECE_BLACK] |= (rule == FREESTYLE) ? (uint16_t)pattern_to_layer2_table[pieces[pos.row][pos.col].pattern[PIECE_BLACK][d4]][pieces[pos.row][pos.col].pattern[PIECE_WHITE][d4]] << (d4 * 4) : (uint16_t)pattern_to_layer2_table_ban[pieces[pos.row][pos.col].pattern[PIECE_BLACK][d4]][pieces[pos.row][pos.col].pattern[PIECE_WHITE][d4]] << (d4 * 4);
            pieces[pos.row][pos.col].layer2[PIECE_WHITE] |= (uint16_t)pattern_to_layer2_table[pieces[pos.row][pos.col].pattern[PIECE_WHITE][d4]][pieces[pos.row][pos.col].pattern[PIECE_BLACK][d4]] << (d4 * 4);

            pieces[pos.row][pos.col].layer3[PIECE_BLACK] = layer2_to_layer3_table[pieces[pos.row][pos.col].layer2[PIECE_BLACK]][rule];
            pieces[pos.row][pos.col].layer3[PIECE_WHITE] = layer2_to_layer3_table[pieces[pos.row][pos.col].layer2[PIECE_WHITE]][FREESTYLE];
        }
    }


    global_chesstype_count[PIECE_BLACK][pieces[pos.row][pos.col].layer3[PIECE_BLACK]]++;
    global_chesstype_count[PIECE_WHITE][pieces[pos.row][pos.col].layer3[PIECE_WHITE]]++;

    // 朝8个方向更新layer
    for (uint8_t d8 = 0; d8 < DIRECTION8_COUNT; ++d8)
    {
        int d4 = d8 >> 1; // d8/2
        Position temp = pos;for (int i = 0; i < 4 && temp.displace8(d8); ++i)
        {
            pieces[temp.row][temp.col].pattern[side][d4] ^= pattern_mask[d8][i];
            if (pieces[temp.row][temp.col].layer1 == PIECE_BLANK)
            {
                pieces[temp.row][temp.col].layer2[PIECE_BLACK] &= layer2_mask[d4];
                pieces[temp.row][temp.col].layer2[PIECE_WHITE] &= layer2_mask[d4];
                global_chesstype_count[PIECE_BLACK][pieces[temp.row][temp.col].layer3[PIECE_BLACK]]--;
                global_chesstype_count[PIECE_WHITE][pieces[temp.row][temp.col].layer3[PIECE_WHITE]]--;

                if (rule == STANDARD)
                {
                    pieces[temp.row][temp.col].layer2[PIECE_BLACK] |= (uint16_t)pattern_to_layer2_table_ban[pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]][pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]] << (d4 * 4);
                    pieces[temp.row][temp.col].layer2[PIECE_WHITE] |= (uint16_t)pattern_to_layer2_table_ban[pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]][pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]] << (d4 * 4);

                    pieces[temp.row][temp.col].layer3[PIECE_BLACK] = layer2_to_layer3_table[pieces[temp.row][temp.col].layer2[PIECE_BLACK]][STANDARD];
                    pieces[temp.row][temp.col].layer3[PIECE_WHITE] = layer2_to_layer3_table[pieces[temp.row][temp.col].layer2[PIECE_WHITE]][STANDARD];
                }
                else
                {
                    pieces[temp.row][temp.col].layer2[PIECE_BLACK] |= (rule == FREESTYLE) ? (uint16_t)pattern_to_layer2_table[pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]][pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]] << (d4 * 4) : (uint16_t)pattern_to_layer2_table_ban[pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]][pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]] << (d4 * 4);
                    pieces[temp.row][temp.col].layer2[PIECE_WHITE] |= (uint16_t)pattern_to_layer2_table[pieces[temp.row][temp.col].pattern[PIECE_WHITE][d4]][pieces[temp.row][temp.col].pattern[PIECE_BLACK][d4]] << (d4 * 4);

                    pieces[temp.row][temp.col].layer3[PIECE_BLACK] = layer2_to_layer3_table[pieces[temp.row][temp.col].layer2[PIECE_BLACK]][rule];
                    pieces[temp.row][temp.col].layer3[PIECE_WHITE] = layer2_to_layer3_table[pieces[temp.row][temp.col].layer2[PIECE_WHITE]][FREESTYLE];
                }

                global_chesstype_count[PIECE_BLACK][pieces[temp.row][temp.col].layer3[PIECE_BLACK]]++;
                global_chesstype_count[PIECE_WHITE][pieces[temp.row][temp.col].layer3[PIECE_WHITE]]++;
            }
        }
    }
}

void ChessBoard::init2to3table()
{
    for (uint16_t d1 = 0; d1 < CHESSTYPE_COUNT; ++d1)
    {
        for (uint16_t d2 = 0; d2 < CHESSTYPE_COUNT; ++d2)
        {
            for (uint16_t d3 = 0; d3 < CHESSTYPE_COUNT; ++d3)
            {
                for (uint16_t d4 = 0; d4 < CHESSTYPE_COUNT; ++d4)
                {
                    layer2_to_layer3_table[d1 + (d2 << 4) + (d3 << 8) + (d4 << 12)][FREESTYLE] = layer2_to_layer3(d1, d2, d3, d4, FREESTYLE);
                    layer2_to_layer3_table[d1 + (d2 << 4) + (d3 << 8) + (d4 << 12)][STANDARD] = layer2_to_layer3(d1, d2, d3, d4, STANDARD);
                    layer2_to_layer3_table[d1 + (d2 << 4) + (d3 << 8) + (d4 << 12)][RENJU] = layer2_to_layer3(d1, d2, d3, d4, RENJU);
                }
            }
        }
    }
}

uint8_t ChessBoard::layer2_to_layer3(uint16_t d1, uint16_t d2, uint16_t d3, uint16_t d4, GAME_RULE rule)
{
    int count[CHESSTYPE_COUNT] = { 0 };
    ++count[d1]; ++count[d2]; ++count[d3]; ++count[d4];

    if (count[CHESSTYPE_5] > 0) return CHESSTYPE_5;//有5连可无视禁手
    if (count[CHESSTYPE_BAN] > 0) return rule == STANDARD ? CHESSTYPE_0 : CHESSTYPE_BAN; //长连
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

    for (uint8_t type = CHESSTYPE_D4P; type > CHESSTYPE_0; --type)
    {
        if (count[type] > 0) return type;
    }
    return CHESSTYPE_0;
}

int ChessBoard::getSimpleTotalScore(uint8_t side)
{
    int result = 0;
    ForEachMove(this)
    {

        result += getChessTypeInfo(pieces[pos.row][pos.col].layer3[side]).rating;

    }
    return result;
}

bool ChessBoard::moveNull()
{
    lastStep.changeSide();
    return true;
}

bool ChessBoard::move(Position pos, uint8_t side, GAME_RULE ban)
{
    if (pieces[pos.row][pos.col].layer1 != PIECE_BLANK)
    {
        return false;//已有棋子
    }
    lastStep.step++;
    lastStep.setState(side);
    lastStep.pos = pos;
    lastStep.chessType = getChessType(pos, side);

    pieces[pos.row][pos.col].layer1 = side;

    //update_layer_old(row, col, rule);
    update_layer(pos, side, ban);

    updateHashPair(pos, side, true);
    return true;
}

bool ChessBoard::moveMultiReplies(vector<Position> &moves, GAME_RULE ban)
{
    if (moves.empty())
    {
        return false;
    }
    lastStep.pos = moves[0];
    lastStep.step++;
    lastStep.changeSide();

    for (size_t i = 0; i < moves.size(); ++i)
    {
        pieces[moves[i].row][moves[i].col].layer1 = lastStep.state;

        //update_layer_old(moves[i].row, moves[i].col, rule);
        update_layer(moves[i], lastStep.state, ban);

        //updateHashPair(moves[i].row, moves[i].col, lastStep.state, true);
    }
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

    //update_layer_old(pos.row, pos.col, rule);
    update_layer_undo(pos, side, ban);

    updateHashPair(pos, side, false);

    return true;
}


void ChessBoard::getDependentThreatCandidates(Position pos, int level, vector<StepCandidateItem>& moves, bool extend)
{
    uint8_t side = lastStep.getOtherSide();
    if (hasChessType(side, CHESSTYPE_5))
    {
        ForEachMove(this)
        {
            if (getChessType(pos, side) == CHESSTYPE_5)
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

    for (int d8 = 0; d8 < DIRECTION8_COUNT; ++d8)
    {
        int d4 = d8 >> 1;
        
        Position temppos = pos; for (int8_t i = 0; i < 4 && temppos.displace8(d8); ++i)
        {
            if (getState(temppos) == PIECE_BLANK)
            {
                if (getChessType(temppos, side) == CHESSTYPE_BAN)
                {
                    continue;
                }

                if (Util::isthreat(getLayer2(temppos, side, d4)))
                {
                    if (level < 2 && Util::isalive3or33(getChessType(temppos, side)))
                    {
                        continue;
                    }

                    moves.emplace_back(temppos, getLayer2(temppos, side, d4), d4);
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
            else if (getState(temppos) == side)
            {
                continue;
            }
            else
            {
                break;
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
        for (int symbol = 0; symbol < 2; ++symbol)//正反
        {
            int d8 = direction * 2 + symbol;
            Position temppos = pos; for (int8_t offset = 0; offset < 4 && temppos.displace8(d8); ++offset)
            {
                if (getState(temppos) == PIECE_BLANK)
                {
                    if (getLayer2(temppos, side, direction) == CHESSTYPE_5)
                    {
                        if (getChessType(temppos, lastStep.getOtherSide()) != CHESSTYPE_BAN)
                        {
                            reply.emplace_back(temppos);
                        }
                    }
                }
                else if (getState(temppos) == side)
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
        for (int symbol = 0; symbol < 2; ++symbol)//正反
        {
            int d8 = direction * 2 + symbol;
            Position temppos = pos; for (int8_t offset = 0; offset < 4 && temppos.displace8(d8); ++offset)
            {
                if (getState(temppos) == PIECE_BLANK)
                {
                    if (getLayer2(temppos, side, direction) == CHESSTYPE_5)
                    {
                        if (getChessType(temppos, lastStep.getOtherSide()) != CHESSTYPE_BAN)
                        {
                            reply.emplace_back(temppos);
                        }
                        return;
                    }
                }
                else if (getState(temppos) == side)
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
        for (int symbol = 0; symbol < 2; ++symbol)//正反
        {
            int d8 = direction * 2 + symbol;
            Position temppos = pos; for (int8_t offset = 0; offset < 4 && temppos.displace8(d8); ++offset)
            {
                if (getState(temppos) == PIECE_BLANK)
                {
                    if (getLayer2(temppos, side, direction) == CHESSTYPE_4)
                    {
                        count++;
                        center = temppos;
                        if (getChessType(temppos, lastStep.getOtherSide()) != CHESSTYPE_BAN)
                        {
                            reply.emplace_back(temppos);
                        }
                    }
                }
                else if (getState(temppos) == side)
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
        if (count > 1) return;
        //假活三
        if (count == 0) return;
        //跳三
        for (int symbol = 0; symbol < 2; ++symbol)//正反
        {
            int d8 = direction * 2 + symbol;
            Position temppos = center; for (int8_t offset = 0; offset < 4 && temppos.displace8(d8); ++offset)
            {
                if (getState(temppos) == PIECE_BLANK)
                {
                    if (Util::isdead4(getLayer2(temppos, side, direction)) ||
                        getLayer2(temppos, side, direction) == CHESSTYPE_44)//special
                    {
                        if (getChessType(temppos, lastStep.getOtherSide()) != CHESSTYPE_BAN)
                        {
                            reply.emplace_back(temppos);
                        }
                    }
                    break;
                }
                else if (getState(temppos) == side)
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
            for (int symbol = 0; symbol < 2; ++symbol)//正反
            {
                int d8 = direction * 2 + symbol;
                Position temppos = pos; for (int8_t offset = 0; offset < 4 && temppos.displace8(d8); ++offset)
                {
                    if (getState(temppos) == PIECE_BLANK && Util::isSpecialType(getChessType(temppos, side)))
                    {
                        if (getChessType(temppos, lastStep.getOtherSide()) != CHESSTYPE_BAN)
                        {
                            reply.emplace_back(temppos);
                        }
                        ChessBoard tempboard = *this;
                        tempboard.move(temppos, side, rule);
                        for (uint8_t d = 0; d < DIRECTION4::DIRECTION4_COUNT; ++d)
                        {
                            if (Util::isthreat(getLayer2(temppos, side, d)))
                            {
                                tempboard.getThreatReplies(temppos, getLayer2(temppos, side, d), d, reply, rule);
                            }
                        }
                        return;
                    }
                    else if (getState(temppos) == side)
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
    //现在该防守方落子
    uint8_t defendside = getLastStep().getOtherSide();//防守方
    uint8_t atackside = getLastStep().state;//进攻方
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
            if (getLayer2(pos, atackside, d) == CHESSTYPE_4) break;
        }
        //判断是哪种棋型
        int defend_point_count = 1;

        for (int symbol = 0; symbol < 2; ++symbol)//正反
        {
            int d8 = d * 2 + symbol;
            Position temppos = pos; for (int8_t offset = 0; offset < 4 && temppos.displace8(d8); ++offset)
            {
                if (getState(temppos) == PIECE_BLANK)
                {

                    if (Util::isdead4(getLayer2(temppos, atackside, d)))
                    {
                        moves.emplace_back(temppos);
                        break;
                    }
                    else if (getLayer2(temppos, atackside, d) == CHESSTYPE_4)
                    {
                        if (moves.size() > 1) moves.pop_back();
                        moves.emplace_back(temppos);
                        return;
                    }
                }
                else if (getState(temppos) == atackside)
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
            if (getLayer2(pos, atackside, d) == CHESSTYPE_44)
            {
                directions.push_back(d * 2);
                directions.push_back(d * 2 + 1);
                break;
            }
            else if (Util::isdead4(getLayer2(pos, atackside, d)))
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
            if (Util::isdead4(getLayer2(pos, atackside, d)) || Util::isalive3(getLayer2(pos, atackside, d)))
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
            if (Util::isalive3(getLayer2(pos, atackside, d)))
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
        for (Position temppos = pos; temppos.displace8(n);) //如果不超出边界
        {
            if (getState(temppos) == PIECE_BLANK)
            {
                blankCount++;
                uint8_t tempType = getLayer2(temppos, atackside, n / 2);
                if (tempType > CHESSTYPE_0)
                {
                    if (getChessType(temppos, defendside) != CHESSTYPE_BAN)//被禁手了
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
            else if (getState(temppos) == defendside)
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
    ForEachMove(this)
    {
        uint8_t type = getChessType(pos, side);

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
    //找出是否有禁手点
    Position temppos;

    vector<Position> banset;
    for (uint8_t d8 = 0; d8 < DIRECTION8_COUNT; ++d8)
    {
        Position temppos = center; for (int i = 0; i < 4 && temppos.displace8(d8); ++i)
        {
            if ( pieces[temppos.row][temppos.col].layer1 == Util::otherside(side))//equal otherside
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

    for (auto banpos : banset)
    {
        for (uint8_t d8 = 0; d8 < DIRECTION8_COUNT; ++d8)
        {
            Position temppos = banpos; for (int i = 0; i < 4 && temppos.displace8(d8); ++i)
            {
                if (pieces[temppos.row][temppos.col].layer1 == side)//equal otherside
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


size_t ChessBoard::getPNCandidates(vector<StepCandidateItem>& moves, bool isatack)
{
    uint8_t side = getLastStep().getOtherSide();

    if (hasChessType(side, CHESSTYPE_5))
    {
        ForEachMove(this)
        {
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
        ForEachMove(this)
        {
            if (getChessType(pos, Util::otherside(side)) == CHESSTYPE_5)
            {
                moves.emplace_back(pos, 10);
                return 1;
            }
        }
        return 0;
    }


    ForEachMove(this)
    {
        if (!(useful(pos)))
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

size_t ChessBoard::getUsefulCandidates(vector<StepCandidateItem>& moves)
{
    uint8_t side = getLastStep().getOtherSide();
    uint8_t otherside = getLastStep().state;

    bool atack = false;
    bool defend = false;
    uint8_t atack_highest = 0;
    uint8_t defend_highest = 0;

    if (hasChessType(side, CHESSTYPE_5))
    {
        atack = true;
        atack_highest = CHESSTYPE_5;
    }
    else if (hasChessType(otherside, CHESSTYPE_5))//防5连
    {
        defend = true;
        defend_highest = CHESSTYPE_5;
    }
    else if (hasChessType(otherside, CHESSTYPE_4) || hasChessType(otherside, CHESSTYPE_44))
    {
        defend = true;
        defend_highest = CHESSTYPE_D4;
        atack = true;
        atack_highest = CHESSTYPE_D4;
    }
    else if (hasChessType(otherside, CHESSTYPE_43))
    {
        defend = true;
        defend_highest = CHESSTYPE_D3;
        atack = true;
        atack_highest = CHESSTYPE_D4;
    }

    ForEachMove(this)
    {
        if (!(useful(pos)))
        {
            continue;
        }

        uint8_t selftype = getChessType(pos, side);

        if (selftype == CHESSTYPE_BAN)
        {
            continue;
        }

        uint8_t otherp = getChessType(pos, Util::otherside(side));

        if (atack && defend)
        {
            if (selftype < atack_highest && otherp < defend_highest) continue;
        }
        else if (atack)
        {
            if (selftype < atack_highest) continue;
        }
        else if (defend)
        {
            if (otherp < defend_highest) continue;
        }
        else
        {
            /*int atack = getRelatedFactor(pos, side);
            int defend = getRelatedFactor(pos, Util::otherside(side), true);
            if (atack < 6 && otherp < CHESSTYPE_3)
            {
                continue;
            }*/
            continue;
        }

        moves.emplace_back(pos, atack + defend, 0, selftype);
    }

    std::sort(moves.begin(), moves.end(), CandidateItemCmp);


    return moves.size();
}

size_t ChessBoard::getNormalCandidates(vector<StepCandidateItem>& moves, bool isAtacker)
{
    uint8_t side = getLastStep().getOtherSide();
    uint8_t otherside = getLastStep().state;
    bool atack = false;
    bool defend = false;
    uint8_t atack_highest = 0;
    uint8_t defend_highest = 0;

    if (hasChessType(side, CHESSTYPE_5))
    {
        atack = true;
        atack_highest = CHESSTYPE_5;
    }
    else if (hasChessType(otherside, CHESSTYPE_5))//防5连
    {
        defend = true;
        defend_highest = CHESSTYPE_5;
    }
    else if (hasChessType(otherside, CHESSTYPE_4) || hasChessType(otherside, CHESSTYPE_44))
    {
        defend = true;
        defend_highest = CHESSTYPE_D4;
        atack = true;
        atack_highest = CHESSTYPE_D4;
    }
    else if (hasChessType(otherside, CHESSTYPE_43))
    {
        defend = true;
        defend_highest = CHESSTYPE_D3;//fix J3 to D3
        atack = true;
        atack_highest = CHESSTYPE_D4;
    }

    ForEachMove(this)
    {

        uint8_t selftype = getChessType(pos, side);
        if (selftype == CHESSTYPE_BAN)
        {
            continue;
        }
        uint8_t othertype = getChessType(pos, Util::otherside(side));

        if (atack && defend)
        {
            if (selftype < atack_highest && othertype < defend_highest) continue;
            moves.emplace_back(pos, selftype + othertype, 0, selftype);
            continue;
        }
        else if (atack)
        {
            if (selftype < atack_highest) continue;
            moves.emplace_back(pos, selftype, 0, selftype);
            continue;
        }
        else if (defend)
        {
            if (othertype < defend_highest) continue;
            moves.emplace_back(pos, othertype, 0, selftype);
            continue;
        }
        else
        {
            if (lastStep.step < 10)
            {
                if (selftype < CHESSTYPE_J2 && othertype < CHESSTYPE_J2) continue;
            }
            else if (selftype < CHESSTYPE_J2 && othertype < CHESSTYPE_J3)
            {
                continue;
            }

            //int atack_score = getRelatedFactor(pos, side);
            //int defend_socre = getRelatedFactor(pos, Util::otherside(side), true);

            //moves.emplace_back(pos, atack_score + defend_socre, 0, selftype);

            moves.emplace_back(pos, selftype + othertype, 0, selftype);
            continue;
        }
    }

    if (moves.empty())//fix special situation
    {
        ForEachMove(this)
        {

            uint8_t selftype = getChessType(pos, side);
            if (selftype == CHESSTYPE_BAN)
            {
                continue;
            }
            uint8_t othertype = getChessType(pos, Util::otherside(side));

            if (selftype < CHESSTYPE_J2 && othertype < CHESSTYPE_J2) continue;

            moves.emplace_back(pos, selftype + othertype, 0, selftype);
            continue;
        }
        if (moves.empty())
        {
            ForEachMove(this)
            {

                uint8_t selftype = getChessType(pos, side);
                if (selftype == CHESSTYPE_BAN)
                {
                    continue;
                }
                uint8_t othertype = getChessType(pos, Util::otherside(side));

                moves.emplace_back(pos, selftype + othertype, 0, selftype);
                continue;
            }
        }
    }
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
    { 120  ,   0,   6 },           //CHESSTYPE_d4, -CHESSTYPE_D4*2 +CHESSTYPE_5 (0) 优先级降低
    { 150  ,   0,   6 },           //CHESSTYPE_d4p -CHESSTYPE_D4P*1 -CHESSTYPE_D4 +CHESSTYPE_5 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
    { 250  ,  15,  10 },           //CHESSTYPE_33, -CHESSTYPE_33*1 -CHESSTYPE_3*0-2 -CHESSTYPE_J3*2-4 +CHESSTYPE_4*2-4 +CHESSTYPE_D4*2-4 (CHESSTYPE_4*2)
    { 450  ,  50,  12 },           //CHESSTYPE_43, -CHESSTYPE_43*1 -CHESSTYPE_D4*1 -CHESSTYPE_J3*2 -CHESSTYPE_3*1 +CHESSTYPE_5*1 +CHESSTYPE_4*2 (CHESSTYPE_4*2)
    { 500  , 100,  12 },           //CHESSTYPE_44, -CHESSTYPE_44 -CHESSTYPE_D4*2 +2个CHESSTYPE_5    (CHESSTYPE_5)
    { 500  , 100,  15 },           //CHESSTYPE_4,  -CHESSTYPE_4*1-2 -CHESSTYPE_D4*1-2 +CHESSTYPE_5*2 (CHESSTYPE_5)
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
        base_factor += defend ? chesstypes[getLayer2(pos, side, d)].defendBaseFactor : chesstypes[getLayer2(pos, side, d)].atackBaseFactor;
        continue;
        int iterbase = getLayer2(pos, side, d) > CHESSTYPE_0 ? 2 : 1;
        //related factor, except base 
        int related_count_3[2] = { 0,0 };
        int related_count_d4[2] = { 0,0 };
        int blank[2] = { 0,0 }; // 0 left 1 right
        int chess[2] = { 0,0 };

        for (int i = 0; i < 2; ++i)//正反
        {
            for(Position temppos = pos; temppos.displace8(d * 2 + i);)
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

                            if (getLayer2(temppos, side, d2) > CHESSTYPE_3)
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
                            else if (getLayer2(temppos, side, d2) > CHESSTYPE_D3)
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
        //if (relatedsituation[blank[0]][chess[0]][blank[1]][chess[1]])//至少要5才能有威胁 加上自身
        //{
        //    if (defend)
        //    {
        //        if (related_count_3[0] + related_count_3[1] + related_count_d4[0] + related_count_d4[1] > 1)//在五子范围内存在两个related
        //        {
        //            related_factor += (related_count_d4[0] + related_count_d4[1]) * 1 + (related_count_3[0] + related_count_3[1]) * 1;
        //        }
        //    }
        //    else
        //    {
        //        if (related_count_3[0] + related_count_3[1] + related_count_d4[0] + related_count_d4[1] > 1)
        //        {
        //            related_factor += (related_count_d4[0] + related_count_d4[1]) * 2 + (related_count_3[0] + related_count_3[1]) * 1;
        //        }
        //        else
        //        {
        //            related_factor += (related_count_d4[0] + related_count_d4[1]) * 2 + (related_count_3[0] + related_count_3[1]) * 1;
        //        }
        //    }

        //}
    }
    base_factor += related_factor;
    return base_factor;
}



struct StaticEvaluate
{
    int atack;
    int defend;
};

//策略一：表现最好 21：15
//const StaticEvaluate staticEvaluate[CHESSTYPE_COUNT] = {
//{ 0,    0 },           //CHESSTYPE_0,  +CHESSTYPE_2*2 +CHESSTYPE_J2*2 (0)
//{ 0,    0 },           //CHESSTYPE_dj2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
//{ 2,    2 },           //CHESSTYPE_j2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
//{ 6,    6 },           //CHESSTYPE_2,  -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*2 +CHESSTYPE_J3*2 (0)
//{ 2,    2 },           //CHESSTYPE_d3, -CHESSTYPE_D3*2 +CHESSTYPE_D4*2 (0)
//{ 10,  10 },           //CHESSTYPE_J3  -CHESSTYPE_3*1 -CHESSTYPE_J3*2 +CHESSTYPE_4*1 +CHESSTYPE_D4*2 (0)
//{ 16,  16 },           //CHESSTYPE_3,  -CHESSTYPE_3*2 -CHESSTYPE_J3*2 +CHESSTYPE_4*2 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
//{ 18,  18 },           //CHESSTYPE_d4, -CHESSTYPE_D4*2 +CHESSTYPE_5 (0) 优先级降低
//{ 26,  26 },           //CHESSTYPE_d4p -CHESSTYPE_D4P*1 -CHESSTYPE_D4 +CHESSTYPE_5 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
//{ 100,  30 },           //CHESSTYPE_33, -CHESSTYPE_33*1 -CHESSTYPE_3*0-2 -CHESSTYPE_J3*2-4 +CHESSTYPE_4*2-4 +CHESSTYPE_D4*2-4 (CHESSTYPE_4*2)
//{ 200,  40 },           //CHESSTYPE_43, -CHESSTYPE_43*1 -CHESSTYPE_D4*1 -CHESSTYPE_J3*2 -CHESSTYPE_3*1 +CHESSTYPE_5*1 +CHESSTYPE_4*2 (CHESSTYPE_4*2)
//{ 400,  40 },           //CHESSTYPE_44, -CHESSTYPE_44 -CHESSTYPE_D4*2 +2个CHESSTYPE_5    (CHESSTYPE_5)
//{ 200,  20 },           //CHESSTYPE_4,  -CHESSTYPE_4*1-2 -CHESSTYPE_D4*1-2 +CHESSTYPE_5*2 (CHESSTYPE_5)
//{ 1000, 50 },           //CHESSTYPE_5,
//{ -10,-10 },           //CHESSTYPE_BAN,
//};
//#define ATACK_PAYMENT 60

//策略二：表现神勇  25：11
const StaticEvaluate staticEvaluate[CHESSTYPE_COUNT] = {
{ 0,    0 },           //CHESSTYPE_0,  +CHESSTYPE_2*2 +CHESSTYPE_J2*2 (0)
{ 0,    0 },           //CHESSTYPE_dj2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
{ 2,    2 },           //CHESSTYPE_j2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
{ 6,    6 },           //CHESSTYPE_2,  -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*2 +CHESSTYPE_J3*2 (0)
{ 4,    4 },           //CHESSTYPE_d3, -CHESSTYPE_D3*2 +CHESSTYPE_D4*2 (0)
{ 12,  12 },           //CHESSTYPE_J3  -CHESSTYPE_3*1 -CHESSTYPE_J3*2 +CHESSTYPE_4*1 +CHESSTYPE_D4*2 (0)
{ 12,  12 },           //CHESSTYPE_3,  -CHESSTYPE_3*2 -CHESSTYPE_J3*2 +CHESSTYPE_4*2 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
{ 18,  18 },           //CHESSTYPE_d4, -CHESSTYPE_D4*2 +CHESSTYPE_5 (0) 优先级降低
{ 20,  20 },           //CHESSTYPE_d4p -CHESSTYPE_D4P*1 -CHESSTYPE_D4 +CHESSTYPE_5 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
{ 100, 30 },           //CHESSTYPE_33, -CHESSTYPE_33*1 -CHESSTYPE_3*0-2 -CHESSTYPE_J3*2-4 +CHESSTYPE_4*2-4 +CHESSTYPE_D4*2-4 (CHESSTYPE_4*2)
{ 200, 40 },           //CHESSTYPE_43, -CHESSTYPE_43*1 -CHESSTYPE_D4*1 -CHESSTYPE_J3*2 -CHESSTYPE_3*1 +CHESSTYPE_5*1 +CHESSTYPE_4*2 (CHESSTYPE_4*2)
{ 400, 40 },           //CHESSTYPE_44, -CHESSTYPE_44 -CHESSTYPE_D4*2 +2个CHESSTYPE_5    (CHESSTYPE_5)
{ 200, 20 },           //CHESSTYPE_4,  -CHESSTYPE_4*1-2 -CHESSTYPE_D4*1-2 +CHESSTYPE_5*2 (CHESSTYPE_5)
{ 1000,50 },           //CHESSTYPE_5,
{ -10,-10 },           //CHESSTYPE_BAN,
};
#define ATACK_PAYMENT 50

//weight是对于side方的偏向，默认100
int ChessBoard::getGlobalEvaluate(uint8_t side, int weight)
{
    //始终是以进攻方(atackside)为正
    uint8_t defendside = lastStep.state;
    uint8_t atackside = Util::otherside(defendside);

    int atack_evaluate = 0;
    int defend_evaluate = 0;

    if (global_chesstype_count[defendside][CHESSTYPE_5] > 1) defend_evaluate += 600;
    else if(global_chesstype_count[defendside][CHESSTYPE_4] > 2) defend_evaluate += 200;
    else if(global_chesstype_count[defendside][CHESSTYPE_44] > 1) defend_evaluate += 200;
    else if(global_chesstype_count[defendside][CHESSTYPE_43] > 1) defend_evaluate += 80;

    //遍历所有棋子
    ForEachMove(this)
    {
        Piece& piece = pieces[pos.row][pos.col];
        if (piece.layer3[PIECE_BLACK] == CHESSTYPE_0 && piece.layer3[PIECE_WHITE] == CHESSTYPE_0)
        {
            continue;
        }
        if (piece.layer3[atackside] > CHESSTYPE_D3 && piece.layer3[atackside] < CHESSTYPE_33)
        {
            atack_evaluate += int(staticEvaluate[piece.layer3[atackside]].atack * getStaticFactor(pos, atackside));
        }
        else
        {
            atack_evaluate += staticEvaluate[piece.layer3[atackside]].atack;
        }
        
        if (piece.layer3[defendside] > CHESSTYPE_D3 && piece.layer3[defendside] < CHESSTYPE_33)
        {
            defend_evaluate += int(staticEvaluate[piece.layer3[defendside]].defend * getStaticFactor(pos, defendside));
        }
        else
        {
            defend_evaluate += staticEvaluate[piece.layer3[defendside]].defend;
        }
    }

    return side == atackside ? atack_evaluate - defend_evaluate + ATACK_PAYMENT : -(atack_evaluate - defend_evaluate + ATACK_PAYMENT);
}

double extension_factor[16] = {
    1.0,// 0000
    0.8,// 0001
    0.8,// 0010
    0.8,// 0011
    0.8,// 0100
    0.8,// 0101
    0.7,// 0110
    0.7,// 0111
    0.8,// 1000
    0.7,// 1001
    0.7,// 1010
    0.7,// 1011
    0.8,// 1100
    0.7,// 1101
    0.7,// 1110
    0.7,// 1111
};

// 主要算活三死四的扩展性，进一步精确评估活三死四的价值
double ChessBoard::getStaticFactor(Position pos, uint8_t side)
{
    Piece& piece = pieces[pos.row][pos.col];
    uint8_t layer3type = piece.layer3[side];
    if (layer3type < CHESSTYPE_J3 || layer3type > CHESSTYPE_D4P)
    {
        return 1.0;
    }

    double base_factor = 1.0;//初始值

    bool findself = false;
    double gain_factor = 1.0;
    for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
    {
        if (!findself && getLayer2(pos, side, d) == layer3type)
        {
            findself = true;
            continue;
        }
        else if (getLayer2(pos, side, d) > CHESSTYPE_DJ2)
        {
            base_factor += 0.5;
            continue;
        }

        //related factor, except base 

        // 中间四位
        uint8_t relatedBitMap = (piece.pattern[Util::otherside(side)][d] & 0b00111100) >> 2;
        if (relatedBitMap == 0)
        {
            for (int symbol = 0; symbol < 2; ++symbol)//正反
            {
                int d8 = d * 2 + symbol;
                Position temppos = pos; for (int8_t offset = 0; offset < 2; ++offset)
                {
                    temppos.displace8_unsafe(d8);
                    if (getChessType(temppos, side) > CHESSTYPE_D3)//有连通性
                    {
                        gain_factor *= 1.2;
                    }
                }
            }
        }
        else
        {
            gain_factor *= extension_factor[relatedBitMap];
        }
    }
    return base_factor * gain_factor;
}

void ChessBoard::printGlobalEvaluate(string &s)
{
    //始终是以进攻方(atackside)为正
    uint8_t defendside = lastStep.state;
    uint8_t atackside = Util::otherside(defendside);
    stringstream ss;
    int atack = 0, defend = 0;
    ss << "/" << "\t";
    for (int index = 0; index < Util::BoardSize; index++)
    {
        ss << index << "\t";
    }
    //遍历所有棋子
    ForEachMove(this)
    {
        if (pos.col == 0)
        {
            ss << "\r\n\r\n\r\n";
            ss << (int)pos.row << "\t";
        }


        int atack_evaluate = (int)(staticEvaluate[pieces[pos.row][pos.col].layer3[atackside]].atack*getStaticFactor(pos, atackside));
        atack += atack_evaluate;


        int defend_evaluate = (int)(staticEvaluate[pieces[pos.row][pos.col].layer3[defendside]].defend*getStaticFactor(pos, defendside));
        defend += defend_evaluate;

        ss << (atackside == PIECE_BLACK ? atack_evaluate : defend_evaluate) << "|" << (atackside == PIECE_WHITE ? atack_evaluate : defend_evaluate) << "\t";
    }
    else
    {
        //已有棋子的不做计算
        if (pos.col == 0)
        {
            ss << "\r\n\r\n\r\n";
            ss << (int)pos.row << "\t";
        }

        ss << 0 << "|" << 0 << "\t";
        continue;
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

void ChessBoard::updateHashPair(Position pos, uint8_t side, bool add)
{
    if (add) //添加棋子
    {
        hash.check_key ^= zcheck[pos.row][pos.col][PIECE_BLANK];//原来是空的
        hash.check_key ^= zcheck[pos.row][pos.col][side];
        hash.hash_key ^= zkey[pos.row][pos.col][PIECE_BLANK];
        hash.hash_key ^= zkey[pos.row][pos.col][side];
    }
    else //拿走棋子
    {
        hash.check_key ^= zcheck[pos.row][pos.col][side];       //原来是有子的
        hash.check_key ^= zcheck[pos.row][pos.col][PIECE_BLANK];
        hash.hash_key ^= zkey[pos.row][pos.col][side];
        hash.hash_key ^= zkey[pos.row][pos.col][PIECE_BLANK];
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