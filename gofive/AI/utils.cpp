#include "utils.h"


ChessModeData chessMode[TRIE_COUNT] = {
    { "o?ooo?o",  12000, 12000, 6 },
    { "oo?oo?oo", 12000, 12000, 7 },
    { "ooo?o?ooo",12000, 12000, 8 },
    { "o?oooo?",  12000, 12000, 5 },
    { "?oooo?o",  12000, 12000, 6 },
    { "o?oooox",  1211,  1000,  5 },
    { "xoooo?o",  1211,  1000,  6 },
    { "oo?ooo??", 1300,  1030,  5 },
    { "??ooo?oo", 1300,  1030,  7 },
    { "ooo?oo",   1210,  999,   5 },
    { "oo?ooo",   1210,  999,   5 },
    { "oooooo",   100000,100000,5 },
    { "ooooo",    100000,100000,4 },
    { "?oooo?",   12000, 12000, 4 },
    { "?oooox",   1211,  1000,  4 },
    { "xoooo?",   1211,  1000,  4 },
    { "o?ooo??",  1300,  1030,  4 },
    { "??ooo?o",  1300,  1030,  6 },
    { "ooo?o",    1210,  999,   4 },
    { "o?ooo",    1210,  999,   4 },
    { "oo?oo",    1210,  999,   4 },
    { "?ooo??",   1100,  1200,  3 },
    { "??ooo?",   1100,  1200,  4 },
    { "?o?oo?",   1080,  100,   5 },
    { "?oo?o?",   1080,  100,   5 },
    { "?ooo?",    20,    20,    3 },
    { "??ooox",   20,    20,    4 },
    { "xooo??",   20,    20,    3 },
    { "?o?oox",   5,     5,     4 },
    { "xoo?o?",   5,     5,     4 },
    { "?oo?ox",   10,    10,    4 },
    { "xo?oo?",   10,    10,    4 },
    { "?oo?",     35,    10,    2 },
    { "?o?o?",    30,    5,     3 }
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

void TrieTreeNode::clearTrieTree()
{
    if (childs[0] != NULL)
    {
        childs[0]->clearTrieTree();
        delete childs[0];
    }
    if (childs[1] != NULL)
    {
        childs[1]->clearTrieTree();
        delete childs[1];
    }
    if (childs[2] != NULL)
    {
        childs[2]->clearTrieTree();
        delete childs[2];
    }
}

bool TrieTreeNode::buildTrieTree()
{
    TrieTreeNode *head = this;
    TrieTreeNode *node;
    int index;
    //建立字典树
    for (int i = 0; i < TRIE_COUNT; i++)
    {
        node = head;
        chessMode[i].pat_len = uint8_t(strlen(chessMode[i].pat));
        for (int n = 0; n < chessMode[i].pat_len; n++)
        {
            index = char2index(chessMode[i].pat[n]);
            if (index < 0)
            {
                clearTrieTree();
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
            node->deep = chessMode[i].pat_len;
        }
        else
        {
            clearTrieTree();
            return false;
        }
    }

    //添加失败节点，一层一层的添加，必须按顺序
    queue<TrieTreeNode*> failQueue;
    head->failNode = head;
    failQueue.push(head);
    //广度优先遍历树
    while (!failQueue.empty())
    {
        node = failQueue.front();
        failQueue.pop();
        for (int i = 0; i < 3; ++i)
        {
            if (node->childs[i])
            {
                node->childs[i]->failNode = node->failNode->childs[i];
                if (node->childs[i]->failNode == NULL)
                {
                    node->childs[i]->failNode = head;
                }
                failQueue.push(node->childs[i]);
            }
        }
    }


    return true;
}

SearchResult TrieTreeNode::searchAC(char *str)
{
    TrieTreeNode *node = this;
    TrieTreeNode *head = this;
    SearchResult result = { -1, 0 };
    int index;
    for (int i = 0; i < FORMAT_LENGTH; ++i)
    {
        index = char2index(str[i]);
        if (index < 0)
        {
            return SearchResult{ -1,0 };
        }
    retry:
        if (node->childs[index] == NULL)//失败
        {
            if (node->chessType > -1 && (i + chessMode[node->chessType].left_offset) > SEARCH_MIDDLE && (i - node->deep) < SEARCH_LENGTH)
            {
                return SearchResult{ node->chessType, i };
            }
            if (result.chessMode > -1)
            {
                return result;
            }
            if (node == head)
            {
                continue;
            }
            node = node->failNode;
            goto retry;
        }
        else//往下走
        {
            node = node->childs[index];
            if (node->chessType > -1 && (i + chessMode[node->chessType].left_offset) > SEARCH_MIDDLE && (i - node->deep) < SEARCH_LENGTH)
            {
                if (i == FORMAT_LENGTH - 1)
                {
                    return SearchResult{ node->chessType, i };
                }
                else
                {
                    result.chessMode = node->chessType;
                    result.pos = i;
                }
            }
        }
    }
    return result;
}

SearchResult TrieTreeNode::searchTrie(char *str)
{
    int search_range = SEARCH_LENGTH + 1;
    TrieTreeNode *node;
    SearchResult result = { -1,0 };
    int index;
    for (int i = 0; i < search_range; i++)
    {
        node = this;
        for (int j = i; j < FORMAT_LENGTH; j++)
        {
            index = char2index(str[j]);
            if (index < 0)
            {
                return SearchResult{ -1,0 };
            }
            if (node->childs[index] == NULL)
            {
                if (node->chessType > -1 && (i + chessMode[node->chessType].left_offset) > SEARCH_MIDDLE)
                {
                    if (result.chessMode < 0 || chessMode[node->chessType].pat_len > chessMode[result.chessMode].pat_len)
                    {
                        result.chessMode = node->chessType;
                        result.pos = j;
                    }
                    //return SearchResult{ node->chessType, j };
                }
                /*if (result.chessMode > -1)
                {
                    return result;
                }*/
                break;
            }
            else
            {
                node = node->childs[index];
                if (node->chessType > -1 && (i + chessMode[node->chessType].left_offset) > SEARCH_MIDDLE)
                {
                    if (j == FORMAT_LENGTH - 1)
                    {
                        if (result.chessMode < 0 || chessMode[node->chessType].pat_len > chessMode[result.chessMode].pat_len)
                        {
                            result.chessMode = node->chessType;
                            result.pos = j;
                        }
                    }
                    else
                    {
                        if (result.chessMode < 0 || chessMode[node->chessType].pat_len > chessMode[result.chessMode].pat_len)
                        {
                            result.chessMode = node->chessType;
                            result.pos = j;
                        }
                    }
                }
            }
        }
    }
    return result;
}

string TrieTreeNode::testSearch()
{
    char *pat = "??o?ooo?o??????";
    if (search(pat).chessMode != TRIE_4_DOUBLE_BAN1)
    {
        return string(pat);
    }
    return string();
}