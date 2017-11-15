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

enum TerminateType
{
    FAIL,
    OVERTRY,
    OVEVTIME,
    SUCCESS,
    REFUTEPOS,
};

class DBNode;
class DBSearch
{
public:
    static int node_count;
    DBSearch(ChessBoard* board, GAME_RULE rule, uint8_t searchLevel) :board(board), rule(rule), searchLevel(searchLevel)
    {
    }
    ~DBSearch()
    {
        clearTree(root);
    }
    bool doVCTSearch(vector<Position> &path);
    bool doDBSearch(vector<Position> &path);
    void printWholeTree();
    void setRefute(set<Position> *related)
    {
        isRefuteSearch = true;
        relatedpos = related;
    }
    int getWinningSequenceCount()
    {
        return winning_sequence_count;
    }
    TerminateType getResult()
    {
        return terminate_type;
    }
private:
    GAME_RULE rule;
    uint8_t level = 1;
    uint8_t searchLevel = 2; // 0:only 5;1:only 4;2:all threat 
    DBNode* root = NULL;
    ChessBoard *board;
#define MAX_WINNING_COUNT 100
    int winning_sequence_count = 0;

    void clearTree(DBNode* root);

    void addDependencyStage();

    void addCombinationStage(DBNode* node, ChessBoard *board, vector<DBNode*> &sequence);
    void addDependentChildren(DBNode* node, ChessBoard *board, vector<DBNode*> &sequence);
    void addDependentChildrenWithCandidates(DBNode* node, ChessBoard *board, vector<DBNode*> &sequence, vector<StepCandidateItem> &legalMoves);
    void getDependentCandidates(DBNode* node, ChessBoard *board, vector<StepCandidateItem>& moves);
    void findAllCombinationNodes(DBNode* partner, vector<DBNode*> &partner_sequence, ChessBoard *partner_board, DBNode* node, vector<DBNode*> &combine_sequence);

    bool testAndAddCombination(DBNode* partner, vector<DBNode*> &partner_sequence, ChessBoard *board, Position node_atack, vector<DBNode*> &combine_sequence);

    bool inConflict(ChessBoard *board, DBMetaOperator &opera);

    bool proveWinningThreatSequence(vector<DBNode*> &sequence);
    bool proveWinningThreatSequence(ChessBoard *board, set<Position> relatedpos, queue<DBNode*> sequence);
    TerminateType doRefuteExpand(ChessBoard *board, set<Position> &relatedpos);


    vector<DBMetaOperator> operatorList;
    vector<DBNode*> addDependencyStageCandidates;
    bool treeSizeIncreased = false;

    bool isRefuteSearch = false;
    set<Position> *relatedpos = NULL;
    bool terminate = false;
    TerminateType terminate_type = FAIL;
};

class DBNode
{
public:
    DBNode(NodeType type, uint8_t level) :type(type), level(level)
    {
        DBSearch::node_count++;
    }
    ~DBNode()
    {

    }
    DBMetaOperator opera;
    NodeType type;
    uint8_t level;
    uint8_t chessType;
    bool hasCombined = false;//A combine B后防止B再combine A
    bool hasRefute = false;
    bool isGoal = false; // 叶节点五杀为true，checkRefute后被Refute了也为true
    vector<DBNode*> child;
};
#endif