#include "TrieTree.h"
#include <queue>

int8_t TrieTreeNode::algType = 2;
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
    std::queue<TrieTreeNode*> failQueue;
    TrieTreeNode *parent;
    head->failNode = head;
    //第一层节点单独构造
    for (int i = 0; i < 3; ++i)
    {
        if (head->childs[i])
        {
            head->childs[i]->failNode = head;
            failQueue.push(head->childs[i]);
        }
    }
    //广度优先遍历树
    while (!failQueue.empty())
    {
        node = failQueue.front();
        failQueue.pop();
        for (int i = 0; i < 3; ++i)
        {
            if (node->childs[i])
            {
                parent = node;
                while (true)//failNode一层一层的找上去
                {
                    node->childs[i]->failNode = parent->failNode->childs[i];
                    if (node->childs[i]->failNode != NULL)//!=NULL
                    {
                        break;
                    }
                    if (parent == head)
                    {
                        node->childs[i]->failNode = head;
                        break;
                    }
                    parent = parent->failNode;
                }
                failQueue.push(node->childs[i]);
            }
        }
    }


    return true;
}

SearchResult TrieTreeNode::searchAC(uint32_t chessInt, int fix_start, int len)
{
    TrieTreeNode *node = this;
    TrieTreeNode *head = this;
    SearchResult result = { -1, 0 };
    int index;
    for (int i = fix_start; i < FORMAT_MAX_LENGTH && i < len; ++i)
    {
        index = (chessInt >> i) & 1;
        if (index < 0)
        {
            return SearchResult{ -1,0 };
        }
    retry:
        if (node->childs[index] == NULL)//失败
        {
            if (node->chessType > -1 &&
                (i + 1 - node->deep + chessMode[node->chessType].left_offset) > SEARCH_MIDDLE && (i - node->deep) < SEARCH_LENGTH)
            {
                if (result.chessMode < 0 || node->chessType < result.chessMode)
                {
                    result.chessMode = node->chessType;
                    result.pos = i;
                }
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
            if (node->chessType > -1 &&
                (i + 1 - node->deep + chessMode[node->chessType].left_offset) > SEARCH_MIDDLE && (i - node->deep) < SEARCH_LENGTH)
            {
                if (i == FORMAT_MAX_LENGTH - 1)
                {
                    if (result.chessMode < 0 || node->chessType < result.chessMode)
                    {
                        result.chessMode = node->chessType;
                        result.pos = i;
                    }
                }
                else
                {
                    if (result.chessMode < 0 || node->chessType < result.chessMode)
                    {
                        result.chessMode = node->chessType;
                        result.pos = i;
                    }
                }
            }
        }
    }
    return result;
}

SearchResult TrieTreeNode::searchTrie(uint32_t chessInt)
{
    int search_range = SEARCH_LENGTH + 1;
    TrieTreeNode *node;
    SearchResult result = { -1,0 };
    int index;
    for (int i = 0; i < search_range; i++)
    {
        node = this;
        for (int j = i; j < FORMAT_MAX_LENGTH; j++)
        {
            index = (chessInt >> j * 2) & 3;
            if (index < 0)
            {
                return SearchResult{ -1,0 };
            }
            if (node->childs[index] == NULL)
            {
                if (node->chessType > -1 && (i + chessMode[node->chessType].left_offset) > SEARCH_MIDDLE)
                {
                    if (result.chessMode < 0 || node->chessType < result.chessMode)
                    {
                        result.chessMode = node->chessType;
                        result.pos = j;
                    }
                }
                break;
            }
            else
            {
                node = node->childs[index];
                if (node->chessType > -1 && (i + chessMode[node->chessType].left_offset) > SEARCH_MIDDLE)
                {
                    if (j == FORMAT_MAX_LENGTH - 1)
                    {
                        if (result.chessMode < 0 || node->chessType < result.chessMode)
                        {
                            result.chessMode = node->chessType;
                            result.pos = j;
                        }
                    }
                    else
                    {
                        if (result.chessMode < 0 || node->chessType < result.chessMode)
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

SearchResult TrieTreeNode::searchString(string &str)
{
    uint32_t chessInt = 0;
    int len = 0;
    int fix = 0;

    for (auto ch : str)
    {
        if (ch == 'x')
        {
            if (len < str.size() / 2)
            {
                fix = len + 1;
                len = 0;
                chessInt = 0;
                continue;
            }
            else
            {
                break;
            }
        }
        chessInt |= char2index(ch) << len;
        len++;
    }
    return searchAC(chessInt, fix, len);
}

string TrieTreeNode::testSearch()
{
    string pat;

    pat = "xooooo?oox?";
    if (searchString(pat).chessMode != TRIE_5_CONTINUE)
    {
        return pat;
    }

    pat = "??o?ooo?o??";
    if (searchString(pat).chessMode != TRIE_4_DOUBLE_BAN1)
    {
        return pat;
    }

    pat = "??oo?oo?oo?";
    if (searchString(pat).chessMode != TRIE_4_DOUBLE_BAN2)
    {
        return pat;
    }

    pat = "?ooo?o?ooo?";
    if (searchString(pat).chessMode != TRIE_4_DOUBLE_BAN3)
    {
        return pat;
    }

    pat = "??o?oooo???";
    if (searchString(pat).chessMode != TRIE_4_CONTINUE_BAN)
    {
        return pat;
    }

    pat = "??oooo?o???";
    if (searchString(pat).chessMode != TRIE_4_CONTINUE_BAN_R)
    {
        return pat;
    }
    pat = "??o?oooox??";
    if (searchString(pat).chessMode != TRIE_4_CONTINUE_DEAD_BAN)
    {
        return pat;
    }
    pat = "??xoooo?o??";
    if (searchString(pat).chessMode != TRIE_4_CONTINUE_DEAD_BAN_R)
    {
        return pat;
    }
    pat = "??oo?ooo???";
    if (searchString(pat).chessMode != TRIE_4_BLANK_BAN)
    {
        return pat;
    }
    pat = "???ooo?oo??";
    if (searchString(pat).chessMode != TRIE_4_BLANK_BAN_R)
    {
        return pat;
    }
    pat = "??oo?ooo?x?";
    if (searchString(pat).chessMode != TRIE_4_BLANK_DEAD_BAN_R)
    {
        return pat;
    }
    pat = "?x?ooo?oo??";
    if (searchString(pat).chessMode != TRIE_4_BLANK_DEAD_BAN)
    {
        return pat;
    }
    pat = "?xooooooo??";
    if (searchString(pat).chessMode != TRIE_6_CONTINUE)
    {
        return pat;
    }
    pat = "?x?ooooo???";
    if (searchString(pat).chessMode != TRIE_5_CONTINUE)
    {
        return pat;
    }
    pat = "?x??oooo???";
    if (searchString(pat).chessMode != TRIE_4_CONTINUE)
    {
        return pat;
    }
    pat = "?x?oooox???";
    if (searchString(pat).chessMode != TRIE_4_CONTINUE_DEAD)
    {
        return pat;
    }
    pat = "?xxoooo????";
    if (searchString(pat).chessMode != TRIE_4_CONTINUE_DEAD_R)
    {
        return pat;
    }
    pat = "?o?ooo?????";
    if (searchString(pat).chessMode != TRIE_4_BLANK)
    {
        return pat;
    }
    pat = "???ooo?o???";
    if (searchString(pat).chessMode != TRIE_4_BLANK_R)
    {
        return pat;
    }
    pat = "?x?ooo?o???";//解决一个BUG
    if (searchString(pat).chessMode != TRIE_4_BLANK_DEAD)
    {
        return pat;
    }
    pat = "?o?ooo?x???";
    if (searchString(pat).chessMode != TRIE_4_BLANK_DEAD_R)
    {
        return pat;
    }
    pat = "?oo?oo?????";
    if (searchString(pat).chessMode != TRIE_4_BLANK_M)
    {
        return pat;
    }
    pat = "xoo?oox????";
    if (searchString(pat).chessMode != TRIE_4_BLANK_M)
    {
        return pat;
    }
    pat = "x?oo?oo?x??";
    if (searchString(pat).chessMode != TRIE_4_BLANK_M)
    {
        return pat;
    }
    pat = "????ooo????";
    if (searchString(pat).chessMode != TRIE_3_CONTINUE_TRUE)
    {
        return pat;
    }
    pat = "??x?ooo????";
    if (searchString(pat).chessMode != TRIE_3_CONTINUE)
    {
        return pat;
    }
    pat = "????ooo?x??";
    if (searchString(pat).chessMode != TRIE_3_CONTINUE_R)
    {
        return pat;
    }
    pat = "???o?oo????";
    if (searchString(pat).chessMode != TRIE_3_BLANK)
    {
        return pat;
    }
    pat = "????oo?o???";
    if (searchString(pat).chessMode != TRIE_3_BLANK_R)
    {
        return pat;
    }
    pat = "??x?ooo?x??";
    if (searchString(pat).chessMode != TRIE_3_CONTINUE_F)
    {
        return pat;
    }
    pat = "????oooxx??";
    if (searchString(pat).chessMode != TRIE_3_CONTINUE_DEAD)
    {
        return pat;
    }
    pat = "??xxooo????";
    if (searchString(pat).chessMode != TRIE_3_CONTINUE_DEAD_R)
    {
        return pat;
    }
    pat = "???o?ooxx??";
    if (searchString(pat).chessMode != TRIE_3_BLANK_DEAD1)
    {
        return pat;
    }
    pat = "??xxoo?o???";
    if (searchString(pat).chessMode != TRIE_3_BLANK_DEAD1_R)
    {
        return pat;
    }
    pat = "????oo?oxx?";
    if (searchString(pat).chessMode != TRIE_3_BLANK_DEAD2)
    {
        return pat;
    }
    pat = "???xxo?oo??";
    if (searchString(pat).chessMode != TRIE_3_BLANK_DEAD2_R)
    {
        return pat;
    }
    pat = "?????oo????";
    if (searchString(pat).chessMode != TRIE_2_CONTINUE_J3)
    {
        return pat;
    }
    pat = "???x?oo????";
    if (searchString(pat).chessMode != TRIE_2_CONTINUE)
    {
        return pat;
    }
    pat = "???o?o?????";
    if (searchString(pat).chessMode != TRIE_2_BLANK)
    {
        return pat;
    }
    return string("success");
}
