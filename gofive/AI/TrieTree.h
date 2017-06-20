#ifndef TRIETREE_H
#define TRIETREE_H
#include "defines.h"

//先采用长的覆盖短的策略，（或者可以使用上面的覆盖下面的）
enum CHESSMODE2
{
    TRIE_6_CONTINUE,		    //"oooooo",   SCORE_5_CONTINUE,SCORE_5_CONTINUE,5                                   特殊棋型,禁手,非禁手等同于TRIE_5_CONTINUE
    TRIE_5_CONTINUE,			//"ooooo",    SCORE_5_CONTINUE,SCORE_5_CONTINUE,4
    TRIE_4_DOUBLE_BAN1,         //"o?ooo?o",  12000, 12000,                     4                                   特殊棋型
    TRIE_4_DOUBLE_BAN2,         //"oo?oo?oo", 12000, 12000,                     4                                   特殊棋型
    TRIE_4_DOUBLE_BAN3,         //"ooo?o?ooo",12000, 12000,                     4                                   特殊棋型
    TRIE_4_CONTINUE_BAN,        //"o?oooo?",  12000, 12000,                     5                                   特殊棋型
    TRIE_4_CONTINUE_BAN_R,      //"?oooo?o",  12000, 12000,                     4                                   特殊棋型
    TRIE_4_BLANK_BAN,           //"oo?ooo??", 1300,  1030,                      5                                   特殊棋型
    TRIE_4_BLANK_BAN_R,         //"??ooo?oo", 1300,  1030,                      4                                   特殊棋型
    TRIE_4_CONTINUE_DEAD_BAN,   //"o?oooox",  1211,  1000,                      5                                   特殊棋型
    TRIE_4_CONTINUE_DEAD_BAN_R, //"xoooo?o",  1211,  1000,                      4                                   特殊棋型
    TRIE_4_BLANK_DEAD_BAN,      //"ooo?oo",   1210,  999,                       5                                   特殊棋型
    TRIE_4_BLANK_DEAD_BAN_R,    //"oo?ooo",   1210,  999,                       5                                   特殊棋型
    TRIE_4_CONTINUE,			//"?oooo?",   12000, 12000,                     4
    TRIE_4_BLANK,			    //"o?ooo??",  1300,  1030,                      4                                   优先级max
    TRIE_4_BLANK_R,             //"??ooo?o",  1300,  1030,                      4
    TRIE_4_CONTINUE_DEAD,       //"?oooox",   1211,  1001,                      4                                   优先级max，一颗堵完，对方的：优先级max；自己的：优先级可以缓一下
    TRIE_4_CONTINUE_DEAD_R,     //"xoooo?",   1211,  1001,                      4
    TRIE_4_BLANK_DEAD,		    //"ooo?o",    1210,  999,                       4                                   优先级max，一颗堵完
    TRIE_4_BLANK_DEAD_R,        //"o?ooo",    1210,  999,                       4
    TRIE_4_BLANK_M,			    //"oo?oo",    1210,  999,                       4                                   优先级max，一颗堵完
    TRIE_3_CONTINUE,			//"?ooo??",   1100,  1200,                      3                                   活三
    TRIE_3_CONTINUE_R,			//"??ooo?",   1100,  1200,                      4                                   活三
    TRIE_3_BLANK,			    //"?o?oo?",   1080,  100,                       5                                   活三
    TRIE_3_BLANK_R,			    //"?oo?o?",   1080,  100,                       5                                   活三
    TRIE_3_BLANK_DEAD2,		    //"?oo?ox",   10,    10,                        4
    TRIE_3_BLANK_DEAD2_R,		//"xo?oo?",   10,    10,                        4
    TRIE_3_CONTINUE_F,		    //"?ooo?",    20,    20,                        3                                   假活三
    TRIE_3_CONTINUE_DEAD,	    //"??ooox",   20,    20,                        4
    TRIE_3_CONTINUE_DEAD_R,	    //"xooo??",   20,    20,                        3
    TRIE_3_BLANK_DEAD1,		    //"?o?oox",   5,     5,                         4
    TRIE_3_BLANK_DEAD1_R,		//"xoo?o?",   5,     5,                         4
    TRIE_2_CONTINUE,			//"?oo?",     35,    10,                        2
    TRIE_2_BLANK,			    //"?o?o?",    30,    5,                         3
    TRIE_COUNT
};

//棋型
#define FORMAT_LENGTH  11
#define FORMAT_LAST_INDEX 10
#define SEARCH_LENGTH  5
#define SEARCH_MIDDLE  4

/*
char* pat;
int evaluation;
int evaluation_defend;
uint8_t range;
uint8_t pat_len;
*/
struct ChessModeData
{
    char* pat;
    int evaluation;
    int evaluation_defend;
    uint8_t left_offset;//从左到右，保证棋型包含最中间的那颗棋子
    uint8_t pat_len;
};

struct SearchResult
{
    int chessMode;
    int pos;
};

const ChessModeData chessMode[TRIE_COUNT] = {
    { "oooooo",   100000,100000,5, 6 },
    { "ooooo",    100000,100000,4, 5 },
    { "o?ooo?o",  12000, 12000, 4, 7 },
    { "oo?oo?oo", 12000, 12000, 4, 8 },
    { "ooo?o?ooo",12000, 12000, 4, 9 },
    { "o?oooo?",  12000, 12000, 5, 7 },
    { "?oooo?o",  12000, 12000, 4, 7 },
    { "oo?ooo??", 1300,  1030,  5, 8 },
    { "??ooo?oo", 1300,  1030,  4, 8 },
    { "o?oooox",  1211,  1000,  5, 7 },
    { "xoooo?o",  1211,  1000,  4, 7 },
    { "ooo?oo",   1210,  999,   5, 6 },
    { "oo?ooo",   1210,  999,   5, 6 },
    { "?oooo?",   12000, 12000, 4, 6 },
    { "o?ooo??",  1300,  1030,  4, 7 },
    { "??ooo?o",  1300,  1030,  4, 7 },
    { "?oooox",   1211,  1001,  4, 6 },
    { "xoooo?",   1211,  1001,  4, 6 },
    { "ooo?o",    1210,  999,   4, 5 },
    { "o?ooo",    1210,  999,   4, 5 },
    { "oo?oo",    1210,  999,   4, 5 },
    { "?ooo??",   1100,  1200,  3, 6 },
    { "??ooo?",   1100,  1200,  4, 6 },
    { "?o?oo?",   1080,  100,   5, 6 },
    { "?oo?o?",   1080,  100,   5, 6 },
    { "?oo?ox",   25,    25,    4, 6 },
    { "xo?oo?",   25,    25,    4, 6 },
    { "?ooo?",    10,    10,    3, 5 },
    { "??ooox",   10,    10,    4, 6 },
    { "xooo??",   10,    10,    3, 6 },
    { "?o?oox",   10,    10,    4, 6 },
    { "xoo?o?",   10,    10,    4, 6 },
    { "?oo?",     10,    10,    2, 4 },
    { "?o?o?",    10,    10,    3, 5 },
};

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

    static TrieTreeNode* getInstance()
    {
        static TrieTreeNode* instance = NULL;
        if (instance == NULL)
        {
            instance = new TrieTreeNode();
        }
        return instance;
    }

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