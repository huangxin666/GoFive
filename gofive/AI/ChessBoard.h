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
    csidx index;
    uint8_t chesstype;
};

class ChessBoard
{
public:
    ChessBoard();
    ~ChessBoard();

    inline uint8_t getState(csidx index)
    {
        return pieces_layer1[index];
    }
    inline uint8_t getState(uint8_t row, uint8_t col)
    {
        return pieces_layer1[Util::xy2index(row, col)];
    }
    inline uint8_t setState(uint8_t row, uint8_t col, uint8_t state)
    {
        pieces_layer1[Util::xy2index(row, col)] = state;
    }
    inline uint8_t getChessType(int8_t row, int8_t col, uint8_t side)
    {
        return side == PIECE_BLANK ? 0 : pieces_layer3[Util::xy2index(row, col)][side];
    }
    inline uint8_t getChessType(csidx index, uint8_t side)
    {
        return side == PIECE_BLANK ? 0 : pieces_layer3[index][side];
    }
    inline bool canMove(csidx index)
    {
        return pieces_layer1[index] == PIECE_BLANK;
    }
    inline bool canMove(int8_t row, int8_t col)
    {
        return canMove(Util::xy2index(row, col));
    }
    inline bool useful(csidx index)
    {
        return pieces_layer3[index][0] > CHESSTYPE_0 || pieces_layer3[index][1] > CHESSTYPE_0;
    }
    inline bool useful(int8_t row, int8_t col)
    {
        return useful(Util::xy2index(row, col));
    }
    inline ChessStep getLastStep()
    {
        return lastStep;
    }
    inline int getTotalRating(uint8_t side)
    {
        if (!update_info_flag[side])
        {
            initChessInfo(side);
        }
        return totalRatings[side];
    }
    inline PieceInfo getHighestInfo(uint8_t side)
    {
        if (!update_info_flag[side])
        {
            initChessInfo(side);
        }
        return highestRatings[side];
    }
    inline HashPair getBoardHash()
    {
        return hash;
    }

    int getSituationRating(uint8_t side);//局面评估,不好评

    void formatChess2Int(uint32_t chessInt[DIRECTION4_COUNT], int row, int col, int state);

    void initBoard();

    void initHash();

    void updateHashPair(csidx index, uint8_t side, bool add);

    void getAtackReletedPos(set<csidx>& releted, csidx center, uint8_t side);

    bool moveNull();
    bool move(csidx index);
    bool move(int8_t row, int8_t col)
    {
        return move(Util::xy2index(row, col));
    }
    bool unmove(csidx index, ChessStep last);
    bool unmove(int8_t row, int8_t col, ChessStep last)
    {
        return unmove(Util::xy2index(row, col), last);
    }

    double getRelatedFactor(csidx index, uint8_t side, bool defend = false);

    double getStaticFactor(csidx index, uint8_t side, bool defend = false);

    int getGlobalEvaluate(uint8_t side);

public:
    void printGlobalEvaluate(string &s);
    static uint32_t z32[BOARD_ROW_MAX*BOARD_COL_MAX][3];
    static uint64_t z64[BOARD_ROW_MAX*BOARD_COL_MAX][3];
    static void initZobrist();

    static bool ban;
    static void setBan(bool ban);

    static uint8_t* chessModeHashTable[16];
    static uint8_t* chessModeHashTableBan[16];

    static void initChessModeHashTable();

    static string debugInfo;
private:

    void getAtackReletedPos2(set<csidx>& releted, csidx center, uint8_t side);

    void getBanReletedPos(set<csidx>& releted, csidx center, uint8_t side);

    void init_layer1();

    void init_layer2();

    void update_layer2(csidx index)
    {
        update_layer2(index, PIECE_BLACK);
        update_layer2(index, PIECE_WHITE);
    }

    void update_layer2(csidx index, int side);

    void init_layer3();

    void updatePoint_layer3(csidx index)
    {
        updatePoint_layer3(index, PIECE_BLACK);
        updatePoint_layer3(index, PIECE_WHITE);
    }
    void updatePoint_layer3(csidx index, int side);
    void updateArea_layer3(csidx index)
    {
        updateArea_layer3(index, PIECE_BLACK);
        updateArea_layer3(index, PIECE_WHITE);
    }
    void updateArea_layer3(csidx index, uint8_t side);

    void initChessInfo(uint8_t side);

public:
    uint8_t pieces_layer1[256] = { 0 };
    uint8_t pieces_layer2[256][4][2] = { 0 };
    uint8_t pieces_layer3[256][2] = { 0 };
    ChessStep lastStep;
    HashPair hash;

private:
    int totalRatings[2] = { 0 };
    PieceInfo highestRatings[2];
    bool update_info_flag[2] = { false,false };
};

#endif 