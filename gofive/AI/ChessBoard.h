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
    int rating;
    int atackBaseFactor;
    int defendBaseFactor;
};
const int around_max_offset = 3;

const uint16_t layer2_mask[4] = { 0xfff0,0xff0f,0xf0ff,0x0fff };

class ChessBoard
{
public:
    ChessBoard();
    ~ChessBoard();

    inline uint8_t getState(Position pos)
    {
        return pieces[pos.row][pos.col].layer1;
    }
    inline uint8_t getLayer2(Position pos, uint8_t side, uint8_t d)
    {
        return (pieces[pos.row][pos.col].layer2[side] >> (4 * d)) & 0xf;
        //return pieces[pos.row][pos.col].layer2[side][d];
    }
    inline uint8_t getChessType(Position pos, uint8_t side)
    {
        return side == PIECE_BLANK ? 0 : pieces[pos.row][pos.col].layer3[side];
    }
    inline uint8_t getChessDirection(Position pos, uint8_t side)
    {
        uint8_t ret = 0;
        uint8_t best = 0;
        for (uint8_t d = 0; d < DIRECTION4::DIRECTION4_COUNT; ++d)
        {
            if (pieces[pos.row][pos.col].layer3[side] == getLayer2(pos, side, d))
            {
                return d;
            }
            else if (getLayer2(pos, side, d) > best)
            {
                best = getLayer2(pos, side, d);
                ret = d;
            }
        }
        return ret;
    }

    inline bool useful(Position pos)
    {
        return pieces[pos.row][pos.col].layer3[PIECE_BLACK] > CHESSTYPE_0 || pieces[pos.row][pos.col].layer3[PIECE_WHITE] > CHESSTYPE_0;
    }

    inline ChessStep getLastStep()
    {
        return lastStep;
    }

    inline uint8_t getHighestType(uint8_t side)
    {
        for (uint8_t i = CHESSTYPE_5; i > 0; --i)
        {
            if (global_chesstype_count[side][i] > 0) return i;
        }
        return CHESSTYPE_0;
    }

    inline bool hasChessType(uint8_t side, uint8_t type)
    {
        return global_chesstype_count[side][type] > 0;
    }

    inline uint8_t countChessType(uint8_t side, uint8_t type)
    {
        return global_chesstype_count[side][type];
    }

    inline HashPair getBoardHash()
    {
        return hash;
    }

    void initBoard();

    void initHash();

    void updateHashPair(Position pos, uint8_t side, bool add);

    bool moveNull();
    bool move(Position pos, uint8_t side, GAME_RULE ban);

    bool move(Position pos, GAME_RULE ban)
    {
        return move(pos, lastStep.getOtherSide(), ban);
    }

    bool unmove(Position pos, ChessStep last, GAME_RULE ban);

    bool moveMultiReplies(vector<Position> &moves, GAME_RULE ban);

    int getRelatedFactor(Position pos, uint8_t side, bool defend = false);

    double getStaticFactor(Position pos, uint8_t side);

    int getGlobalEvaluate(uint8_t side, int weight = 100);

    int getSimpleTotalScore(uint8_t side);

    void getFourkillDefendCandidates(Position pos, vector<StepCandidateItem>& moves, GAME_RULE ban);

    void getFourkillDefendCandidates(Position pos, vector<Position>& moves, GAME_RULE ban);

    void getThreatReplies(Position pos, uint8_t type, uint8_t direction, vector<Position>& reply, GAME_RULE ban);

    void getThreatReplies(Position pos, uint8_t type, uint8_t direction, Position* reply, uint8_t &num, GAME_RULE ban);

    void getThreatCandidates(int level, vector<StepCandidateItem>& moves, bool extend = false);

    void getDependentThreatCandidates(Position pos, int level, vector<StepCandidateItem>& moves, bool extend = false);

    size_t getNormalCandidates(vector<StepCandidateItem>& moves, bool atack);

    size_t getUsefulCandidates(vector<StepCandidateItem>& moves);

    size_t getPNCandidates(vector<StepCandidateItem>& moves, bool atack);

public:
    void printGlobalEvaluate(string &s);
    static ChessTypeInfo getChessTypeInfo(uint8_t type);
    static void initStaticHelper();
private:

    void getBanReletedPos(set<Position>& releted, Position center, uint8_t side);

    void init_layer1();

    void init_layer2();

    void init_layer3();

    void init_pattern();

    void update_layer(Position pos, uint8_t side, GAME_RULE ban);

    void update_layer_undo(Position pos, uint8_t side, GAME_RULE ban);

public:
    struct Piece
    {
        uint8_t layer1;
        //uint8_t layer2[2][4];
        uint16_t layer2[2];//4 * 4 -> CHESSTYPE_COUNT < 16
        uint8_t layer3[2];
        uint8_t pattern[2][4];
        //uint8_t around[2];//00000000 对应八个方向
    };

    Piece pieces[BOARD_SIZE_MAX][BOARD_SIZE_MAX];
    ChessStep lastStep;
    HashPair hash;

private:
    uint8_t global_chesstype_count[2][CHESSTYPE_COUNT] = { 0 };

    static uint32_t zkey[BOARD_SIZE_MAX][BOARD_SIZE_MAX][PIECE_TYPE_COUNT];
    static uint32_t zcheck[BOARD_SIZE_MAX][BOARD_SIZE_MAX][PIECE_TYPE_COUNT];
    static void initZobrist();

    static uint8_t* layer2_table[BOARD_SIZE_MAX + 1];
    static uint8_t* layer2_table_ban[BOARD_SIZE_MAX + 1];

    static uint8_t pattern_to_layer2_table[UINT8_MAX + 1][UINT8_MAX + 1];//2^8
    static uint8_t pattern_to_layer2_table_ban[UINT8_MAX + 1][UINT8_MAX + 1];//2^8
    static void initLayer2Table();

    //static uint8_t layer2_to_layer3_table[CHESSTYPE_COUNT][CHESSTYPE_COUNT][CHESSTYPE_COUNT][CHESSTYPE_COUNT][3];
    static uint8_t layer2_to_layer3_table[UINT16_MAX + 1][3];
    static double position_weight[UINT8_MAX + 1];//2^8 衡量一个threat的有效程度
    static double chessboard_weight[BOARD_SIZE_MAX][BOARD_SIZE_MAX];
    static void initPositionWeightTable();
    static void init2to3table();
    static uint8_t layer2_to_layer3(uint16_t d1, uint16_t d2, uint16_t d3, uint16_t d4, GAME_RULE ban);
    static void initPatternToLayer2Table();
};

#endif 