#ifndef TRIETREE_H
#define TRIETREE_H
#include "utils.h"


class TrieTreeNode
{
public:
    TrieTreeNode* childs[3];// 0-o,1-x,2-?
    int chessType;//-1˵��������ֹ״̬
    int deep;//��ȼ�Ϊģʽ������
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

    inline SearchResult search(char *str)
    {
        return algType == 1 ? searchTrie(str) : searchAC(str);
    }
    SearchResult searchTrie(char *str);
    SearchResult searchAC(char *str);
    string testSearch();

    inline int char2index(const char &a)
    {
        if (a == 'o')
        {
            return 0;
        }
        else if (a == 'x')
        {
            return 1;
        }
        else if (a == '?')
        {
            return 2;
        }
        else
        {
            return -1;
        }
    };
};

#endif