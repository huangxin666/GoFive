#include "utils.h"
#include <string>

std::string tree_pats[TRIE_COUNT] = {
    "oooooo",
    "ooooo",
    "?oooo?",
    "o?oooo?",
    "?oooo?o",
    "?oooox",
    "xoooo?",
    "o?oooox",
    "xoooo?o",
    "o?ooo??",
    "??ooo?o",
    "oo?ooo??",
    "??ooo?oo",
    "ooo?o",
    "o?ooo",
    "ooo?oo",
    "oo?ooo",
    "oo?oo",
    "?ooo??",
    "??ooo?",
    "?o?oo?",
    "?oo?o?",
    "?ooo?",
    "??ooox",
    "xooo??",
    "?o?oox",
    "xoo?o?",
    "?oo?ox",
    "xo?oo?",
    "?oo?",
    "?o?o?"
};
const int SEARCH_LENGTH = 6;


int fastfind(int f[], const string &p, int size_o, char o[], int range)
{
    int size_p = p.length();
    int i = 6 - range, j = 0;
    size_o = size_o - 6 + range;
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
    int temp;
    for (int i = 0; i < TRIE_COUNT; i++)
    {
        node = head;
        for (size_t n = 0; n < tree_pats[i].size(); n++)
        {
            if (tree_pats[i][n] == 'o')
            {
                temp = 0;
            }
            else if (tree_pats[i][n] == 'x')
            {
                temp = 1;
            }
            else if (tree_pats[i][n] == '?')
            {
                temp = 2;
            }
            else
            {
                clearStringTree();
                return false;
            }
            if (node->childs[temp] == NULL)
            {
                node->childs[temp] = new TrieTreeNode();
            }
            node = node->childs[temp];

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

int TrieTreeNode::search(char *str, TrieTreeResult *result)
{
    int len = SEARCH_LENGTH + 1;
    TrieTreeNode *node;
    int index;
    int range = SEARCH_LENGTH * 2 + 1;
    for (int i = 0; i < len; i++)
    {
        node = this;
        for (int j = i; j < range; j++)
        {
            if (str[j] == 'o')
            {
                index = 0;
            }
            else if (str[j] == 'x')
            {
                index = 1;
            }
            else if (str[j] == '?')
            {
                index = 2;
            }
            if (node->childs[index] != NULL)
            {
                node = node->childs[index];
            }
            else
            {
                if (node->chessType == -1)
                {
                    break;
                }
                if (result == NULL)
                {
                    return node->chessType;
                }
                result->result[node->chessType] = 1;
                break;
            }
            if (j == range - 1)
            {
                if (node->chessType == -1)
                {
                    break;
                }
                if (result == NULL)
                {
                    return node->chessType;
                }
                result->result[node->chessType] = 1;
                break;
            }
            
        }
        
    }
    return -1;
}
