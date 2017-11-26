#ifndef __DBSEARCHPLUS_H__
#define __DBSEARCHPLUS_H__

#include "ChessBoard.h"
#include "TransTable.h"
#include "DBSearch.h"

enum VCXRESULT :uint8_t
{
    VCXRESULT_NOSEARCH,
    VCXRESULT_UNSURE,   // both unsure
    VCXRESULT_FAIL, // VCF和VCT都fail
    VCXRESULT_SUCCESS,  // 100
};

class DBPlusNode;

struct ThreatMove
{
    Position atack;
    Position reply_applied;
    DBPlusNode* node;
    vector<Position> replies;
};


class DBSearchPlus
{
public:
    static int node_count;
    DBSearchPlus(ChessBoard* board, GAME_RULE rule, uint8_t searchLevel,bool extend =false) :board(board), rule(rule), searchLevel(searchLevel), extend(extend)
    {
    }
    ~DBSearchPlus()
    {
        clearTree(root);
    }
    bool doDBSearchPlus(vector<Position> &path);
    void setRefute(map<Position, int> *related)
    {
        isRefuteSearch = true;
        goalset = related;
    }
    int getWinningSequenceCount()
    {
        return winning_sequence_count;
    }
    int getMaxDepth()
    {
        return max_depth;
    }
    TerminateType getResult()
    {
        return terminate_type;
    }
private:
    int max_depth = 0;
    GAME_RULE rule;
    uint8_t maxPly;
    uint8_t level = 1;
    uint8_t searchLevel = 3; // 0:only 5; 1:only 4; 2:alive3,44ext,43ext; 3:33ext 
    DBPlusNode* root = NULL;
    ChessBoard *board;
    int winning_sequence_count = 0;

    void clearTree(DBPlusNode* root);

    //void getDependentCandidates(DBPlusNode* node, ChessBoard *board, vector<StepCandidateItem>& moves);

    //void addCombinationStage(DBPlusNode* node, ChessBoard *board, vector<DBPlusNode*> &sequence);
    //void findAllCombinationNodes(DBPlusNode* partner, vector<DBPlusNode*> &partner_sequence, ChessBoard *partner_board, DBPlusNode* node, vector<DBPlusNode*> &combine_sequence);
    //bool testAndAddCombination(DBPlusNode* partner, vector<DBPlusNode*> &partner_sequence, ChessBoard *board, Position node_atack, vector<DBPlusNode*> &combine_sequence);
    //bool inConflict(ChessBoard *board, DBMetaOperator &opera);

    VCXRESULT addDependentChildren(DBPlusNode* node, ChessBoard *board, vector<ThreatMove> &sequence);
    VCXRESULT addDependentChildrenWithCandidates(DBPlusNode* node, ChessBoard *board, vector<ThreatMove> &sequence, vector<StepCandidateItem> &legalMoves);
    bool proveWinningThreatSequence(vector<ThreatMove> &sequence);
    TerminateType doRefuteExpand(ChessBoard *board, map<Position, int> &relatedpos);

    vector<DBPlusNode*> addDependencyStageCandidates;

    vector<ThreatMove> winningThreatSequence;
    bool treeSizeIncreased = false;
    bool extend = false;
    bool isRefuteSearch = false;
    map<Position, int> *goalset = NULL;
    bool terminate = false;
    TerminateType terminate_type = FAIL;
};

enum DBPlusNodeType
{
    ROOT,
    ATACK,
    DEFEND
};

class DBPlusNode
{
public:
    DBPlusNode(DBPlusNodeType type, uint8_t depth) :type(type), depth(depth)
    {
        DBSearchPlus::node_count++;
    }
    ~DBPlusNode()
    {

    }
    Position pos;
    DBPlusNodeType type;
    uint8_t depth;
    uint8_t chessType;
    bool hasCombined = false;//A combine B后防止B再combine A
    bool hasRefute = false; // 敌方存在vcf
    bool isGoal = false; // 叶节点五杀为true，checkRefute后被Refute了也为true
    vector<DBPlusNode*> child;
};
#endif