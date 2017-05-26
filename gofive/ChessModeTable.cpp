#include "ChessBoard.h"

void ChessBoard::initChessModeHashTable()
{
    uint32_t searchMode;
    uint64_t searchModeTemp;
    int hash_size = 2 * 2 * 2 * 2 * 2; //2^5
    int chess_mode_len = 5;
    for (; chess_mode_len < 16; ++chess_mode_len, hash_size *= 2)
    {
        chessModeHashTable[chess_mode_len] = new uint8_t[hash_size*chess_mode_len];
        chessModeHashTableBan[chess_mode_len] = new uint8_t[hash_size*chess_mode_len];
        for (int index = 0; index < hash_size; ++index)
        {
            searchModeTemp = 0;
            searchMode = 0;
            for (int l = 0; l < chess_mode_len; ++l)
            {
                if ((index >> l) & 0x1)
                {
                    searchModeTemp |= ((uint64_t)0) << l * 2;
                }
                else//blank
                {
                    searchModeTemp |= ((uint64_t)2) << l * 2;
                }
            }
            searchModeTemp |= ((uint64_t)1) << chess_mode_len * 2;
            searchModeTemp = (searchModeTemp << 2) + 1;
            //len = chess_mode_len + 2

            searchModeTemp = searchModeTemp << 4 * 2;
            for (int l = 0; l < chess_mode_len; ++l)
            {
                if ((index >> l) & 0x1)//已有棋子，置0
                {
                    chessModeHashTable[chess_mode_len][index*chess_mode_len + l] = MODE_BASE_0;
                    chessModeHashTableBan[chess_mode_len][index*chess_mode_len + l] = MODE_BASE_0;
                    continue;
                }
                int type = normalTypeHandleSpecial(searchTrieTree->search((uint32_t)((searchModeTemp >> l * 2) | (2 << 5 * 2))));
                chessModeHashTable[chess_mode_len][index*chess_mode_len + l] = normalType2HashType(type, false);
                chessModeHashTableBan[chess_mode_len][index*chess_mode_len + l] = normalType2HashType(type, true);
            }
        }
    }

}

int ChessBoard::normalTypeHandleSpecial(SearchResult result)
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
    case -1:
        return 0;
    default:
        return result.chessMode;
    }
}

CHESSMODE ChessBoard::normalType2HashType(int chessModeType, bool ban)
{
    if (chessModeType == TRIE_6_CONTINUE)
    {
        return ban ? MODE_ADV_BAN : MODE_BASE_5;
    }
    else if (chessModeType == TRIE_5_CONTINUE)
    {
        return MODE_BASE_5;
    }
    else if (chessModeType == TRIE_4_DOUBLE_BAN1 || chessModeType == TRIE_4_DOUBLE_BAN2 || chessModeType == TRIE_4_DOUBLE_BAN3)
    {
        return ban ? MODE_ADV_BAN : MODE_ADV_44;//特殊双四
    }
    else if (chessModeType == TRIE_4_CONTINUE_BAN || chessModeType == TRIE_4_CONTINUE_BAN_R)
    {
        return ban ? MODE_BASE_d4 : MODE_BASE_4;
    }
    else if (chessModeType == TRIE_4_BLANK_BAN || chessModeType == TRIE_4_BLANK_BAN_R)
    {
        return ban ? MODE_BASE_d3p : MODE_BASE_d4p;
    }
    else if (chessModeType >= TRIE_4_CONTINUE_DEAD_BAN && chessModeType <= TRIE_4_BLANK_DEAD_BAN_R)
    {
        return ban ? MODE_BASE_0 : MODE_BASE_d4;
    }
    else if (chessModeType == TRIE_4_CONTINUE)
    {
        return MODE_BASE_4;
    }
    else if (chessModeType == TRIE_4_BLANK || chessModeType == TRIE_4_BLANK_R)
    {
        return MODE_BASE_d4p;
    }
    else if (chessModeType >= TRIE_4_CONTINUE_DEAD && chessModeType <= TRIE_4_BLANK_M)
    {
        return MODE_BASE_d4;
    }
    else if (chessModeType >= TRIE_3_CONTINUE && chessModeType <= TRIE_3_BLANK_R)
    {
        return MODE_BASE_3;
    }
    else if (chessModeType == TRIE_3_BLANK_DEAD2 || chessModeType == TRIE_3_BLANK_DEAD2_R)
    {
        return MODE_BASE_d3p;
    }
    else if (chessModeType >= TRIE_3_CONTINUE_F && chessModeType <= TRIE_3_BLANK_DEAD1_R)
    {
        return MODE_BASE_d3;
    }
    else if (chessModeType == TRIE_2_CONTINUE)
    {
        return MODE_BASE_2;
    }
    else if (chessModeType == TRIE_2_BLANK)
    {
        return MODE_BASE_j2;
    }
    else
    {
        return MODE_BASE_0;
    }
}