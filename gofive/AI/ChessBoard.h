#ifndef CHESSBOARD_H
#define CHESSBOARD_H
#include <functional>

#include "defines.h"
#include "TrieTree.h"


struct HashPair
{
    uint32_t z32key;
    uint64_t z64key;
};

class ChessBoard
{
public:
    ChessBoard();
    ~ChessBoard();

    inline bool isHot(uint8_t row, uint8_t col)
    {
        return pieces_hot[util::xy2index(row, col)];
    }
    inline void setHot(uint8_t row, uint8_t col, bool hot)
    {
        pieces_hot[util::xy2index(row, col)] = hot;
    }
    inline uint8_t getState(uint8_t row, uint8_t col)
    {
        return pieces_layer1[util::xy2index(row, col)];
    }
    inline uint8_t setState(uint8_t row, uint8_t col, uint8_t state)
    {
        pieces_layer1[util::xy2index(row, col)] = state;
    }

    inline uint8_t getChessType(int8_t row, int8_t col, uint8_t side)
    {
        return side == PIECE_BLANK ? 0 : pieces_layer3[util::xy2index(row, col)][side];
    }

    inline uint8_t getChessType(uint8_t index, uint8_t side)
    {
        return side == PIECE_BLANK ? 0 : pieces_layer3[index][side];
    }

    inline int getThreat(uint8_t index, uint8_t side)
    {
        return side == PIECE_BLANK ? 0 : util::type2score(pieces_layer3[index][side]);
    }

    inline int getThreat(int8_t row, int8_t col, uint8_t side)
    {
        return side == PIECE_BLANK ? 0 : util::type2score(pieces_layer3[util::xy2index(row, col)][side]);
    }

    inline bool canMove(int8_t row, int8_t col)
    {
        return pieces_hot[util::xy2index(row, col)] && pieces_layer1[util::xy2index(row, col)] == PIECE_BLANK;
    }

    inline bool canMove(uint8_t index)
    {
        return pieces_hot[index] && pieces_layer1[index] == PIECE_BLANK;
    }
    inline ChessStep getLastStep()
    {
        return lastStep;
    }

    void initHotArea();//重置搜索区（悔棋专用）
    void updateHotArea(uint8_t index);
    int getUpdateThreat(uint8_t index, uint8_t side);
    int getTotalRating(uint8_t side)
    {
        return totalRatings[side];
    }

    int getHighestScore(uint8_t side)
    {
        return util::type2score(highestRatings[side].chessmode);
    }

    PieceInfo getHighestInfo(uint8_t side)
    {
        return highestRatings[side];
    }

    int getSituationRating(uint8_t side)//局面评估
    {
        int rating = totalRatings[side] - totalRatings[util::otherside(side)];
        return (side == lastStep.getColor()) ? (rating - util::type2score(highestRatings[lastStep.getColor()].chessmode)) : (rating + util::type2score(highestRatings[lastStep.getColor()].chessmode));
    }

    //int getStepScores(const int& row, const int& col, const int& state, const bool& isdefend);
    //bool doNextStep(const int& row, const int& col, const int& side);
    //void setGlobalThreat(bool defend = true);//代价为一次全扫getStepScores*2
    //int setThreat(const int& row, const int& col, const int& side, bool defend = true);//代价为一次getStepScores  
    //int updateThreat(const int& row, const int& col, const int& side, bool defend = true);
    //int handleSpecial(const SearchResult &result, const int &state, uint8_t chessModeCount[TRIE_COUNT]);

    void formatChess2Int(uint32_t chessInt[DIRECTION4_COUNT], int row, int col, int state);

    void initBoard();
    bool nextPosition(int& row, int& col, int i, int direction);
    void formatChessInt(uint32_t chessInt, char chessStr[FORMAT_LENGTH]);
    void initHash();
    void updateHashPair(uint8_t row, uint8_t col, uint8_t side, bool add = true);
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
    static uint8_t* chessModeHashTableBan[16];

    static void initChessModeHashTable();
    static int normalTypeHandleSpecial(SearchResult result);
    static CHESSTYPE normalType2HashType(int chessModeType, bool ban);

    void init_layer1();

    void init_layer2();

    void update_layer2(uint8_t index)
    {
        update_layer2(index, PIECE_BLACK);
        update_layer2(index, PIECE_WHITE);
    }

    void update_layer2(uint8_t index, int side);

    void init_layer3();

    void updatePoint_layer3(uint8_t index)
    {
        updatePoint_layer3(index, PIECE_BLACK);
        updatePoint_layer3(index, PIECE_WHITE);
    }
    void updatePoint_layer3(uint8_t index, int side);
    void updateArea_layer3(uint8_t index)
    {
        updateArea_layer3(index, PIECE_BLACK);
        updateArea_layer3(index, PIECE_WHITE);
    }
    void updateArea_layer3(uint8_t index, uint8_t side);

    void initTotalRatings();
    void initHighestRatings();

    bool move(uint8_t index);
    bool unmove(uint8_t index, uint8_t lastIndex);


    static string debugInfo;
public:
    //Piece pieces[BOARD_ROW_MAX][BOARD_COL_MAX];

    uint8_t pieces_layer1[256] = { 0 };
    uint8_t pieces_layer2[256][4][2] = { 0 };
    uint8_t pieces_layer3[256][2] = { 0 };

    bool pieces_hot[256] = { false };
    ChessStep lastStep;
    HashPair hash;
    int totalRatings[2];
    PieceInfo highestRatings[2];
};

#endif 