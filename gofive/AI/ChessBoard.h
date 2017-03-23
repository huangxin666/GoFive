#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#include "utils.h"

class ChessBoard
{
public:
    ChessBoard();
    ~ChessBoard();
    inline Piece &getPiece(int row, int col) {
        return pieces[row][col];
    }
    inline Piece &getLastPiece() {
        return pieces[lastStep.uRow][lastStep.uCol];
    }
    inline int getStepScores(int row, int col, int state, bool isdefend)
    {
        return algType == 1 ? getStepScoresKMP(row, col, state, isdefend) : getStepScoresTrie(row, col, state, isdefend);
    }
    inline int getLastStepScores(bool isdefend)
    {
        return getStepScores(lastStep.uRow, lastStep.uCol, lastStep.getColor(), isdefend);
    }
    int getStepScoresKMP(int row, int col, int state, bool isdefend);
    int getStepScoresTrie(int row, int col, int state, bool isdefend);
    bool doNextStep(int row, int col, int side);
    void resetHotArea();//重置搜索区（悔棋专用）
    void updateHotArea(int row, int col);
    ThreatInfo getThreatInfo(int side);
    int getStepSituation(int row, int col, int state);
    void setGlobalThreat();//代价为一次全扫getStepScores*2
    void setThreat(int row, int col, int side, bool defend = true);//代价为一次getStepScores
    void updateThreat(int side = 0, bool defend = true);
    void updateThreat(int row, int col, int side, bool defend = true);
    int getAtackScore(int currentScore, int threat);
    int getAtackScoreHelp(int row, int col, int color, int &resultScore, char irow, char icol);
    bool getDirection(int& row, int& col, int i, int direction);
    void formatChess2String(char chessStr[][FORMAT_LENGTH], int row, int col, int state, bool reverse = false);
    int handleSpecial(SearchResult result, int state, uint8_t chessModeCount[TRIE_COUNT]);
    static bool buildTrieTree();
    static void setBan(bool ban);
    static void setLevel(int8_t level);
public:
    Piece pieces[BOARD_ROW_MAX][BOARD_COL_MAX];
    STEP lastStep;
    static TrieTreeNode* searchTrieTree;
    static bool ban;
    static int8_t level;
    static int8_t algType;
};

#endif 