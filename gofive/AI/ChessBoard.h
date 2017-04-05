#ifndef CHESSBOARD_H
#define CHESSBOARD_H
#include <functional>

#include "utils.h"
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
    /*inline int getStepScores(int row, int col, int state, bool isdefend)
    {
        return algType == 1 ? getStepScoresKMP(row, col, state, isdefend) : getStepScoresTrie(row, col, state, isdefend);
    }*/
    inline int getLastStepScores(bool isdefend)
    {
        return getStepScores(lastStep.row, lastStep.col, lastStep.getColor(), isdefend);
    };
    inline void updateThreat(int side = 0, bool defend = true)
    {
        if (side == 0)
        {
            updateThreat(lastStep.row, lastStep.col, 1, defend);
            updateThreat(lastStep.row, lastStep.col, -1, defend);
        }
        else
        {
            updateThreat(lastStep.row, lastStep.col, side, defend);
        }
    };
    int getStepScores(const int& row, const int& col, const int& state, const bool& isdefend);
    bool doNextStep(const int& row, const int& col, const int& side);
    void resetHotArea();//����������������ר�ã�
    void updateHotArea(int row, int col);
    RatingInfo getRatingInfo(int side);
    int getChessModeDirection(int row, int col, int state);
    void setGlobalThreat(bool defend = true);//����Ϊһ��ȫɨgetStepScores*2
    void setThreat(const int& row, const int& col, const int& side, bool defend = true);//����Ϊһ��getStepScores  
    void updateThreat(const int& row, const int& col, const int& side, bool defend = true);
    void updateThreat2(const int& row, const int& col, const int& side, bool defend = true);
    int getAtackScore(int currentScore, int threat);
    int getAtackScoreHelp(int row, int col, int color, int &resultScore, int direction);
    bool applyDirection(int& row, int& col, int i, int direction);
    void formatChess2String(char chessStr[][FORMAT_LENGTH], const int& row, const int& col, const int& state, bool reverse = false);
    int handleSpecial(const SearchResult &result, const int &state, uint8_t chessModeCount[TRIE_COUNT]);
    static bool buildTrieTree();
    static void initZobrist();
    static void setBan(bool ban);
    static void setLevel(int8_t level);
    string toString();
    HashPair toHash();
    void updateHashPair(HashPair &pair, const int& row, const int& col, const int& side);
public:
    Piece pieces[BOARD_ROW_MAX][BOARD_COL_MAX];
    ChessStep lastStep;
    static TrieTreeNode* searchTrieTree;
    static uint32_t z32[BOARD_ROW_MAX][BOARD_COL_MAX][3];
    static uint64_t z64[BOARD_ROW_MAX][BOARD_COL_MAX][3];
    static bool ban;
    static int8_t level;
    static string debugInfo;
};

#endif 