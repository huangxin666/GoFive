#ifndef __PNSEARCH_H__
#define __PNSEARCH_H__

#include "defines.h"
#include "ChessBoard.h"

enum PNNodeType
{
    AND,
    OR
};

enum PNVALUE
{
    DISPROVEN,
    PROVEN,
    UNKNOWN
};

#define MAX_VALUE 255

struct PNNode
{
    uint8_t proof;
    uint8_t disproof;
    uint8_t type;
    uint8_t value;
    bool expanded;
    vector<PNNode*>child;
    vector<PNNode*>parent;
};


class PNSearch
{
private:
    void PNS(PNNode* root);
    void setProofAndDisproofNumbers(PNNode* n);
    PNNode* selectMostProvingNode(PNNode* n);
    void expandNode(PNNode* n);
    PNNode* updateAncestors(PNNode* n, PNNode* root);
};


#endif // !__PNSEARCH_H__
