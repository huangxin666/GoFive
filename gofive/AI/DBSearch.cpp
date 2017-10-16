#include "DBSearch.h"
#include <queue>

void DBSearch::clearTree(DBNode* root)
{
    if (root)
    {
        size_t len = root->child.size();
        for (size_t i = 0; i < len; ++i)
        {
            clearTree(root->child[i]);
        }
        delete root;
    }
    if (board)
    {
        delete board;
        board = NULL;
    }
}

bool DBSearch::doDBSearch()
{
    root = new DBNode(Root, 0);
    vector<DBNode*> sequence;
    addDependentChildren(root, board, sequence);
    if (terminate)
    {
        for (auto node : sequence)
        {
            operatorList.push_back(node->opera);
        }
        return true;
    }
    //addDependencyStageCandidates.push_back(root);
    while (treeSizeIncreased)
    {
        //addDependencyStage();
        //if (terminate)
        //{
        //    return true;
        //}
        treeSizeIncreased = false;
        vector<DBNode*> sequence2;
        addCombinationStage(root, board, sequence2);
        if (terminate)
        {
            for (auto node : sequence2)
            {
                operatorList.push_back(node->opera);
            }
            return true;
        }
        level++;
    }
    return false;
}

void DBSearch::addDependencyStage()
{
    while (!addDependencyStageCandidates.empty())
    {
        DBNode* node = addDependencyStageCandidates.back();
        addDependencyStageCandidates.pop_back();
        if (level == node->level + 1 && (node->type == Root || node->type == Combination))
        {
            vector<DBNode*> sequence;
            addDependentChildren(node, board, sequence);
            if (terminate)
            {
                return;
            }
        }
    }
}


void DBSearch::addDependentChildren(DBNode* node, ChessBoard *board, vector<DBNode*> &sequence)
{
    vector<StepCandidateItem> legalMoves;
    getDependentCandidates(node, board, legalMoves);

    size_t len = legalMoves.size();

    for (size_t i = 0; i < len; ++i)
    {
        treeSizeIncreased = true;
        DBNode* childnode = new DBNode(Dependency, level);
        sequence.push_back(childnode);
        ChessBoard tempboard = *board;
        childnode->chessType = tempboard.getChessType(legalMoves[i].pos, tempboard.getLastStep().getOtherSide());
        tempboard.move(legalMoves[i].pos, rule);
        childnode->opera.atack = legalMoves[i].pos;
        tempboard.getFourkillDefendCandidates(legalMoves[i].pos, childnode->opera.replies, rule);
        tempboard.moveMultiReplies(childnode->opera.replies, rule);
        if (childnode->opera.replies.empty())// find winning threat sequence
        {
            childnode->isGoal = true;
            if (isRefuteSearch)
            {
                terminate = true;
                return;
            }
            else if (proveWinningThreatSequence(sequence))
            {
                terminate = true;
                return;
            }
        }
        node->child.push_back(childnode);

        addDependentChildren(childnode, &tempboard, sequence);
        if (terminate)
        {
            return;
        }
        sequence.pop_back();
    }
}


void DBSearch::getDependentCandidates(DBNode* node, ChessBoard *board, vector<StepCandidateItem>& moves)
{
    uint8_t side = board->getLastStep().getOtherSide();
    if (node->type == Root)
    {
        board->getVCFCandidates(moves, NULL);
        if (!isRefuteSearch)
        {
            board->getVCTCandidates(moves, NULL);
        }
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
                        if (isRefuteSearch && Util::isalive3or33(board->getLayer2(temppos.row, temppos.col, side, d)))
                        {
                            continue;
                        }
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

void DBSearch::addCombinationStage(DBNode* node, ChessBoard *board, vector<DBNode*> &sequence)
{

    if (level == node->level && node->type == Dependency)
    {
        vector<DBNode*> combine_sequence;
        findAllCombinationNodes(node, sequence, board, root, combine_sequence);
        if (terminate)
        {
            return;
        }
        node->hasCombined = true;
    }
    size_t len = node->child.size();
    for (int i = 0; i < len; ++i)
    {
        ChessBoard tempboard = *board;
        tempboard.move(node->child[i]->opera.atack, rule);
        tempboard.moveMultiReplies(node->child[i]->opera.replies, rule);

        sequence.push_back(node->child[i]);
        addCombinationStage(node->child[i], &tempboard, sequence);
        sequence.pop_back();
    }

}

void DBSearch::findAllCombinationNodes(DBNode* partner, vector<DBNode*> &partner_sequence, ChessBoard *combined_board, DBNode* node, vector<DBNode*> &combine_sequence)
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
            combine_sequence.push_back(node->child[i]);
            ChessBoard tempboard = *combined_board;
            tempboard.move(node->child[i]->opera.atack, rule);
            tempboard.moveMultiReplies(node->child[i]->opera.replies, rule);
            if (node->child[i]->type == Dependency)
            {
                if (testAndAddCombination(partner, partner_sequence, &tempboard, node->child[i]->opera.atack, combine_sequence))
                {
                    treeSizeIncreased = true;
                }
                if (terminate)
                {
                    return;
                }
            }
            findAllCombinationNodes(partner, partner_sequence, &tempboard, node->child[i], combine_sequence);
            if (terminate)
            {
                return;
            }
            combine_sequence.pop_back();
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

bool DBSearch::testAndAddCombination(DBNode* partner, vector<DBNode*> &partner_sequence, ChessBoard *board, Position node_atack, vector<DBNode*> &combine_sequence)
{
    Position partner_atack = partner->opera.atack;
    int row = partner_atack.row - node_atack.row;
    int col = partner_atack.col - node_atack.col;
    uint8_t direction;
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
                if (isRefuteSearch && Util::isalive3or33(board->getLayer2(temppos.row, temppos.col, side, direction)))
                {
                    continue;
                }
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
                if (isRefuteSearch && Util::isalive3or33(board->getLayer2(temppos.row, temppos.col, side, direction)))
                {
                    continue;
                }
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

    size_t len;
    //start combine node
    vector<DBNode*> sequence = partner_sequence;
    DBNode* combine_node = partner;
    len = combine_sequence.size();
    for (size_t i = 0; i < len; ++i)
    {
        sequence.push_back(combine_sequence[i]);
        DBNode* tempnode = new DBNode(Combination, level);
        tempnode->opera = combine_sequence[i]->opera;
        tempnode->chessType = combine_sequence[i]->chessType;
        combine_node->child.push_back(tempnode);
        combine_node = tempnode;
    }

    level++;
    len = candidates0.size();
    for (size_t i = 0; i < len; ++i)
    {
        DBNode* childnode = new DBNode(Dependency, level);
        ChessBoard tempboard = *board;
        tempboard.move(candidates0[i].pos, rule);
        childnode->opera.atack = candidates0[i].pos;
        tempboard.getFourkillDefendCandidates(candidates0[i].pos, childnode->opera.replies, rule);
        tempboard.moveMultiReplies(childnode->opera.replies, rule);
        combine_node->child.push_back(childnode);

        sequence.push_back(childnode);
        addDependentChildren(childnode, &tempboard, sequence);
        if (terminate)
        {
            return true;
        }
        sequence.pop_back();
    }
    level--;

    return true;
}


bool DBSearch::proveWinningThreatSequence(vector<DBNode*> &sequence)
{
    //init relatedpos
    set<Position> relatedpos;
    ChessBoard currentboard = *board;
    for (size_t i = 0; i < sequence.size(); ++i)
    {

        relatedpos.insert(sequence[i]->opera.atack);
        for (size_t j = 0; j < sequence[i]->opera.replies.size(); ++j)
        {
            relatedpos.insert(sequence[i]->opera.replies[j]);
        }
    }

    for (size_t i = 0; i < sequence.size(); ++i)
    {
        currentboard.move(sequence[i]->opera.atack, rule);
        currentboard.moveMultiReplies(sequence[i]->opera.replies, rule);

        if (Util::isalive3or33(sequence[i]->chessType))
        {
            if (doRefuteExpand(&currentboard, relatedpos))
            {
                sequence[i]->hasRefute = true;
                winning_sequence_count++;
                return false;
            }
        }
        relatedpos.erase(sequence[i]->opera.atack);
        for (size_t j = 0; j < sequence[i]->opera.replies.size(); ++j)
        {
            relatedpos.erase(sequence[i]->opera.replies[j]);
        }
    }
    return true;
}

bool DBSearch::doRefuteExpand(ChessBoard *board, set<Position> &relatedpos)
{
    DBSearch s(board, FREESTYLE);
    s.setRefute(&relatedpos);
    return s.doDBSearch();
}


void DBSearch::printWholeTree()
{
    fstream of("dbsearch_tree.txt", ios::out);
    stringstream ss;
    queue<DBNode*> q1;
    queue<DBNode*> q2;
    queue<DBNode*> *current = &q1;
    queue<DBNode*> *back = &q2;
    for (size_t i = 0; i < root->child.size(); ++i)
    {
        current->push(root->child[i]);
    }

    while (true)
    {
        if (current->empty())
        {
            if (back->empty())
            {
                break;
            }
            else
            {
                queue<DBNode*> *temp = current;
                current = back;
                back = temp;
                ss << "\n";
            }
        }

        DBNode* node = current->front();
        current->pop();
        if (node)
        {
            ss << "(" << (int)node->opera.atack.row << "," << (int)node->opera.atack.col << ")" << " ";
            for (size_t i = 0; i < node->child.size(); ++i)
            {
                back->push(node->child[i]);
            }
            back->push(NULL);
        }
        else
        {
            ss << "|& ";
        }
    }
    of << ss.str();
    of.close();
}