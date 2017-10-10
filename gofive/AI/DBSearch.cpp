#include "DBSearch.h"


void DBSearch::clearTree(DBNode* root)
{

}

void DBSearch::doDBSearch()
{
    root = new DBNode(Root, 0);
    addDependencyStageCandidates.push_back(root);
    while (1)
    {
        addDependencyStage();
        addCombinationStage(root, board);
        level++;
    }
}


void DBSearch::addDependencyStage()
{
    while (!addDependencyStageCandidates.empty())
    {
        DBNode* node = addDependencyStageCandidates.back();
        addDependencyStageCandidates.pop_back();
        if (level == node->level + 1 && node->type != Dependency)
        {
            addDependentChildren(node, board);
        }
    }
}

void DBSearch::getDependentCandidates(DBNode* node, ChessBoard *board, vector<StepCandidateItem>& moves)
{
    uint8_t side = board->getLastStep().getOtherSide();
    if (node->type == Root)
    {
        board->getVCFCandidates(moves, NULL);
        board->getVCTCandidates(moves, NULL);
    }
    else
    {
        for (int d = 0; d < DIRECTION4_COUNT; ++d)
        {
            for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
            {
                Position temppos = node->opera.atack;
                for (int8_t offset = 1; offset < 6; ++offset)
                {
                    if (!temppos.displace4(symbol, d))//equal otherside
                    {
                        break;
                    }
                    if (board->getState(temppos.row, temppos.col) == PIECE_BLANK)
                    {
                        if (Util::isthreat(board->getLayer2(temppos.row, temppos.col, side, d)))
                        {
                            moves.emplace_back(temppos, 0);
                        }
                    }
                    else if (board->getState(temppos.row, temppos.col) == side)
                    {
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
    }
}

void DBSearch::addDependentChildren(DBNode* node, ChessBoard *board)
{
    vector<StepCandidateItem> legalMoves;
    getDependentCandidates(node, board, legalMoves);

    size_t len = legalMoves.size();

    for (size_t i = 0; i < len; ++i)
    {
        DBNode* childnode = new DBNode(Dependency, level);

        ChessBoard tempboard = *board;
        tempboard.move(legalMoves[i].pos, rule);
        childnode->opera.atack = legalMoves[i].pos;
        tempboard.getFourkillDefendCandidates(legalMoves[i].pos, childnode->opera.replies, rule);
        tempboard.moveMultiReplies(childnode->opera.replies, rule);

        node->child.push_back(childnode);

        addDependentChildren(childnode, &tempboard);
    }
}



void DBSearch::addCombinationStage(DBNode* node, ChessBoard *board)
{

    if (level == node->level && node->type == Dependency)
    {
        findAllCombinationNodes(node, board, root);
        node->hasCombined = true;
    }
    size_t len = node->child.size();
    for (int i = 0; i < len; ++i)
    {
        ChessBoard tempboard = *board;
        tempboard.move(node->child[i]->opera.atack, rule);
        tempboard.moveMultiReplies(node->child[i]->opera.replies, rule);
        addCombinationStage(node->child[i], &tempboard);
    }

}

void DBSearch::findAllCombinationNodes(DBNode* partner, ChessBoard *combined_board, DBNode* node)
{
    if (node->hasCombined && level == node->level)
    {
        return;
    }
    size_t len = node->child.size();
    for (int i = 0; i < len; ++i)
    {
        if (!inConflict(combined_board, node->child[i]->opera))//未冲突
        {
            ChessBoard tempboard = *combined_board;
            tempboard.move(node->child[i]->opera.atack, rule);
            tempboard.moveMultiReplies(node->child[i]->opera.replies, rule);
            if (node->child[i]->type == Dependency)
            {
                testAndAddCombination(partner, &tempboard, node->child[i]->opera.atack);
            }
            findAllCombinationNodes(partner, &tempboard, node->child[i]);
        }
        else//冲突了就没必要再检查node->child[i]的儿子了
        {
            continue;
        }
    }
}

bool DBSearch::inConflict(ChessBoard *board, DBMetaOperator &opera)
{
    if (board->getState(opera.atack) != PIECE_BLANK)
    {
        return false;
    }
    size_t len = opera.replies.size();
    for (size_t i = 0; i < len; ++i)
    {
        if (board->getState(opera.replies[i]) != PIECE_BLANK)
        {
            return false;
        }
    }
    return true;
}

bool DBSearch::testAndAddCombination(DBNode* partner,ChessBoard *board, Position node_atack)
{
    Position partner_atack = partner->opera.atack;
    int row = partner_atack.row - node_atack.row;
    int col = partner_atack.col - node_atack.col;
    uint8_t direction;
    uint8_t offset;
    Position startpos;
    //首先判断是否在一条直线上
    if (row == 0)
    {
        direction = DIRECTION4::DIRECTION4_LR;
    }
    else if (col == 0)
    {
        direction = DIRECTION4::DIRECTION4_UD;
    }
    else if (row == col)
    {
        direction = DIRECTION4::DIRECTION4_RD;
    }
    else if (row == -col)
    {
        direction = DIRECTION4::DIRECTION4_RU;
    }
    else
    {
        return false;
    }
    uint8_t side = board->getLastStep().getOtherSide();
    vector<StepCandidateItem> candidates1;
    for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
    {
        Position temppos = partner_atack;
        for (int8_t offset = 1; offset < 6; ++offset)
        {
            if (!temppos.displace4(symbol, direction))//equal otherside
            {
                break;
            }
            if (board->getState(temppos.row, temppos.col) == PIECE_BLANK)
            {
                if (Util::isthreat(board->getLayer2(temppos.row, temppos.col, side, direction)))
                {
                    candidates1.emplace_back(temppos, 0);
                }
            }
            else if (board->getState(temppos.row, temppos.col) == side)
            {
                continue;
            }
            else
            {
                break;
            }
        }
    }
    vector<StepCandidateItem> candidates2;
    for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
    {
        Position temppos = partner_atack;
        for (int8_t offset = 1; offset < 6; ++offset)
        {
            if (!temppos.displace4(symbol, direction))//equal otherside
            {
                break;
            }
            if (board->getState(temppos.row, temppos.col) == PIECE_BLANK)
            {
                if (Util::isthreat(board->getLayer2(temppos.row, temppos.col, side, direction)))
                {
                    candidates2.emplace_back(temppos, 0);
                }
            }
            else if (board->getState(temppos.row, temppos.col) == side)
            {
                continue;
            }
            else
            {
                break;
            }
        }
    }
    vector<StepCandidateItem> candidates0;

    for (size_t i = 0; i < candidates1.size(); ++i)
    {
        for (size_t j = 0; j < candidates2.size(); ++j)
        {
            if (candidates1[i].pos == candidates2[j].pos)
            {
                candidates0.emplace_back(candidates1[i].pos, 0);
            }
        }
    }
    if (candidates0.empty()) return false;

    DBNode* combine_node = new DBNode(Combination, level);
    
}


void DBSearch::proveWinningThreatSequence(vector<DBMetaOperator> &sequence)
{

}


void DBSearch::printWholeTree()
{

}