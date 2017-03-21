#include "utils.h"

ChessModeData chessMode[TRIE_COUNT] = {
    { "oooooo",  100000,100000,5 },
    { "ooooo",   100000,100000,4 },
    { "?oooo?",  12000, 12000, 4 },
    { "o?oooo?", 12000, 12000, 5 },
    { "?oooo?o", 12000, 12000, 6 },
    { "?oooox",  1211,  1000,  4 },
    { "xoooo?",  1211,  1000,  4 },
    { "o?oooox", 1211,  1000,  5 },
    { "xoooo?o", 1211,  1000,  6 },
    { "o?ooo??", 1300,  1030,  4 },
    { "??ooo?o", 1300,  1030,  7 },
    { "oo?ooo??",1300,  1030,  5 },
    { "??ooo?oo",1300,  1030,  7 },
    { "ooo?o",   1210,  999,   4 },
    { "o?ooo",   1210,  999,   4 },
    { "ooo?oo",  1210,  999,   5 },
    { "oo?ooo",  1210,  999,   5 },
    { "oo?oo",   1210,  999,   4 },
    { "?ooo??",  1100,  1200,  3 },
    { "??ooo?",  1100,  1200,  4 },
    { "?o?oo?",  1080,  100,   5 },
    { "?oo?o?",  1080,  100,   5 },
    { "?ooo?",   20,    20,    3 },
    { "??ooox",  20,    20,    4 },
    { "xooo??",  20,    20,    3 },
    { "?o?oox",  5,     5,     4 },
    { "xoo?o?",  5,     5,     4 },
    { "?oo?ox",  10,    10,    4 },
    { "xo?oo?",  10,    10,    4 },
    { "?oo?",    35,    10,    2 },
    { "?o?o?",   30,    5,     3 }
};

int fastfind(int f[], const string &p, int size_o, char o[], int range)
{
    int size_p = p.length();
    int i = SEARCH_LENGTH - range, j = 0;
    size_o = size_o - SEARCH_LENGTH + range;
    while (i < size_o) {
        if (o[i] == p[j]) {
            i++; j++;
        }
        else {
            if (j == 0)
                i++;
            else
                j = f[j - 1] + 1;
        }
        if (j == size_p) {
            /*sum++; j = f[j - 1] + 1;*/
            return 1;
        }
    }
    return 0;
}

void TrieTreeNode::clearStringTree()
{
    if (childs[0] != NULL)
    {
        childs[0]->clearStringTree();
        delete childs[0];
    }
    if (childs[1] != NULL) 
    {
        childs[1]->clearStringTree();
        delete childs[1];
    }
    if (childs[2] != NULL)
    {
        childs[2]->clearStringTree();
        delete childs[2];
    }
}

bool TrieTreeNode::buildStringTree()
{
    TrieTreeNode *head = this;
    TrieTreeNode *node;
    int index,patlen;
    for (int i = 0; i < TRIE_COUNT; i++)
    {
        node = head;
        patlen = strlen(chessMode[i].pat);
        for (int n = 0; n < patlen; n++)
        {
            index = char2index(chessMode[i].pat[n]);
            if (index < 0)
            {
                clearStringTree();
                return false;
            }
            if (node->childs[index] == NULL)
            {
                node->childs[index] = new TrieTreeNode();
            }
            node = node->childs[index];
        }
        if (node->chessType < 0)
        {
            node->chessType = i;
        }
        else
        {
            clearStringTree();
            return false;
        }
       
    }
    return true;
}

int TrieTreeNode::search(char *str, uint8_t result[TRIE_COUNT])
{
    int search_range = SEARCH_LENGTH + 1;
    TrieTreeNode *node;
    int index;
    for (int i = 0; i < search_range; i++)
    {
        node = this;
        for (int j = i; j < FORMAT_LENGTH; j++)
        {
            index = char2index(str[j]);
            if (index < 0)
            {
                return -1;
            }
            if (node->childs[index] == NULL)
            {
                if (node->chessType > -1 && (j + chessMode[node->chessType].max_offset) > SEARCH_MIDDLE)
                {
                    if (result)
                    {
                        result[node->chessType] += 1;
                    }
                    else
                    {
                        return node->chessType;
                    }
                }
                break;
            }
            else
            {
                node = node->childs[index];
                if (node->chessType > -1 && (j + chessMode[node->chessType].max_offset) > SEARCH_MIDDLE)
                {
                    if (result)
                    {
                        result[node->chessType] += 1;
                    }
                    else
                    {
                        if (j == FORMAT_LENGTH - 1)
                        {
                            return node->chessType;
                        }
                    }
                }
            }       
        }  
    }
    return -1;
}
