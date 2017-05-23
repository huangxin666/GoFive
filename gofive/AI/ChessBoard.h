#ifndef CHESSBOARD_H
#define CHESSBOARD_H
#include <functional>

#include "defines.h"
#include "TrieTree.h"


struct Piece
{
    int blackscore;
    int whitescore;
    int8_t state;	    //����״̬��0��ʾ���ӣ�1��ʾ�ڣ�-1��ʾ��	
    bool hot;			//�Ƿ�Ӧ������
public:
    Piece() :hot(false), state(0), blackscore(0), whitescore(0) { };
    inline void clearThreat() {
        blackscore = 0;
        blackscore = 0;
    };
    inline void setThreat(int score, int side) {
        // 0Ϊ���� 1Ϊ����
        if (side == 1) {
            blackscore = score;
        }
        else if (side == -1) {
            whitescore = score;
        }
    };
    // 0Ϊ���� 1Ϊ����
    inline int getThreat(int side) {
        if (side == 1) {
            return blackscore;
        }
        else if (side == -1) {
            return whitescore;
        }
        else if (side == 0) {
            return blackscore + whitescore;
        }
        else return 0;
    };
};


struct HashPair
{
    uint32_t z32key;
    uint64_t z64key;
};


inline bool operator==(const HashPair& a, const HashPair& b)
{
    return a.z32key == b.z32key && a.z64key == b.z64key;
}

inline size_t hash_value(const HashPair& p)
{
    return p.z32key ^ (p.z64key>>32) ^ (p.z64key & 0xffffffff);
}

class ChessBoard
{
public:
    ChessBoard();
    ~ChessBoard();
    inline Piece &getPiece(const int& row, const int& col) {
        return pieces[row][col];
    };
    inline Piece &getLastPiece() {
        return pieces[lastStep.row][lastStep.col];
    };
    inline int getLastStepScores(bool isdefend)
    {
        return getStepScores(lastStep.row, lastStep.col, lastStep.getColor(), isdefend);
    };
    inline int updateThreat(int side = 0, bool defend = true)
    {
        if (side == 0)
        {
            updateThreat(lastStep.row, lastStep.col, 1, defend);
            updateThreat(lastStep.row, lastStep.col, -1, defend);
            return 0;
        }
        else
        {
            return updateThreat(lastStep.row, lastStep.col, side, defend);
        }
    };
    int getStepScores(const int& row, const int& col, const int& state, const bool& isdefend);
    bool doNextStep(const int& row, const int& col, const int& side);
    void resetHotArea();//����������������ר�ã�
    void updateHotArea(int row, int col);
    RatingInfo getRatingInfo(int side);
    void setGlobalThreat(bool defend = true);//����Ϊһ��ȫɨgetStepScores*2
    int setThreat(const int& row, const int& col, const int& side, bool defend = true);//����Ϊһ��getStepScores  
    int updateThreat(const int& row, const int& col, const int& side, bool defend = true);
    bool nextPosition(int& row, int& col, int i, int direction);
    void formatChess2Int(uint32_t chessInt[DIRECTION4_COUNT], const int& row, const int& col, const int& state);
    void formatChessInt(uint32_t chessInt,char chessStr[FORMAT_LENGTH]);
    int handleSpecial(const SearchResult &result, const int &state, uint8_t chessModeCount[TRIE_COUNT]);
    string toString();
    HashPair toHash();
    void updateHashPair(HashPair &pair, const int& row, const int& col, const int& side);
public:
    static bool buildTrieTree();
    static TrieTreeNode* searchTrieTree;

    static uint32_t z32[BOARD_ROW_MAX][BOARD_COL_MAX][3];
    static uint64_t z64[BOARD_ROW_MAX][BOARD_COL_MAX][3];
    static void initZobrist();
#define CHESSMODE_LEN 5
#define CHESSMODE_TABLE_SIZE 512
    //���� 2^9 0000 00000  00 00Ϊ�������� 00000Ϊ����
    //00 ���� _
    //01 ֱ�Ӷ� x
    //10 �ӳ� o
    //11 ����
    static uint8_t chessModeTable[CHESSMODE_TABLE_SIZE][CHESSMODE_LEN];
    static void initChessModeTable();
    void updatePoint_layer2(Position2 index);
    void updateArea_layer2(Position2 index);
    void updatePoint_layer3(Position2 index);
    void updateArea_layer3(Position2 index);
    static bool ban;
    static void setBan(bool ban);

    static int8_t level;
    static void setLevel(int8_t level);

    static string debugInfo;
public:
    Piece pieces[BOARD_ROW_MAX][BOARD_COL_MAX];
    uint8_t pieces_layer1[256];
    uint8_t pieces_layer2[256][4][2];
    uint8_t pieces_layer3[256][2];
    ChessStep lastStep;
};

#endif 