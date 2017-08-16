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
    Position pos;
    uint8_t chesstype;
};

struct ChessTypeInfo
{
    int32_t rating;
    double atackBaseFactor;
    double defendBaseFactor;
    int16_t atackPriority;
    int16_t defendPriority;
};

class ChessBoard
{
public:
    ChessBoard();
    ~ChessBoard();

    inline uint8_t getState(Position pos)
    {
        return pieces_layer1[pos.row][pos.col];
    }
    inline uint8_t getState(int8_t row, int8_t col)
    {
        return pieces_layer1[row][col];
    }
    inline uint8_t setState(int8_t row, int8_t col, uint8_t state)
    {
        pieces_layer1[row][col] = state;
    }
    inline uint8_t getChessType(int8_t row, int8_t col, uint8_t side)
    {
        return side == PIECE_BLANK ? 0 : pieces_layer3[row][col][side];
    }
    inline uint8_t getChessType(Position pos, uint8_t side)
    {
        return side == PIECE_BLANK ? 0 : pieces_layer3[pos.row][pos.col][side];
    }
    inline uint8_t getChessDirection(Position pos, uint8_t side)
    {
        for (uint8_t d = 0; d < DIRECTION4::DIRECTION4_COUNT; ++d)
        {
            if (pieces_layer3[pos.row][pos.col][side] == pieces_layer2[pos.row][pos.col][d][side])
            {
                return d;
            }
        }
        return 4;
    }
    inline bool canMove(Position pos)
    {
        return pieces_layer1[pos.row][pos.col] == PIECE_BLANK;
    }
    inline bool canMove(int8_t row, int8_t col)
    {
        return pieces_layer1[row][col] == PIECE_BLANK;
    }
    inline bool useful(Position pos)
    {
        return pieces_layer3[pos.row][pos.col][0] > CHESSTYPE_0 || pieces_layer3[pos.row][pos.col][1] > CHESSTYPE_0;
    }
    inline bool useful(int8_t row, int8_t col)
    {
        return pieces_layer3[row][col][0] > CHESSTYPE_0 || pieces_layer3[row][col][1] > CHESSTYPE_0;
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

    void formatChess2Int(uint32_t chessInt[DIRECTION4_COUNT], int row, int col, int state);

    void initBoard();

    void initHash();

    void updateHashPair(int8_t row, int8_t col, uint8_t side, bool add);

    void getAtackReletedPos(set<Position>& releted, Position center, uint8_t side);

    bool moveNull();
    bool move(int8_t row, int8_t col, uint8_t side);
    bool move(Position pos)
    {
        return move(pos.row, pos.col, lastStep.getOtherSide());
    }
    bool move(int8_t row, int8_t col)
    {
        return move(row, col, lastStep.getOtherSide());
    }
    
    bool unmove(int8_t row, int8_t col, ChessStep last);

    bool unmove(Position pos, ChessStep last)
    {
        return unmove(pos.row, pos.col, last);
    }

    double getRelatedFactor(Position pos, uint8_t side, bool defend = false);

    double getStaticFactor(Position pos, uint8_t side, bool defend = false);

    int getGlobalEvaluate(uint8_t side);

    static ChessTypeInfo getChessTypeInfo(uint8_t type);

public:
    void printGlobalEvaluate(string &s);
    static uint32_t z32[BOARD_SIZE_MAX][BOARD_SIZE_MAX][3];
    static uint64_t z64[BOARD_SIZE_MAX][BOARD_SIZE_MAX][3];
    static void initZobrist();

    static bool ban;
    static void setBan(bool ban);

    static uint8_t* chessModeHashTable[BOARD_SIZE_MAX + 1];
    static uint8_t* chessModeHashTableBan[BOARD_SIZE_MAX + 1];

    static void initChessModeHashTable();

    static string debugInfo;
private:

    void getAtackReletedPos2(set<Position>& releted, Position center, uint8_t side);

    void getBanReletedPos(set<Position>& releted, Position center, uint8_t side);

    void init_layer1();

    void init_layer2();

    void update_layer2(int8_t row, int8_t col)
    {
        update_layer2(row, col, PIECE_BLACK);
        update_layer2(row, col, PIECE_WHITE);
    }

    void update_layer2(int8_t row, int8_t col, uint8_t side);

    void init_layer3();

    void updatePoint_layer3(int8_t row, int8_t col)
    {
        updatePoint_layer3(row, col, PIECE_BLACK);
        updatePoint_layer3(row, col, PIECE_WHITE);
    }
    void updatePoint_layer3(int8_t row, int8_t col, int side);
    void updateArea_layer3(int8_t row, int8_t col)
    {
        updateArea_layer3(row, col, PIECE_BLACK);
        updateArea_layer3(row, col, PIECE_WHITE);
    }
    void updateArea_layer3(int8_t row, int8_t col, uint8_t side);

    void initChessInfo(uint8_t side);

public:
    uint8_t pieces_layer1[BOARD_SIZE_MAX][BOARD_SIZE_MAX] = { 0 };
    uint8_t pieces_layer2[BOARD_SIZE_MAX][BOARD_SIZE_MAX][4][2] = { 0 };
    uint8_t pieces_layer3[BOARD_SIZE_MAX][BOARD_SIZE_MAX][2] = { 0 };
    ChessStep lastStep;
    HashPair hash;

private:
    int totalRatings[2] = { 0 };
    PieceInfo highestRatings[2];
    bool update_info_flag[2] = { false,false };
};

#endif 