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
    //if (n->board) delete n->board;
    delete n;
}

void PNSearch::start()
{
    root = new PNNode(OR, 0);
    //root->board = new ChessBoard(*board);
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

        //PNNode* mostProving = selectMostProvingNode(current,NULL);
        //expandNode(mostProving, NULL);
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
    //currentBoard = n->board;
    uint8_t side = currentBoard->getLastStep().getOtherSide();
    vector<StepCandidateItem> moves;
    size_t len = currentBoard->getPNCandidates(moves, n->type == OR);
    for (size_t i = 0; i < len; ++i)
    {
        ChessBoard tempboard = *currentBoard;

        PNNode *c = new PNNode(1 - n->type, n->depth + 1);
        //c->board = new ChessBoard(*currentBoard);

        nodeCount++;

        if (currentBoard->getChessType(moves[i].pos, side) == CHESSTYPE_5)
        {
            if (c->type == OR) c->value = DISPROVEN;
            else c->value = PROVEN;
        }
        else if (currentBoard->getChessType(moves[i].pos, side) == CHESSTYPE_BAN)
        {
            if (c->type == OR) c->value = PROVEN;
            else  c->value = DISPROVEN;
        }
        else
        {
            tempboard.move(moves[i].pos, rule);
            evaluate(c, &tempboard);
            /*c->board->move(moves[i].pos, rule);
            evaluate(c, c->board);*/
        }

        setProofAndDisproofNumbers(c);

        if (Util::isdead4(currentBoard->getChessType(moves[i].pos, side)) ||
            currentBoard->getChessType(moves[i].pos, Util::otherside(side)) == CHESSTYPE_5)//冲四或防冲四
        {
            c->depth = n->depth;
        }
        else if (Util::isalive3(currentBoard->getChessType(moves[i].pos, side)) && n->type == AND)//活三
        {
            c->depth = n->depth;
        }
        else
        {
            c->depth = n->depth + 1;
        }
        
        c->parent.push_back(n);
        c->move = moves[i].pos;
        n->child.push_back(c);

        nodeMaxDepth = nodeMaxDepth > c->depth ? nodeMaxDepth : c->depth;

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
    //delete n->board;
    //n->board = NULL;
}

void PNSearch::evaluate(PNNode* n, ChessBoard* currentBoard)
{
    if (n->value == INIT)
    {
        TransTableDBData data;
        if (DBSearch::transTable[currentBoard->getLastStep().step].get(currentBoard->getBoardHash().hash_key, data))
        {
            if (data.checkHash == currentBoard->getBoardHash().check_key)
            {
                if (data.result)
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
                hit++;
                return;
            }
        }

        DBSearch dbs(currentBoard, rule, 2);
        vector<Position> optimalPath;
        bool ret = dbs.doDBSearch(optimalPath);
        DBNodeCount += DBSearch::node_count;
        DBSearch::node_count = 0;
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

        data.result = ret;
        data.checkHash = currentBoard->getBoardHash().check_key;
        DBSearch::transTable[currentBoard->getLastStep().step].insert(currentBoard->getBoardHash().hash_key, data);
        miss++;
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
