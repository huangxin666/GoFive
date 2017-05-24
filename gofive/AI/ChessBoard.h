#ifndef CHESSBOARD_H
#define CHESSBOARD_H
#include <functional>

#include "defines.h"
#include "TrieTree.h"


struct Piece
{
    int blackscore;
    int whitescore;
    int8_t state;	    //格子状态：0表示无子；1表示黑；-1表示白	
    bool hot;			//是否应被搜索
public:
    Piece() :hot(false), state(0), blackscore(0), whitescore(0) { };
    inline void clearThreat() {
        blackscore = 0;
        blackscore = 0;
    };
    inline void setThreat(int score, int side) {
        // 0为黑棋 1为白棋
        if (side == 1) {
            blackscore = score;
        }
        else if (side == -1) {
            whitescore = score;
        }
    };
    // 0为黑棋 1为白棋
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
    return p.z32key ^ (p.z64key >> 32) ^ (p.z64key & 0xffffffff);
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
    void resetHotArea();//重置搜索区（悔棋专用）
    void updateHotArea(int row, int col);
    RatingInfo getRatingInfo(int side);
    void setGlobalThreat(bool defend = true);//代价为一次全扫getStepScores*2
    int setThreat(const int& row, const int& col, const int& side, bool defend = true);//代价为一次getStepScores  
    int updateThreat(const int& row, const int& col, const int& side, bool defend = true);
    bool nextPosition(int& row, int& col, int i, int direction);
    void formatChess2Int(uint32_t chessInt[DIRECTION4_COUNT], const int& row, const int& col, const int& state);
    void formatChessInt(uint32_t chessInt, char chessStr[FORMAT_LENGTH]);
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

    static bool ban;
    static void setBan(bool ban);
    static int8_t level;
    static void setLevel(int8_t level);

    //5[32][5];//2^5
    //6[64][6];
    //7[128][7];
    //8[256][8];
    //9[512][9];
    //10[1024][10];
    //11[2048][11];
    //12[4096][12];
    //13[8192][13];
    //14[16384][14];
    //15[32768][15];
    static uint8_t* chessModeHashTable[16];
    static void initChessModeHashTable();
    static int normalTypeHandleSpecial(SearchResult result);
    static CHESSMODE_BASE normalType2HashType(int chessModeType);

    //ban 取决于ban开关和是否黑方
    int hashTypeBase2ADV(uint8_t type, bool ban);

    void init_layer2();

    void update_layer2()
    {
        update_layer2(Position2(lastStep.row, lastStep.col), PIECE_BLACK);
        update_layer2(Position2(lastStep.row, lastStep.col), PIECE_WHITE);
    }

    void update_layer2(Position2 index, int side);
    void updatePoint_layer3(Position2 index)
    {
        updatePoint_layer3(index, PIECE_BLACK);
        updatePoint_layer3(index, PIECE_WHITE);
    }
    void updatePoint_layer3(Position2 index,int side);
    void updateArea_layer3(Position2 index)
    {
        updateArea_layer3(index, PIECE_BLACK);
        updateArea_layer3(index, PIECE_WHITE);
    }
    void updateArea_layer3(Position2 index, int side);
    bool move(Position2 index, int side);
    bool unmove();


    static string debugInfo;
public:
    Piece pieces[BOARD_ROW_MAX][BOARD_COL_MAX];
    uint8_t pieces_hot[256];
    uint8_t pieces_layer1[256];
    uint8_t pieces_layer2[256][4][2];
    uint8_t pieces_layer3[256][2];
    ChessStep lastStep;
};

#endif 