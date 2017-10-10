#ifndef __DBSEARCH_H__
#define __DBSEARCH_H__

#include "ChessBoard.h"

struct DBMetaOperator
{
    Position atack;
    vector<Position> replies;
};

enum NodeType :uint8_t
{
    Root,
    Dependency,
    Combination,
};

class DBNode
{
public:
    DBNode(NodeType type, uint8_t level) :type(type), level(level)
    {

    }
    ~DBNode()
    {

    }

    NodeType type;
    uint8_t level;
    uint8_t chessType;
    bool hasCombined = false;//A combine B∫Û∑¿÷πB‘Ÿcombine A
    ChessBoard* board = NULL;
    vector<DBNode*> child;
    DBMetaOperator opera;
};


class DBSearch
{
public:
    DBSearch(ChessBoard* board, GAME_RULE rule) :board(board), rule(rule)
    {
    }
    ~DBSearch()
    {
        clearTree(root);
    }
    void doDBSearch();

private:
    GAME_RULE rule;
    uint8_t level = 1;
    DBNode* root = NULL;
    ChessBoard *board;
    int winning_sequence_count = 0;

    vector<DBMetaOperator> operatorList;

    void clearTree(DBNode* root);

    void addDependencyStage();
    void proveWinningThreatSequence(vector<DBMetaOperator> &sequence);
    void addCombinationStage(DBNode* node, ChessBoard *board);
    void addDependentChildren(DBNode* node, ChessBoard *board);
    void getDependentCandidates(DBNode* node, ChessBoard *board, vector<StepCandidateItem>& moves);
    void findAllCombinationNodes(DBNode* partner, ChessBoard *partner_board, DBNode* node);

    bool testAndAddCombination(DBNode* partner,ChessBoard *board,Position node_atack);

    bool inConflict(ChessBoard *board, DBMetaOperator &opera);


    vector<DBNode*> addDependencyStageCandidates;


    void printWholeTree();
};


#endif