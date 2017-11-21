#include "PNSearch.h"
#include "DBSearch.h"

#include <algorithm>



void PNSearch::clearNode(PNNode* n)
{
    if (n == NULL)
    {
        return;
    }

    for (auto c : n->child)
    {
        if (c->parent.size() > 1)
        {
            for (auto i = c->parent.begin(); i != c->parent.end(); ++i)
            {
                if (*i == n)
                {
                    c->parent.erase(i);
                    break;
                }
            }
        }
        else
        {
            clearNode(c);
        }
    }
    delete n;
}

void PNSearch::start()
{
    root = new PNNode(OR, 0);
    evaluate(root, board);
    setProofAndDisproofNumbers(root);
    PNS();
}

void PNSearch::continu()
{
    PNS();
}

PNVALUE PNSearch::getResult()
{
    if (root->proof == 0) return PROVEN;
    else if (root->disproof == 0) return DISPROVEN;
    else return UNKNOWN;
}

void PNSearch::getSequence(vector<Position>& proveSequence)
{
    PNNode* current = root;
    while (!current->child.empty())
    {
        if (current->type == OR)
        {
            for (auto c : current->child)
            {
                if (c->proof == 0)
                {
                    proveSequence.push_back(c->move);
                    current = c;
                    break;
                }
            }
            if (current == root) return;
        }
        else
        {
            int value = MAX_VALUE + 1;
            PNNode* best = NULL;
            for (auto c : current->child)
            {
                if (c->proof == 0 && c->disproof < value)
                {
                    value = c->disproof;
                    best = c;
                }
            }
            if (best == NULL)
            {
                break;
            }
            proveSequence.push_back(best->move);
            current = best;
        }
        
    }
}

void PNSearch::PNS()
{

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
    if (nodeMaxDepth > maxDepth)
    {
        return false;
    }
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
        case INIT:      n->proof = 1 + n->depth / 2; n->disproof = 1 + n->depth / 2; break;
        case DISPROVEN: n->proof = MAX_VALUE; n->disproof = 0; break;
        case PROVEN:    n->proof = 0; n->disproof = MAX_VALUE; break;
        case UNKNOWN:   n->proof = 1 + n->depth / 2; n->disproof = 1 + n->depth / 2; break;
        }
    }
}

PNNode* PNSearch::selectMostProvingNode(PNNode* n, ChessBoard* currentBoard)
{
    while (n->expanded)
    {
        int value = MAX_VALUE + 1;
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
    size_t len = currentBoard->getPNCandidates(moves, n->type == OR);
    for (size_t i = 0; i < len; ++i)
    {
        ChessBoard tempboard = *currentBoard;

        PNNode *c = new PNNode(1 - n->type, n->depth + 1);
        c->depth = n->depth + 1;
        c->parent.push_back(n);
        c->move = moves[i].pos;

        n->child.push_back(c);

        nodeCount++;
        nodeMaxDepth = nodeMaxDepth > c->depth ? nodeMaxDepth : c->depth;

        if (tempboard.getChessType(moves[i].pos, tempboard.getLastStep().getOtherSide()) == CHESSTYPE_5)
        {
            if (c->type == OR) c->value = DISPROVEN;
            else c->value = PROVEN;
        }
        else if (tempboard.getChessType(moves[i].pos, tempboard.getLastStep().getOtherSide()) == CHESSTYPE_BAN)
        {
            if (c->type == OR) c->value = PROVEN;
            else  c->value = DISPROVEN;
        }
        else
        {
            tempboard.move(moves[i].pos, rule);
            evaluate(c, &tempboard);
        }

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
                n->value = PROVEN;
            }
            else
            {
                n->value = DISPROVEN;
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
