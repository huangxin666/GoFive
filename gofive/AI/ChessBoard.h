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
    inline Piece &getPiece(STEP step) {
        return pieces[step.uRow][step.uCol];
    }
    inline Piece &getLastPiece() {
        return pieces[lastStep.uRow][lastStep.uCol];
    }
    bool doNextStep(int row, int col, int side);
    void resetHotArea();//重置搜索区（悔棋专用）
    void updateHotArea(int row, int col);
    ThreatInfo getThreatInfo(int side);
    int getStepSituation(int row, int col, int state);
    int getStepScores(bool ban, bool isdefend);
    int getStepScores(int row, int col, int state, bool ban);
    int getStepScores(int row, int col, int state, bool ban, bool isdefend);
    int getStepScores2(int row, int col, int state, bool ban, bool isdefend);
    void setGlobalThreat(bool);//代价为一次全扫getStepScores*2
    void setThreat(int row, int col, int side, bool ban);//代价为一次getStepScores
    void updateThreat(bool ban, int = 0);
    void updateThreat(int row, int col, int side, bool ban);
    int getAtackScore(int currentScore, int threat, bool ban);
    int getAtackScoreHelp(int row, int col, int color, int &resultScore, char irow, char icol);
    bool getDirection(int&, int&, int, int);
    void formatChess2String(char chessStr[][FORMAT_LENGTH], int row, int col, int state, bool reverse = false);
    static bool buildTrieTree();
public:
    Piece pieces[BOARD_ROW_MAX][BOARD_COL_MAX];
    STEP lastStep;
    static TrieTreeNode* searchTrieTree;
};

#endif 