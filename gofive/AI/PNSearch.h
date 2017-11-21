#ifndef __PNSEARCH_H__
#define __PNSEARCH_H__

#include "defines.h"
#include "ChessBoard.h"
#include "TransTable.h"

enum PNNodeType
{
    AND,//defend to move
    OR  //atack to move
};

enum PNVALUE
{
    INIT,
    DISPROVEN,
    PROVEN,
    UNKNOWN
};

#define MAX_VALUE 255

struct PNNode
{
    PNNode(uint8_t type, uint8_t depth) :type(type), depth(depth)
    {

    }
    uint8_t proof;
    uint8_t disproof;
    uint8_t type;
    uint8_t value = INIT;
    uint8_t depth;
    Position move;
    bool expanded = false;
    vector<PNNode*>child;
    vector<PNNode*>parent;
};


struct TransTablePnData
{
    uint32_t checkHash = 0;
    PNNode* node;
};

class PNSearch
{
public:
    PNSearch(ChessBoard* board, GAME_RULE rule) :board(board), rule(rule)
    {

    }
    ~PNSearch()
    {
        clearNode(root);
    }
    void start();
    void continu();
    PNVALUE getResult();
    void setMaxDepth(int depth)
    {
        maxDepth = depth;
    }
private:
    int nodeCount = 0;
    int nodeMaxDepth = 0;
    int maxDepth = 1024;
    void PNS();
    bool resourcesAvailable();
    void setProofAndDisproofNumbers(PNNode* n);
    PNNode* selectMostProvingNode(PNNode* n, ChessBoard* currentBoard);
    void expandNode(PNNode* n, ChessBoard* currentBoard);
    void updateAncestors(PNNode* n);
    void evaluate(PNNode* n, ChessBoard* currentBoard);

    void clearNode(PNNode* n);
    PNNode* root;
    ChessBoard* board;
    GAME_RULE rule;
};


#endif // !__PNSEARCH_H__
