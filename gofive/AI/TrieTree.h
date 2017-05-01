#ifndef TRIETREE_H
#define TRIETREE_H
#include "utils.h"


class TrieTreeNode
{
public:
    TrieTreeNode* childs[3];// 0-o,1-x,2-?
    int chessType;//-1说明不是终止状态
    int deep;//深度即为模式串长度
    TrieTreeNode* failNode;
    static int8_t algType;
    TrieTreeNode()
    {
        chessType = -1;
        deep = 0;
        childs[0] = childs[1] = childs[2] = NULL;
        failNode = NULL;
    };
    ~TrieTreeNode()
    {

    }
    void clearTrieTree();
    bool buildTrieTree();

    inline SearchResult search(uint32_t chessInt)
    {
        return algType == 1 ? searchTrie(chessInt) : searchAC(chessInt);
    }
    SearchResult searchTrie(uint32_t chessInt);
    SearchResult searchAC(uint32_t chessInt);
    string testSearch();
    uint32_t string2int(string str);

    int char2index(const char &a)
    {
        if (a == 'o')
        {
            return 2;
        }
        else if (a == 'x')
        {
            return 0;
        }
        else if (a == '?')
        {
            return 1;
        }
        else
        {
            return -1;
        }
    };
};

#endif