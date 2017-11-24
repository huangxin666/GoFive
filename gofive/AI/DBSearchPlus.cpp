#include "DBSearchPlus.h"
#include <queue>

int DBSearchPlus::node_count = 0;
#define MAX_PROVE_FAIL_COUNT 12

void DBSearchPlus::clearTree(DBPlusNode* node)
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

bool DBSearchPlus::doDBSearchPlus(vector<Position> &path)
{
    uint8_t side = board->getLastStep().getOtherSide();
    root = new DBPlusNode(ROOT, 0);
    level = 0;
    treeSizeIncreased = true;
    vector<StepCandidateItem> legalMoves;
    board->getVCFCandidates(legalMoves, NULL);
    if (searchLevel > 1)
    {
        board->getVCTCandidates(legalMoves, NULL);
    }
    std::sort(legalMoves.begin(), legalMoves.begin(), CandidateItemCmp);

    for (size_t i = 0; i < legalMoves.size(); ++i)
    {
        legalMoves[i].value = board->getChessDirection(legalMoves[i].pos, side);
    }
    vector<ThreatMove> sequence;
    VCXRESULT ret = addDependentChildrenWithCandidates(root, board, sequence, legalMoves);

    return ret == VCXRESULT_SUCCESS;
}

VCXRESULT DBSearchPlus::addDependentChildren(DBPlusNode* node, ChessBoard *board, vector<ThreatMove> &sequence)
{
    uint8_t side = board->getLastStep().getOtherSide();
    vector<StepCandidateItem> legalMoves;

    uint8_t current_search_level = searchLevel;
    uint8_t highest = board->getHighestType(Util::otherside(side));

    if (highest > CHESSTYPE_D4P)
    {
        if (highest == CHESSTYPE_5)//defender has a 4 or d4
        {
            current_search_level = 0;
        }
        else if (highest == CHESSTYPE_33)
        {
            current_search_level = current_search_level < 2 ? current_search_level : 2;
        }
        else // double 4
        {
            current_search_level = current_search_level < 1 ? current_search_level : 1;
        }
    }

    board->getDependentThreatCandidates(sequence.back().atack, current_search_level, legalMoves, true);
    return addDependentChildrenWithCandidates(node, board, sequence, legalMoves);
}

VCXRESULT DBSearchPlus::addDependentChildrenWithCandidates(DBPlusNode* node, ChessBoard *board, vector<ThreatMove> &sequence, vector<StepCandidateItem> &legalMoves)
{
    uint8_t side = board->getLastStep().getOtherSide();
    //node -> DEFEND
    if (sequence.size() > max_depth)
    {
        max_depth = (int)sequence.size();
    }
    size_t len = legalMoves.size();

    for (size_t i = 0; i < len; ++i)
    {
        treeSizeIncreased = true;
        DBPlusNode* childnode = new DBPlusNode(ATACK, node->depth + 1);
        node->child.push_back(childnode);
        ChessBoard tempboard = *board;
        uint8_t type = tempboard.getChessType(legalMoves[i].pos, side);
        if (type == CHESSTYPE_43) //本以为是3，其实是d4
        {
            type = CHESSTYPE_D4;
            legalMoves[i].value = board->getChessDirection(legalMoves[i].pos, side);
        }
        else
        {
            type = tempboard.getLayer2(legalMoves[i].pos.row, legalMoves[i].pos.col, side, (uint8_t)legalMoves[i].value);
        }
        childnode->chessType = type;

        childnode->pos = legalMoves[i].pos;
        tempboard.move(legalMoves[i].pos, rule);

        if (tempboard.getHighestType(side) < CHESSTYPE_33)
        {
            continue;
        }

        if (childnode->chessType < CHESSTYPE_5 && tempboard.hasChessType(tempboard.getLastStep().getOtherSide(), CHESSTYPE_5))
        {
            continue;//被反杀了
        }

        if (isRefuteSearch)
        {
            if (goalset->find(legalMoves[i].pos) != goalset->end())
            {
                return VCXRESULT_SUCCESS;
            }
        }

        if (childnode->chessType == CHESSTYPE_5)// find winning threat sequence
        {
            childnode->isGoal = true;
            if (isRefuteSearch)
            {
                return VCXRESULT_SUCCESS;
            }
            else if (proveWinningThreatSequence(sequence))
            {
                return VCXRESULT_SUCCESS;
            }
            continue;
        }

        ThreatMove threat;
        threat.atack = legalMoves[i].pos;
        threat.node = childnode;
        tempboard.getThreatReplies(legalMoves[i].pos, childnode->chessType, (uint8_t)legalMoves[i].value, threat.replies);

        VCXRESULT result = VCXRESULT_SUCCESS;
        for (auto reply : threat.replies)
        {
            ChessBoard defendboard = tempboard;
            defendboard.move(reply, rule);
            threat.reply_applied = reply;
            DBPlusNode* defendNode = new DBPlusNode(DEFEND, node->depth + 2);
            defendNode->pos = reply;
            childnode->child.push_back(defendNode);
            sequence.push_back(threat);
            VCXRESULT ret = addDependentChildren(defendNode, &defendboard, sequence);
            sequence.pop_back();
            if (ret != VCXRESULT_SUCCESS)
            {
                result = ret;
                break;
            }
        }

        if (result == VCXRESULT_SUCCESS)
        {
            return VCXRESULT_SUCCESS;
        }
    }
    return VCXRESULT_FAIL;
}




bool DBSearchPlus::proveWinningThreatSequence(vector<ThreatMove> &sequence)
{
    //init relatedpos
    map<Position, int> relatedpos;

    for (size_t i = 0; i < sequence.size(); ++i)
    {
        if (sequence[i].node->hasRefute)
        {
            return false;
        }
        relatedpos[sequence[i].atack]++;
        for (uint8_t j = 0; j < sequence[i].replies.size(); ++j)
        {
            relatedpos[sequence[i].replies[j]]++;
        }
    }

    ChessBoard currentboard = *board;
    for (size_t i = 0; i < sequence.size(); ++i)
    {
        currentboard.move(sequence[i].atack, rule);
        if (--relatedpos[sequence[i].atack] < 1)
        {
            relatedpos.erase(sequence[i].atack);
        }
        if (Util::isalive3or33(sequence[i].node->chessType))
        {
            TerminateType result = doRefuteExpand(&currentboard, relatedpos);
            if (result == SUCCESS || result == REFUTE_POS)
            {
                if (result == SUCCESS)
                {
                    sequence[i].node->hasRefute = true;
                }
                ++winning_sequence_count;
                return false;
            }
        }
        else if (Util::hasdead4(sequence[i].node->chessType))
        {
            //ToDo 检查之前冲四
            if (currentboard.hasChessType(currentboard.getLastStep().getOtherSide(), CHESSTYPE_5))
            {
                sequence[i].node->hasRefute = true;
                ++winning_sequence_count;
                return false;
            }
        }
        else if (sequence[i].node->chessType == CHESSTYPE_5)
        {
            return true;
        }
        currentboard.move(sequence[i].reply_applied, rule);

        for (uint8_t j = 0; j < sequence[i].replies.size(); ++j)
        {
            if (--relatedpos[sequence[i].replies[j]] < 1)
            {
                relatedpos.erase(sequence[i].replies[j]);
            }
        }
    }
    return true;
}

TerminateType DBSearchPlus::doRefuteExpand(ChessBoard *board, map<Position, int> &relatedpos)
{
    DBSearchPlus dbs(board, FREESTYLE, 1);
    dbs.setRefute(&relatedpos);
    vector<Position> path;
    bool ret = dbs.doDBSearchPlus(path);
    return dbs.getResult();
}

//void DBSearchPlus::getDependentCandidates(DBPlusNode* node, ChessBoard *board, vector<StepCandidateItem>& moves)
//{
//    uint8_t side = board->getLastStep().getOtherSide();
//    if (board->hasChessType(side, CHESSTYPE_5))
//    {
//        ForEachPosition
//        {
//            if (!board->canMove(pos.row,pos.col)) continue;
//            if (board->getChessType(pos,side) == CHESSTYPE_5)
//            {
//                moves.emplace_back(pos, board->getChessDirection(pos, side));
//                return;
//            }
//        }
//    }
//
//
//
//
//    if (node->type == Root)
//    {
//        board->getVCFCandidates(moves, NULL);
//        if (searchLevel > 1)
//        {
//            board->getVCTCandidates(moves, NULL);
//        }
//        std::sort(moves.begin(), moves.begin(), CandidateItemCmp);
//
//        for (size_t i = 0; i < moves.size(); ++i)
//        {
//            moves[i].value = board->getChessDirection(moves[i].pos, side);
//        }
//    }
//    else
//    {
//        for (int d = 0; d < DIRECTION4_COUNT; ++d)
//        {
//            for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
//            {
//                Position temppos = node->opera.atack;
//                for (int8_t offset = 1; offset < 5; ++offset)
//                {
//                    if (!temppos.displace4(symbol, d))//equal otherside
//                    {
//                        break;
//                    }
//                    if (board->getState(temppos.row, temppos.col) == PIECE_BLANK)
//                    {
//                        if (Util::isthreat(board->getLayer2(temppos.row, temppos.col, side, d)))
//                        {
//                            if (search_level_temp < 2 && Util::isalive3or33(board->getChessType(temppos, side)))
//                            {
//                                continue;
//                            }
//                            if (board->getChessType(temppos, side) == CHESSTYPE_BAN)
//                            {
//                                continue;
//                            }
//                            moves.emplace_back(temppos, d);
//                        }
//                    }
//                    else if (board->getState(temppos.row, temppos.col) == side)
//                    {
//                        continue;
//                    }
//                    else
//                    {
//                        break;
//                    }
//                }
//            }
//        }
//    }
//}

//void DBSearchPlus::addCombinationStage(DBPlusNode* node, ChessBoard *board, vector<DBPlusNode*> &sequence)
//{
//
//    if (level == node->level && node->type == Dependency)
//    {
//        vector<DBPlusNode*> combine_sequence;
//        findAllCombinationNodes(node, sequence, board, root, combine_sequence);
//        node->hasCombined = true;
//        if (terminate)
//        {
//            return;
//        }
//    }
//    size_t len = node->child.size();
//    for (int i = 0; i < len; ++i)
//    {
//        if (node->child[i]->isGoal)
//        {
//            continue;
//        }
//        ChessBoard tempboard = *board;
//        tempboard.move(node->child[i]->opera.atack, rule);
//        tempboard.moveMultiReplies(node->child[i]->opera.replies, node->child[i]->opera.replies_size, rule);
//        sequence.push_back(node->child[i]);
//        addCombinationStage(node->child[i], &tempboard, sequence);
//        if (terminate)
//        {
//            return;
//        }
//        sequence.pop_back();
//    }
//
//}
//
//void DBSearchPlus::findAllCombinationNodes(DBPlusNode* partner, vector<DBPlusNode*> &partner_sequence, ChessBoard *combined_board, DBPlusNode* node, vector<DBPlusNode*> &combine_sequence)
//{
//    if (node->hasCombined && level == node->level)
//    {
//        return;
//    }
//    size_t len = node->child.size();
//    for (int i = 0; i < len; ++i)
//    {
//        if (node->child[i]->isGoal)
//        {
//            continue;
//        }
//
//        if (!inConflict(combined_board, node->child[i]->opera))//未冲突
//        {
//            combine_sequence.push_back(node->child[i]);
//            ChessBoard tempboard = *combined_board;
//            tempboard.move(node->child[i]->opera.atack, rule);
//            tempboard.moveMultiReplies(node->child[i]->opera.replies, node->child[i]->opera.replies_size, rule);
//            if (node->child[i]->type == Dependency)
//            {
//                if (testAndAddCombination(partner, partner_sequence, &tempboard, node->child[i]->opera.atack, combine_sequence))
//                {
//                    treeSizeIncreased = true;
//                }
//                if (terminate)
//                {
//                    return;
//                }
//            }
//            findAllCombinationNodes(partner, partner_sequence, &tempboard, node->child[i], combine_sequence);
//            if (terminate)
//            {
//                return;
//            }
//            combine_sequence.pop_back();
//        }
//        else//冲突了就没必要再检查node->child[i]的儿子了
//        {
//            continue;
//        }
//    }
//}
//
//bool DBSearchPlus::inConflict(ChessBoard *board, DBMetaOperator &opera)
//{
//    if (board->getState(opera.atack) != PIECE_BLANK)
//    {
//        return true;
//    }
//
//    if (board->getChessType(opera.atack, board->getLastStep().getOtherSide()) == CHESSTYPE_BAN)
//    {
//        return true;
//    }
//
//    for (uint8_t i = 0; i < opera.replies_size; ++i)
//    {
//        if (board->getState(opera.replies[i]) != PIECE_BLANK)
//        {
//            return true;
//        }
//    }
//    return false;
//}
//
//bool DBSearchPlus::testAndAddCombination(DBPlusNode* partner, vector<DBPlusNode*> &partner_sequence, ChessBoard *board, Position node_atack, vector<DBPlusNode*> &combine_sequence)
//{
//    Position partner_atack = partner->opera.atack;
//    int row = partner_atack.row - node_atack.row;
//    int col = partner_atack.col - node_atack.col;
//    uint8_t direction;
//    Position startpos;
//    //首先判断是否在一条直线上
//    if (row == 0)
//    {
//        direction = DIRECTION4::DIRECTION4_LR;
//    }
//    else if (col == 0)
//    {
//        direction = DIRECTION4::DIRECTION4_UD;
//    }
//    else if (row == col)
//    {
//        direction = DIRECTION4::DIRECTION4_RD;
//    }
//    else if (row == -col)
//    {
//        direction = DIRECTION4::DIRECTION4_RU;
//    }
//    else
//    {
//        return false;
//    }
//
//    uint8_t side = board->getLastStep().getOtherSide();
//
//    if (searchLevel == 0)
//    {
//        return false;
//    }
//    uint8_t search_level_temp = searchLevel;
//    if (!isRefuteSearch)
//    {
//        if (board->hasChessType(Util::otherside(side), CHESSTYPE_5))//defender has a 4 or d4
//        {
//            search_level_temp = 0;
//        }
//        else if (board->hasChessType(Util::otherside(side), CHESSTYPE_4))//defender has a 3 or j3
//        {
//            search_level_temp = 1;
//        }
//    }
//
//    vector<StepCandidateItem> candidates1;
//    for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
//    {
//        Position temppos = partner_atack;
//        for (int8_t offset = 1; offset < 5; ++offset)
//        {
//            if (!temppos.displace4(symbol, direction))//equal otherside
//            {
//                break;
//            }
//            if (board->getState(temppos.row, temppos.col) == PIECE_BLANK)
//            {
//                if (Util::isthreat(board->getLayer2(temppos.row, temppos.col, side, direction)))
//                {
//                    if (search_level_temp < 2)
//                    {
//                        if (search_level_temp == 0)
//                        {
//                            if (board->getChessType(temppos, side) != CHESSTYPE_5)
//                            {
//                                continue;
//                            }
//                        }
//                        else if (Util::isalive3or33(board->getChessType(temppos, side)))
//                        {
//                            continue;
//                        }
//                    }
//                    candidates1.emplace_back(temppos, direction);
//                }
//            }
//            else if (board->getState(temppos.row, temppos.col) == side)
//            {
//                continue;
//            }
//            else
//            {
//                break;
//            }
//        }
//    }
//    if (candidates1.empty()) return false;
//    vector<StepCandidateItem> candidates2;
//    for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
//    {
//        Position temppos = partner_atack;
//        for (int8_t offset = 1; offset < 5; ++offset)
//        {
//            if (!temppos.displace4(symbol, direction))//equal otherside
//            {
//                break;
//            }
//            if (board->getState(temppos.row, temppos.col) == PIECE_BLANK)
//            {
//                if (Util::isthreat(board->getLayer2(temppos.row, temppos.col, side, direction)))
//                {
//                    if (search_level_temp < 2)
//                    {
//                        if (search_level_temp == 0)
//                        {
//                            if (board->getChessType(temppos, side) != CHESSTYPE_5)
//                            {
//                                continue;
//                            }
//                        }
//                        else if (Util::isalive3or33(board->getChessType(temppos, side)))
//                        {
//                            continue;
//                        }
//                    }
//                    candidates2.emplace_back(temppos, direction);
//                }
//            }
//            else if (board->getState(temppos.row, temppos.col) == side)
//            {
//                continue;
//            }
//            else
//            {
//                break;
//            }
//        }
//    }
//    if (candidates2.empty()) return false;
//
//    vector<StepCandidateItem> candidates0;
//
//    for (size_t i = 0; i < candidates1.size(); ++i)
//    {
//        for (size_t j = 0; j < candidates2.size(); ++j)
//        {
//            if (candidates1[i].pos == candidates2[j].pos)
//            {
//                candidates0.emplace_back(candidates1[i].pos, candidates1[i].value);
//            }
//        }
//    }
//    if (candidates0.empty()) return false;
//
//    size_t len;
//    //start combine node
//    vector<DBPlusNode*> sequence = partner_sequence;
//    DBPlusNode* combine_node = partner;
//    len = combine_sequence.size();
//    for (size_t i = 0; i < len; ++i)
//    {
//        sequence.push_back(combine_sequence[i]);
//        DBPlusNode* tempnode = new DBPlusNode(Combination, level);
//        tempnode->opera = combine_sequence[i]->opera;
//        tempnode->chessType = combine_sequence[i]->chessType;
//        combine_node->child.push_back(tempnode);
//        combine_node = tempnode;
//    }
//
//    level++;
//    addDependentChildrenWithCandidates(combine_node, board, sequence, candidates0);
//    level--;
//
//    return true;
//}
