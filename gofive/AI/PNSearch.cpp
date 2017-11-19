#include "PNSearch.h"
#include <algorithm>

void PNSearch::PNS(PNNode * root)
{
    evaluate(root);
    setProofAndDisproofNumbers(root);
    PNNode* current = root;
    while (root->proof != 0 && root->disproof != 0 && resourcesAvailable()) {
        PNNode* mostProving = selectMostProvingNode(current);
        expandNode(mostProving);
        current = updateAncestors(mostProving, root);
    }
}

void PNSearch::setProofAndDisproofNumbers(PNNode * n)
{
    if (n->expanded) { /* interior node */
        if (n->type == AND) {
            n->proof = 0;  n->disproof = 255;
            for (auto c :n->child) {
                n->proof += c->proof;
                n->disproof = std::min(n->disproof, c->disproof);
            }
        }
        else { /* OR node */
            n->proof = 255;  n->disproof = 0;
            for (auto c : n->child) {
                n->disproof += c->disproof;
                n->proof = std::min(n->proof, c->proof);
            }
        }
    }
    else { /* terminal node or none terminal leaf */
        switch (n->value) {
        case DISPROVEN: n->proof = 255; n->disproof = 0; break;
        case PROVEN:    n->proof = 0; n->disproof = 255; break;
        case UNKNOWN:   n->proof = 1; n->disproof = 1; break;
        }
    }
}

PNNode * PNSearch::selectMostProvingNode(PNNode * n)
{
    while (n->expanded) {
        int value = MAX_VALUE;
        PNNode * best;
        if (n->type == AND) {
            for (auto c : n->child) {
                if (value > c->disproof) {
                    best = c;
                    value = c->disproof;
                }
            }
        }
        else { /* OR node */
            for (auto c : n->child) {
                if (value > c->proof) {
                    best = c;
                    value = c->proof;
                }
            }
        }
        n = best;
    }
    return n;
}

void PNSearch::expandNode(PNNode * n)
{
    generateChildren(n);
    for (auto c : n->child) {
        evaluate(c);
        setProofAndDisproofNumbers(c);
        if (n->type == AND) {
            if (c->disproof == 0) break;
        }
        else {  /* OR node */
            if (c->proof == 0) break;
        }
    }
    n->expanded = true;
}

PNNode * PNSearch::updateAncestors(PNNode * n, PNNode * root)
{
    while (n != root) {
        int oldProof = n->proof;
        int oldDisproof = n->disproof;
        setProofAndDisproofNumbers(n);
        if (n->proof == oldProof && n->disproof == oldDisproof)
            return n;
        n = n->parent;
    }
    setProofAndDisproofNumbers(root);
    return root;
}
