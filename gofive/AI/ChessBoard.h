#ifndef __CHESSBOARD_H__
#define __CHESSBOARD_H__

#include "defines.h"

struct HashPair
{
    uint32_t z32key;
    uint64_t z64key;
};

struct PieceInfo
{
    uint8_t index;
    uint8_t chessmode;
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

    inline uint8_t getState(uint8_t index)
    {
        return pieces_layer1[index];
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
    inline int getTotalRating(uint8_t side)
    {
        return totalRatings[side];
    }

    inline int getHighestScore(uint8_t side)
    {
        return util::type2score(highestRatings[side].chessmode);
    }

    inline PieceInfo getHighestInfo(uint8_t side)
    {
        return highestRatings[side];
    }

    inline int getSituationRating(uint8_t side)//局面评估,不好评
    {
        return (side == lastStep.getColor()) ? totalRatings[side] / 2 - totalRatings[util::otherside(side)] :
            totalRatings[side] - totalRatings[util::otherside(side)] / 2;
    }

    inline HashPair getBoardHash()
    {
        return hash;
    }

    //int getStepScores(const int& row, const int& col, const int& state, const bool& isdefend);
    //bool doNextStep(const int& row, const int& col, const int& side);
    //void setGlobalThreat(bool defend = true);//代价为一次全扫getStepScores*2
    //int setThreat(const int& row, const int& col, const int& side, bool defend = true);//代价为一次getStepScores  
    //int updateThreat(const int& row, const int& col, const int& side, bool defend = true);
    //int handleSpecial(const SearchResult &result, const int &state, uint8_t chessModeCount[TRIE_COUNT]);

    void formatChess2Int(uint32_t chessInt[DIRECTION4_COUNT], int row, int col, int state);

    void initBoard();
    static bool nextPosition(int& row, int& col, int8_t offset, uint8_t direction);
    void initHash();
    void updateHashPair(uint8_t row, uint8_t col, uint8_t side, bool add = true);
public:

    static uint32_t z32[BOARD_ROW_MAX][BOARD_COL_MAX][3];
    static uint64_t z64[BOARD_ROW_MAX][BOARD_COL_MAX][3];
    static void initZobrist();

    static bool ban;
    static void setBan(bool ban);
    static int8_t level;
    static void setLevel(int8_t level);


    static uint8_t* chessModeHashTable[16];
    static uint8_t* chessModeHashTableBan[16];

    static void initChessModeHashTable();

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

    bool inRelatedArea(uint8_t index, uint8_t lastindex);

    bool moveTemporary(uint8_t index);

    bool move(uint8_t index);
    bool move(int8_t row, int8_t col)
    {
        return move(util::xy2index(row, col));
    }
    bool unmove(uint8_t index, ChessStep last);
    bool unmove(int8_t row, int8_t col, ChessStep last)
    {
        return unmove(util::xy2index(row, col), last);
    }

    double getRelatedFactor(uint8_t index, uint8_t side, bool for_evaluate = false);

    int getGlobalEvaluate(uint8_t side);

    static string debugInfo;
public:

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