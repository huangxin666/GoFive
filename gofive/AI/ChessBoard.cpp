#include "ChessBoard.h"
#include "TrieTree.h"
#include <random>
using namespace std;


int8_t Util::BoardSize = 15;
int Util::BoardIndexUpper = 15 * 15;

bool ChessBoard::ban = false;
string ChessBoard::debugInfo = "";
uint32_t ChessBoard::z32[BOARD_ROW_MAX*BOARD_COL_MAX][3] = { 0 };
uint64_t ChessBoard::z64[BOARD_ROW_MAX*BOARD_COL_MAX][3] = { 0 };
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
    default_random_engine e(407618);//fixed seed
    uniform_int_distribution<uint64_t> rd64;
    uniform_int_distribution<uint32_t> rd32;

    for (csidx index = 0; index < BOARD_ROW_MAX*BOARD_COL_MAX; ++index)
    {
        for (int k = 0; k < 3; ++k)
        {
            z32[index][k] = rd32(e);
            z64[index][k] = rd64(e);
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
    for (csidx i = 0; i < BOARD_ROW_MAX*BOARD_COL_MAX; ++i)
    {
        pieces_layer1[i] = PIECE_BLANK;
    }
}

void ChessBoard::init_layer2()
{
    for (csidx i = 0; i < BOARD_ROW_MAX*BOARD_COL_MAX; ++i)
    {
        if (pieces_layer1[i] == PIECE_BLANK)
        {
            continue;
        }
        update_layer2(i, PIECE_BLACK);
        update_layer2(i, PIECE_WHITE);
    }
}

void ChessBoard::update_layer2(csidx index, int side)//落子处
{
    Position pos(index);
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
            if (pieces_layer1[temp.toIndex()] == Util::otherside(side))
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

            if (pieces_layer1[temp.toIndex()] == Util::otherside(side))
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

        if (pieces_layer1[index] == Util::otherside(side))
        {
            int len = l_offset;
            int index_offset = l_hash_index * len;
            uint8_t index_fix = index - Util::get_index_offset(d) * l_offset;
            //update
            for (int i = 0; i < len; ++i, index_fix += Util::get_index_offset(d))
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
            index_fix = index + Util::get_index_offset(d);
            //update
            for (int i = 0; i < len; ++i, index_fix += Util::get_index_offset(d))
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
            int index_fix = index - Util::get_index_offset(d) * l_offset;
            //update
            for (int i = 0; i < len; ++i, index_fix += Util::get_index_offset(d))
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
    for (uint8_t index = 0; Util::valid(index); ++index)
    {
        updatePoint_layer3(index, PIECE_BLACK);
        updatePoint_layer3(index, PIECE_WHITE);
    }
}

void ChessBoard::updatePoint_layer3(csidx index, int side)//空白处
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
            if (Util::isalive3(pieces_layer2[index][d][side]))
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

void ChessBoard::updateArea_layer3(csidx index, uint8_t side)//落子处
{
    int blankCount, chessCount, r, c;
    uint8_t tempindex;
    Position movepos(index);
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
        r = movepos.row, c = movepos.col;
        blankCount = 0;
        chessCount = 0;
        while (Util::displace(r, c, 1, i)) //如果不超出边界
        {
            tempindex = Util::xy2index(r, c);
            if (pieces_layer1[tempindex] == PIECE_BLANK)
            {
                blankCount++;
                //if (pieces_hot[Util::xy2index(r, c)] || pieces_layer1[index] == PIECE_BLANK)//unmove的时候无视hot flag
                {
                    //totalRatings[side] -= chesstypes[pieces_layer3[Util::xy2index(r, c)][side]].rating;
                    updatePoint_layer3(tempindex, side);
                    //totalRatings[side] += chesstypes[pieces_layer3[Util::xy2index(r, c)][side]].rating;
                }
            }
            else if (pieces_layer1[tempindex] == Util::otherside(side))
            {
                break;//遇到敌方棋子，停止更新
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

bool ChessBoard::moveNull()
{
    lastStep.black = !lastStep.black;
    return true;
}

bool ChessBoard::move(int8_t row, int8_t col, uint8_t side)
{
    csidx index = Util::xy2index(row, col);
    if (pieces_layer1[index] != PIECE_BLANK || lastStep.step > 224)
    {
        return false;//已有棋子
    }
    lastStep.step++;
    lastStep.setColor(side);
    lastStep.index = index;
    lastStep.chessType = getChessType(index, side);

    pieces_layer1[index] = side;
    update_layer2(index);
    updateArea_layer3(index);
    updateHashPair(index, side, true);
    update_info_flag[0] = false;
    update_info_flag[1] = false;
    return true;
}

bool ChessBoard::unmove(csidx index, ChessStep last)
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
    updateHashPair(index, side, false);

    update_info_flag[0] = false;
    update_info_flag[1] = false;
    return true;
}

void ChessBoard::initChessInfo(uint8_t side)
{
    highestRatings[side] = { 0,CHESSTYPE_BAN };
    totalRatings[side] = -10000;
    for (uint8_t index = 0; Util::valid(index); ++index)
    {
        if (pieces_layer1[index] == PIECE_BLANK)
        {
            totalRatings[side] += getChessTypeInfo(pieces_layer3[index][side]).rating;
            if (getChessTypeInfo(pieces_layer3[index][side]).rating > getChessTypeInfo(highestRatings[side].chesstype).rating)
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
    if (!update_info_flag[Util::otherside(side)])
    {
        initChessInfo(Util::otherside(side));
    }
    return (side == lastStep.getSide()) ? totalRatings[side] / 2 - totalRatings[Util::otherside(side)] :
        totalRatings[side] - totalRatings[Util::otherside(side)] / 2;
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
            chessInt[DIRECTION4_LR] |= PIECE_WHITE << i * 2;
        }
        else
        {
            if (pieces_layer1[Util::xy2index(row, colstart)] == PIECE_BLANK)
            {
                chessInt[DIRECTION4_LR] |= (PIECE_BLANK) << i * 2;
            }
            else
            {
                chessInt[DIRECTION4_LR] |= (pieces_layer1[Util::xy2index(row, colstart)] == side ? PIECE_BLACK : PIECE_WHITE) << i * 2;
            }

        }

        //纵向
        if (rowstart < 0 || rowstart > 14)
        {
            chessInt[DIRECTION4_UD] |= PIECE_WHITE << i * 2;
        }
        else
        {
            if (pieces_layer1[Util::xy2index(rowstart, col)] == PIECE_BLANK)
            {
                chessInt[DIRECTION4_UD] |= (PIECE_BLANK) << i * 2;
            }
            else
            {
                chessInt[DIRECTION4_UD] |= (pieces_layer1[Util::xy2index(rowstart, col)] == side ? 0 : 1) << i * 2;
            }
        }
        //右下向
        if (colstart < 0 || rowstart < 0 || colstart > 14 || rowstart > 14)
        {
            chessInt[DIRECTION4_RD] |= PIECE_WHITE << i * 2;
        }
        else
        {
            if (pieces_layer1[Util::xy2index(rowstart, colstart)] == PIECE_BLANK)
            {
                chessInt[DIRECTION4_RD] |= (PIECE_BLANK) << i * 2;
            }
            else
            {
                chessInt[DIRECTION4_RD] |= (pieces_layer1[Util::xy2index(rowstart, colstart)] == side ? 0 : 1) << i * 2;
            }

        }

        //右上向
        if (colstart < 0 || rowend > 14 || colstart > 14 || rowend < 0)
        {
            chessInt[DIRECTION4_RU] |= PIECE_WHITE << i * 2;
        }
        else
        {
            if (pieces_layer1[Util::xy2index(rowend, colstart)] == PIECE_BLANK)
            {
                chessInt[DIRECTION4_RU] |= (PIECE_BLANK) << i * 2;
            }
            else
            {
                chessInt[DIRECTION4_RU] |= (pieces_layer1[Util::xy2index(rowend, colstart)] == side ? 0 : 1) << i * 2;
            }
        }
    }
}

void ChessBoard::getAtackReletedPos(set<csidx>& releted, csidx center, uint8_t side)
{
    Position pos(center);
    Position temppos;
    uint8_t tempindex;
    for (int d = 0; d < DIRECTION4_COUNT; ++d)
    {
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
        {
            int blankcount = 0;
            for (int8_t offset = 1; offset < 6; ++offset)
            {
                temppos = pos.getNextPosition(d, offset*symbol);
                tempindex = temppos.toIndex();
                if (!temppos.valid() || pieces_layer1[tempindex] == Util::otherside(side))//equal otherside
                {
                    break;
                }

                if (pieces_layer1[tempindex] == side)
                {
                    continue;
                }
                else if (pieces_layer1[tempindex] == PIECE_BLANK)
                {
                    blankcount++;
                    releted.insert(tempindex);
                    getAtackReletedPos2(releted, tempindex, side);
                }
                if (blankcount == 2)
                {
                    break;
                }
            }
        }
    }

    getBanReletedPos(releted, lastStep.index, Util::otherside(side));
}


void ChessBoard::getAtackReletedPos2(set<csidx>& releted, csidx center, uint8_t side)
{
    Position pos(center);
    Position temppos;
    uint8_t tempindex;
    for (int d = 0; d < DIRECTION4_COUNT; ++d)
    {
        if (pieces_layer2[center][d][side] == CHESSTYPE_0)
        {
            continue;
        }
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
        {
            int blankcount = 0;
            for (int8_t offset = 1; offset < 5; ++offset)
            {
                temppos = pos.getNextPosition(d, offset*symbol);
                tempindex = temppos.toIndex();
                if (!temppos.valid() || pieces_layer1[tempindex] == Util::otherside(side))//equal otherside
                {
                    break;
                }

                if (pieces_layer1[tempindex] == side)
                {
                    continue;
                }
                else if (pieces_layer1[tempindex] == PIECE_BLANK)
                {
                    blankcount++;
                    if (pieces_layer2[tempindex][d][side] >= CHESSTYPE_J3)
                    {
                        releted.insert(tempindex);
                    }
                    else if (pieces_layer2[tempindex][d][side] > CHESSTYPE_0)
                    {
                        if (pieces_layer3[tempindex][side] > CHESSTYPE_J3)
                        {
                            releted.insert(tempindex);
                        }
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

void ChessBoard::getBanReletedPos(set<csidx>& releted, csidx center, uint8_t side)
{
    //找出是否有禁手点
    Position pos(center);
    Position temppos;
    uint8_t tempindex;
    vector<uint8_t> banset;
    for (int d = 0; d < DIRECTION4_COUNT; ++d)
    {
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
        {
            for (int8_t offset = 1; offset < 5; ++offset)
            {
                temppos = pos.getNextPosition(d, offset*symbol);
                tempindex = temppos.toIndex();
                if (!temppos.valid() || pieces_layer1[tempindex] == Util::otherside(side))//equal otherside
                {
                    break;
                }

                if (pieces_layer1[tempindex] == side)
                {
                    continue;
                }
                else if (pieces_layer1[tempindex] == PIECE_BLANK)
                {
                    if (pieces_layer3[tempindex][side] == CHESSTYPE_BAN)
                    {
                        banset.emplace_back(tempindex);
                    }
                }
            }
        }
    }

    for (auto banpos : banset)
    {
        Position pos(banpos);
        Position temppos;
        uint8_t tempindex;
        for (int d = 0; d < DIRECTION4_COUNT; ++d)
        {
            for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
            {
                for (int8_t offset = 1; offset < 5; ++offset)
                {
                    temppos = pos.getNextPosition(d, offset*symbol);
                    tempindex = temppos.toIndex();
                    if (!temppos.valid() || pieces_layer1[tempindex] == side)//equal otherside
                    {
                        break;
                    }

                    if (pieces_layer1[tempindex] == PIECE_BLANK)
                    {
                        if (pieces_layer3[tempindex][Util::otherside(side)] >= CHESSTYPE_J3)
                        {
                            releted.insert(tempindex);
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


//double ChessBoard::getRelatedFactor(csidx index, uint8_t side, bool defend)
//{
//    //进行情景细分，主要聚焦于对局势的影响趋势，而非局势本身
//    //所以就算本身为CHESSTYPE_0，也有可能对局势有很大影响
//
//    double base_factor = 1.0;//初始值
//
//    Position pos(index);
//    Position temppos;
//    uint8_t tempindex;
//
//    for (int d = 0; d < DIRECTION4_COUNT; ++d)
//    {
//        if (pieces_layer2[index][d][side] == pieces_layer3[index][side])
//        {
//            if (pieces_layer3[index][side] < CHESSTYPE_J3)
//            {
//                //do nothing
//            }
//            else
//            {
//                continue;//过滤自身那条线
//            }
//        }
//
//        //related factor, except base 
//        double related_factor = 1.0;
//        int contiune = 0;
//        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
//        {
//            int blank = 0;
//            for (int8_t offset = 1; offset < 5; ++offset)
//            {
//                temppos = pos.getNextPosition(d, offset*symbol);
//                tempindex = temppos.toIndex();
//                if (!temppos.valid() || pieces_layer1[tempindex] == Util::otherside(side))//equal otherside
//                {
//                    break;
//                }
//                else if (pieces_layer1[tempindex] == side)
//                {
//                    continue;
//                }
//                else//blank
//                {
//                    blank++;
//                    if (pieces_layer2[tempindex][d][side] > (offset == 1 ? CHESSTYPE_0 : CHESSTYPE_J2))
//                    {
//                        if (pieces_layer2[tempindex][d][side] < CHESSTYPE_J3)
//                        {
//                            if (pieces_layer3[tempindex][side] >= CHESSTYPE_J3)
//                            {
//                                contiune++;
//                                related_factor = 1.5 > related_factor ? 1.5 : related_factor;
//                            }
//                            else
//                            {
//                                related_factor = 1.25 > related_factor ? 1.25 : related_factor;
//                                for (int dd = 0; dd < DIRECTION4_COUNT; ++dd)
//                                {
//                                    if (dd == d) continue;
//                                    if (pieces_layer2[tempindex][dd][side] > CHESSTYPE_0)
//                                    {
//                                        related_factor = 1.35 > related_factor ? 1.35 : related_factor;
//                                    }
//                                }
//                            }
//                        }
//                        else//已经是四杀了
//                        {
//                            related_factor = 1.5 > related_factor ? 1.5 : related_factor;
//                        }
//                    }
//                    else
//                    {
//                        if (pieces_layer3[tempindex][side] >= CHESSTYPE_J3)
//                        {
//                            contiune++;
//                        }
//                    }
//                }
//                if (defend)
//                {
//                    if (blank == 1)break;
//                }
//                else if (blank == 2)
//                {
//                    break;
//                }
//
//            }
//        }
//        if (contiune >= 2)
//        {
//            related_factor = 2.0;
//        }
//        //factor += related_factor;
//        base_factor *= related_factor;
//    }
//    return base_factor;
//}

//const ChessTypeInfo chesstypes[CHESSTYPE_COUNT] = {
//    { 0    , 0, 0,     0,  0 },           //CHESSTYPE_0,
//    { 10   , 0, 0,     0,  0 },           //CHESSTYPE_j2,
//    { 10   , 1, 1,     4,  2 },           //CHESSTYPE_2, 
//    { 10   , 1, 1,     6,  3 },           //CHESSTYPE_d3,
//    { 80   , 1, 1,    12,  6 },           //CHESSTYPE_J3
//    { 100  , 2, 2,    25, 12 },           //CHESSTYPE_3, 
//    { 120  , 0, 2,    20, 16 },           //CHESSTYPE_d4,
//    { 150  , 2, 2,    30, 20 },           //CHESSTYPE_d4p
//    { 250  , 8, 6,   100, 40 },           //CHESSTYPE_33,
//    { 450  ,10, 6,   200, 50 },           //CHESSTYPE_43,
//    { 500  ,12, 6,   250, 45 },           //CHESSTYPE_44,
//    { 500  ,13,10,   500, 50 },           //CHESSTYPE_4,
//    { 10000,15,15, 10000,100 },           //CHESSTYPE_5,
//    { -100 ,-9, 5,   -10, -5 },           //CHESSTYPE_BAN,
//};

const ChessTypeInfo chesstypes[CHESSTYPE_COUNT] = {
    { 0    , 0.0, 0.0,     0,  0 },           //CHESSTYPE_0,  +CHESSTYPE_2*2 +CHESSTYPE_J2*2 (0)
    { 10   , 0.5, 0.0,     0,  0 },           //CHESSTYPE_j2, -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*1 +CHESSTYPE_J3*2 (0)
    { 10   , 1.0, 1.0,     4,  1 },           //CHESSTYPE_2,  -CHESSTYPE_J2*2 -CHESSTYPE_2*2 +CHESSTYPE_3*2 +CHESSTYPE_J3*2 (0)
    { 10   , 1.0, 1.0,     6,  4 },           //CHESSTYPE_d3, -CHESSTYPE_D3*2 +CHESSTYPE_D4*2 (0)
    { 80   , 1.5, 2.0,    12,  6 },           //CHESSTYPE_J3  -CHESSTYPE_3*1 -CHESSTYPE_J3*2 +CHESSTYPE_4*1 +CHESSTYPE_D4*2 (0)
    { 100  , 2.0, 3.0,    25, 12 },           //CHESSTYPE_3,  -CHESSTYPE_3*2 -CHESSTYPE_J3*2 +CHESSTYPE_4*2 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
    { 120  , 0.0, 2.0,    20, 16 },           //CHESSTYPE_d4, -CHESSTYPE_D4*2 +CHESSTYPE_5 (0) 优先级降低
    { 150  , 2.5, 3.0,    30, 20 },           //CHESSTYPE_d4p -CHESSTYPE_D4P*1 -CHESSTYPE_D4 +CHESSTYPE_5 +CHESSTYPE_D4*2 (CHESSTYPE_D4*2)
    { 250  , 8.0, 4.0,   100, 40 },           //CHESSTYPE_33, -CHESSTYPE_33*1 -CHESSTYPE_3*0-2 -CHESSTYPE_J3*2-4 +CHESSTYPE_4*2-4 +CHESSTYPE_D4*2-4 (CHESSTYPE_4*2)
    { 450  ,10.0, 4.0,   200, 50 },           //CHESSTYPE_43, -CHESSTYPE_43*1 -CHESSTYPE_D4*1 -CHESSTYPE_J3*2 -CHESSTYPE_3*1 +CHESSTYPE_5*1 +CHESSTYPE_4*2 (CHESSTYPE_4*2)
    { 500  ,12.0, 5.0,   250, 60 },           //CHESSTYPE_44, -CHESSTYPE_44 -CHESSTYPE_D4*2 +2个CHESSTYPE_5    (CHESSTYPE_5)
    { 500  ,12.0, 6.0,   500, 70 },           //CHESSTYPE_4,  -CHESSTYPE_4*1-2 -CHESSTYPE_D4*1-2 +CHESSTYPE_5*2 (CHESSTYPE_5)
    { 10000,15.0,15.0, 10000,100 },           //CHESSTYPE_5,
    { -100 ,-9, 5,   -10, -5 },             //CHESSTYPE_BAN,
};


ChessTypeInfo ChessBoard::getChessTypeInfo(uint8_t type)
{
    return chesstypes[type];
}

//进行情景细分，主要聚焦于对局势的影响趋势，而非局势本身
//所以就算本身为CHESSTYPE_0，也有可能对局势有很大影响
double ChessBoard::getRelatedFactor(csidx index, uint8_t side, bool defend)
{
    double base_factor = 0.0;//初始值

    if (defend)
    {
        base_factor = chesstypes[pieces_layer3[index][side]].defendBaseFactor;
        if (pieces_layer3[index][side] > CHESSTYPE_D4P)
        {
            return base_factor;
        }

        for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
        {
            if (pieces_layer2[index][d][side] == pieces_layer3[index][side])
            {
                if (pieces_layer3[index][side] == CHESSTYPE_J3)//特殊处理
                {
                    Position pos(index);
                    Position temppos;
                    for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
                    {
                        temppos = pos.getNextPosition(d, symbol);
                        
                        if (!temppos.valid() || pieces_layer2[temppos.toIndex()][d][side] != CHESSTYPE_3)
                        {
                            continue;
                        }
                        temppos = pos.getNextPosition(d, 4*symbol);
                        if (temppos.valid() && pieces_layer2[temppos.toIndex()][d][side] == CHESSTYPE_3)//?!?oo??? 无用
                        {
                            base_factor = 0.5;
                        }
                    }
                }
                else
                {
                    continue;//过滤自身那条线
                }
            }
            else
            {
                if (pieces_layer2[index][d][side] > CHESSTYPE_0)
                {
                    base_factor += 1.0;
                }
            }
        }

        return base_factor;
    }

    base_factor = chesstypes[pieces_layer3[index][side]].atackBaseFactor;
    if (pieces_layer3[index][side] > CHESSTYPE_D4P)
    {
        return base_factor;
    }
    Position pos(index);
    Position temppos;
    uint8_t tempindex;
    for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
    {
        if (pieces_layer2[index][d][side] == pieces_layer3[index][side])
        {

            if (pieces_layer3[index][side] < CHESSTYPE_J3)
            {
                //do nothing
            }
            else
            {
                continue;//过滤自身那条线
            }
        }
        else
        {
            if (pieces_layer2[index][d][side] > CHESSTYPE_0)
            {
                base_factor += 1.0;
            }
        }

        //related factor, except base 
        double related_factor = 0.0;
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
        {
            int blank = 0;
            for (int8_t offset = 1; offset < 5; ++offset)
            {
                temppos = pos.getNextPosition(d, offset*symbol);
                tempindex = temppos.toIndex();
                if (!temppos.valid() || pieces_layer1[tempindex] == Util::otherside(side))//equal otherside
                {
                    break;
                }
                else if (pieces_layer1[tempindex] == side)
                {
                    continue;
                }
                else//blank
                {
                    blank++;
                    //pieces_layer2[index][d][side]一定是 < CHESSTYPE_J3
                    if (pieces_layer2[index][d][side] > CHESSTYPE_J2)//CHESSTYPE_2 || CHESSTYPE_D3
                    {
                        //o??!o

                        if (pieces_layer3[tempindex][side] > CHESSTYPE_D3)
                        {
                            if (pieces_layer2[tempindex][d][side] < CHESSTYPE_2)
                            {
                                temppos = temppos.getNextPosition(d, offset*symbol);
                                if (temppos.valid() && pieces_layer1[tempindex] != Util::otherside(side))//equal otherside
                                {
                                    related_factor += 0.5;
                                }
                            }
                            else
                            {
                                related_factor += 0.5;
                            }
                        }
                        else if (pieces_layer3[tempindex][side] > CHESSTYPE_2)
                        {
                            if (pieces_layer2[tempindex][d][side] < CHESSTYPE_2)
                            {
                                temppos = temppos.getNextPosition(d, offset*symbol);
                                if (temppos.valid() && pieces_layer1[tempindex] != Util::otherside(side))//equal otherside
                                {
                                    related_factor += 0.2;
                                }
                            }
                            else
                            {
                                related_factor += 0.2;
                            }
                        }
                    }
                    else//pieces_layer2[index][d][side] == CHESSTYPE_0 || CHESSTYPE_J2
                    {
                        if (pieces_layer2[tempindex][d][side] > CHESSTYPE_3)//yes:?oo??!  no:xooo??!
                        {
                            //do nothing
                            if (blank == 2)
                            {
                                break;
                            }
                            else
                            {
                                continue;
                            }
                        }
                        else if (pieces_layer2[tempindex][d][side] > CHESSTYPE_2)//xoo??! o?o?!形成冲四
                        {
                            related_factor += 0.25;//两个点
                        }
                        else if (pieces_layer2[tempindex][d][side] > CHESSTYPE_J2)// o??! 形成三？
                        {
                            //CHESSTYPE_2
                            related_factor += 0.25;
                        }

                        if (pieces_layer3[tempindex][side] > CHESSTYPE_D3)
                        {
                            if (pieces_layer2[tempindex][d][side] < CHESSTYPE_2)
                            {
                                temppos = temppos.getNextPosition(d, offset*symbol);
                                if (temppos.valid() && pieces_layer1[tempindex] != Util::otherside(side))//equal otherside
                                {
                                    related_factor += 0.5;
                                }
                            }
                            else
                            {
                                related_factor += 0.5;
                            }
                        }
                        else if (pieces_layer3[tempindex][side] > CHESSTYPE_J2)
                        {
                            if (pieces_layer2[tempindex][d][side] < CHESSTYPE_2)
                            {
                                temppos = temppos.getNextPosition(d, offset*symbol);
                                if (temppos.valid() && pieces_layer1[tempindex] != Util::otherside(side))//equal otherside
                                {
                                    related_factor += 0.2;
                                }
                            }
                            else
                            {
                                related_factor += 0.2;
                            }
                        }
                    }
                }
                if (blank == 2)
                {
                    break;
                }
            }
        }

        base_factor += related_factor;
    }
    return base_factor;
}

//进行情景细分，主要聚焦于局势本身
double ChessBoard::getStaticFactor(csidx index, uint8_t side, bool defend)
{
    if (pieces_layer3[index][side] >= CHESSTYPE_33)
    {
        return 1.0;
    }
    double base_factor = 1.0;//初始值
    Position pos(index);
    Position temppos;
    uint8_t tempindex;
    double related_factor = 1.0;
    for (uint8_t d = 0; d < DIRECTION4_COUNT; ++d)
    {
        if (pieces_layer2[index][d][side] == pieces_layer3[index][side])
        {

            if (pieces_layer3[index][side] < CHESSTYPE_J3)
            {
                //do nothing
            }
            else
            {
                continue;//过滤自身那条线
            }
        }
        else
        {
            if (pieces_layer2[index][d][side] > CHESSTYPE_0)
            {
                base_factor += 0.5;
            }
        }

        //related factor, except base 
        double related_factor = 0.0;
        for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
        {
            int blank = 0;
            for (int8_t offset = 1; offset < 5; ++offset)
            {
                temppos = pos.getNextPosition(d, offset*symbol);
                tempindex = temppos.toIndex();
                if (!temppos.valid() || pieces_layer1[tempindex] == Util::otherside(side))//equal otherside
                {
                    break;
                }
                else if (pieces_layer1[tempindex] == side)
                {
                    continue;
                }
                else//blank
                {
                    blank++;
                    //pieces_layer2[index][d][side]一定是 < CHESSTYPE_J3
                    if (pieces_layer2[index][d][side] > CHESSTYPE_J2)//CHESSTYPE_2 || CHESSTYPE_D3
                    {
                        //o??!o

                        if (pieces_layer3[tempindex][side] > CHESSTYPE_D3)
                        {
                            if (pieces_layer2[tempindex][d][side] < CHESSTYPE_2)
                            {
                                temppos = temppos.getNextPosition(d, offset*symbol);
                                if (temppos.valid() && pieces_layer1[tempindex] != Util::otherside(side))//equal otherside
                                {
                                    if (pieces_layer2[tempindex][d][side] == CHESSTYPE_0)
                                    {
                                        related_factor += 0.25;
                                    }
                                    else//CHESSTYPE_J2
                                    {
                                        if (pieces_layer3[index][side] > CHESSTYPE_D3)
                                        {
                                            related_factor += 2.0;
                                        }
                                        else
                                        {
                                            related_factor += 0.5;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                if (pieces_layer3[index][side] > CHESSTYPE_D3)
                                {
                                    related_factor += 2.0;
                                }
                                else
                                {
                                    related_factor += 0.5;
                                }
                            }
                        }
                        else if (pieces_layer3[tempindex][side] > CHESSTYPE_2)
                        {
                            if (pieces_layer2[tempindex][d][side] < CHESSTYPE_2)
                            {
                                temppos = temppos.getNextPosition(d, offset*symbol);
                                if (temppos.valid() && pieces_layer1[tempindex] != Util::otherside(side))//equal otherside
                                {
                                    related_factor += 0.2;
                                }
                            }
                            else
                            {
                                related_factor += 0.2;
                            }
                        }
                    }
                    else//pieces_layer2[index][d][side] == CHESSTYPE_0 || CHESSTYPE_J2
                    {
                        if (pieces_layer2[tempindex][d][side] > CHESSTYPE_3)//yes:?oo??!  no:xooo??!
                        {
                            //do nothing
                            if (blank == 2)
                            {
                                break;
                            }
                            else
                            {
                                continue;
                            }
                        }
                        else if (pieces_layer2[tempindex][d][side] > CHESSTYPE_2)//xoo??! o?o?!形成冲四
                        {
                            related_factor += 0.2;//两个点
                        }
                        else if (pieces_layer2[tempindex][d][side] > CHESSTYPE_J2)// o??! 形成三？
                        {
                            //CHESSTYPE_2
                            related_factor += 0.1;
                        }

                        if (pieces_layer3[tempindex][side] > CHESSTYPE_D3)
                        {
                            if (pieces_layer2[tempindex][d][side] < CHESSTYPE_2)
                            {
                                temppos = temppos.getNextPosition(d, offset*symbol);
                                if (temppos.valid() && pieces_layer1[tempindex] != Util::otherside(side))//equal otherside
                                {
                                    related_factor += 0.25;
                                }
                            }
                            else
                            {
                                related_factor += 0.25;
                            }
                        }
                        else if (pieces_layer3[tempindex][side] > CHESSTYPE_J2)
                        {
                            if (pieces_layer2[tempindex][d][side] < CHESSTYPE_2)
                            {
                                temppos = temppos.getNextPosition(d, offset*symbol);
                                if (temppos.valid() && pieces_layer1[tempindex] != Util::otherside(side))//equal otherside
                                {
                                    related_factor += 0.2;
                                }
                            }
                            else
                            {
                                related_factor += 0.2;
                            }
                        }
                    }
                }
                if (blank == 2)
                {
                    break;
                }
            }
        }

        base_factor += related_factor;
    }
    return base_factor;
}

int ChessBoard::getGlobalEvaluate(uint8_t side)
{
    //始终是以进攻方(atackside)为正
    uint8_t defendside = lastStep.getSide();
    uint8_t atackside = Util::otherside(defendside);

    int evaluate = 0;
    //遍历所有棋子
    for (uint8_t index = 0; index < Util::BoardIndexUpper; index++)
    {
        //已有棋子的不做计算
        if (!canMove(index) || !useful(index))
        {
            continue;
        }

        if (pieces_layer3[index][atackside] > CHESSTYPE_2 && pieces_layer3[index][atackside] < CHESSTYPE_33)
        {
            evaluate += (int)(chesstypes[pieces_layer3[index][atackside]].atackPriority*getStaticFactor(index, atackside));
        }
        else
        {
            evaluate += chesstypes[pieces_layer3[index][atackside]].atackPriority;
        }

        if (pieces_layer3[index][defendside] > CHESSTYPE_2 && pieces_layer3[index][defendside] < CHESSTYPE_33)
        {
            evaluate -= (int)(chesstypes[pieces_layer3[index][defendside]].defendPriority*getStaticFactor(index, defendside));
        }
        else
        {
            evaluate -= chesstypes[pieces_layer3[index][defendside]].defendPriority;
        }
    }

    return side == atackside ? evaluate : -evaluate;
}

void ChessBoard::printGlobalEvaluate(string &s)
{
    //始终是以进攻方(atackside)为正
    uint8_t defendside = lastStep.getSide();
    uint8_t atackside = Util::otherside(defendside);
    stringstream ss;
    int atack = 0, defend = 0;
    ss << "/" << "\t";
    for (int index = 0; index < BOARD_COL_MAX; index++)
    {
        ss << index << "\t";
    }
    //遍历所有棋子
    for (uint8_t index = 0; index < BOARD_INDEX_BOUND; index++)
    {
        if (index % 15 == 0)
        {
            ss << "\r\n\r\n\r\n";
            ss << index / 15 << "\t";
        }
        //已有棋子的不做计算
        if (!canMove(index) || !useful(index))
        {
            ss << 0 << "|" << 0 << "\t";
            continue;
        }

        if (pieces_layer3[index][atackside] > CHESSTYPE_2 && pieces_layer3[index][atackside] < CHESSTYPE_33)
        {
            atack += (int)(chesstypes[pieces_layer3[index][atackside]].atackPriority*getStaticFactor(index, atackside));
            ss << (int)(chesstypes[pieces_layer3[index][atackside]].atackPriority*getStaticFactor(index, atackside));
        }
        else
        {
            atack += chesstypes[pieces_layer3[index][atackside]].atackPriority;
            ss << chesstypes[pieces_layer3[index][atackside]].atackPriority;
        }
        ss << "|";
        if (pieces_layer3[index][defendside] > CHESSTYPE_2 && pieces_layer3[index][defendside] < CHESSTYPE_33)
        {
            defend += (int)(chesstypes[pieces_layer3[index][defendside]].defendPriority*getStaticFactor(index, defendside));
            ss << (int)(chesstypes[pieces_layer3[index][defendside]].defendPriority*getStaticFactor(index, defendside));
        }
        else
        {
            defend += chesstypes[pieces_layer3[index][defendside]].defendPriority;
            ss << chesstypes[pieces_layer3[index][defendside]].defendPriority;
        }
        ss << "\t";
    }
    ss << "\r\n\r\n\r\n";
    ss << "atack:" << atack << "|" << "defend:" << defend;
    s = ss.str();
}

void ChessBoard::initHash()
{
    for (csidx index = 0; index < 225; ++index)
    {
        hash.z32key ^= z32[index][pieces_layer1[index]];
        hash.z64key ^= z64[index][pieces_layer1[index]];
    }
}
void ChessBoard::updateHashPair(csidx index, uint8_t side, bool add)
{
    if (add) //添加棋子
    {
        hash.z32key ^= z32[index][PIECE_BLANK];//原来是空的
        hash.z32key ^= z32[index][side];
        hash.z64key ^= z64[index][PIECE_BLANK];
        hash.z64key ^= z64[index][side];
    }
    else //拿走棋子
    {
        hash.z32key ^= z32[index][side];       //原来是有子的
        hash.z32key ^= z32[index][PIECE_BLANK];
        hash.z64key ^= z64[index][side];
        hash.z64key ^= z64[index][PIECE_BLANK];
    }
}
