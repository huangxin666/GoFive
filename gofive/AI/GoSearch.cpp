#include "GoSearch.h"
#include <algorithm>

#define GOSEARCH_DEBUG

HashStat GoSearchEngine::transTableStat;
string GoSearchEngine::textout;

bool CandidateItemCmp(const StepCandidateItem &a, const StepCandidateItem &b)
{
    return a.priority > b.priority;
}

GoSearchEngine::GoSearchEngine() :board(NULL)
{
}

GoSearchEngine::~GoSearchEngine()
{

}

void GoSearchEngine::initSearchEngine(ChessBoard* board, ChessStep lastStep)
{
    GoSearchEngine::transTableStat = { 0,0,0 };
    this->board = board;
    this->startStep = lastStep;
    //this->maxSearchTime = 20;
    textout.clear();
}

void GoSearchEngine::textOutSearchInfo(OptimalPath& optimalPath)
{
    if (optimalPath.path.size() == 0)
    {
        textout = "error";
        return;
    }
    uint8_t nextIndex = optimalPath.path[0];
    char text[1024];
    int len = 0;
    len += snprintf(text + len, 1024, "currentDepth:%d-%d, caculatetime:%llus, rating:%d, next:%d,%d\r\n",
        global_currentMaxDepth, optimalPath.endStep - startStep.step, std::time(NULL) - global_startSearchTime, optimalPath.situationRating, util::getrow(nextIndex), util::getcol(nextIndex));
    textout += text;
}

void GoSearchEngine::textOutPathInfo(OptimalPath& optimalPath)
{
    char text[1024];
    int len = 0;
    len += snprintf(text + len, 1024, "path:");
    for (auto p : optimalPath.path)
    {
        len += snprintf(text + len, 1024, " %d,%d ", util::getrow(p), util::getcol(p));
    }
    len += snprintf(text + len, 1024, "\r\n");
    textout += text;
}

uint8_t GoSearchEngine::getBestStep()
{
    global_isOverTime = false;

    global_startSearchTime = std::time(NULL);

    OptimalPath optimalPath;
    for (global_currentMaxDepth = minAlphaBetaDepth;
        global_currentMaxDepth <= maxAlphaBetaDepth;
        ++global_currentMaxDepth)
    {
        if (std::time(NULL) - global_startSearchTime >= maxSearchTime / 5)
        {
            break;
        }
        OptimalPath temp = solveBoard(board);
        if (global_currentMaxDepth > minAlphaBetaDepth && global_isOverTime)
        {
            break;
        }
        optimalPath = temp;
    }

    transTable.clear();
    transTableSpecial.clear();
    textOutPathInfo(optimalPath);
    return optimalPath.path[0];
}


OptimalPath GoSearchEngine::solveBoard(ChessBoard* board)
{
    int alpha = INT_MIN, beta = INT_MAX;
    OptimalPath optimalPath;
    optimalPath.startStep = startStep.step;
    optimalPath.endStep = startStep.step;
    optimalPath.situationRating = board->getGlobalEvaluate(getAISide());

    uint8_t side = util::otherside(board->getLastStep().getColor());

    PieceInfo otherhighest = board->getHighestInfo(util::otherside(side));
    PieceInfo selfhighest = board->getHighestInfo(side);

    vector<StepCandidateItem> moves;
    if (selfhighest.chessmode == CHESSTYPE_5)
    {
        optimalPath.path.push_back(selfhighest.index);
        optimalPath.endStep += 1;
        goto end;
    }
    else if (otherhighest.chessmode == CHESSTYPE_5)//敌方马上5连
    {
        optimalPath.path.push_back(otherhighest.index);
        optimalPath.endStep += 1;
        goto end;
    }
    else if (util::hasfourkill(selfhighest.chessmode))//我方有4杀
    {
        optimalPath.path.push_back(selfhighest.index);
        optimalPath.endStep += 1;
        goto end;
    }
    else if (doVCFSearch(board, side, optimalPath))
    {
        goto end;
    }
    else if (util::hasfourkill(otherhighest.chessmode))//敌方有4杀
    {
        getFourkillDefendSteps(board, otherhighest.index, moves);
    }
    else if (doVCTSearch(board, side, optimalPath))
    {
        goto end;
    }
    else if (otherhighest.chessmode == CHESSTYPE_33)
    {
        getFourkillDefendSteps(board, otherhighest.index, moves);
    }
    else
    {
        getNormalSteps(board, moves);
    }

    for (size_t i = 0; i < moves.size(); ++i)
    {
        ChessBoard tempboard = *board;
        tempboard.move(moves[i].index);

        OptimalPath tempPath;
        tempPath.startStep = board->getLastStep().step;
        tempPath.path.push_back(moves[i].index);
        tempPath.endStep = board->getLastStep().step + 1;

        if (board->getChessType(moves[i].index, side) == CHESSTYPE_BAN)
        {
            tempPath.situationRating = -(side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5));
        }
        else
        {
            doAlphaBetaSearch(&tempboard, alpha, beta, tempPath);
        }

        if (tempPath.path.size() == 1)
        {
            tempPath.situationRating = tempboard.getGlobalEvaluate(getAISide());
            if (side == getAISide())
            {
                tempPath.situationRating = -tempPath.situationRating;
            }
            //tempPath.situationRating = tempboard.getSituationRating(getAISide());
        }
    }

    if (optimalPath.path.empty())
    {
        optimalPath.situationRating = -10000;
        optimalPath.endStep++;
        optimalPath.path.push_back(otherhighest.index);
    }

end:
    textOutSearchInfo(optimalPath);
    return optimalPath;
}


void GoSearchEngine::doAlphaBetaSearchWrapper(ChessBoard* board, int alpha, int beta, OptimalPath& optimalPath)
{
    if (board->getLastStep().step < startStep.step + global_currentMaxDepth)
    {
        TransTableData data;
        if (getTransTable(board->getBoardHash().z32key, data))
        {
            if (data.checkHash == board->getBoardHash().z64key)
            {
                if ((data.isEnd() || data.endStep >= startStep.step + global_currentMaxDepth))
                {
                    transTableStat.hit++;
                    optimalPath.situationRating = data.situationRating;
                    optimalPath.endStep = data.endStep;
                    return;
                }
                else
                {
                    transTableStat.cover++;
                }
            }
            else
            {
                transTableStat.clash++;
            }
        }
        else
        {
            transTableStat.miss++;
        }
        doAlphaBetaSearch(board, alpha, beta, optimalPath);
        if (optimalPath.path.size() > 1)//下一步就结束了的没必要写进置换表
        {
            data.checkHash = board->getBoardHash().z64key;
            data.endStep = optimalPath.endStep;
            data.situationRating = optimalPath.situationRating;
            putTransTable(board->getBoardHash().z32key, data);
        }
    }
    else
    {
        doAlphaBetaSearch(board, alpha, beta, optimalPath);
    }
}

void GoSearchEngine::doAlphaBetaSearch(ChessBoard* board, int alpha, int beta, OptimalPath& optimalPath)
{
    uint8_t steps = board->getLastStep().step;
    steps = steps;
    uint8_t side = util::otherside(board->getLastStep().getColor());

    if (board->getHighestInfo(side).chessmode == CHESSTYPE_5)
    {
        optimalPath.situationRating = side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
        optimalPath.path.push_back(board->getHighestInfo(side).index);
        optimalPath.endStep = board->getLastStep().step + 1;
        return;
    }

    //超时
    if (global_isOverTime || std::time(NULL) - global_startSearchTime > maxSearchTime)
    {
        global_isOverTime = true;
        return;
    }

    if (board->getLastStep().step - startStep.step >= global_currentMaxDepth)
    {
        if (doVCFSearch(board, side, optimalPath))
        {
            return;
        }
        return;
    }

    PieceInfo otherhighest = board->getHighestInfo(util::otherside(side));
    PieceInfo selfhighest = board->getHighestInfo(side);

    vector<StepCandidateItem> moves;

    if (otherhighest.chessmode == CHESSTYPE_5)//敌方马上5连
    {
        if (board->getChessType(otherhighest.index, side) == CHESSTYPE_BAN)
        {
            optimalPath.path.push_back(otherhighest.index);
            optimalPath.endStep = board->getLastStep().step + 1;
            optimalPath.situationRating = -(side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5));
            return;
        }
        moves.emplace_back(otherhighest.index, 10);
    }
    else if (util::hasfourkill(selfhighest.chessmode))//我方有4杀
    {
        moves.emplace_back(selfhighest.index, 10);
    }
    else if (doVCFSearch(board, side, optimalPath))
    {
        return;
    }
    else if (util::hasfourkill(otherhighest.chessmode))//敌方有4杀
    {
        getFourkillDefendSteps(board, otherhighest.index, moves);
    }
    else if (doVCTSearch(board, side, optimalPath))
    {
        return;
    }
    else if (otherhighest.chessmode == CHESSTYPE_33)
    {
        getFourkillDefendSteps(board, otherhighest.index, moves);
    }
    else
    {
        getNormalSteps(board, moves);
    }

    OptimalPath bestPath;
    if (side != startStep.getColor())//build AI
    {
        bestPath.situationRating = INT_MIN;
    }
    else
    {
        bestPath.situationRating = INT_MAX;
    }

    for (size_t i = 0; i < moves.size(); ++i)
    {
        ChessBoard tempboard = *board;
        tempboard.move(moves[i].index);

        OptimalPath tempPath;
        tempPath.startStep = board->getLastStep().step;
        tempPath.path.push_back(moves[i].index);
        tempPath.endStep = board->getLastStep().step + 1;

        if (board->getChessType(moves[i].index, side) == CHESSTYPE_BAN)
        {
            tempPath.situationRating = -(side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5));
        }
        else
        {
            doAlphaBetaSearchWrapper(&tempboard, alpha, beta, tempPath);
        }

        if (tempPath.path.size() == 1)
        {
            tempPath.situationRating = tempboard.getGlobalEvaluate(getAISide());
            if (side == getAISide())
            {
                tempPath.situationRating = -tempPath.situationRating;
            }
            //tempPath.situationRating = tempboard.getSituationRating(getAISide());
        }

        if (side != startStep.getColor())//build AI
        {
            if (tempPath.situationRating > bestPath.situationRating
                || (tempPath.situationRating == bestPath.situationRating && tempPath.endStep < bestPath.endStep)
                )
            {
                bestPath.situationRating = tempPath.situationRating;
                bestPath.startStep = tempPath.startStep;
                bestPath.path.swap(tempPath.path);
                bestPath.endStep = tempPath.endStep;
            }

            if (tempPath.situationRating > beta || tempPath.situationRating == util::type2score(CHESSTYPE_5))//beta cut
            {
                break;
            }
            if (tempPath.situationRating > alpha)//update alpha
            {
                alpha = tempPath.situationRating;
            }
        }
        else // build player
        {
            if (tempPath.situationRating < bestPath.situationRating
                || (tempPath.situationRating == bestPath.situationRating && tempPath.endStep > bestPath.endStep)
                )
            {
                bestPath.situationRating = tempPath.situationRating;
                bestPath.startStep = tempPath.startStep;
                bestPath.path.swap(tempPath.path);
                bestPath.endStep = tempPath.endStep;
            }

            if (tempPath.situationRating < alpha || tempPath.situationRating == -util::type2score(CHESSTYPE_5))//alpha cut
            {
                break;
            }
            if (tempPath.situationRating < beta)//update beta
            {
                beta = tempPath.situationRating;
            }
        }
    }

    if (!bestPath.path.empty())
    {
        optimalPath.situationRating = bestPath.situationRating;
        optimalPath.cat(bestPath);
    }
}

void GoSearchEngine::getNormalSteps(ChessBoard* board, vector<StepCandidateItem>& childs)
{
    uint8_t side = util::otherside(board->getLastStep().getColor());

    for (uint8_t index = 0; util::valid(index); ++index)
    {
        if (!board->canMove(index))
        {
            continue;
        }

        uint8_t selfp = board->getChessType(index, side);
        uint8_t otherp = board->getChessType(index, util::otherside(side));

        int8_t priority = chesstypes[selfp].atackPriority + chesstypes[otherp].defendPriority;

        if (priority <= 0)
        {
            continue;
        }

        if (util::inLocalArea(index, board->getLastStep().index, LOCAL_SEARCH_RANGE))
        {
            priority++;
        }

        childs.emplace_back(index, priority);
    }
    std::sort(childs.begin(), childs.end(), CandidateItemCmp);
    //if (childs.size() > MAX_CHILD_NUM)
    //{
    //    childs.erase(childs.begin() + MAX_CHILD_NUM, childs.end());//只保留10个
    //}
}

void GoSearchEngine::getFourkillDefendSteps(ChessBoard* board, uint8_t index, vector<StepCandidateItem>& moves)
{
    uint8_t side = util::otherside(board->getLastStep().getColor());
    uint8_t defendType = board->getChessType(index, board->getLastStep().getColor());
    ChessStep lastStep = board->getLastStep();
    vector<int> direction;

    moves.emplace_back(index, 0);

    if (defendType == CHESSTYPE_4)
    {
        for (int d = 0; d < DIRECTION4_COUNT; ++d)
        {
            if (board->pieces_layer2[index][d][util::otherside(side)] == CHESSTYPE_4)
            {
                direction.push_back(d * 2);
                direction.push_back(d * 2 + 1);
                break;
            }
        }
    }
    else if (defendType == CHESSTYPE_44)
    {
        for (int d = 0; d < DIRECTION4_COUNT; ++d)
        {
            if (board->pieces_layer2[index][d][util::otherside(side)] == CHESSTYPE_44)
            {
                direction.push_back(d * 2);
                direction.push_back(d * 2 + 1);
                break;
            }
            else if (util::isdead4(board->pieces_layer2[index][d][util::otherside(side)]))
            {
                direction.push_back(d * 2);
                direction.push_back(d * 2 + 1);
            }
        }
    }
    else if (defendType == CHESSTYPE_43)
    {
        for (int d = 0; d < DIRECTION4_COUNT; ++d)
        {
            if (util::isdead4(board->pieces_layer2[index][d][util::otherside(side)]) || util::isalive3(board->pieces_layer2[index][d][util::otherside(side)]))
            {
                direction.push_back(d * 2);
                direction.push_back(d * 2 + 1);
            }
        }
    }
    else if (defendType == CHESSTYPE_33)
    {
        for (int d = 0; d < DIRECTION4_COUNT; ++d)
        {
            if (util::isalive3(board->pieces_layer2[index][d][util::otherside(side)]))
            {
                direction.push_back(d * 2);
                direction.push_back(d * 2 + 1);
            }
        }
    }
    else
    {
        return;
    }

    uint8_t tempType;
    for (int n : direction)
    {
        int r = util::getrow(index), c = util::getcol(index);
        int blankCount = 0, chessCount = 0;
        while (board->nextPosition(r, c, 1, n)) //如果不超出边界
        {
            if (board->getState(r, c) == PIECE_BLANK)
            {
                blankCount++;
                if (!board->isHot(r, c))
                {
                    continue;
                }

                tempType = board->getChessType(r, c, util::otherside(side));
                if (tempType > CHESSTYPE_D3P)
                {
                    tempType = board->getThreat(r, c, side);
                    if (tempType == CHESSTYPE_BAN)//被禁手了
                    {
                        continue;
                    }
                    board->move(r, c);
                    if (board->getHighestInfo(util::otherside(side)).chessmode < CHESSTYPE_33)
                    {
                        board->unmove(r, c, lastStep);
                        moves.emplace_back(util::xy2index(r, c), 10);
                    }
                    else
                    {
                        board->unmove(r, c, lastStep);
                        break;
                    }
                }
            }
            else if (board->getState(r, c) == side)
            {
                break;
            }
            else
            {
                chessCount++;
            }

            if (blankCount == 2
                || chessCount > 3)
            {
                break;
            }
        }
    }
}

void GoSearchEngine::getVCFAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, bool global)
{
    uint8_t side = util::otherside(board->getLastStep().getColor());
    if (board->getHighestInfo(util::otherside(side)).chessmode == CHESSTYPE_5)
    {
        if (util::hasdead4(board->getChessType(board->getHighestInfo(util::otherside(side)).index, side)))
        {
            moves.emplace_back(board->getHighestInfo(util::otherside(side)).index, 0);
        }
        return;
    }

    for (uint8_t index = 0; util::valid(index); ++index)
    {
        if (!board->canMove(index))
        {
            continue;
        }
        if (!global)//局部
        {
            if (util::inLocalArea(index, board->getLastStep().index, LOCAL_SEARCH_RANGE))
            {
                if (util::hasdead4(board->getChessType(index, side)))
                {
                    moves.emplace_back(index, 0);
                }
            }
            else
            {
                continue;
            }
        }
        else
        {
            if (util::inLocalArea(index, board->getLastStep().index, LOCAL_SEARCH_RANGE))
            {
                if (util::hasdead4(board->getChessType(index, side)))
                {
                    moves.emplace_back(index, 1);
                }
            }
            else
            {
                if (util::hasdead4(board->getChessType(index, side)))
                {
                    moves.emplace_back(index, 0);
                }
            }
        }


    }
    std::sort(moves.begin(), moves.end(), CandidateItemCmp);
}

bool GoSearchEngine::doVCFSearchWrapper(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, bool global)
{
    TransTableDataSpecial data;
    if (getTransTableSpecial(board->getBoardHash().z32key, data))
    {
        if (data.checkHash == board->getBoardHash().z64key)
        {
            if (data.VCFflag != TransTableSpecialFlag_UNKNOWN)
            {
                transTableStat.hit++;
                return data.VCFflag == TransTableSpecialFlag_TRUE ? true : false;
            }
            else
            {
                transTableStat.cover++;
            }
        }
        else
        {
            transTableStat.clash++;
        }
    }
    else
    {
        transTableStat.miss++;
    }
    bool flag = doVCFSearch(board, side, optimalPath);
    if (optimalPath.path.size() > 1)//下一步就结束了的没必要写进置换表
    {
        data.checkHash = board->getBoardHash().z64key;
        data.VCFflag = flag ? TransTableSpecialFlag_TRUE : TransTableSpecialFlag_FALSE;
        data.type = TRANSTYPE_VCF;
        putTransTableSpecial(board->getBoardHash().z32key, data);
    }
    return flag;
}

bool GoSearchEngine::doVCFSearch(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, bool global)
{

    if (board->getHighestInfo(side).chessmode == CHESSTYPE_5)
    {
        optimalPath.path.push_back(board->getHighestInfo(side).index);
        optimalPath.endStep = board->getLastStep().step + 1;
        optimalPath.situationRating = side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
        return true;
    }

    if (board->getLastStep().step - startStep.step > maxVCFDepth)
    {
        return false;
    }

    if (global_isOverTime || std::time(NULL) - global_startSearchTime > maxSearchTime)
    {
        global_isOverTime = true;
        return false;
    }

    vector<StepCandidateItem> moves;
    getVCFAtackSteps(board, moves);

    for (auto item : moves)
    {
        ChessBoard tempboard = *board;
        tempboard.move(item.index);//冲四

        OptimalPath tempPath;
        tempPath.startStep = optimalPath.endStep;
        tempPath.endStep = optimalPath.endStep + 1;
        tempPath.path.push_back(item.index);

        if (tempboard.getHighestInfo(util::otherside(side)).chessmode == CHESSTYPE_5)
        {
            return false;
        }
        if (tempboard.getChessType(tempboard.getHighestInfo(side).index, util::otherside(side)) == CHESSTYPE_BAN)//敌方触发禁手，VCF成功
        {
            optimalPath.situationRating = side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
            optimalPath.cat(tempPath);
            return true;
        }
        tempboard.move(tempboard.getHighestInfo(side).index);//防五连

        tempPath.endStep = tempPath.endStep + 1;
        tempPath.path.push_back(tempboard.getHighestInfo(side).index);

        if (doVCFSearchWrapper(&tempboard, side, tempPath))
        {
            optimalPath.situationRating = side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
            optimalPath.cat(tempPath);
            return true;
        }
    }
    return false;
}

bool GoSearchEngine::doVCTSearchWrapper(ChessBoard* board, uint8_t side, OptimalPath& optimalPath)
{
    TransTableDataSpecial data;
    if (getTransTableSpecial(board->getBoardHash().z32key, data))
    {
        if (data.checkHash == board->getBoardHash().z64key)
        {
            if (data.VCTflag != TransTableSpecialFlag_UNKNOWN)
            {
                transTableStat.hit++;
                return data.VCTflag == TransTableSpecialFlag_TRUE ? true : false;
            }
            else
            {
                transTableStat.cover++;
            }
        }
        else
        {
            transTableStat.clash++;
        }
    }
    else
    {
        transTableStat.miss++;
    }
    bool flag = doVCTSearch(board, side, optimalPath);
    if (optimalPath.path.size() > 1)//下一步就结束了的没必要写进置换表
    {
        data.checkHash = board->getBoardHash().z64key;
        data.VCTflag = flag ? TransTableSpecialFlag_TRUE : TransTableSpecialFlag_FALSE;
        data.type = TRANSTYPE_VCT;
        putTransTableSpecial(board->getBoardHash().z32key, data);
    }
    return flag;
}


bool GoSearchEngine::doVCTSearch(ChessBoard* board, uint8_t side, OptimalPath& optimalPath)
{
    uint8_t test = board->getLastStep().index;
    if (board->getHighestInfo(side).chessmode == CHESSTYPE_5)
    {
        optimalPath.path.push_back(board->getHighestInfo(side).index);
        optimalPath.endStep += 1;
        optimalPath.situationRating = side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
        return true;
    }
    else if (board->getHighestInfo(util::otherside(side)).chessmode == CHESSTYPE_5)
    {
        return false;
    }

    if (board->getLastStep().step - startStep.step > maxVCTDepth)
    {
        return false;
    }

    if (global_isOverTime || std::time(NULL) - global_startSearchTime > maxSearchTime)
    {
        global_isOverTime = true;
        return false;
    }

    if (doVCFSearch(board, side, optimalPath))
    {
        return true;
    }

    vector<StepCandidateItem> moves;
    getVCTAtackSteps(board, moves);

    for (auto item : moves)
    {
        ChessBoard tempboard = *board;
        tempboard.move(item.index);

        OptimalPath tempPath;
        tempPath.startStep = optimalPath.endStep;
        tempPath.endStep = optimalPath.endStep + 1;
        tempPath.path.push_back(item.index);

        if (doVCFSearch(&tempboard, util::otherside(side), tempPath))
        {
            continue;
        }
        vector<StepCandidateItem> defendmoves;
        getFourkillDefendSteps(&tempboard, tempboard.getHighestInfo(side).index, defendmoves);

        bool flag = true;
        OptimalPath tempPath2;
        tempPath2.startStep = tempPath.endStep;
        tempPath2.endStep = tempPath.endStep;
        for (auto defend : defendmoves)
        {
            ChessBoard tempboard2 = tempboard;
            tempboard2.move(defend.index);
            tempPath2.path.clear();
            tempPath2.endStep = tempPath.endStep + 1;
            tempPath2.path.push_back(defend.index);
            if (!doVCTSearchWrapper(&tempboard2, side, tempPath2))
            {
                flag = false;
                break;
            }
        }
        if (flag)
        {
            tempPath.cat(tempPath2);
            optimalPath.cat(tempPath);
            optimalPath.situationRating = side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
            return true;
        }
    }
    return false;
}

void GoSearchEngine::getVCTAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, bool global)
{
    uint8_t side = util::otherside(board->getLastStep().getColor());


    for (uint8_t index = 0; util::valid(index); ++index)
    {
        if (!board->canMove(index))
        {
            continue;
        }
        if (!global)//局部
        {
            if (!util::inLocalArea(index, board->getLastStep().index, LOCAL_SEARCH_RANGE))
            {
                continue;
            }
        }

        if (util::isalive3(board->getChessType(index, side)) || board->getChessType(index, side) == CHESSTYPE_33)
        {
            if (util::inLocalArea(index, board->getLastStep().index, LOCAL_SEARCH_RANGE))
            {
                moves.emplace_back(index, 10);
            }
            else
            {
                moves.emplace_back(index, 8);
            }
        }
        else if (util::isdead4(board->getChessType(index, side)))
        {
            vector<int> direction;
            for (int i = 0; i < DIRECTION4::DIRECTION4_COUNT; ++i)
            {
                if (util::isdead4(board->pieces_layer2[index][i][side]))
                {
                    continue;
                }
                direction.push_back(i * 2);
                direction.push_back(i * 2 + 1);
            }
            if (direction.empty())
            {
                continue;
            }
            ChessBoard tempboard = *board;
            tempboard.moveTemporary(index);

            for (int n : direction)
            {
                int r = util::getrow(index), c = util::getcol(index);
                int blankCount = 0, chessCount = 0;
                while (ChessBoard::nextPosition(r, c, 1, n)) //如果不超出边界
                {
                    if (tempboard.getState(r, c) == PIECE_BLANK)
                    {
                        blankCount++;
                        if (!tempboard.isHot(r, c))
                        {
                            continue;
                        }
                        if (util::isalive3(tempboard.getChessType(r, c, (side))))
                        {
                            if (util::inLocalArea(util::xy2index(r, c), board->getLastStep().index, LOCAL_SEARCH_RANGE))
                            {
                                moves.emplace_back(util::xy2index(r, c), 9);
                            }
                            else
                            {
                                moves.emplace_back(util::xy2index(r, c), 7);
                            }
                        }
                    }
                    else if (tempboard.getState(r, c) == side)
                    {
                        break;
                    }
                    else
                    {
                        chessCount++;
                    }

                    if (blankCount == 2 || chessCount == 2)
                    {
                        break;
                    }
                }
            }
        }
    }

    std::sort(moves.begin(), moves.end(), CandidateItemCmp);
    if (moves.size() > MAX_CHILD_NUM)
    {
        moves.erase(moves.begin() + MAX_CHILD_NUM, moves.end());//只保留10个
    }
}