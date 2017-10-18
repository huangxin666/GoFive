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
}

bool DBSearch::doDBSearch()
{
    root = new DBNode(Root, 0);
    vector<DBNode*> sequence;
    addDependentChildren(root, board, sequence);
    if (terminate)
    {
        if (winning_sequence_count > MAX_WINNING_COUNT)
        {
            return false;
        }
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
            if (winning_sequence_count > MAX_WINNING_COUNT)
            {
                return false;
            }
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
    addDependentChildrenWithCandidates(node, board, sequence, legalMoves);
}

void DBSearch::addDependentChildrenWithCandidates(DBNode* node, ChessBoard *board, vector<DBNode*> &sequence, vector<StepCandidateItem> &legalMoves)
{
    size_t len = legalMoves.size();

    for (size_t i = 0; i < len; ++i)
    {
        treeSizeIncreased = true;
        DBNode* childnode = new DBNode(Dependency, level);

        ChessBoard tempboard = *board;
        uint8_t type = tempboard.getChessType(legalMoves[i].pos, tempboard.getLastStep().getOtherSide());
        if (type == CHESSTYPE_43) //本以为是3，其实是d4
        {
            type = CHESSTYPE_D4;
            legalMoves[i].priority = board->getChessDirection(legalMoves[i].pos, tempboard.getLastStep().getOtherSide());
        }
        else
        {
            type = tempboard.getLayer2(legalMoves[i].pos.row, legalMoves[i].pos.col, tempboard.getLastStep().getOtherSide(), legalMoves[i].priority);
        }
        childnode->chessType = type;

        childnode->opera.atack = legalMoves[i].pos;
        tempboard.move(legalMoves[i].pos, rule);

        tempboard.getThreatReplies(legalMoves[i].pos, childnode->chessType, legalMoves[i].priority, childnode->opera.replies);
        tempboard.moveMultiReplies(childnode->opera.replies, rule);

        node->child.push_back(childnode);
        sequence.push_back(childnode);
        if (isRefuteSearch)
        {
            if (relatedpos->find(legalMoves[i].pos) != relatedpos->end())
            {
                terminate = true;
                terminate_type = TerminateType::REFUTEPOS;
                return;
            }
        }
        
        if (childnode->opera.replies.empty())// find winning threat sequence
        {
            if (childnode->chessType >= CHESSTYPE_4)
            {
                childnode->isGoal = true;
                if (isRefuteSearch)
                {
                    terminate = true;
                    terminate_type = TerminateType::SUCCESS;
                    return;
                }
                else if (proveWinningThreatSequence(sequence))
                {
                    terminate = true;
                    terminate_type = TerminateType::SUCCESS;
                    return;
                }
                sequence.pop_back();
                continue;
            }
            else
            {
                sequence.pop_back();
                return;//unexpected
            }

        }

        
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
    if (board->getHighestInfo(side).chesstype == CHESSTYPE_5)
    {
        Position pos = board->getHighestInfo(side).pos;
        moves.emplace_back(pos, board->getChessDirection(pos, side));
        return;
    }

    if (node->type == Root)
    {
        board->getVCFCandidates(moves, NULL);
        if (!isRefuteSearch)
        {
            board->getVCTCandidates(moves, NULL);
        }
        std::sort(moves.begin(), moves.begin(), CandidateItemCmp);

        for (size_t i = 0; i < moves.size(); ++i)
        {
            moves[i].priority = board->getChessDirection(moves[i].pos, side);
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
                            moves.emplace_back(temppos, d);
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
        return true;
    }
    size_t len = opera.replies.size();
    for (size_t i = 0; i < len; ++i)
    {
        if (board->getState(opera.replies[i]) != PIECE_BLANK)
        {
            return true;
        }
    }
    return false;
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
    addDependentChildrenWithCandidates(combine_node, board, sequence, candidates0);
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
        if (sequence[i]->hasRefute)
        {
            return false;
        }
        relatedpos.insert(sequence[i]->opera.atack);
        for (size_t j = 0; j < sequence[i]->opera.replies.size(); ++j)
        {
            relatedpos.insert(sequence[i]->opera.replies[j]);
        }
    }

    for (size_t i = 0; i < sequence.size(); ++i)
    {
        currentboard.move(sequence[i]->opera.atack, rule);

        if (Util::isalive3or33(sequence[i]->chessType))
        {
            TerminateType result = doRefuteExpand(&currentboard, relatedpos);
            if (result == SUCCESS || result == REFUTEPOS)
            {
                if (result == SUCCESS)
                {
                    sequence[i]->hasRefute = true;
                }

                if (++winning_sequence_count > MAX_WINNING_COUNT)
                {
                    terminate = true;
                    terminate_type = TerminateType::OVERTRY;
                }
                return false;
            }
        }
        else if (Util::hasdead4(sequence[i]->chessType))
        {
            if (currentboard.getHighestInfo(currentboard.getLastStep().getOtherSide()).chesstype == CHESSTYPE_5)
            {
                sequence[i]->hasRefute = true;
                if (++winning_sequence_count > MAX_WINNING_COUNT)
                {
                    terminate = true;
                    terminate_type = TerminateType::OVERTRY;
                }
                return false;
            }
        }

        currentboard.moveMultiReplies(sequence[i]->opera.replies, rule);

        relatedpos.erase(sequence[i]->opera.atack);
        for (size_t j = 0; j < sequence[i]->opera.replies.size(); ++j)
        {
            relatedpos.erase(sequence[i]->opera.replies[j]);
        }
    }
    return true;
}

TerminateType DBSearch::doRefuteExpand(ChessBoard *board, set<Position> &relatedpos)
{
    DBSearch dbs(board, FREESTYLE);
    dbs.setRefute(&relatedpos);
    bool ret = dbs.doDBSearch();
    return dbs.getResult();
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
            if (node->isGoal)
            {
                ss << "[" << (int)node->opera.atack.row << "," << (int)node->opera.atack.col << "]" << " ";
            }
            else
            {
                if (node->type == Combination)
                {
                    ss << "{" << (int)node->opera.atack.row << "," << (int)node->opera.atack.col << "}" << " ";
                }
                else
                {
                    ss << "(" << (int)node->opera.atack.row << "," << (int)node->opera.atack.col << ")" << " ";
                }
            }
            
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