#ifndef __CHESSBOARD_H__
#define __CHESSBOARD_H__

#include "defines.h"

struct HashPair
{
    uint32_t check_key;
    uint32_t hash_key;
};

struct PieceInfo
{
    Position pos;
    uint8_t chesstype;
};

struct ChessTypeInfo
{
    int32_t rating;
    int16_t atackBaseFactor;
    int16_t defendBaseFactor;
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
    inline PieceInfo getHighestInfo(uint8_t side)
    {
        if (update_info_flag[side] != NONEED)
        {
            updateChessInfo(side);
        }
        return highestRatings[side];
    }
    inline HashPair getBoardHash()
    {
        return hash;
    }

    void formatChess2Int(char chessInt[DIRECTION4_COUNT][11], int row, int col, int state);

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

    int getRelatedFactor(Position pos, uint8_t side, bool defend = false);

    double getStaticFactor(Position pos, uint8_t side, bool defend = false);

    int getGlobalEvaluate(uint8_t side, int weight = 100);

    int getSimpleTotalScore(uint8_t side);

    static ChessTypeInfo getChessTypeInfo(uint8_t type);

public:
    void printGlobalEvaluate(string &s);
    static void initZobrist();
    static bool ban;
    static void setBan(bool ban);

    static uint8_t* chessModeHashTable[BOARD_SIZE_MAX + 1];
    static uint8_t* chessModeHashTableBan[BOARD_SIZE_MAX + 1];

    static void initChessModeHashTable();

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

    void update_layer3_with_layer2(int8_t row, int8_t col, int side, uint8_t direction, int len, int chessHashIndex);

    void updatePoint_layer3(int8_t row, int8_t col, int side);
    void updateArea_layer3(int8_t row, int8_t col)
    {
        updateArea_layer3(row, col, PIECE_BLACK);
        updateArea_layer3(row, col, PIECE_WHITE);
    }
    void updateArea_layer3(int8_t row, int8_t col, uint8_t side);

    void updateChessInfo(uint8_t side);

public:
    uint8_t pieces_layer1[BOARD_SIZE_MAX][BOARD_SIZE_MAX];
    uint8_t pieces_layer2[BOARD_SIZE_MAX][BOARD_SIZE_MAX][4][2];
    uint8_t pieces_layer3[BOARD_SIZE_MAX][BOARD_SIZE_MAX][2];
    ChessStep lastStep;
    HashPair hash;

private:
    PieceInfo highestRatings[2];
    enum UpdateFlag
    {
        NONEED,
        NEED,
        UNSURE //代表老的最高分被更新了并且比原来分数低
    };
    uint8_t update_info_flag[2] = { NONEED,NONEED };
    static uint32_t zkey[BOARD_SIZE_MAX][BOARD_SIZE_MAX][PIECE_TYPE_COUNT];
    static uint32_t zcheck[BOARD_SIZE_MAX][BOARD_SIZE_MAX][PIECE_TYPE_COUNT];
};

#endif 