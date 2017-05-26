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

    inline bool isHot(uint8_t row, uint8_t col)
    {
        return pieces_hot[Util::xy2index(row, col)];
    }
    inline void setHot(uint8_t row, uint8_t col, bool hot)
    {
        pieces_hot[Util::xy2index(row, col)] = hot;
    }
    inline uint8_t getState(uint8_t row, uint8_t col)
    {
        return pieces_layer1[Util::xy2index(row, col)];
    }
    inline uint8_t setState(uint8_t row, uint8_t col, uint8_t state)
    {
        pieces_layer1[Util::xy2index(row, col)] = state;
    }

    inline int getThreat(uint8_t row, uint8_t col, uint8_t side)
    {
        if (side == PIECE_BLACK)
        {
            return pieces_layer3[Util::xy2index(row, col)][side];
        }
        else if (side == PIECE_WHITE)
        {
            return pieces_layer3[Util::xy2index(row, col)][side];
        }
        else
        {
            return 0;
        }
    }

    void initHotArea();//����������������ר�ã�
    void updateHotArea(uint8_t index);

    int getTotalRating(uint8_t side)
    {
        return ratings[side];
    }

    PieceInfo getHighestInfo(uint8_t side)
    {
        PieceInfo result = { 0,0 };
        for (uint8_t index = 0; Util::valid(index); ++index)
        {
            if (pieces_hot[index] && pieces_layer1[index] == PIECE_BLANK)
            {
                if (chess_ratings[pieces_layer3[index][side]] > chess_ratings[result.chessmode])
                {
                    result.chessmode = pieces_layer3[index][side];
                    result.index = index;
                }
            }
        }
        return result;
    }



    //int getStepScores(const int& row, const int& col, const int& state, const bool& isdefend);
    //bool doNextStep(const int& row, const int& col, const int& side);
    //void setGlobalThreat(bool defend = true);//����Ϊһ��ȫɨgetStepScores*2
    //int setThreat(const int& row, const int& col, const int& side, bool defend = true);//����Ϊһ��getStepScores  
    //int updateThreat(const int& row, const int& col, const int& side, bool defend = true);
    void formatChess2Int(uint32_t chessInt[DIRECTION4_COUNT], int row, int col, int state);
    //int handleSpecial(const SearchResult &result, const int &state, uint8_t chessModeCount[TRIE_COUNT]);

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
    static CHESSMODE normalType2HashType(int chessModeType, bool ban);

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
    void updateArea_layer3(uint8_t index, int side);

    void initRatings();

    bool move(uint8_t index, int side);
    bool unmove(uint8_t index);


    static string debugInfo;
public:
    //Piece pieces[BOARD_ROW_MAX][BOARD_COL_MAX];

    uint8_t pieces_layer1[256] = { 0 };
    uint8_t pieces_layer2[256][4][2] = { 0 };
    uint8_t pieces_layer3[256][2] = { 0 };

    bool pieces_hot[256] = { false };
    //ChessStep lastStep;
    HashPair hash;
    int ratings[2];
};

#endif 