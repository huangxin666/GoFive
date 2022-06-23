#include "DBSearch.h"
#include <queue>

TransTableArray<TransTableDBData> DBSearch::transTable;
int DBSearch::node_count = 0;
#define MAX_PROVE_FAIL_COUNT 12

void DBSearch::clearTree(DBNode* node)
{
    if (node)
    {
        size_t len = node->child.size();
        for (size_t i = 0; i < len; ++i)
        {
            clearTree(node->child[i]);
        }
        delete node;
    }
}

bool DBSearch::doDBSearch(vector<Position>* path)
{
    root = new DBNode(Root, 0, 0);
    level = 0;
    treeSizeIncreased = true;
    while (treeSizeIncreased)
    {
        treeSizeIncreased = false;
        vector<DBNode*> sequence;
        if (level == 0)
        {
            level++;
            addDependentChildren(root, board, sequence);
        }
        else
        {
            addCombinationStage(root, board, sequence);
            level++;
        }
        if (terminate)
        {
            if (path != NULL)
            {
                for (auto node : sequence)
                {
                    path->push_back(node->opera.atack);
                    if (!node->opera.replies.empty())
                    {
                        path->push_back(node->opera.replies[0]);
                    }
                    else
                    {
                        break;
                    }
                }
            }
            return true;
        }
        if (winning_sequence_count > MAX_PROVE_FAIL_COUNT)
        {
            terminate_type = OVER_WINNING_PROVE;
            return false;
        }
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
    if (sequence.size() > max_depth)
    {
        max_depth = (int)sequence.size();
    }
    size_t len = legalMoves.size();

    for (size_t i = 0; i < len; ++i)
    {
        treeSizeIncreased = true;
        DBNode* childnode = new DBNode(Dependency, level, node->depth + 1);

        ChessBoard tempboard = *board;
        uint8_t type = tempboard.getChessType(legalMoves[i].pos, tempboard.getLastStep().getOtherSide());
        if (type == CHESSTYPE_43) //本以为是3，其实是d4
        {
            type = CHESSTYPE_D4;
            legalMoves[i].direction = board->getChessDirection(legalMoves[i].pos, tempboard.getLastStep().getOtherSide());
        }
        else
        {
            type = tempboard.getLayer2(legalMoves[i].pos, tempboard.getLastStep().getOtherSide(), legalMoves[i].direction);
        }
        childnode->chessType = type;

        childnode->opera.atack = legalMoves[i].pos;
        tempboard.move(legalMoves[i].pos, rule);

        if (childnode->chessType < CHESSTYPE_5 && tempboard.hasChessType(tempboard.getLastStep().getOtherSide(), CHESSTYPE_5))
        {
            delete childnode;
            continue;//被反杀了
        }

        if (isRefuteSearch)
        {
            if (goalset->find(legalMoves[i].pos) != goalset->end())
            {
                terminate = true;
                terminate_type = TerminateType::REFUTE_POS;
                delete childnode;
                return;
            }
        }

        tempboard.getThreatReplies(legalMoves[i].pos, childnode->chessType, legalMoves[i].direction, childnode->opera.replies, rule);

        sequence.push_back(childnode);

        //添加抓禁的情况
        if (childnode->chessType == CHESSTYPE_4 || childnode->chessType == CHESSTYPE_5 || (childnode->opera.replies.empty() && Util::isdead4(childnode->chessType)))// find winning threat sequence
        {
            childnode->isGoal = true;
            if (isRefuteSearch)
            {
                terminate = true;
                terminate_type = TerminateType::SUCCESS;
                delete childnode;
                return;
            }
            else if (proveWinningThreatSequence(sequence))
            {
                terminate = true;
                terminate_type = TerminateType::SUCCESS;
                node->child.push_back(childnode);
                return;
            }
            sequence.pop_back();
            delete childnode;
            continue;
        }
        else if (childnode->opera.replies.empty())
        {
            //unexpected
            sequence.pop_back();
            delete childnode;
            return;
        }

        tempboard.moveMultiReplies(childnode->opera.replies, rule);

        node->child.push_back(childnode);

        addDependentChildren(childnode, &tempboard, sequence);
        if (terminate)
        {
            return;
        }
        sequence.pop_back();
        if (node->hasRefute)
        {
            return;
        }
    }
}


void DBSearch::getDependentCandidates(DBNode* node, ChessBoard *board, vector<StepCandidateItem>& moves)
{
    uint8_t side = board->getLastStep().getOtherSide();
    if (board->hasChessType(side, CHESSTYPE_5))
    {
        ForEachMove(board)
        {
            if (board->getChessType(pos,side) == CHESSTYPE_5)
            {
                moves.emplace_back(pos, board->getChessDirection(pos, side));
                return;
            }
        }
    }

    if (searchLevel == 0)
    {
        return;
    }

    uint8_t search_level_temp = searchLevel;
    if (!isRefuteSearch)
    {
        if (board->hasChessType(Util::otherside(side), CHESSTYPE_5))//defender has a 4 or d4
        {
            return;
        }
        else if (board->hasChessType(Util::otherside(side), CHESSTYPE_4))//defender has a 3 or j3
        {
            search_level_temp = 1;
        }
    }


    if (node->type == Root)
    {
        board->getThreatCandidates(search_level_temp, moves);
    }
    else
    {
        board->getDependentThreatCandidates(node->opera.atack, search_level_temp, moves);
    }
}

void DBSearch::addCombinationStage(DBNode* node, ChessBoard *board, vector<DBNode*> &sequence)
{

    if (level == node->level && node->type == Dependency)
    {
        vector<DBNode*> combine_sequence;
        findAllCombinationNodes(node, sequence, board, root, combine_sequence);
        node->hasCombined = true;
        if (terminate)
        {
            return;
        }
    }
    size_t len = node->child.size();
    for (int i = 0; i < len; ++i)
    {
        if (node->child[i]->isGoal)
        {
            continue;
        }
        ChessBoard tempboard = *board;
        tempboard.move(node->child[i]->opera.atack, rule);
        tempboard.moveMultiReplies(node->child[i]->opera.replies, rule);
        sequence.push_back(node->child[i]);
        addCombinationStage(node->child[i], &tempboard, sequence);
        if (terminate)
        {
            return;
        }
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
        if (node->child[i]->isGoal)
        {
            continue;
        }

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

    if (board->getChessType(opera.atack, board->getLastStep().getOtherSide()) == CHESSTYPE_BAN)
    {
        return true;
    }

    for (uint8_t i = 0; i < opera.replies.size(); ++i)
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

    if (searchLevel == 0)
    {
        return false;
    }
    uint8_t search_level_temp = searchLevel;
    if (!isRefuteSearch)
    {
        if (board->hasChessType(Util::otherside(side), CHESSTYPE_5))//defender has a 4 or d4
        {
            search_level_temp = 0;
        }
        else if (board->hasChessType(Util::otherside(side), CHESSTYPE_4))//defender has a 3 or j3
        {
            search_level_temp = 1;
        }
    }

    vector<StepCandidateItem> candidates1;
    for (int symbol = 0; symbol < 2; ++symbol)//正反
    {
        int d8 = direction * 2 + symbol;
        Position temppos = partner_atack; for (int8_t offset = 0; offset < 4 && temppos.displace8(d8); ++offset)
        {
            if (board->getState(temppos) == PIECE_BLANK)
            {
                if (Util::isthreat(board->getLayer2(temppos, side, direction)))
                {
                    if (search_level_temp < 2)
                    {
                        if (search_level_temp == 0)
                        {
                            if (board->getChessType(temppos, side) != CHESSTYPE_5)
                            {
                                continue;
                            }
                        }
                        else if (Util::isalive3or33(board->getChessType(temppos, side)))
                        {
                            continue;
                        }
                    }
                    candidates1.emplace_back(temppos, 10, direction);
                }
            }
            else if (board->getState(temppos) == side)
            {
                continue;
            }
            else
            {
                break;
            }
        }
    }
    if (candidates1.empty()) return false;
    vector<StepCandidateItem> candidates2;
    for (int symbol = 0; symbol < 2; ++symbol)//正反
    {
        int d8 = direction * 2 + symbol;
        Position temppos = partner_atack; for (int8_t offset = 0; offset < 4 && temppos.displace8(d8); ++offset)
        {
            if (board->getState(temppos) == PIECE_BLANK)
            {
                if (Util::isthreat(board->getLayer2(temppos, side, direction)))
                {
                    if (search_level_temp < 2)
                    {
                        if (search_level_temp == 0)
                        {
                            if (board->getChessType(temppos, side) != CHESSTYPE_5)
                            {
                                continue;
                            }
                        }
                        else if (Util::isalive3or33(board->getChessType(temppos, side)))
                        {
                            continue;
                        }
                    }
                    candidates2.emplace_back(temppos, 10, direction);
                }
            }
            else if (board->getState(temppos) == side)
            {
                continue;
            }
            else
            {
                break;
            }
        }
    }
    if (candidates2.empty()) return false;

    vector<StepCandidateItem> candidates0;

    for (size_t i = 0; i < candidates1.size(); ++i)
    {
        for (size_t j = 0; j < candidates2.size(); ++j)
        {
            if (candidates1[i].pos == candidates2[j].pos)
            {
                candidates0.emplace_back(candidates1[i].pos, candidates1[i].value, candidates1[i].direction);
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
        DBNode* tempnode = new DBNode(Combination, level, combine_node->depth + 1);
        tempnode->opera = combine_sequence[i]->opera;
        tempnode->chessType = combine_sequence[i]->chessType;
        combine_node->child.push_back(tempnode);
        combine_node = tempnode;
    }

    level++;
    addDependentChildrenWithCandidates(combine_node, board, sequence, candidates0);
    level--;
    if (terminate && terminate_type == TerminateType::SUCCESS)
    {
        partner_sequence = sequence;
    }

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
        for (uint8_t j = 0; j < sequence[i]->opera.replies.size(); ++j)
        {
            relatedpos.insert(sequence[i]->opera.replies[j]);
        }
    }

    return proveWinningThreatSequence(&currentboard, relatedpos, sequence, 0);
}

bool DBSearch::proveWinningThreatSequence(ChessBoard *board, set<Position> relatedpos, vector<DBNode*> &sequence, int sequence_index)
{
    if (sequence_index == sequence.size())
    {
        return true;
    }
    DBNode* node = sequence[sequence_index];

    relatedpos.erase(node->opera.atack);

    ChessBoard currentboard = *board;
    currentboard.move(node->opera.atack, rule);

    if (Util::isalive3or33(node->chessType))
    {
        TerminateType result = doRefuteExpand(&currentboard, relatedpos);
        if (result == SUCCESS || result == REFUTE_POS)
        {
            if (result == SUCCESS)
            {
                node->hasRefute = true;
                for (; sequence_index < sequence.size(); ++sequence_index)
                {
                    sequence[sequence_index]->hasRefute = true;
                }
            }
            ++winning_sequence_count;
            return false;
        }
    }
    else if (Util::hasdead4(node->chessType))
    {
        //ToDo 检查之前冲四
        if (currentboard.hasChessType(currentboard.getLastStep().getOtherSide(), CHESSTYPE_5))
        {
            node->hasRefute = true;
            for (; sequence_index < sequence.size(); ++sequence_index)
            {
                sequence[sequence_index]->hasRefute = true;
            }
            ++winning_sequence_count;
            return false;
        }
    }


    for (size_t j = 0; j < node->opera.replies.size(); ++j)
    {
        relatedpos.erase(node->opera.replies[j]);
    }

    for (size_t i = 0; i < node->opera.replies.size(); ++i)
    {
        ChessBoard tempboard = currentboard;
        tempboard.move(node->opera.replies[i], rule);
        bool ret = proveWinningThreatSequence(&tempboard, relatedpos, sequence, sequence_index + 1);
        if (ret == false)
        {
            return false;
        }
    }
    return true;
}

TerminateType DBSearch::doRefuteExpand(ChessBoard *board, set<Position> &relatedpos)
{
    DBSearch dbs(board, rule, 1);
    dbs.setRefute(&relatedpos);
    vector<Position> path;
    bool ret = dbs.doDBSearch(NULL);
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
                ss << "@[" << (int)node->opera.atack.row << "," << (int)node->opera.atack.col << "]" << " ";
            }
            else
            {
                if (node->type == Combination)
                {
                    ss << "&{" << (int)node->opera.atack.row << "," << (int)node->opera.atack.col << "}" << " ";
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
            ss << "& ";
        }
    }
    of << ss.str();
    of.close();
}