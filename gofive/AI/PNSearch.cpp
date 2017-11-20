#include "PNSearch.h"
#include "DBSearch.h"

#include <algorithm>

void PNSearch::PNS()
{
    root = new PNNode(AND);

    evaluate(root, board);
    setProofAndDisproofNumbers(root);
    PNNode* current = root;
    while (root->proof != 0 && root->disproof != 0 && resourcesAvailable()) 
    {
        ChessBoard currentBoard = *board;
        PNNode* mostProving = selectMostProvingNode(current, &currentBoard);
        expandNode(mostProving, &currentBoard);
        updateAncestors(mostProving);
    }
}

bool PNSearch::resourcesAvailable()
{
    return true;
}


void PNSearch::setProofAndDisproofNumbers(PNNode* n)
{
    if (n->expanded) 
    { /* interior node */
        if (n->type == AND) 
        {
            n->proof = 0;  n->disproof = MAX_VALUE;
            for (auto c : n->child) 
            {
                n->proof += c->proof;
                n->disproof = std::min(n->disproof, c->disproof);
            }
        }
        else { /* OR node */
            n->proof = MAX_VALUE;  n->disproof = 0;
            for (auto c : n->child) 
            {
                n->disproof += c->disproof;
                n->proof = std::min(n->proof, c->proof);
            }
        }
    }
    else 
    { /* terminal node or none terminal leaf */
        switch (n->value) 
        {
        case INIT:      n->proof = 1; n->disproof = 1; break;
        case DISPROVEN: n->proof = MAX_VALUE; n->disproof = 0; break;
        case PROVEN:    n->proof = 0; n->disproof = MAX_VALUE; break;
        case UNKNOWN:   n->proof = 1; n->disproof = 1; break;
        }
    }
}

PNNode* PNSearch::selectMostProvingNode(PNNode* n, ChessBoard* currentBoard)
{
    while (n->expanded) 
    {
        int value = MAX_VALUE;
        PNNode* best;
        if (n->type == AND) 
        {
            for (auto c : n->child) 
            {
                if (value > c->disproof) 
                {
                    best = c;
                    value = c->disproof;
                }
            }
        }
        else 
        { /* OR node */
            for (auto c : n->child) 
            {
                if (value > c->proof) 
                {
                    best = c;
                    value = c->proof;
                }
            }
        }
        n = best;
        currentBoard->move(best->move, rule);
    }
    return n;
}

void PNSearch::expandNode(PNNode* n, ChessBoard* currentBoard)
{
    vector<StepCandidateItem> moves;
    size_t len = currentBoard->getPNCandidates(moves, n->type == AND);
    for (auto move : moves) 
    {
        ChessBoard tempboard = *currentBoard;

        PNNode *c = new PNNode(1 - n->type);
        c->ply = n->ply + 1;
        c->parent.push_back(n);
        c->move = move.pos;

        n->child.push_back(c);

        nodeCount++;
        nodeMaxDepth = nodeMaxDepth > c->ply ? nodeMaxDepth : c->ply;

        evaluate(c, &tempboard);
        setProofAndDisproofNumbers(c);
        if (n->type == AND) 
        {
            if (c->disproof == 0) break;
        }
        else 
        {  /* OR node */
            if (c->proof == 0) break;
        }
    }
    n->expanded = true;
}

void PNSearch::evaluate(PNNode* n, ChessBoard* currentBoard)
{
    if (n->value == INIT)
    {
        DBSearch dbs(currentBoard, rule, 2);
        vector<Position> optimalPath;
        bool ret = dbs.doDBSearch(optimalPath);
        if (ret)
        {
            if (n->type == OR)
            {
                n->value = DISPROVEN;
            }
            else
            {
                n->value = PROVEN;
            }
        }
        else
        {
            n->value = UNKNOWN;
        }
    }
}

void PNSearch::updateAncestors(PNNode* n)
{
    if (n == root)
    {
        setProofAndDisproofNumbers(n);
        return;
    }
    int oldProof = n->proof;
    int oldDisproof = n->disproof;
    setProofAndDisproofNumbers(n);
    if (n->proof == oldProof && n->disproof == oldDisproof)
        return;
    for (auto c : n->parent)
    {
        updateAncestors(c);
    }
}
