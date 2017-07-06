#include "ChessBoard.h"
#include "TrieTree.h"
#include "utils.h"
#include <random>
using namespace std;

bool ChessBoard::ban = false;
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
    Position pos = { util::getrow(index),util::getcol(index) };
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
            if (util::isalive3(pieces_layer2[index][d][side]))
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
    int8_t row = util::getrow(index);
    int8_t col = util::getcol(index);
    if (pieces_layer1[index] == PIECE_BLANK)
    {
        //ratings[side] -= chess_ratings[pieces_layer3[index][side]];
        updatePoint_layer3(index, side);
        //totalRatings[side] += chesstypes[pieces_layer3[index][side]].rating;
    }
    else
    {
        //totalRatings[side] -= chesstypes[pieces_layer3[index][side]].rating;
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
                //if (pieces_hot[util::xy2index(r, c)] || pieces_layer1[index] == PIECE_BLANK)//unmove的时候无视hot flag
                {
                    //totalRatings[side] -= chesstypes[pieces_layer3[util::xy2index(r, c)][side]].rating;
                    updatePoint_layer3(util::xy2index(r, c), side);
                    //totalRatings[side] += chesstypes[pieces_layer3[util::xy2index(r, c)][side]].rating;
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
    int8_t row = util::getrow(index);
    int8_t col = util::getcol(index);
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
                result += chesstypes[pieces_layer3[util::xy2index(r, c)][side]].rating;

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

bool ChessBoard::moveTemporary(uint8_t index)
{
    if (pieces_layer1[index] != PIECE_BLANK || lastStep.step > 224)
    {
        return false;//已有棋子
    }
    lastStep.step++;
    lastStep.black = !lastStep.black;
    lastStep.index = index;
    lastStep.chessType = getChessType(index, lastStep.getColor());
    pieces_layer1[index] = lastStep.getColor();
    //updateHotArea(index);
    update_layer2(index);
    updateArea_layer3(index);//and update highest ratings

    update_info_flag[0] = false;
    update_info_flag[1] = false;
    return true;
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
    lastStep.chessType = getChessType(index, lastStep.getColor());

    pieces_layer1[index] = lastStep.getColor();
    update_layer2(index);
    updateArea_layer3(index);//and update highest ratings
    updateHashPair(util::getrow(index), util::getcol(index), lastStep.getColor());

    update_info_flag[0] = false;
    update_info_flag[1] = false;
    return true;
}

bool ChessBoard::unmove(uint8_t index, ChessStep last)
{
    uint8_t side = pieces_layer1[index];
    if (side == PIECE_BLANK || lastStep.step < 1)
    {
        return false;//没有棋子
    }
    lastStep = last;

    pieces_layer1[index] = PIECE_BLANK;
    update_layer2(index);
    updateArea_layer3(index);
    updateHashPair(util::getrow(index), util::getcol(index), side, false);

    update_info_flag[0] = false;
    update_info_flag[1] = false;
    return true;
}

void ChessBoard::initChessInfo(uint8_t side)
{
    highestRatings[side] = { 0,0 };
    totalRatings[side] = 0;
    for (uint8_t index = 0; util::valid(index); ++index)
    {
        if (pieces_layer1[index] == PIECE_BLANK)
        {
            totalRatings[side] += chesstypes[pieces_layer3[index][side]].rating;
            if (chesstypes[pieces_layer3[index][side]].rating > chesstypes[highestRatings[side].chesstype].rating)
            {
                highestRatings[side].chesstype = pieces_layer3[index][side];
                highestRatings[side].index = index;
            }
        }
    }
    update_info_flag[side] = true;
}

int ChessBoard::getSituationRating(uint8_t side)//局面评估,不好评
{
    if (!update_info_flag[side])
    {
        initChessInfo(side);
    }
    if (!update_info_flag[util::otherside(side)])
    {
        initChessInfo(util::otherside(side));
    }
    return (side == lastStep.getColor()) ? totalRatings[side] / 2 - totalRatings[util::otherside(side)] :
        totalRatings[side] - totalRatings[util::otherside(side)] / 2;
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


bool ChessBoard::inRelatedArea(uint8_t index, uint8_t lastindex)
{
    return false;
}

double ChessBoard::getStaticFactor(uint8_t index, uint8_t side)
{
    //只考虑自身的静态因子
    if (pieces_layer3[index][side] > CHESSTYPE_D4P)
    {
        return 1.0;
    }

    double factor = 0.0;//初始值
    Position pos(index);
    Position temppos;
    uint8_t tempindex;
    for (int d = 0; d < DIRECTION4_COUNT; ++d)
    {
        //base factor
        if (pieces_layer2[index][d][side] == pieces_layer3[index][side])
        {
            factor += 1.0;
            continue;//过滤自身那条线
        }
        else if (pieces_layer2[index][d][side] > CHESSTYPE_0)
        {
            factor += 0.5;
        }

        //related factor, except base 
        double related_factor = 0.0;
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
        {
            double branch_factor = 0.0;
            for (int8_t offset = 1; offset < 4; ++offset)
            {
                temppos = pos.getNextPosition(d, offset*symbol);

                if (!temppos.valid() || pieces_layer1[temppos.toIndex()] == util::otherside(side))//equal otherside
                {
                    if (offset == 1)//整条线都不能接受了
                    {
                        branch_factor = -1.0;
                    }
                    else if (offset == 2)//本侧不能接受了，另一边还可以接受
                    {
                        branch_factor = 0.0;
                    }
                    else //offset == 3 本侧还可以接受一点点
                    {
                        branch_factor = branch_factor / 2;
                    }
                    break;
                }
                else
                {
                    tempindex = temppos.toIndex();
                    if (pieces_layer1[tempindex] == side)
                    {
                        continue;
                    }
                    else if (pieces_layer1[tempindex] == PIECE_BLANK)
                    {
                        if (pieces_layer3[tempindex][side] > CHESSTYPE_0)
                        {
                            //except self
                            if (pieces_layer3[tempindex][side] != pieces_layer2[tempindex][d][side])
                            {
                                if (pieces_layer3[tempindex][side] < CHESSTYPE_J3)
                                {
                                    branch_factor += 0.2;
                                }
                                else
                                {
                                    branch_factor += 0.5;
                                }
                            }
                        }
                    }
                }
            }
            if (branch_factor < 0)
            {
                related_factor = 0.0;
                break;
            }
            related_factor += branch_factor;
        }

        factor += related_factor;

    }
    return factor;
}

double ChessBoard::getRelatedFactor(uint8_t index, uint8_t side, bool defend)
{
    //不仅考虑自身的相关因子，还考虑对手的
    if (pieces_layer3[index][side] > CHESSTYPE_33)
    {
        return 1.0;
    }

    double factor = 0.0;//初始值
    Position pos(index);
    Position temppos;
    uint8_t tempindex;
    for (int d = 0; d < DIRECTION4_COUNT; ++d)
    {
        //base factor
        if (pieces_layer2[index][d][side] == pieces_layer3[index][side])
        {
            factor += 1.0;
            continue;//过滤自身那条线
        }
        else if (pieces_layer2[index][d][side] > CHESSTYPE_0)
        {
            factor += 0.5;
        }

        //related factor, except base 
        double related_factor = 0.0;
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
        {
            double branch_factor = 0.0;
            for (int8_t offset = 1; offset < 4; ++offset)
            {
                temppos = pos.getNextPosition(d, offset*symbol);

                if (!temppos.valid() || pieces_layer1[temppos.toIndex()] == util::otherside(side))//equal otherside
                {
                    if (offset == 1)//整条线都不能接受了
                    {
                        branch_factor = -1.0;
                    }
                    else if (offset == 2)//本侧不能接受了，另一边还可以接受
                    {
                        branch_factor = 0.0;
                    }
                    else //offset == 3 本侧还可以接受一点点
                    {
                        branch_factor = branch_factor / 2;
                    }
                    break;
                }
                else
                {
                    tempindex = temppos.toIndex();
                    if (pieces_layer1[tempindex] == side)
                    {
                        continue;
                    }
                    else if (pieces_layer1[tempindex] == PIECE_BLANK)
                    {
                        if (pieces_layer3[tempindex][side] > CHESSTYPE_0)
                        {
                            //except self
                            if (pieces_layer3[tempindex][side] != pieces_layer2[tempindex][d][side])
                            {
                                if (pieces_layer3[tempindex][side] < CHESSTYPE_J3)
                                {
                                    branch_factor += 0.2;
                                }
                                else
                                {
                                    branch_factor += 0.5;
                                }
                            }
                        }
                    }
                }
            }
            if (branch_factor < 0)
            {
                related_factor = 0.0;
                break;
            }
            related_factor += branch_factor;
        }

        factor += related_factor;

    }
    if (defend)
    {
        uint8_t other=pieces_layer3[index][util::otherside(side)];
        if (other> CHESSTYPE_J3)
        {
            if (other < CHESSTYPE_33)
            {
                factor += 0.5;
            }
            else
            {
                factor += 1;
            }
        }
    }
    return factor;
}

int ChessBoard::getGlobalEvaluate(uint8_t side)
{
    //始终是以进攻方(atackside)为正
    uint8_t defendside = lastStep.getColor();
    uint8_t atackside = util::otherside(defendside);

    int evaluate = 0;
    //遍历所有棋子
    for (uint8_t index = 0; index < BOARD_INDEX_BOUND; index++)
    {
        //已有棋子的不做计算
        if (!canMove(index))
        {
            continue;
        }
        if (pieces_layer3[index][atackside] != CHESSTYPE_BAN && pieces_layer3[index][atackside] > CHESSTYPE_2)
        {
            if (pieces_layer3[index][atackside] < CHESSTYPE_33)
            {
                evaluate += (int)(chesstypes[pieces_layer3[index][atackside]].atackFactor*getStaticFactor(index, atackside));
            }
            else
            {
                evaluate += chesstypes[pieces_layer3[index][atackside]].atackFactor;
            }
        }
        if (pieces_layer3[index][defendside] != CHESSTYPE_BAN && pieces_layer3[index][defendside] > CHESSTYPE_2)
        {
            if (pieces_layer3[index][defendside] < CHESSTYPE_33)
            {
                evaluate -= (int)(chesstypes[pieces_layer3[index][defendside]].defendFactor*getStaticFactor(index, defendside));
            }
            else
            {
                evaluate -= chesstypes[pieces_layer3[index][defendside]].defendFactor;
            }
        }
    }

    return side == atackside ? evaluate : -evaluate;
}

bool ChessBoard::nextPosition(int& row, int& col, int8_t offset, uint8_t direction)
{
    switch (direction)
    {
    case DIRECTION8_L:
        col -= offset;
        if (col < 0) return false;
        break;
    case DIRECTION8_R:
        col += offset;
        if (col > 14) return false;
        break;
    case DIRECTION8_U:
        row -= offset;
        if (row < 0) return false;
        break;
    case DIRECTION8_D:
        row += offset;
        if (row > 14) return false;
        break;
    case DIRECTION8_LU:
        row -= offset; col -= offset;
        if (row < 0 || col < 0) return false;
        break;
    case DIRECTION8_RD:
        col += offset; row += offset;
        if (row > 14 || col > 14) return false;
        break;
    case DIRECTION8_LD:
        col -= offset; row += offset;
        if (row > 14 || col < 0) return false;
        break;
    case DIRECTION8_RU:
        col += offset; row -= offset;
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
        hash.z32key ^= z32[util::getrow(index)][util::getcol(index)][pieces_layer1[index]];
        hash.z64key ^= z64[util::getrow(index)][util::getcol(index)][pieces_layer1[index]];
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
