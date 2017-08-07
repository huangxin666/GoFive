#ifndef __TRIETREE_H__
#define __TRIETREE_H__
#include <cstddef>
#include <cstdint>
#include <string>
using namespace std;

//先采用长的覆盖短的策略，（或者可以使用上面的覆盖下面的）
enum CHESSMODE2
{
    TRIE_6_CONTINUE,		    //"oooooo",   5                                   特殊棋型,禁手,非禁手等同于TRIE_5_CONTINUE
    TRIE_5_CONTINUE,			//"ooooo",    4
    TRIE_4_DOUBLE_BAN1,         //"o?ooo?o",  4                                   特殊棋型
    TRIE_4_DOUBLE_BAN2,         //"oo?oo?oo", 4                                   特殊棋型
    TRIE_4_DOUBLE_BAN3,         //"ooo?o?ooo",4                                   特殊棋型
    TRIE_4_CONTINUE_BAN,        //"o?oooo?",  5                                   特殊棋型
    TRIE_4_CONTINUE_BAN_R,      //"?oooo?o",  4                                   特殊棋型
    TRIE_4_BLANK_BAN,           //"oo?ooo??", 5                                   特殊棋型
    TRIE_4_BLANK_BAN_R,         //"??ooo?oo", 4                                   特殊棋型
    TRIE_4_CONTINUE_DEAD_BAN,   //"o?oooox",  5                                   特殊棋型
    TRIE_4_CONTINUE_DEAD_BAN_R, //"xoooo?o",  4                                   特殊棋型
    TRIE_4_BLANK_DEAD_BAN,      //"ooo?oo",   5                                   特殊棋型
    TRIE_4_BLANK_DEAD_BAN_R,    //"oo?ooo",   5                                   特殊棋型
    TRIE_4_CONTINUE,			//"?oooo?",   4
    TRIE_4_BLANK,			    //"o?ooo??",  4                                   优先级max
    TRIE_4_BLANK_R,             //"??ooo?o",  4
    TRIE_4_CONTINUE_DEAD,       //"?oooox",   4                                   优先级max，一颗堵完，对方的：优先级max；自己的：优先级可以缓一下
    TRIE_4_CONTINUE_DEAD_R,     //"xoooo?",   4
    TRIE_4_BLANK_DEAD,		    //"ooo?o",    4                                   优先级max，一颗堵完
    TRIE_4_BLANK_DEAD_R,        //"o?ooo",    4
    TRIE_4_BLANK_M,			    //"oo?oo",    4                                   优先级max，一颗堵完
    TRIE_3_CONTINUE_TRUE,       //"??ooo??",  4                                   活三   两防点  
    TRIE_3_CONTINUE,			//"x?ooo??",  4                                   活三   三防点
    TRIE_3_CONTINUE_R,			//"??ooo?x",  4                                   活三   三防点
    TRIE_3_BLANK,			    //"?o?oo?",   5                                   活三   三防点
    TRIE_3_BLANK_R,			    //"?oo?o?",   5                                   活三   三防点
    TRIE_3_BLANK_DEAD2,		    //"?oo?ox",   4
    TRIE_3_BLANK_DEAD2_R,		//"xo?oo?",   4
    TRIE_3_CONTINUE_F,		    //"?ooo?",    3                                   假活三
    TRIE_3_CONTINUE_DEAD,	    //"??ooox",   4
    TRIE_3_CONTINUE_DEAD_R,	    //"xooo??",   3
    TRIE_3_BLANK_DEAD1,		    //"?o?oox",   4
    TRIE_3_BLANK_DEAD1_R,		//"xoo?o?",   4
    TRIE_2_CONTINUE,			//"?oo??",    2
    TRIE_2_CONTINUE_R,			//"??oo?",    2
    TRIE_2_BLANK,			    //"?o?o?",    3
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
    uint8_t left_offset;//从左到右，保证棋型包含最中间的那颗棋子
    uint8_t pat_len;
};

struct SearchResult
{
    int chessMode;
    int pos;
};

const ChessModeData chessMode[TRIE_COUNT] = {
    { "oooooo",   5, 6 },
    { "ooooo",    4, 5 },
    { "o?ooo?o",  4, 7 },
    { "oo?oo?oo", 4, 8 },
    { "ooo?o?ooo",4, 9 },
    { "o?oooo?",  5, 7 },
    { "?oooo?o",  4, 7 },
    { "oo?ooo??", 5, 8 },
    { "??ooo?oo", 4, 8 },
    { "o?oooox",  5, 7 },
    { "xoooo?o",  4, 7 },
    { "ooo?oo",   5, 6 },
    { "oo?ooo",   5, 6 },
    { "?oooo?",   4, 6 },
    { "o?ooo??",  4, 7 },
    { "??ooo?o",  4, 7 },
    { "?oooox",   4, 6 },
    { "xoooo?",   4, 6 },
    { "ooo?o",    4, 5 },
    { "o?ooo",    4, 5 },
    { "oo?oo",    4, 5 },
    { "??ooo??",  4, 7 },
    { "x?ooo??",  4, 7 },
    { "??ooo?x",  4, 7 },
    { "?o?oo?",   5, 6 },
    { "?oo?o?",   5, 6 },
    { "?oo?ox",   4, 6 },
    { "xo?oo?",   4, 6 },
    { "?ooo?",    3, 5 },
    { "??ooox",   4, 6 },
    { "xooo??",   3, 6 },
    { "?o?oox",   4, 6 },
    { "xoo?o?",   4, 6 },
    { "?oo??",    2, 5 },
    { "??oo?",    3, 5 },
    { "?o?o?",    3, 5 },
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