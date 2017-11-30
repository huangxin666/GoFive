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
class ChessBoard
{
public:
    ChessBoard();
    ~ChessBoard();

    inline uint8_t getState(Position pos)
    {
        return pieces[pos.row][pos.col].layer1;
    }
    inline uint8_t getState(int8_t row, int8_t col)
    {
        return pieces[row][col].layer1;
    }
    inline uint8_t getLayer2(int8_t row, int8_t col, uint8_t side, uint8_t d)
    {
        return pieces[row][col].layer2[side][d];
    }
    inline uint8_t getLayer2(Position pos, uint8_t side, uint8_t d)
    {
        return pieces[pos.row][pos.col].layer2[side][d];
    }
    inline uint8_t setState(int8_t row, int8_t col, uint8_t state)
    {
        pieces[row][col].layer1 = state;
    }
    inline uint8_t getChessType(int8_t row, int8_t col, uint8_t side)
    {
        return side == PIECE_BLANK ? 0 : pieces[row][col].layer3[side];
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
            if (pieces[pos.row][pos.col].layer3[side] == pieces[pos.row][pos.col].layer2[side][d])
            {
                return d;
            }
            else if (pieces[pos.row][pos.col].layer2[side][d] > best)
            {
                best = pieces[pos.row][pos.col].layer2[side][d];
                ret = d;
            }
        }
        return ret;
    }
    inline bool canMove(Position pos)
    {
        return pieces[pos.row][pos.col].layer1 == PIECE_BLANK;
    }
    inline bool canMove(int8_t row, int8_t col)
    {
        return pieces[row][col].layer1 == PIECE_BLANK;
    }
    inline bool useful(Position pos)
    {
        return pieces[pos.row][pos.col].layer3[0] > CHESSTYPE_0 || pieces[pos.row][pos.col].layer3[1] > CHESSTYPE_0;
    }
    inline bool useful(int8_t row, int8_t col)
    {
        return pieces[row][col].layer3[0] > CHESSTYPE_0 || pieces[row][col].layer3[1] > CHESSTYPE_0;
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

    void updateHashPair(int8_t row, int8_t col, uint8_t side, bool add);

    bool moveNull();
    bool move(int8_t row, int8_t col, uint8_t side, GAME_RULE ban);

    bool move(Position pos, GAME_RULE ban)
    {
        return move(pos.row, pos.col, lastStep.getOtherSide(), ban);
    }

    bool unmove(Position pos, ChessStep last, GAME_RULE ban);

    bool moveMultiReplies(vector<Position> &moves, GAME_RULE ban);

    int getRelatedFactor(Position pos, uint8_t side, bool defend = false);

    double getStaticFactor(Position pos, uint8_t side, bool defend = false);

    int getGlobalEvaluate(uint8_t side, int weight = 100);

    int getSimpleTotalScore(uint8_t side);

    void getFourkillDefendCandidates(Position pos, vector<StepCandidateItem>& moves, GAME_RULE ban);

    void getFourkillDefendCandidates(Position pos, vector<Position>& moves, GAME_RULE ban);

    void getThreatReplies(Position pos, uint8_t type, uint8_t direction, vector<Position>& reply, GAME_RULE ban);

    void getThreatReplies(Position pos, uint8_t type, uint8_t direction, Position* reply, uint8_t &num, GAME_RULE ban);

    void getThreatCandidates(int level, vector<StepCandidateItem>& moves, bool extend = false);

    void getDependentThreatCandidates(Position pos, int level, vector<StepCandidateItem>& moves, bool extend = false);

    size_t getNormalCandidates(vector<StepCandidateItem>& moves, bool atack);

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

    void update_layer(int8_t row, int8_t col, uint8_t side, GAME_RULE ban);

    void update_layer_undo(int8_t row, int8_t col, uint8_t side, GAME_RULE ban);

    void update_layer_old(int8_t row, int8_t col, GAME_RULE ban)
    {
        update_layer_old(row, col, PIECE_BLACK, ban);
        update_layer_old(row, col, PIECE_WHITE, ban);
    }
    void update_layer_old(int8_t row, int8_t col, uint8_t side, GAME_RULE ban);
    void update_layer3_old(int8_t row, int8_t col, uint8_t side, GAME_RULE ban, uint8_t direction, int len, int chessHashIndex);

public:
    //uint8_t pieces_layer1[BOARD_SIZE_MAX][BOARD_SIZE_MAX];
    //uint8_t pieces_layer2[BOARD_SIZE_MAX][BOARD_SIZE_MAX][4][2];
    //uint8_t pieces_layer3[BOARD_SIZE_MAX][BOARD_SIZE_MAX][2];
    //uint16_t pieces_pattern[BOARD_SIZE_MAX][BOARD_SIZE_MAX][4][2];
    //uint8_t pieces_pattern_offset[BOARD_SIZE_MAX][BOARD_SIZE_MAX][4][2][2];

    struct Piece
    {
        uint8_t layer1;
        uint8_t layer2[2][4];
        uint8_t layer3[2];
        uint8_t pattern[2][4];
        uint8_t around;//��Χ�հ���
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

    static uint8_t pattern_to_layer2_table[256][256];//2^8
    static uint8_t pattern_to_layer2_table_ban[256][256];//2^8
    static void initLayer2Table();

    static uint8_t layer2_to_layer3_table[CHESSTYPE_COUNT][CHESSTYPE_COUNT][CHESSTYPE_COUNT][CHESSTYPE_COUNT][3];
    static void init2to3table();
    static uint8_t layer2_to_layer3(uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4, GAME_RULE ban);
    static void initPatternToLayer2Table();

    static bool relatedsituation[5][5][5][5]; // left_blank left_chess right_blank right_chess 
    static void initRelatedSituation();
};

#endif 