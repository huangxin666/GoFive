#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#include "utils.h"
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

union board
{

};

class ChessBoard
{
public:
    ChessBoard();
    ~ChessBoard();
    inline Piece &getPiece(int row, int col) {
        return pieces[row][col];
    }
    inline Piece &getLastPiece() {
        return pieces[lastStep.row][lastStep.col];
    }
    /*inline int getStepScores(int row, int col, int state, bool isdefend)
    {
        return algType == 1 ? getStepScoresKMP(row, col, state, isdefend) : getStepScoresTrie(row, col, state, isdefend);
    }*/
    inline int getLastStepScores(bool isdefend)
    {
        return getStepScores(lastStep.row, lastStep.col, lastStep.getColor(), isdefend);
    }
    //int getStepScoresKMP(int row, int col, int state, bool isdefend);
    int getStepScores(int row, int col, int state, bool isdefend);
    bool doNextStep(int row, int col, int side);
    void resetHotArea();//重置搜索区（悔棋专用）
    void updateHotArea(int row, int col);
    ThreatInfo getThreatInfo(int side);
    int getChessModeDirection(int row, int col, int state);
    void setGlobalThreat(bool defend = true);//代价为一次全扫getStepScores*2
    void setThreat(int row, int col, int side, bool defend = true);//代价为一次getStepScores
    void updateThreat(int side = 0, bool defend = true);
    void updateThreat(int row, int col, int side, bool defend = true);
    int getAtackScore(int currentScore, int threat);
    int getAtackScoreHelp(int row, int col, int color, int &resultScore, int direction);
    bool applyDirection(int& row, int& col, int i, int direction);
    void formatChess2String(char chessStr[][FORMAT_LENGTH], int row, int col, int state, bool reverse = false);
    int handleSpecial(SearchResult &result, int &state, uint8_t chessModeCount[TRIE_COUNT]);
    static bool buildTrieTree();
    static void setBan(bool ban);
    static void setLevel(int8_t level);
    string toString();
public:
    Piece pieces[BOARD_ROW_MAX][BOARD_COL_MAX];
    ChessStep lastStep;
    static TrieTreeNode* searchTrieTree;
    static bool ban;
    static int8_t level;
    static string debugInfo;
};

#endif 