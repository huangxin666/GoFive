#ifndef __TRIETREE_H__
#define __TRIETREE_H__
#include <cstddef>
#include <cstdint>
#include <string>
using namespace std;

//�Ȳ��ó��ĸ��Ƕ̵Ĳ��ԣ������߿���ʹ������ĸ�������ģ�
enum CHESSMODE2
{
    TRIE_6_CONTINUE,		    //"oooooo",   5                                   ��������,����,�ǽ��ֵ�ͬ��TRIE_5_CONTINUE
    TRIE_5_CONTINUE,			//"ooooo",    4
    TRIE_4_DOUBLE_BAN1,         //"o?ooo?o",  4                                   ��������
    TRIE_4_DOUBLE_BAN2,         //"oo?oo?oo", 4                                   ��������
    TRIE_4_DOUBLE_BAN3,         //"ooo?o?ooo",4                                   ��������
    TRIE_4_CONTINUE_BAN,        //"o?oooo?",  5                                   ��������
    TRIE_4_CONTINUE_BAN_R,      //"?oooo?o",  4                                   ��������
    TRIE_4_BLANK_BAN,           //"oo?ooo??", 5                                   ��������
    TRIE_4_BLANK_BAN_R,         //"??ooo?oo", 4                                   ��������
    TRIE_4_CONTINUE_DEAD_BAN,   //"o?oooox",  5                                   ��������
    TRIE_4_CONTINUE_DEAD_BAN_R, //"xoooo?o",  4                                   ��������
    TRIE_4_BLANK_DEAD_BAN,      //"ooo?oo",   5                                   ��������
    TRIE_4_BLANK_DEAD_BAN_R,    //"oo?ooo",   5                                   ��������
    TRIE_4_CONTINUE,			//"?oooo?",   4
    TRIE_4_BLANK,			    //"o?ooo??",  4                                   ���ȼ�max
    TRIE_4_BLANK_R,             //"??ooo?o",  4
    TRIE_4_CONTINUE_DEAD,       //"?oooox",   4                                   ���ȼ�max��һ�Ŷ��꣬�Է��ģ����ȼ�max���Լ��ģ����ȼ����Ի�һ��
    TRIE_4_CONTINUE_DEAD_R,     //"xoooo?",   4
    TRIE_4_BLANK_DEAD,		    //"ooo?o",    4                                   ���ȼ�max��һ�Ŷ���
    TRIE_4_BLANK_DEAD_R,        //"o?ooo",    4
    TRIE_4_BLANK_M,			    //"oo?oo",    4                                   ���ȼ�max��һ�Ŷ���
    TRIE_3_CONTINUE_TRUE,       //"??ooo??",  4                                   ����   ������  
    TRIE_3_CONTINUE,			//"x?ooo??",  4                                   ����   ������
    TRIE_3_CONTINUE_R,			//"??ooo?x",  4                                   ����   ������
    TRIE_3_BLANK,			    //"?o?oo?",   5                                   ����   ������
    TRIE_3_BLANK_R,			    //"?oo?o?",   5                                   ����   ������
    TRIE_3_BLANK_DEAD2,		    //"?oo?ox",   4
    TRIE_3_BLANK_DEAD2_R,		//"xo?oo?",   4
    TRIE_3_CONTINUE_F,		    //"?ooo?",    3                                   �ٻ���
    TRIE_3_CONTINUE_DEAD,	    //"??ooox",   4
    TRIE_3_CONTINUE_DEAD_R,	    //"xooo??",   3
    TRIE_3_BLANK_DEAD1,		    //"?o?oox",   4
    TRIE_3_BLANK_DEAD1_R,		//"xoo?o?",   4
    TRIE_2_CONTINUE,			//"?oo??",    2
    TRIE_2_CONTINUE_R,			//"??oo?",    2
    TRIE_2_BLANK,			    //"?o?o?",    3
    TRIE_COUNT
};

//����
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
    uint8_t left_offset;//�����ң���֤���Ͱ������м���ǿ�����
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