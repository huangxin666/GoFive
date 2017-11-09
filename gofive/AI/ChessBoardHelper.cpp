#include "ChessBoard.h"
#include "TrieTree.h"

int normalTypeHandleSpecial(SearchResult result)
{
    result.pos = chessMode[result.chessMode].pat_len - (result.pos - SEARCH_LENGTH);
    switch (result.chessMode)
    {
    case TRIE_4_DOUBLE_BAN1:
        if (result.pos < 6 && result.pos > 2)
        {
            return TRIE_4_DOUBLE_BAN1;
        }
        else
        {
            return TRIE_4_BLANK_DEAD;
        }
    case TRIE_4_DOUBLE_BAN2:
        if (result.pos < 6 && result.pos > 3)
        {
            return TRIE_4_DOUBLE_BAN2;
        }
        else
        {
            return TRIE_4_BLANK_M;
        }
    case TRIE_4_DOUBLE_BAN3:
        if (result.pos == 5)
        {
            return TRIE_4_DOUBLE_BAN3;
        }
        else
        {
            return TRIE_4_BLANK_DEAD;
        }
    case TRIE_4_CONTINUE_BAN:
        if (result.pos > 2)
        {
            return TRIE_4_CONTINUE_BAN;
        }
        else
        {
            return TRIE_4_CONTINUE_DEAD_BAN;
        }
    case TRIE_4_CONTINUE_BAN_R:
        if (result.pos < 6)
        {
            return TRIE_4_CONTINUE_BAN_R;
        }
        else
        {
            return TRIE_4_CONTINUE_DEAD_BAN_R;
        }
    case TRIE_4_BLANK_BAN:
        if (result.pos > 3)
        {
            return TRIE_4_BLANK_BAN;
        }
        else
        {
            return TRIE_4_BLANK_DEAD_BAN_R;
        }
    case TRIE_4_BLANK_BAN_R:
        if (result.pos < 6)
        {
            return TRIE_4_BLANK_BAN_R;
        }
        else
        {
            return TRIE_4_BLANK_DEAD_BAN;
        }
    default:
        return result.chessMode;
    }
}

CHESSTYPE normalType2HashType(int chessModeType, bool ban)
{
    if (chessModeType == TRIE_6_CONTINUE)
    {
        return ban ? CHESSTYPE_BAN : CHESSTYPE_5;
    }
    else if (chessModeType == TRIE_5_CONTINUE)
    {
        return CHESSTYPE_5;
    }
    else if (chessModeType == TRIE_4_DOUBLE_BAN1 || chessModeType == TRIE_4_DOUBLE_BAN2 || chessModeType == TRIE_4_DOUBLE_BAN3)
    {
        return CHESSTYPE_44;//特殊双四
    }
    else if (chessModeType == TRIE_4_CONTINUE_BAN || chessModeType == TRIE_4_CONTINUE_BAN_R)
    {
        return ban ? CHESSTYPE_D4 : CHESSTYPE_4;
    }
    else if (chessModeType == TRIE_4_BLANK_BAN || chessModeType == TRIE_4_BLANK_BAN_R)
    {
        return ban ? CHESSTYPE_D3 : CHESSTYPE_D4P;
    }
    else if (chessModeType >= TRIE_4_CONTINUE_DEAD_BAN && chessModeType <= TRIE_4_BLANK_DEAD_BAN_R)
    {
        return ban ? CHESSTYPE_0 : CHESSTYPE_D4;
    }
    else if (chessModeType == TRIE_4_CONTINUE)
    {
        return CHESSTYPE_4;
    }
    else if (chessModeType == TRIE_4_BLANK || chessModeType == TRIE_4_BLANK_R)
    {
        return CHESSTYPE_D4P;
    }
    else if (chessModeType >= TRIE_4_CONTINUE_DEAD && chessModeType <= TRIE_4_BLANK_M)
    {
        return CHESSTYPE_D4;
    }
    else if (chessModeType == TRIE_3_CONTINUE_TRUE)
    {
        return CHESSTYPE_3;
    }
    else if (chessModeType == TRIE_3_BLANK
        || chessModeType == TRIE_3_BLANK_R
        || chessModeType == TRIE_3_CONTINUE
        || chessModeType == TRIE_3_CONTINUE_R)
    {
        return CHESSTYPE_J3;
    }
    else if (chessModeType >= TRIE_3_BLANK_DEAD2 && chessModeType <= TRIE_3_BLANK_DEAD1_R)
    {
        return CHESSTYPE_D3;
    }
    else if (chessModeType == TRIE_2_CONTINUE_J3)
    {
        return CHESSTYPE_2;
    }
    else if (chessModeType == TRIE_2_BLANK || chessModeType == TRIE_2_CONTINUE || chessModeType == TRIE_2_CONTINUE_R || chessModeType == TRIE_2_DOUBLE_BLANK)
    {
        return CHESSTYPE_J2;
    }
    else
    {
        return CHESSTYPE_0;
    }
}

void ChessBoard::initLayer2Table()
{
    uint32_t hash_size = 2;
    int chess_mode_len = 1;
    for (; chess_mode_len < 5; ++chess_mode_len, hash_size *= 2)
    {
        layer2_table[chess_mode_len] = new uint8_t[hash_size*chess_mode_len];
        memset(layer2_table[chess_mode_len], 0, hash_size*chess_mode_len);
        layer2_table_ban[chess_mode_len] = new uint8_t[hash_size*chess_mode_len];
        memset(layer2_table_ban[chess_mode_len], 0, hash_size*chess_mode_len);
    }

    hash_size = 2 * 2 * 2 * 2 * 2; //2^5
    chess_mode_len = 5;
    for (; chess_mode_len < 16; ++chess_mode_len, hash_size *= 2)
    {
        layer2_table[chess_mode_len] = new uint8_t[hash_size*chess_mode_len];
        layer2_table_ban[chess_mode_len] = new uint8_t[hash_size*chess_mode_len];
        for (uint32_t index = 0; index < hash_size; ++index)//00001 对应 o????
        {
            for (int offset = 0; offset < chess_mode_len; ++offset)
            {
                if ((index >> offset) & 0x1)//已有棋子，置0
                {
                    layer2_table[chess_mode_len][index*chess_mode_len + offset] = CHESSTYPE_0;
                    layer2_table_ban[chess_mode_len][index*chess_mode_len + offset] = CHESSTYPE_0;
                    continue;
                }
                uint32_t chessInt = index | (1 << offset);//第offset位 置1

                int loffset = SEARCH_LENGTH - offset;
                int fix_start = 0;
                if (loffset > 0)
                {
                    chessInt = chessInt << loffset;
                    fix_start = loffset;
                }
                else
                {
                    chessInt = chessInt >> (-loffset);
                }
                int fix_len = loffset + chess_mode_len;

                int type = normalTypeHandleSpecial(TrieTreeNode::getInstance()->searchAC(chessInt, fix_start, fix_len));//5位的o????填充到11位，offset = 0 时 roffset = 10 loffset = 5(middle)
                layer2_table[chess_mode_len][index*chess_mode_len + offset] = normalType2HashType(type, false);
                layer2_table_ban[chess_mode_len][index*chess_mode_len + offset] = normalType2HashType(type, true);
            }
        }
    }

}

static uint8_t bit_mask[5] = { 0x0F,0x07,0x03,0x01,0 };
void ChessBoard::initPatternToLayer2Table()
{
    for (uint16_t i = 0; i < 256; ++i)
    {
        for (uint16_t j = 0; j < 256; ++j)
        {
            int loffset = 0, roffset = 0;
            //left
            for (uint8_t l = 0, p = j >> 4; l < 4; ++l, p >>= 1)
            {
                if ((p & 1) == 1)
                {
                    loffset = 4 - l;
                    break;
                }
            }
            //right
            for (uint8_t l = 0, p = (j & 0x0F); l < 4; ++l, p >>= 1)
            {
                if ((p & 1) == 1)
                {
                    roffset = l + 1;
                }
            }
            int len = 9 - loffset - roffset;
            if (len < 5)
            {
                pattern_to_layer2_table[i][j] = CHESSTYPE_0;
                pattern_to_layer2_table_ban[i][j] = CHESSTYPE_0;
            }
            else
            {
                uint32_t index = 0;
                //左边四位
                index |= (i >> 4)&bit_mask[loffset];
                //中间一位
                index = index << 1;
                index += 1;
                //右边四位
                index <<= 4;
                index |= i & 0x0F;
                index >>= roffset; //fix

                pattern_to_layer2_table[i][j] = layer2_table[len][index];
                pattern_to_layer2_table_ban[i][j] = layer2_table_ban[len][index];
            }
        }
    }
}