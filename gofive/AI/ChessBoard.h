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
    inline int getStepScores(int row, int col, int state, bool ban, bool isdefend)
    {
        return getStepScoresKMP(row, col, state, 0, ban, isdefend);
    }
    inline int getStepScores(int row, int col, int state, int level, bool ban, bool isdefend)
    {
        return getStepScoresKMP(row, col, state, level, ban, isdefend);
    }
    inline int getLastStepScores(bool ban, bool isdefend)
    {
        return getStepScores(lastStep.uRow, lastStep.uCol, lastStep.getColor(), ban, isdefend);
    }
    int getStepScoresKMP(int row, int col, int state, int level, bool ban, bool isdefend);
    int getStepScoresTrie(int row, int col, int state, int level, bool ban, bool isdefend);
    bool doNextStep(int row, int col, int side);
    void resetHotArea();//����������������ר�ã�
    void updateHotArea(int row, int col);
    ThreatInfo getThreatInfo(int side);
    int getStepSituation(int row, int col, int state);
    void setGlobalThreat(bool);//����Ϊһ��ȫɨgetStepScores*2
    void setThreat(int row, int col, int side, bool ban);//����Ϊһ��getStepScores
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