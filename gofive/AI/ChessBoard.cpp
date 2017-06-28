#include "ChessBoard.h"
#include "TrieTree.h"
#include <random>
using namespace std;

bool ChessBoard::ban = false;
int8_t ChessBoard::level = AILEVEL_UNLIMITED;
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

void ChessBoard::update_layer2(uint8_t index, int side)//���Ӵ�
{
    Position pos = { util::getrow(index),util::getcol(index) };
    Position temp;
    for (int d = 0; d < DIRECTION4_COUNT; ++d)
    {
        uint16_t l_hash_index = 0, r_hash_index = 0;
        int l_offset = 0, r_offset = 0;
        while (1)//����
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
        while (1)//����
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

void ChessBoard::updatePoint_layer3(uint8_t index, int side)//�հ״�
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
        else if (pieces_layer2[index][d][side] == CHESSTYPE_5)//5�����
        {
            pieces_layer3[index][side] = CHESSTYPE_5;
            return;
        }
        else if (pieces_layer2[index][d][side] == CHESSTYPE_BAN)
        {
            pieces_layer3[index][side] = CHESSTYPE_BAN;
            return;
        }
        else if (pieces_layer2[index][d][side] == CHESSTYPE_44)//˫���൱�ڻ��ģ��ǽ�������
        {
            pieces_layer3[index][side] = CHESSTYPE_44;
            return;
        }
    }
    pieces_layer3[index][side] = result;
    //�������
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

void ChessBoard::updateArea_layer3(uint8_t index, uint8_t side)//���Ӵ�
{
    int blankCount, chessCount, r, c;
    int8_t row = util::getrow(index);
    int8_t col = util::getcol(index);
    if (pieces_layer1[index] == PIECE_BLANK)
    {
        //ratings[side] -= chess_ratings[pieces_layer3[index][side]];
        updatePoint_layer3(index, side);
        totalRatings[side] += chesstypes[pieces_layer3[index][side]].rating;
    }
    else
    {
        totalRatings[side] -= chesstypes[pieces_layer3[index][side]].rating;
    }

    for (int i = 0; i < DIRECTION8_COUNT; ++i)//8������
    {
        r = row, c = col;
        blankCount = 0;
        chessCount = 0;
        while (nextPosition(r, c, 1, i)) //����������߽�
        {
            if (pieces_layer1[util::xy2index(r, c)] == PIECE_BLANK)
            {
                blankCount++;
                if (pieces_hot[util::xy2index(r, c)] || pieces_layer1[index] == PIECE_BLANK)//unmove��ʱ������hot flag
                {
                    totalRatings[side] -= chesstypes[pieces_layer3[util::xy2index(r, c)][side]].rating;
                    updatePoint_layer3(util::xy2index(r, c), side);
                    totalRatings[side] += chesstypes[pieces_layer3[util::xy2index(r, c)][side]].rating;
                }
            }
            else if (pieces_layer1[util::xy2index(r, c)] == util::otherside(side))
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

int ChessBoard::getUpdateThreat(uint8_t index, uint8_t side)
{
    int result = 0;
    int blankCount, chessCount, r, c;
    int8_t row = util::getrow(index);
    int8_t col = util::getcol(index);
    for (int i = 0; i < DIRECTION8_COUNT; ++i)//8������
    {
        r = row, c = col;
        blankCount = 0;
        chessCount = 0;
        while (nextPosition(r, c, 1, i)) //����������߽�
        {
            if (pieces_layer1[util::xy2index(r, c)] == PIECE_BLANK)
            {
                blankCount++;
                if (pieces_hot[util::xy2index(r, c)])
                {
                    result += chesstypes[pieces_layer3[util::xy2index(r, c)][side]].rating;
                }
            }
            else if (pieces_layer1[util::xy2index(r, c)] == util::otherside(side))
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
    return result;
}

void ChessBoard::updateHotArea(uint8_t index)
{
    int row = util::getrow(index);
    int col = util::getcol(index);
    int range = 5;//2 * 2 + 1
    int tempcol, temprow;
    for (int i = 0; i < range; ++i)
    {
        //����
        tempcol = col - 2 + i;
        if (tempcol > -1 && tempcol < BOARD_COL_MAX && pieces_layer1[util::xy2index(row, tempcol)] == PIECE_BLANK)
        {
            setHot(row, tempcol, true);
        }
        //����
        temprow = row - 2 + i;
        if (temprow > -1 && temprow < BOARD_ROW_MAX && pieces_layer1[util::xy2index(temprow, col)] == PIECE_BLANK)
        {
            setHot(temprow, col, true);
        }
        //����
        if (temprow > -1 && temprow < BOARD_ROW_MAX && tempcol> -1 && tempcol < BOARD_COL_MAX &&pieces_layer1[util::xy2index(temprow, tempcol)] == PIECE_BLANK)
        {
            setHot(temprow, tempcol, true);
        }
        //����
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

bool ChessBoard::moveTemporary(uint8_t index)
{
    if (pieces_layer1[index] != PIECE_BLANK || lastStep.step > 224)
    {
        return false;//��������
    }
    lastStep.step++;
    lastStep.black = !lastStep.black;
    lastStep.index = index;
    lastStep.chessType = getChessType(index, lastStep.getColor());
    pieces_layer1[index] = lastStep.getColor();
    updateHotArea(index);
    update_layer2(index);
    updateArea_layer3(index);//and update highest ratings
    return true;
}

bool ChessBoard::move(uint8_t index)
{
    if (pieces_layer1[index] != PIECE_BLANK || lastStep.step > 224)
    {
        return false;//��������
    }
    lastStep.step++;
    lastStep.black = !lastStep.black;
    lastStep.index = index;
    lastStep.chessType = getChessType(index, lastStep.getColor());

    pieces_layer1[index] = lastStep.getColor();
    updateHotArea(index);
    update_layer2(index);
    updateArea_layer3(index);//and update highest ratings
    initHighestRatings();
    updateHashPair(util::getrow(index), util::getcol(index), lastStep.getColor());

    return true;
}

bool ChessBoard::unmove(uint8_t index, ChessStep last)
{
    uint8_t side = pieces_layer1[index];
    if (side == PIECE_BLANK || lastStep.step < 1)
    {
        return false;//û������
    }
    lastStep = last;

    pieces_layer1[index] = PIECE_BLANK;
    initHotArea();
    update_layer2(index);
    updateArea_layer3(index);
    initHighestRatings();
    updateHashPair(util::getrow(index), util::getcol(index), side, false);
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
            totalRatings[PIECE_BLACK] += chesstypes[pieces_layer3[index][PIECE_BLACK]].rating;
            totalRatings[PIECE_WHITE] += chesstypes[pieces_layer3[index][PIECE_WHITE]].rating;
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
                if (chesstypes[pieces_layer3[index][side]].rating > chesstypes[highestRatings[side].chessmode].rating)
                {
                    highestRatings[side].chessmode = pieces_layer3[index][side];
                    highestRatings[side].index = index;
                }
            }
        }
    }
}

void ChessBoard::formatChess2Int(uint32_t chessInt[DIRECTION4_COUNT], int row, int col, int side)
{
    //chessInt��Ҫ��ʼ��Ϊ0
    int rowstart = row - SEARCH_LENGTH, colstart = col - SEARCH_LENGTH, rowend = row + SEARCH_LENGTH;

    for (int i = 0; i < FORMAT_LENGTH; ++i, ++rowstart, ++colstart, --rowend)
    {
        //����
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

        //����
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
        //������
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

        //������
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

double ChessBoard::getRelatedFactor(uint8_t index, uint8_t side)
{
    double factor = 1.0;//��ʼֵ
    Position pos(index);
    Position temppos;
    uint8_t tempindex;
    for (int d = 0; d < DIRECTION4_COUNT; ++d)
    {
        if (pieces_layer2[index][d][side] == pieces_layer3[index][side]) continue;//��������������

        for (int8_t offset = -3; offset < 4; ++offset)
        {
            if (offset == 0) continue;//����
            temppos = pos.getNextPosition(d, offset);
            if (!temppos.valid()) continue;
            tempindex = temppos.toIndex();
            
            if (pieces_layer1[tempindex] == side)
            {
                continue;
            }
            else if (pieces_layer1[tempindex] == PIECE_BLANK)
            {
                if (pieces_layer3[tempindex][side] > CHESSTYPE_0)
                {
                    if (pieces_layer3[tempindex][side] < CHESSTYPE_J3)
                    {
                        if (pieces_layer3[tempindex][side] == pieces_layer2[tempindex][d][side])
                        {
                            factor += 0.5;
                        }
                    }
                    else
                    {
                        factor += 1.0;
                    }
                }
            }
            else//otherside
            {
                if (offset < 0) factor = 1.0;//�������ӣ�֮ǰ��ʧЧ�����¼���
                else break;
            }
        }
    }
    return factor;
}

int ChessBoard::getGlobalEvaluate(uint8_t side)
{
    uint8_t atackside = util::otherside(lastStep.getColor());
    uint8_t defendside = lastStep.getColor();
    int ret = 0;
    double factor;
    //������������
    for (uint8_t index = 0; index < BOARD_INDEX_BOUND; index++)
    {
        //�������ӵĲ�������
        if (!canMove(index))
        {
            continue;
        }
        if (pieces_layer3[index][atackside] != CHESSTYPE_BAN)
        {
            if (pieces_layer3[index][atackside] < CHESSTYPE_33 && pieces_layer3[index][atackside] > CHESSTYPE_0)
            {
                factor = getRelatedFactor(index, atackside);
                for (int d = 0; d < DIRECTION4_COUNT; ++d)
                {
                    ret += (int)(chesstypes[pieces_layer2[index][d][atackside]].atackFactor*factor);
                }
            }
            else
            {
                ret += chesstypes[pieces_layer3[index][atackside]].atackFactor;
            }
        }
        if (pieces_layer3[index][defendside] != CHESSTYPE_BAN)
        {
            if (pieces_layer3[index][defendside] < CHESSTYPE_33 && pieces_layer3[index][defendside] > CHESSTYPE_0)
            {
                factor = getRelatedFactor(index, defendside);
                for (int d = 0; d < DIRECTION4_COUNT; ++d)
                {
                    ret -= (int)(chesstypes[pieces_layer2[index][d][defendside]].defendFactor*factor);
                }
            }
            else
            {
                ret -= chesstypes[pieces_layer3[index][defendside]].defendFactor;
            }

        }
    }
    return ret;
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
    if (add) //�������
    {
        hash.z32key ^= z32[row][col][PIECE_BLANK];//ԭ���ǿյ�
        hash.z32key ^= z32[row][col][side];
        hash.z64key ^= z64[row][col][PIECE_BLANK];
        hash.z64key ^= z64[row][col][side];
    }
    else //��������
    {
        hash.z32key ^= z32[row][col][side];       //ԭ�������ӵ�
        hash.z32key ^= z32[row][col][PIECE_BLANK];
        hash.z64key ^= z64[row][col][side];
        hash.z64key ^= z64[row][col][PIECE_BLANK];
    }
}
