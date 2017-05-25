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
    //lastStep.step = 0;
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

void ChessBoard::init_layer1()
{

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

    for (int d = 0; d < DIRECTION4_COUNT; ++d)
    {
        uint16_t l_hash_index = 0, r_hash_index = 0;
        int l_offset = 0, r_offset = 0;
        while (1)//����
        {
            uint8_t temp = index - direction_offset_index[d] * (l_offset + 1);
            if (temp > index)// out of range
            {
                break;
            }
            if (pieces_layer1[temp] == PosUtil::otherside(side))
            {
                break;
            }

            if (pieces_layer1[temp] == side)
            {
                l_hash_index |= 1 << l_offset;
            }
            l_offset++;
        }
        while (1)//����
        {
            uint8_t temp = index + direction_offset_index[d] * (r_offset + 1);

            if (temp < index || !PosUtil::valid(temp))// out of range
            {
                break;
            }

            if (pieces_layer1[temp] == PosUtil::otherside(side))
            {
                break;
            }

            if (pieces_layer1[temp] == side)
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

        if (pieces_layer1[index] == PosUtil::otherside(side))
        {
            int len = l_offset;
            int index_offset = l_hash_index * len;
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
            int index_offset = ((((l_hash_index << 1) + (pieces_layer1[index] == side ? 1 : 0)) << r_offset) + r_hash_index) *len;
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
    for (uint8_t index = 0; PosUtil::valid(index); ++index)
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
    uint8_t count[MODE_COUNT] = { 0 };//BASEMODE
    uint8_t result = 0;
    int deadfour = 0, alivethree = 0;
    for (int d = 0; d < 4; ++d)
    {
        if (pieces_layer2[index][d][side] < MODE_BASE_5)
        {
            if (pieces_layer2[index][d][side] > result)
            {
                result = pieces_layer2[index][d][side];
            }
            if (pieces_layer2[index][d][side] == MODE_BASE_3)
            {
                alivethree++;
            }
            if (pieces_layer2[index][d][side] == MODE_BASE_d4
                || pieces_layer2[index][d][side] == MODE_BASE_d4p)
            {
                deadfour++;
            }
        }
        else if (pieces_layer2[index][d][side] == MODE_BASE_5)//5�����
        {
            pieces_layer3[index][side] = MODE_BASE_5;
            return;
        }
        else if (pieces_layer2[index][d][side] == MODE_ADV_BAN)
        {
            pieces_layer3[index][side] = MODE_ADV_BAN;
            return;
        }
        else if (pieces_layer2[index][d][side] == MODE_ADV_t4)//˫���൱�ڻ��ģ��ǽ�������
        {
            pieces_layer3[index][side] = MODE_ADV_t4;
            return;
        }
    }

    //�������
    if (ban && side == PIECE_BLACK)
    {
        if (result == MODE_BASE_4)
        {
            deadfour++;
        }
        if (deadfour > 1)
        {
            pieces_layer3[index][side] = MODE_ADV_BAN;
        }
        else if (alivethree > 1)
        {
            pieces_layer3[index][side] = MODE_ADV_BAN;
        }
        else if (deadfour == 1 && alivethree == 1 && result != MODE_BASE_4)
        {
            pieces_layer3[index][side] = MODE_ADV_t43;
        }
    }
    else
    {
        if (result == MODE_BASE_4)
        {
            return;
        }
        if (deadfour > 1)
        {
            pieces_layer3[index][side] = MODE_ADV_t4;
        }
        else if (deadfour == 1 && alivethree > 0)
        {
            pieces_layer3[index][side] = MODE_ADV_t43;
        }
        else if (alivethree > 1)
        {
            pieces_layer3[index][side] = MODE_ADV_t3;
        }
    }

}

void ChessBoard::updateArea_layer3(uint8_t index, int side)//���Ӵ�
{
    int result = 0;
    int blankCount, chessCount, r, c;
    uint8_t row = PosUtil::getRow(index);
    uint8_t col = PosUtil::getCol(index);
    if (pieces_layer1[index] == PIECE_BLANK)
    {
        ratings[side] -= chess_ratings[pieces_layer3[index][side]];
        updatePoint_layer3(index, side);
        ratings[side] += chess_ratings[pieces_layer3[index][side]];
        
    }

    for (int i = 0; i < DIRECTION8_COUNT; ++i)//8������
    {
        r = row, c = col;
        blankCount = 0;
        chessCount = 0;
        while (nextPosition(r, c, 1, i)) //����������߽�
        {
            if (pieces_layer1[PosUtil::xy2index(r, c)] == PIECE_BLANK)
            {
                blankCount++;
                if (pieces_hot[PosUtil::xy2index(r, c)])
                {
                    ratings[side] -= chess_ratings[pieces_layer3[PosUtil::xy2index(r, c)][side]];
                    updatePoint_layer3(PosUtil::xy2index(r, c), side);
                    ratings[side] += chess_ratings[pieces_layer3[PosUtil::xy2index(r, c)][side]];
                }
            }
            else if (pieces_layer1[PosUtil::xy2index(r, c)] == PosUtil::otherside(side))
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

void ChessBoard::updateHotArea(uint8_t index)
{
    int row = PosUtil::getRow(index);
    int col = PosUtil::getCol(index);
    int range = 5;//2 * 2 + 1
    int tempcol, temprow;
    for (int i = 0; i < range; ++i)
    {
        //����
        tempcol = col - 2 + i;
        if (tempcol > -1 && tempcol < BOARD_COL_MAX && pieces_layer1[PosUtil::xy2index(row, tempcol)] == PIECE_BLANK)
        {
            setHot(row, tempcol, true);
        }
        //����
        temprow = row - 2 + i;
        if (temprow > -1 && temprow < BOARD_ROW_MAX && pieces_layer1[PosUtil::xy2index(temprow, col)] == PIECE_BLANK)
        {
            setHot(temprow, col, true);
        }
        //����
        if (temprow > -1 && temprow < BOARD_ROW_MAX && tempcol> -1 && tempcol < BOARD_COL_MAX &&pieces_layer1[PosUtil::xy2index(temprow, tempcol)] == PIECE_BLANK)
        {
            setHot(temprow, tempcol, true);
        }
        //����
        temprow = row + 2 - i;
        if (temprow > -1 && temprow < BOARD_ROW_MAX && tempcol> -1 && tempcol < BOARD_COL_MAX && pieces_layer1[PosUtil::xy2index(temprow, tempcol)] == PIECE_BLANK)
        {
            setHot(temprow, tempcol, true);
        }
    }
}

void ChessBoard::initHotArea() {
    for (uint8_t index = 0; PosUtil::valid(index); ++index)
    {
        pieces_hot[index] = false;
    }
    for (uint8_t index = 0; PosUtil::valid(index); ++index)
    {
        if (pieces_layer1[index] != PIECE_BLANK)
        {
            updateHotArea(index);
        }
    }
}

bool ChessBoard::move(uint8_t index, int side)
{
    if (pieces_layer1[index] != PIECE_BLANK)
    {
        return false;//��������
    }

    pieces_layer1[index] = side;
    update_layer2(index);
    updateArea_layer3(index);
    updateHotArea(index);
    updateHashPair(PosUtil::getRow(index), PosUtil::getCol(index), side);
    return true;
}

bool ChessBoard::unmove(uint8_t index)
{
    uint8_t side = pieces_layer1[index];
    if (side == PIECE_BLANK)
    {
        return false;//û������
    }
    pieces_layer1[index] = PIECE_BLANK;
    update_layer2(index);
    updateArea_layer3(index);
    pieces_hot[index] = true;
    updateHashPair(PosUtil::getRow(index), PosUtil::getCol(index), side, false);
    return true;
}

void ChessBoard::initRatings()
{
    ratings[PIECE_BLACK] = 0;
    ratings[PIECE_WHITE] = 0;
    for (uint8_t index = 0; PosUtil::valid(index); ++index)
    {
        if (pieces_hot[index] && pieces_layer1[index] == PIECE_BLANK)
        {
            ratings[PIECE_BLACK] += chess_ratings[pieces_layer3[index][PIECE_BLACK]];
            ratings[PIECE_WHITE] += chess_ratings[pieces_layer3[index][PIECE_WHITE]];
        }
    }
}

void ChessBoard::formatChessInt(uint32_t chessInt, char chessStr[FORMAT_LENGTH])
{
    for (int i = 0; i < FORMAT_LENGTH; ++i)
    {
        chessStr[i] = (chessInt >> i * 2) & 3 + '/'; //ȡ�����λ
    }
}

//void ChessBoard::formatChess2Int(uint32_t chessInt[DIRECTION4_COUNT], const int& row, const int& col, const int& state)
//{
//    //chessInt��Ҫ��ʼ��Ϊ0
//    int rowstart = row - SEARCH_LENGTH, colstart = col - SEARCH_LENGTH, rowend = row + SEARCH_LENGTH;
//
//    for (int i = 0; i < FORMAT_LENGTH; ++i, ++rowstart, ++colstart, --rowend)
//    {
//        //����
//        if (colstart < 0 || colstart > 14)
//        {
//            //chessInt[DIRECTION4_R] |= 0 << i * 2;
//        }
//        else
//        {
//            chessInt[DIRECTION4_R] |= (pieces[row][colstart].state*state + 1) << i * 2;
//        }
//
//        //����
//        if (rowstart < 0 || rowstart > 14)
//        {
//            //chessInt[DIRECTION4_D] |= 0 << i * 2;
//        }
//        else
//        {
//            chessInt[DIRECTION4_D] |= (pieces[rowstart][col].state*state + 1) << i * 2;
//        }
//        //������
//        if (colstart < 0 || rowstart < 0 || colstart > 14 || rowstart > 14)
//        {
//            //chessInt[DIRECTION4_RD] |= 0 << i * 2;
//        }
//        else
//        {
//            chessInt[DIRECTION4_RD] |= (pieces[rowstart][colstart].state*state + 1) << i * 2;
//        }
//
//        //������
//        if (colstart < 0 || rowend > 14 || colstart > 14 || rowend < 0)
//        {
//            //chessInt[DIRECTION4_RU] |= 0 << i * 2;
//        }
//        else
//        {
//            chessInt[DIRECTION4_RU] |= (pieces[rowend][colstart].state*state + 1) << i * 2;
//        }
//    }
//}
//
//int ChessBoard::getStepScores(const int& row, const int& col, const int& state, const bool& isdefend)
//{
//    int stepScore = 0;
//    uint32_t direction[DIRECTION4_COUNT] = { 0 };//�ĸ��������棨0��ʾ�գ�-1��ʾ�ϣ�1��ʾ����
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
//            return chessMode[TRIE_5_CONTINUE].evaluation;//û��Ҫ����
//        }
//        else if (result.chessMode >= TRIE_4_CONTINUE)//����Ҫ���⴦��
//        {
//            chessModeCount[result.chessMode]++;
//            continue;
//        }
//
//        //������������
//        result.pos = chessMode[result.chessMode].pat_len - (result.pos - SEARCH_LENGTH);
//        stepScore = handleSpecial(result, state, chessModeCount);
//        if (stepScore)
//        {
//            return stepScore;
//        }
//    }
//
//    //�������
//    int deadFour =
//        chessModeCount[TRIE_4_CONTINUE_DEAD] +
//        chessModeCount[TRIE_4_CONTINUE_DEAD_R] +
//        chessModeCount[TRIE_4_BLANK] +
//        chessModeCount[TRIE_4_BLANK_R] +
//        chessModeCount[TRIE_4_BLANK_DEAD] +
//        chessModeCount[TRIE_4_BLANK_DEAD_R] +
//        chessModeCount[TRIE_4_BLANK_M];
//
//    if (deadFour + chessModeCount[TRIE_4_CONTINUE] > 1)//˫��
//    {
//        if (ban&&state == STATE_CHESS_BLACK)//�н���
//        {
//            return BAN_DOUBLEFOUR;
//        }
//    }
//
//    if (deadFour > 1 && level >= AILEVEL_INTERMEDIATE) //˫�������޽���
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
//    if (aliveThree == 1 && deadFour == 1 && (level >= AILEVEL_HIGH))//���Ļ���
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
//    if (aliveThree > 1)//˫����
//    {
//        if (ban&&state == STATE_CHESS_BLACK)//�н���
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
//            return chessMode[TRIE_5_CONTINUE].evaluation;//û��Ҫ����
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
//                //��Ч�����Ѿ�����Ӯ�ˣ�����������
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
//                //��Ч�����Ѿ�����Ӯ�ˣ�����������
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
//                //��Ч��
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
//                //��Ч�����Ѿ�����Ӯ�ˣ�����������
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
//                //��Ч��
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
//                //��Ч�����Ѿ�����Ӯ�ˣ�����������
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
//                //��Ч��
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
//                //��Ч��
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
//                //��Ч��
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
//                //��Ч��
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
//                //��Ч��
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
//                //��Ч��
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
    for (uint8_t index; index < 225; ++index)
    {
        hash.z32key ^= z32[PosUtil::getRow(index)][PosUtil::getCol(index)][pieces_layer1[index]];
        hash.z64key ^= z64[PosUtil::getRow(index)][PosUtil::getCol(index)][pieces_layer1[index]];
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
