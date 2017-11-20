#ifndef __PNSEARCH_H__
#define __PNSEARCH_H__

#include "defines.h"
#include "ChessBoard.h"
#include "TransTable.h"

enum PNNodeType
{
    AND,
    OR
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
    PNNode(uint8_t type) :type(type)
    {

    }
    uint8_t proof;
    uint8_t disproof;
    uint8_t type;
    uint8_t value = INIT;
    uint8_t ply;
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
    void startSearch();
    void continueSearch();
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
    PNNode* root;
    ChessBoard* board;
    GAME_RULE rule;
};


#endif // !__PNSEARCH_H__
