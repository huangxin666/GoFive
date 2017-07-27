#include "GoSearch.h"
#include "utils.h"


#define GOSEARCH_DEBUG

HashStat GoSearchEngine::transTableStat;
string GoSearchEngine::textout;

TransTableMap GoSearchEngine::transTable;
shared_mutex GoSearchEngine::transTableLock;
TransTableMapSpecial GoSearchEngine::transTableSpecial;

static set<uint8_t> board_range;


bool CandidateItemCmp(const StepCandidateItem &a, const StepCandidateItem &b)
{
    return a.priority > b.priority;
}

GoSearchEngine::GoSearchEngine() :board(NULL)
{
    if (board_range.empty())
    {
        for (uint8_t index = 0; util::valid(index); ++index)
        {
            board_range.insert(index);
        }
    }
}

GoSearchEngine::~GoSearchEngine()
{

}

void GoSearchEngine::initSearchEngine(ChessBoard* board, ChessStep lastStep, uint64_t startSearchTime, uint64_t maxSearchTime)
{
    GoSearchEngine::transTableStat = { 0,0,0 };
    this->board = board;
    this->startStep = lastStep;
    this->maxSearchTimeMs = (uint32_t)maxSearchTime * 1000;//s to ms
    this->global_startSearchTime = system_clock::from_time_t(startSearchTime);
    textout.clear();
}

void GoSearchEngine::textOutSearchInfo(OptimalPath& optimalPath)
{
    stringstream s;
    if (optimalPath.path.size() == 0)
    {
        textout = "no path";
        return;
    }
    uint8_t nextIndex = optimalPath.path[0];
    s << "depth:" << (int)(optimalPath.endStep - startStep.step) << ", ";
    s << "time:" << duration_cast<milliseconds>(system_clock::now() - global_startSearchTime).count() << "ms, ";
    s << "rating:" << optimalPath.rating << ", next:" << (int)util::getrow(nextIndex) << "," << (int)util::getcol(nextIndex) << "\r\n";
    textold += texttemp + s.str();
    texttemp = "";
    textout = textold;
}

void GoSearchEngine::textOutPathInfo(OptimalPath& optimalPath)
{
    //optimalPath可能为空
    stringstream s;
    s << "table:" << transTable.size() << " stable:" << transTableSpecial.size() << "\r\n";
    s << "rating:" << optimalPath.rating << " depth:" << global_currentMaxDepth << "-" << (int)(optimalPath.endStep - startStep.step) << " bestpath:";
    for (auto p : optimalPath.path)
    {
        s << "(" << (int)util::getrow(p) << "," << (int)util::getcol(p) << ") ";
    }
    s << "\r\n";
    textold += s.str();
    textout = textold;
}

void GoSearchEngine::textSearchList(vector<StepCandidateItem>& moves, uint8_t currentindex, uint8_t best)
{
    stringstream s;
    s << "current:" << (int)util::getrow(currentindex) << "," << (int)util::getcol(currentindex) << "\r\n";
    s << "best:" << (int)util::getrow(best) << "," << (int)util::getcol(best) << "\r\n";

    s << "list:[";
    for (auto move : moves)
    {
        s << "(" << (int)util::getrow(move.index) << "," << (int)util::getcol(move.index) << "|" << (int)move.priority << ") ";
    }
    s << "]\r\n";

    textout = textold + texttemp + s.str();
}

void GoSearchEngine::textForTest(uint8_t currentindex, int rating, int priority)
{
    stringstream s;
    s << "current:" << (int)util::getrow(currentindex) << "," << (int)util::getcol(currentindex) << " rating:" << rating << " priority:" << priority << "\r\n";
    texttemp += s.str();
    textout = textold + texttemp;
}

uint8_t GoSearchEngine::getBestStep()
{
    global_currentMaxDepth = minAlphaBetaDepth;
    //global_startSearchTime = system_clock::now();
    vector<StepCandidateItem> solveList;

    OptimalPath optimalPath = makeSolveList(board, solveList);

    if (solveList.size() == 1 && (optimalPath.rating == 10000 || optimalPath.rating == -10000))
    {
        textOutPathInfo(optimalPath);
        transTable.clear();
        transTableSpecial.clear();

        return solveList[0].index;
    }


    for (;
        global_currentMaxDepth < maxAlphaBetaDepth;
        global_currentMaxDepth += 1, maxVCFDepth += 2/*, maxVCTDepth += 2*/)
    {
        if (duration_cast<milliseconds>(std::chrono::system_clock::now() - global_startSearchTime).count() > maxSearchTimeMs / 3)
        {
            break;
        }
        OptimalPath temp = solveBoard(board, solveList);
        std::sort(solveList.begin(), solveList.end(), CandidateItemCmp);
        if (global_currentMaxDepth > minAlphaBetaDepth && global_isOverTime)
        {
            break;
        }
        optimalPath = temp;
        if (temp.rating >= chesstypes[CHESSTYPE_5].rating || temp.rating <= -chesstypes[CHESSTYPE_5].rating)
        {
            break;
        }

        //已成定局的不需要继续搜索了
        if (solveList.size() == 1)
        {
            break;
        }
        for (size_t i = 0; i < solveList.size(); ++i)
        {
            if (solveList[i].priority == -10000)
            {
                solveList.erase(solveList.begin() + i, solveList.end());
                break;
            }
        }
        if (solveList.empty())
        {
            break;
        }
    }
    textOutPathInfo(optimalPath);
    transTable.clear();
    transTableSpecial.clear();

    if (optimalPath.path.empty())
    {
        optimalPath.path.push_back(board->getHighestInfo(startStep.getSide()).index);
    }

    return optimalPath.path[0];
}


OptimalPath GoSearchEngine::makeSolveList(ChessBoard* board, vector<StepCandidateItem>& solveList)
{
    uint8_t side = util::otherside(board->getLastStep().getSide());

    PieceInfo otherhighest = board->getHighestInfo(util::otherside(side));
    PieceInfo selfhighest = board->getHighestInfo(side);

    OptimalPath optimalPath(startStep.step);
    optimalPath.endStep = startStep.step;
    optimalPath.rating = 0;

    if (selfhighest.chesstype == CHESSTYPE_5)
    {
        optimalPath.push(selfhighest.index);
        optimalPath.rating = 10000;
        solveList.emplace_back(selfhighest.index, 10000);
    }
    else if (otherhighest.chesstype == CHESSTYPE_5)//敌方马上5连
    {
        optimalPath.push(otherhighest.index);
        optimalPath.rating = 0;
        solveList.emplace_back(otherhighest.index, 0);
    }
    else if (doVCFSearch(board, side, optimalPath, NULL) == VCXRESULT_TRUE)
    {
        solveList.emplace_back(optimalPath.path[0], 10000);
    }
    else if (util::isfourkill(otherhighest.chesstype))//敌方有4杀
    {
        getFourkillDefendSteps(board, otherhighest.index, solveList);
    }
    else if (doVCTSearch(board, side, optimalPath, NULL) == VCXRESULT_TRUE)
    {
        solveList.emplace_back(optimalPath.path[0], 10000);
    }
    else
    {
        set<uint8_t> myset;
        getNormalRelatedSet(board, myset);
        getNormalSteps(board, solveList, myset.empty() ? NULL : &myset);
    }

    if (solveList.empty())
    {
        optimalPath.rating = -10000;
        optimalPath.push(otherhighest.index);
        solveList.emplace_back(otherhighest.index, -10000);
    }

    return optimalPath;
}


OptimalPath GoSearchEngine::solveBoard(ChessBoard* board, vector<StepCandidateItem>& solveList)
{
    int alpha = INT_MIN, beta = INT_MAX;
    OptimalPath optimalPath(startStep.step);
    optimalPath.endStep = startStep.step;
    optimalPath.rating = INT_MIN;
    uint8_t side = util::otherside(board->getLastStep().getSide());

    PieceInfo otherhighest = board->getHighestInfo(util::otherside(side));
    PieceInfo selfhighest = board->getHighestInfo(side);

    for (auto it = solveList.begin(); it != solveList.end(); ++it)
    {
        textSearchList(solveList, it->index, optimalPath.path.empty() ? it->index : optimalPath.path[0]);

        OptimalPath tempPath(board->getLastStep().step);

        doAlphaBetaSearch(board, it->index, alpha, beta, tempPath);

        //处理超时
        if (global_isOverTime)
        {
            if (optimalPath.rating == INT_MIN)
            {
                optimalPath = tempPath;
            }
            textOutSearchInfo(optimalPath);
            return optimalPath;
        }

        if (enable_debug)
        {
            textForTest(it->index, tempPath.rating, it->priority);
        }

        it->priority = tempPath.rating;

        if (tempPath.rating > alpha)
        {
            alpha = tempPath.rating;
        }

        if (tempPath.rating > optimalPath.rating)
        {
            optimalPath = tempPath;
        }
        else if (tempPath.rating == optimalPath.rating)
        {
            if ((tempPath.rating == util::type2score(CHESSTYPE_5) && tempPath.endStep < optimalPath.endStep) ||
                (tempPath.rating == -util::type2score(CHESSTYPE_5) && tempPath.endStep > optimalPath.endStep))
            {
                optimalPath = tempPath;
            }
        }
    }

    /*if (optimalPath.rating == -util::type2score(CHESSTYPE_5))
    {
        uint8_t struggleindex;
        struggleFlag++;
        if (doNormalStruggleSearch(board, side, alpha, beta, optimalPath.rating, struggleindex))
        {
            struggleFlag--;
            OptimalPath tempPath(board->getLastStep().step);
            tempPath.push(struggleindex);
            ChessBoard tempboard = *board;
            tempboard.move(struggleindex);

            doAlphaBetaSearch(&tempboard, alpha, beta, tempPath);
            if (global_isOverTime)
            {
                if (optimalPath.rating == INT_MIN)
                {
                    optimalPath = tempPath;
                }
                textOutSearchInfo(optimalPath);
                return optimalPath;
            }
            if (enable_debug)
            {
                textForTest(struggleindex, tempPath.rating, 20);
            }
            if (tempPath.rating > optimalPath.rating)
            {
                optimalPath = tempPath;
            }
            else if (tempPath.rating == optimalPath.rating)
            {
                if (tempPath.rating == -util::type2score(CHESSTYPE_5) && tempPath.endStep > optimalPath.endStep)
                {
                    optimalPath = tempPath;
                }
            }
        }
        else
        {
            struggleFlag--;
        }
    }*/

    textOutSearchInfo(optimalPath);
    return optimalPath;
}

void GoSearchEngine::doAlphaBetaSearch(ChessBoard* board, csidx index, int alpha, int beta, OptimalPath& optimalPath)
{
    uint8_t side = util::otherside(board->getLastStep().getSide());
    uint8_t otherside = board->getLastStep().getSide();
    optimalPath.push(index);
    if (board->getChessType(index, side) == CHESSTYPE_BAN)
    {
        optimalPath.rating = isPlayerSide(side) ? util::type2score(CHESSTYPE_5) : -util::type2score(CHESSTYPE_5);
        return;
    }
    else if (board->getChessType(index, side) == CHESSTYPE_5)
    {
        optimalPath.rating = isPlayerSide(side) ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
        return;
    }
    ChessBoard currentBoard = *board;
    currentBoard.move(index);

    //USE TransTable
    TransTableData data;
    uint64_t z64 = currentBoard.getBoardHash().z64key;
    uint32_t z32 = currentBoard.getBoardHash().z32key;
    if (getTransTable(z64, data))
    {
        if (data.checkHash == z32)
        {
            if (data.depth != global_currentMaxDepth && data.type == TRANSTYPE_UNSURE)
            {
                transTableStat.cover++;
            }
            else
            {
                transTableStat.hit++;
                optimalPath.rating = data.value;
                optimalPath.endStep = data.endStep;
                return;
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
    //end USE TransTable

    OptimalPath VCFPath(currentBoard.getLastStep().step);
    OptimalPath VCTPath(currentBoard.getLastStep().step);

    vector<StepCandidateItem> moves;
    OptimalPath bestPath(currentBoard.getLastStep().step);
    bestPath.rating = isPlayerSide(otherside) ? INT_MAX : INT_MIN;

    if (currentBoard.getHighestInfo(otherside).chesstype == CHESSTYPE_5)
    {
        optimalPath.rating = isPlayerSide(otherside) ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
        optimalPath.push(currentBoard.getHighestInfo(otherside).index);
        return;
    }
    else if (currentBoard.getHighestInfo(side).chesstype == CHESSTYPE_5)//自己马上5连
    {
        if (currentBoard.getChessType(currentBoard.getHighestInfo(side).index, otherside) == CHESSTYPE_BAN)//敌人触发禁手，自己赢了
        {
            optimalPath.push(currentBoard.getHighestInfo(side).index);
            optimalPath.rating = isPlayerSide(side) ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
            return;
        }
        moves.emplace_back(currentBoard.getHighestInfo(side).index, 10);
    }
    else if (global_isOverTime || duration_cast<milliseconds>(std::chrono::system_clock::now() - global_startSearchTime).count() > maxSearchTimeMs)//超时
    {
        global_isOverTime = true;
        return;
    }
    else if (currentBoard.getLastStep().step - startStep.step >= global_currentMaxDepth + extra_alphabeta)
    {
        optimalPath.rating = currentBoard.getGlobalEvaluate(getAISide());
        return;
    }
    else if (doVCFSearch(&currentBoard, otherside, VCFPath, NULL) == VCXRESULT_TRUE)//敌人赢了
    {
        bestPath = VCFPath;
        //goto end;
    }
    else if (util::isfourkill(currentBoard.getHighestInfo(side).chesstype))//防4杀
    {
        getFourkillDefendSteps(&currentBoard, currentBoard.getHighestInfo(side).index, moves);
        if (moves.empty())
        {
            moves.emplace_back(currentBoard.getHighestInfo(side).index, -10000);
        }
    }
    else if (doVCTSearch(&currentBoard, otherside, VCTPath, NULL) == VCXRESULT_TRUE)
    {
        bestPath = VCTPath;
        //goto end;
    }
    else
    {
        set<uint8_t> myset;
        getNormalRelatedSet(&currentBoard, myset);
        getNormalSteps(&currentBoard, moves, myset.empty() ? NULL : &myset);
        if (moves.empty())
        {
            moves.emplace_back(currentBoard.getHighestInfo(side).index, -10000);
        }
    }

    for (size_t i = 0; i < moves.size(); ++i)
    {
        OptimalPath tempPath(currentBoard.getLastStep().step);
        doAlphaBetaSearch(&currentBoard, moves[i].index, alpha, beta, tempPath);

        //剪枝
        if (isPlayerSide(otherside))//build player
        {
            if (tempPath.rating < bestPath.rating)
            {
                bestPath = tempPath;
            }
            else if (tempPath.rating == bestPath.rating)
            {
                if (tempPath.rating == -util::type2score(CHESSTYPE_5) && tempPath.endStep < bestPath.endStep)//赢了，尽量快
                {
                    bestPath = tempPath;
                }
                else if (tempPath.rating == util::type2score(CHESSTYPE_5) && tempPath.endStep > bestPath.endStep)//必输，尽量拖
                {
                    bestPath = tempPath;
                }
            }

            if (tempPath.rating < alpha)//alpha cut
            {
                break;
            }
            if (tempPath.rating < beta)//update beta
            {
                beta = tempPath.rating;
            }

        }
        else // build AI
        {
            if (tempPath.rating > bestPath.rating
                || (tempPath.rating == bestPath.rating && tempPath.endStep < bestPath.endStep))
            {
                bestPath = tempPath;
            }
            else if (tempPath.rating == bestPath.rating)
            {
                if (tempPath.rating == util::type2score(CHESSTYPE_5) && tempPath.endStep < bestPath.endStep)//赢了，尽量快
                {
                    bestPath = tempPath;
                }
                else if (tempPath.rating == -util::type2score(CHESSTYPE_5) && tempPath.endStep > bestPath.endStep)//必输，尽量拖
                {
                    bestPath = tempPath;
                }
            }

            if (tempPath.rating > beta)//beta cut
            {
                break;
            }
            if (tempPath.rating > alpha)//update alpha
            {
                alpha = tempPath.rating;
            }
        }
    }

    //挣扎
    //uint8_t struggleindex;
    //if ((side != startStep.getSide() && bestPath.rating == -util::type2score(CHESSTYPE_5))
    //    || (side == startStep.getSide() && bestPath.rating == util::type2score(CHESSTYPE_5)))
    //{
    //    struggleFlag++;
    //    if (doNormalStruggleSearch(board, side, alpha, beta, bestPath.rating, struggleindex))
    //    {
    //        struggleFlag--;
    //        ChessBoard tempboard = *board;
    //        tempboard.move(struggleindex);
    //        OptimalPath tempPath(board->getLastStep().step);
    //        tempPath.push(struggleindex);
    //        if (board->getChessType(struggleindex, side) == CHESSTYPE_BAN)
    //        {
    //            tempPath.rating = -(side == startStep.getSide() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5));
    //        }
    //        else
    //        {
    //            doAlphaBetaSearchWrapper(&tempboard, alpha, beta, tempPath);
    //            if (tempPath.endStep == board->getLastStep().step + 1)
    //            {
    //                tempPath.rating = tempboard.getGlobalEvaluate(getAISide());
    //            }
    //        }
    //        if (side != startStep.getSide())//build AI
    //        {
    //            if (
    //                tempPath.rating > bestPath.rating
    //                || (tempPath.rating == bestPath.rating && tempPath.endStep < bestPath.endStep)
    //                )
    //            {
    //                bestPath = tempPath;
    //            }
    //        }
    //        else // build player
    //        {
    //            if (
    //                tempPath.rating < bestPath.rating
    //                || (tempPath.rating == bestPath.rating && tempPath.endStep < bestPath.endStep)
    //                )
    //            {
    //                bestPath = tempPath;
    //            }
    //        }
    //    }
    //    else
    //    {
    //        struggleFlag--;
    //    }
    //}

    optimalPath.cat(bestPath);
    optimalPath.rating = bestPath.rating;

    //USE TransTable
    //写入置换表
    data.checkHash = z32;
    data.endStep = optimalPath.endStep;
    data.value = optimalPath.rating;
    if (data.value == util::type2score(CHESSTYPE_5) || data.value == -util::type2score(CHESSTYPE_5))
    {
        data.type = TRANSTYPE_EXACT;
    }
    else
    {
        data.type = TRANSTYPE_UNSURE;
    }
    data.depth = global_currentMaxDepth;
    putTransTable(z64, data);
    //end USE TransTable
}

bool GoSearchEngine::doNormalStruggleSearch(ChessBoard* board, uint8_t side, int alpha, int beta, int rating, uint8_t &nextstep)
{
    return false;
    uint8_t laststep = board->lastStep.step;
    vector<StepCandidateItem> moves;
    getVCFAtackSteps(board, moves, NULL);
    for (auto move : moves)
    {
        OptimalPath tempPath(board->getLastStep().step);
        tempPath.rating = 0;
        ChessBoard tempboard = *board;
        tempPath.push(move.index);
        tempboard.move(move.index);//冲四

        if (tempboard.getHighestInfo(util::otherside(side)).chesstype == CHESSTYPE_5)
        {
            continue;
        }
        if (tempboard.getHighestInfo(side).chesstype != CHESSTYPE_5)//5连是禁手
        {
            continue;
        }
        tempPath.push(tempboard.getHighestInfo(side).index);
        tempboard.move(tempboard.getHighestInfo(side).index);//防五连

        extra_alphabeta += 2;
        //doAlphaBetaSearchWrapper(&tempboard, alpha, beta, tempPath);

        if (tempPath.rating != rating)
        {
            extra_alphabeta -= 2;
            nextstep = move.index;
            return true;
        }

        if (doNormalStruggleSearch(&tempboard, side, alpha, beta, rating, nextstep))
        {
            extra_alphabeta -= 2;
            nextstep = move.index;
            return true;
        }
        extra_alphabeta -= 2;
    }
    return false;
}

void GoSearchEngine::getNormalRelatedSet(ChessBoard* board, set<uint8_t>& reletedset)
{
    return;
    OptimalPath VCFPath(board->getLastStep().step);
    OptimalPath VCTPath(board->getLastStep().step);
    ChessBoard tempboard = *board;
    tempboard.moveNull();
    int oldvct = maxVCTDepth, oldvcf = maxVCFDepth;

    maxVCTDepth = 8;
    maxVCFDepth = 16;
    if (doVCFSearch(&tempboard, board->getLastStep().getSide(), VCFPath, NULL) == VCXRESULT_TRUE)
    {
        for (size_t i = 0; i < VCFPath.path.size(); i++)
        {
            set<uint8_t> tempset;
            tempboard.move(VCFPath.path[i]);
            reletedset.insert(VCFPath.path[i]);

            Position pos(VCFPath.path[i]);
            Position temppos;
            for (int d = 0; d < DIRECTION4_COUNT; ++d)
            {
                for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
                {
                    temppos = pos.getNextPosition(d, symbol);
                    if (temppos.valid() && tempboard.canMove(temppos.toIndex()))
                    {
                        reletedset.insert(temppos.toIndex());
                    }
                }
            }
            /*tempboard.getAtackReletedPos(tempset, VCFPath.path[i], board->getLastStep().getSide());
            util::myset_intersection(&tempset, &reletedset, &reletedset);*/

            i++;
            if (i < VCFPath.path.size())
            {
                tempboard.move(VCFPath.path[i]);
                reletedset.insert(VCFPath.path[i]);
            }
        }
    }
    else if (doVCTSearch(&tempboard, board->getLastStep().getSide(), VCTPath, NULL) == VCXRESULT_TRUE)
    {
        for (size_t i = 0; i < VCTPath.path.size(); i++)
        {
            set<uint8_t> tempset;
            tempboard.move(VCTPath.path[i]);
            reletedset.insert(VCTPath.path[i]);

            Position pos(VCTPath.path[i]);
            Position temppos;
            for (int d = 0; d < DIRECTION4_COUNT; ++d)
            {
                for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
                {
                    temppos = pos.getNextPosition(d, symbol);
                    if (temppos.valid() && tempboard.canMove(temppos.toIndex()))
                    {
                        reletedset.insert(temppos.toIndex());
                    }
                }
            }

            /* tempboard.getAtackReletedPos(tempset, VCTPath.path[i], board->getLastStep().getSide());
             util::myset_intersection(&tempset, &reletedset, &reletedset);*/

            i++;
            if (i < VCTPath.path.size())
            {
                tempboard.move(VCTPath.path[i]);
                reletedset.insert(VCTPath.path[i]);
            }
        }
    }
    maxVCTDepth = oldvct;
    maxVCFDepth = oldvcf;
}

void getNormalSteps1(ChessBoard* board, vector<StepCandidateItem>& childs, set<uint8_t>* reletedset)
{
    uint8_t side = util::otherside(board->getLastStep().getSide());
    if (NULL == reletedset)
    {
        //进攻
        for (auto index : board_range)
        {
            if (!(board->canMove(index) && board->useful(index)))
            {
                continue;
            }

            uint8_t selfp = board->getChessType(index, side);

            if (selfp == CHESSTYPE_BAN)
            {
                continue;
            }

            if (chesstypes[selfp].atackPriority == 0)
            {
                continue;
            }

            uint8_t otherp = board->getChessType(index, util::otherside(side));

            int8_t priority = chesstypes[selfp].atackPriority + chesstypes[otherp].defendPriority / 2;
            childs.emplace_back(index, priority);
        }

        for (size_t i = 0; i < childs.size(); ++i)
        {
            childs[i].priority = (int)(board->getRelatedFactor(childs[i].index, side) * 2);
        }
    }
    else
    {
        //防守
        for (auto index : *reletedset)
        {
            if (!(board->canMove(index) && board->useful(index)))
            {
                continue;
            }

            uint8_t selfp = board->getChessType(index, side);

            if (selfp == CHESSTYPE_BAN)
            {
                continue;
            }

            uint8_t otherp = board->getChessType(index, util::otherside(side));
            if (chesstypes[otherp].defendPriority == 0)
            {
                continue;
            }
            int8_t priority = chesstypes[otherp].defendPriority + chesstypes[selfp].atackPriority / 2;

            childs.emplace_back(index, priority);
        }

        for (size_t i = 0; i < childs.size(); ++i)
        {
            childs[i].priority = (int)(board->getRelatedFactor(childs[i].index, util::otherside(side)) * 2);
        }
    }

    std::sort(childs.begin(), childs.end(), CandidateItemCmp);

    if (childs.size() > MAX_CHILD_NUM)
    {
        childs.erase(childs.begin() + MAX_CHILD_NUM, childs.end());//只保留10个
    }
}

void getNormalSteps2(ChessBoard* board, vector<StepCandidateItem>& childs)
{
    uint8_t side = util::otherside(board->getLastStep().getSide());


    for (uint8_t index = 0; util::valid(index); ++index)
    {
        if (!(board->canMove(index) && board->useful(index)))
        {
            continue;
        }

        uint8_t selfp = board->getChessType(index, side);

        if (selfp == CHESSTYPE_BAN)
        {
            continue;
        }

        uint8_t otherp = board->getChessType(index, util::otherside(side));

        int8_t priority = 0;

        if (selfp < CHESSTYPE_33 && otherp < CHESSTYPE_33)
        {
            if (chesstypes[selfp].atackPriority > chesstypes[otherp].defendFactor)
            {
                priority += chesstypes[board->pieces_layer3[index][side]].atackPriority;
                priority += chesstypes[board->pieces_layer3[index][util::otherside(side)]].defendPriority / 2;
            }
            else
            {
                priority += chesstypes[board->pieces_layer3[index][util::otherside(side)]].defendPriority;
                priority += chesstypes[board->pieces_layer3[index][side]].atackPriority / 2;
            }
        }
        else
        {
            priority += chesstypes[selfp].atackPriority > chesstypes[otherp].defendPriority ? chesstypes[selfp].atackPriority : chesstypes[otherp].defendPriority;
        }

        if (priority <= 0)
        {
            continue;
        }

        childs.emplace_back(index, priority);
    }

    //std::sort(childs.begin(), childs.end(), CandidateItemCmp);
    //for (std::vector<StepCandidateItem>::iterator it = childs.begin(); it != childs.end(); it++)
    //{
    //    if (it->priority <= childs.begin()->priority / 2)
    //    {
    //        childs.erase(it, childs.end());
    //        break;
    //    }
    //}
    for (size_t i = 0; i < childs.size(); ++i)
    {
        if (chesstypes[board->getChessType(childs[i].index, side)].atackPriority > chesstypes[board->getChessType(childs[i].index, util::otherside(side))].defendPriority)
        {
            childs[i].priority = (int)(childs[i].priority * board->getRelatedFactor(childs[i].index, side));
        }
        else
        {
            childs[i].priority = (int)(childs[i].priority * board->getRelatedFactor(childs[i].index, util::otherside(side)));
        }

        if (util::inLocalArea(childs[i].index, board->getLastStep().index, LOCAL_SEARCH_RANGE))
        {
            childs[i].priority += 1;
        }
    }
    //if (childs.size() > MAX_CHILD_NUM)
    //{
    //    childs.erase(childs.begin() + MAX_CHILD_NUM, childs.end());//只保留10个
    //}
    std::sort(childs.begin(), childs.end(), CandidateItemCmp);
}

void getNormalSteps3(ChessBoard* board, vector<StepCandidateItem>& childs)
{
    uint8_t side = util::otherside(board->getLastStep().getSide());


    for (uint8_t index = 0; util::valid(index); ++index)
    {
        if (!(board->canMove(index) && board->useful(index)))
        {
            continue;
        }

        uint8_t selfp = board->getChessType(index, side);

        if (selfp == CHESSTYPE_BAN)
        {
            continue;
        }

        uint8_t otherp = board->getChessType(index, util::otherside(side));

        int8_t priority = 0;

        if (selfp < CHESSTYPE_33 && otherp < CHESSTYPE_33)
        {
            if (chesstypes[selfp].atackPriority > chesstypes[otherp].defendFactor)
            {
                priority += chesstypes[board->pieces_layer3[index][side]].atackPriority;
                priority += chesstypes[board->pieces_layer3[index][util::otherside(side)]].defendPriority / 2;
            }
            else
            {
                priority += chesstypes[board->pieces_layer3[index][util::otherside(side)]].defendPriority;
                priority += chesstypes[board->pieces_layer3[index][side]].atackPriority / 2;
            }
        }
        else
        {
            priority += chesstypes[selfp].atackPriority > chesstypes[otherp].defendPriority ? chesstypes[selfp].atackPriority : chesstypes[otherp].defendPriority;
        }

        if (priority <= 0)
        {
            continue;
        }

        childs.emplace_back(index, (int)(board->getRelatedFactor(index, side)*2+ board->getRelatedFactor(index, util::otherside(side))*2));
    }
    
    std::sort(childs.begin(), childs.end(), CandidateItemCmp);
    if (childs.size() > MAX_CHILD_NUM)
    {
        childs.erase(childs.begin() + MAX_CHILD_NUM, childs.end());//只保留10个
    }
}

void GoSearchEngine::getNormalSteps(ChessBoard* board, vector<StepCandidateItem>& childs, set<uint8_t>* reletedset)
{
    getNormalSteps3(board, childs);
    //getNormalSteps1(board, childs, reletedset);
}


void GoSearchEngine::getFourkillDefendSteps(ChessBoard* board, uint8_t index, vector<StepCandidateItem>& moves)
{
    uint8_t side = util::otherside(board->getLastStep().getSide());
    uint8_t defendType = board->getChessType(index, board->getLastStep().getSide());
    ChessStep lastStep = board->getLastStep();
    vector<int> direction;

    if (board->getChessType(index, side) != CHESSTYPE_BAN)
    {
        moves.emplace_back(index, 0);
    }

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
        while (util::displace(r, c, 1, n)) //如果不超出边界
        {
            if (board->getState(r, c) == PIECE_BLANK)
            {
                blankCount++;
                tempType = board->getChessType(r, c, util::otherside(side));
                if (tempType > CHESSTYPE_0)
                {
                    if (board->getChessType(r, c, side) == CHESSTYPE_BAN)//被禁手了
                    {
                        continue;
                    }
                    ChessBoard tempboard = *board;
                    tempboard.move(util::xy2index(r, c));
                    if (tempboard.getHighestInfo(board->getLastStep().getSide()).chesstype < defendType)
                    {
                        if (board->getChessType(r, c, side) != CHESSTYPE_BAN)
                        {
                            moves.emplace_back(util::xy2index(r, c), 10);
                        }
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

uint8_t GoSearchEngine::doVCFSearchWrapper(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, set<uint8_t>* reletedset)
{
    TransTableDataSpecial data;
    if (getTransTableSpecial(board->getBoardHash().z64key, data))
    {
        if (data.checkHash == board->getBoardHash().z32key)
        {
            if (data.VCFflag == VCXRESULT_NOSEARCH)//还未搜索
            {
                transTableStat.miss++;
            }
            else
            {
                if (data.VCFflag == VCXRESULT_UNSURE && data.VCFDepth < (struggleFlag ? maxVCFDepth + extraVCXDepth : maxVCFDepth))//需要更新
                {
                    transTableStat.cover++;
                }
                else
                {
                    transTableStat.hit++;
                    optimalPath.endStep = data.VCFEndStep;
                    return data.VCFflag;
                }
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
    uint8_t flag = doVCFSearch(board, side, optimalPath, reletedset);
    if (reletedset == NULL || flag != VCXRESULT_UNSURE)//下一步就结束了的没必要写进置换表
    {
        data.checkHash = board->getBoardHash().z32key;
        data.VCFflag = flag;
        data.VCFEndStep = optimalPath.endStep;
        data.VCFDepth = (struggleFlag ? maxVCFDepth + extraVCXDepth : maxVCFDepth);
        putTransTableSpecial(board->getBoardHash().z64key, data);
    }
    return flag;
}

uint8_t GoSearchEngine::doVCTSearchWrapper(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, set<uint8_t>* reletedset)
{
    TransTableDataSpecial data;
    if (getTransTableSpecial(board->getBoardHash().z64key, data))
    {
        if (data.checkHash == board->getBoardHash().z32key)
        {
            if (data.VCTflag == VCXRESULT_NOSEARCH)//还未搜索
            {
                transTableStat.miss++;
            }
            else
            {
                if (data.VCTflag == VCXRESULT_UNSURE && data.VCTDepth < (struggleFlag ? maxVCTDepth + extraVCXDepth : maxVCTDepth))//需要更新
                {
                    transTableStat.cover++;
                }
                else
                {
                    transTableStat.hit++;
                    optimalPath.endStep = data.VCTEndStep;
                    return data.VCTflag;
                }
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
    uint8_t flag = doVCTSearch(board, side, optimalPath, reletedset);
    if (reletedset == NULL || flag != VCXRESULT_UNSURE)//下一步就结束了的没必要写进置换表
    {
        data.checkHash = board->getBoardHash().z32key;
        data.VCTflag = flag;
        data.VCTEndStep = optimalPath.endStep;
        data.VCTDepth = (struggleFlag ? maxVCTDepth + extraVCXDepth : maxVCTDepth);
        putTransTableSpecial(board->getBoardHash().z64key, data);
    }
    return flag;
}

uint8_t GoSearchEngine::doVCFSearch(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, set<uint8_t>* reletedset)
{
    uint8_t lastindex = board->getLastStep().index;
    uint8_t laststep = board->getLastStep().step;
    if (board->getHighestInfo(side).chesstype == CHESSTYPE_5)
    {
        optimalPath.push(board->getHighestInfo(side).index);
        optimalPath.rating = side == startStep.getSide() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
        return VCXRESULT_TRUE;
    }
    else if (board->getLastStep().step - startStep.step > (struggleFlag ? maxVCFDepth + extraVCXDepth : maxVCFDepth))
    {
        return VCXRESULT_UNSURE;
    }
    else if (global_isOverTime || duration_cast<milliseconds>(std::chrono::system_clock::now() - global_startSearchTime).count() > maxSearchTimeMs)
    {
        global_isOverTime = true;
        return VCXRESULT_UNSURE;
    }

    OptimalPath bestPath(board->getLastStep().step);
    bestPath.endStep = optimalPath.endStep;
    bool unsure_flag = false;
    vector<StepCandidateItem> moves;
    getVCFAtackSteps(board, moves, reletedset);

    for (auto item : moves)
    {
        ChessBoard tempboard = *board;
        tempboard.move(item.index);//冲四

        OptimalPath tempPath(board->getLastStep().step);
        tempPath.push(item.index);

        if (tempboard.getHighestInfo(util::otherside(side)).chesstype == CHESSTYPE_5)
        {
            continue;
        }

        if (tempboard.getHighestInfo(side).chesstype != CHESSTYPE_5)//5连是禁手
        {
            continue;
        }

        if (tempboard.getChessType(tempboard.getHighestInfo(side).index, util::otherside(side)) == CHESSTYPE_BAN)//敌方触发禁手，VCF成功
        {
            optimalPath.rating = side == startStep.getSide() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
            optimalPath.cat(tempPath);
            return VCXRESULT_TRUE;
        }
        tempPath.push(tempboard.getHighestInfo(side).index);//防五连
        tempboard.move(tempboard.getHighestInfo(side).index);

        set<uint8_t> atackset;
        /*if (reletedset != NULL)
        {
            set<uint8_t> tempatackset;
            tempboard.getAtackReletedPos(tempatackset, item.index, side);
            util::myset_intersection(reletedset, &tempatackset, &atackset);
        }
        else*/
        {
            tempboard.getAtackReletedPos(atackset, item.index, side);
        }

        uint8_t result = doVCFSearchWrapper(&tempboard, side, tempPath, &atackset);
        if (result == VCXRESULT_TRUE)
        {
            optimalPath.rating = side == startStep.getSide() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
            optimalPath.cat(tempPath);
            return VCXRESULT_TRUE;
        }

        //只要有一个UNSURE并且没有TRUE，那么结果就是UNSURE
        if (result == VCXRESULT_UNSURE)
        {
            unsure_flag = true;
        }

        if (tempPath.endStep > bestPath.endStep)
        {
            bestPath = tempPath;
        }
    }

    optimalPath.endStep = bestPath.endStep;
    if (unsure_flag)
    {
        return VCXRESULT_UNSURE;
    }
    else
    {
        return VCXRESULT_FALSE;
    }

}

uint8_t GoSearchEngine::doVCTSearch(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, set<uint8_t>* reletedset)
{
    uint8_t laststep = board->getLastStep().step;
    uint8_t lastindex = board->getLastStep().index;
    OptimalPath VCFPath(board->getLastStep().step);
    vector<StepCandidateItem> moves;
    if (board->getHighestInfo(side).chesstype == CHESSTYPE_5)
    {
        optimalPath.push(board->getHighestInfo(side).index);
        optimalPath.rating = side == startStep.getSide() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
        return VCXRESULT_TRUE;
    }
    else if (board->getHighestInfo(util::otherside(side)).chesstype == CHESSTYPE_5)
    {
        ChessBoard tempboard = *board;
        tempboard.move(board->getHighestInfo(util::otherside(side)).index);
        if (util::isfourkill(tempboard.getHighestInfo(side).chesstype))
        {
            moves.emplace_back(board->getHighestInfo(util::otherside(side)).index, 10);
        }
        else
        {
            optimalPath.push(board->getHighestInfo(util::otherside(side)).index);
            return VCXRESULT_FALSE;
        }
    }
    else if (global_isOverTime || duration_cast<milliseconds>(std::chrono::system_clock::now() - global_startSearchTime).count() > maxSearchTimeMs)
    {
        global_isOverTime = true;
        return VCXRESULT_UNSURE;
    }
    else if (doVCFSearch(board, side, VCFPath, NULL) == VCXRESULT_TRUE)
    {
        optimalPath.cat(VCFPath);
        optimalPath.rating = VCFPath.rating;
        return VCXRESULT_TRUE;
    }
    else if (board->getLastStep().step - startStep.step > (struggleFlag ? maxVCTDepth + extraVCXDepth : maxVCTDepth))
    {
        return VCXRESULT_UNSURE;
    }
    else
    {
        getVCTAtackSteps(board, moves, reletedset);
    }

    bool unsure_flag = false;
    OptimalPath bestPath(board->getLastStep().step);
    bestPath.endStep = optimalPath.endStep;
    uint8_t tempresult;
    for (auto item : moves)
    {
        ChessBoard tempboard = *board;
        tempboard.move(item.index);

        OptimalPath tempPath(board->getLastStep().step);
        tempPath.push(item.index);

        vector<StepCandidateItem> defendmoves;

        if (tempboard.getHighestInfo(side).chesstype == CHESSTYPE_5)//冲四
        {
            if (tempboard.getHighestInfo(util::otherside(side)).chesstype == CHESSTYPE_5)//失败，对方有5连
            {
                continue;
            }
            if (tempboard.getChessType(tempboard.getHighestInfo(side).index, util::otherside(side)) == CHESSTYPE_BAN)//敌方触发禁手，VCF成功
            {
                optimalPath.rating = side == startStep.getSide() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
                optimalPath.cat(tempPath);
                return VCXRESULT_TRUE;
            }
            defendmoves.emplace_back(tempboard.getHighestInfo(side).index, 10);
        }
        else //活三
        {
            OptimalPath tempPath2(tempboard.getLastStep().step);
            tempresult = doVCFSearch(&tempboard, util::otherside(side), tempPath2, NULL);
            if (tempresult == VCXRESULT_TRUE)
            {
                continue;
            }
            if (tempresult == VCXRESULT_UNSURE)
            {
                unsure_flag = true;
            }
            if (!util::isfourkill(tempboard.getHighestInfo(side).chesstype))//防假活三，连环禁手
            {
                continue;
            }

            getFourkillDefendSteps(&tempboard, tempboard.getHighestInfo(side).index, defendmoves);
        }


        bool flag = true;
        OptimalPath tempPath2(tempboard.lastStep.step);
        for (auto defend : defendmoves)
        {
            tempPath2.endStep = tempPath.endStep;
            tempPath2.path.clear();
            ChessBoard tempboard2 = tempboard;
            tempboard2.move(defend.index);

            tempPath2.push(defend.index);

            set<uint8_t> atackset;
            /*if (reletedset != NULL)
            {
                set<uint8_t> tempatackset;
                tempboard2.getAtackReletedPos(tempatackset, item.index, side);
                util::myset_intersection(reletedset, &tempatackset, &atackset);
            }
            else*/
            {
                tempboard2.getAtackReletedPos(atackset, item.index, side);
            }

            tempresult = doVCTSearchWrapper(&tempboard2, side, tempPath2, &atackset);
            if (tempPath2.endStep > bestPath.endStep)
            {
                bestPath.endStep = tempPath2.endStep;
            }
            if (tempresult == VCXRESULT_UNSURE)
            {
                unsure_flag = true;
            }
            if (tempresult != VCXRESULT_TRUE)
            {
                flag = false;
                break;
            }
        }
        if (flag)
        {
            struggleFlag++;
            uint8_t struggleindex;
            if (doVCTStruggleSearch(&tempboard, struggleindex))
            {
                struggleFlag--;
                continue;
            }
            struggleFlag--;
            tempPath.cat(tempPath2);
            optimalPath.cat(tempPath);
            optimalPath.rating = side == startStep.getSide() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
            return VCXRESULT_TRUE;
        }
    }

    optimalPath.endStep = bestPath.endStep;
    if (unsure_flag)
    {
        return VCXRESULT_UNSURE;
    }
    else
    {
        return VCXRESULT_FALSE;
    }

}

bool GoSearchEngine::doVCTStruggleSearch(ChessBoard* board, uint8_t &nextstep)
{
    uint8_t side = util::otherside(board->lastStep.getSide());
    uint8_t laststep = board->lastStep.step;
    vector<StepCandidateItem> moves;
    getVCFAtackSteps(board, moves, NULL);
    for (auto move : moves)
    {
        ChessBoard tempboard = *board;
        tempboard.move(move.index);//冲四
        if (tempboard.getHighestInfo(util::otherside(side)).chesstype == CHESSTYPE_5)
        {
            continue;
        }
        if (tempboard.getHighestInfo(side).chesstype != CHESSTYPE_5)//5连是禁手
        {
            continue;
        }
        tempboard.move(tempboard.getHighestInfo(side).index);//防五连

        vector<StepCandidateItem> defendmoves;
        getFourkillDefendSteps(&tempboard, tempboard.getHighestInfo(util::otherside(side)).index, defendmoves);
        bool flag = true;

        for (auto defend : defendmoves)
        {
            OptimalPath tempPath(tempboard.lastStep.step);
            ChessBoard tempboard2 = tempboard;
            tempboard2.move(defend.index);
            tempPath.push(defend.index);
            uint8_t result = doVCTSearchWrapper(&tempboard2, util::otherside(side), tempPath, NULL);
            if (result != VCXRESULT_TRUE)//要确定挣扎成功，不能承认因为步数不够导致的成功
            {
                nextstep = move.index;
                return true;
            }
        }
        if (doVCTStruggleSearch(&tempboard, nextstep))
        {
            nextstep = move.index;
            return true;
        }
    }
    return false;
}


void GoSearchEngine::getVCFAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, set<uint8_t>* reletedset)
{
    uint8_t side = util::otherside(board->getLastStep().getSide());

    set<uint8_t>* range;
    if (reletedset == NULL)
    {
        range = &board_range;
    }
    else
    {
        range = reletedset;
    }

    for (auto index : *range)
    {
        if (!board->canMove(index))
        {
            continue;
        }

        if (util::hasdead4(board->getChessType(index, side)))
        {
            if (util::isfourkill(board->getChessType(index, side)))
            {
                if (board->getChessType(index, side) == CHESSTYPE_4)
                {
                    moves.emplace_back(index, 100);
                }
                else if (board->getChessType(index, side) == CHESSTYPE_44)
                {
                    moves.emplace_back(index, 80);
                }
                else
                {
                    moves.emplace_back(index, 50);
                }
                continue;
            }
            moves.emplace_back(index, (int)(board->getRelatedFactor(index, side, true) * 2));
        }
    }

    std::sort(moves.begin(), moves.end(), CandidateItemCmp);
}

void GoSearchEngine::getVCTAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, set<uint8_t>* reletedset)
{
    uint8_t side = util::otherside(board->getLastStep().getSide());

    set<uint8_t>* range;
    if (reletedset == NULL)
    {
        range = &board_range;
    }
    else
    {
        range = reletedset;
    }


    for (auto index : *range)
    {
        if (!board->canMove(index))
        {
            continue;
        }

        if (board->getChessType(index, side) == CHESSTYPE_4)
        {
            moves.emplace_back(index, 100);
            continue;
        }
        else if (board->getChessType(index, side) == CHESSTYPE_44)
        {
            moves.emplace_back(index, 80);
            continue;
        }
        else if (board->getChessType(index, side) == CHESSTYPE_43)
        {
            moves.emplace_back(index, 50);
            continue;
        }
        else if (board->getChessType(index, side) == CHESSTYPE_33)
        {
            moves.emplace_back(index, 40);
            continue;
        }

        if (util::isalive3(board->getChessType(index, side)))
        {
            moves.emplace_back(index, (int)(board->getRelatedFactor(index, side, true) * 2));
        }
        else if (util::isdead4(board->getChessType(index, side)))
        {
            moves.emplace_back(index, (int)(board->getRelatedFactor(index, side, true) * 2));

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
            uint8_t tempindex;
            ChessBoard tempboard;
            for (int n : direction)
            {
                int r = util::getrow(index), c = util::getcol(index);
                int blankCount = 0, chessCount = 0;
                while (util::displace(r, c, 1, n)) //如果不超出边界
                {
                    tempindex = util::xy2index(r, c);
                    if (board->getState(tempindex) == PIECE_BLANK)
                    {
                        blankCount++;
                        if (util::isalive3(board->getChessType(tempindex, side)))
                        {
                            break;
                        }
                        tempboard = *board;
                        tempboard.move(tempindex);
                        if (util::isfourkill(tempboard.getChessType(index, side)))
                        {
                            moves.emplace_back(tempindex, (int)(board->getRelatedFactor(tempindex, side, true) * 2));
                        }
                    }
                    else if (board->getState(tempindex) == util::otherside(side))
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