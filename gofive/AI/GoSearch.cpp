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

void GoSearchEngine::initSearchEngine(ChessBoard* board, ChessStep lastStep, uint64_t maxSearchTime)
{
    GoSearchEngine::transTableStat = { 0,0,0 };
    this->board = board;
    this->startStep = lastStep;
    this->maxSearchTime = (int)maxSearchTime;
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
    s << "depth:" << global_currentMaxDepth << "-" << (int)(optimalPath.endStep - startStep.step) << ", ";
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
    s << "rating:" << optimalPath.rating << " bestpath:";
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
    global_isOverTime = false;

    global_startSearchTime = system_clock::now();
    vector<StepCandidateItem> solveList;

    OptimalPath optimalPath = makeSolveList(board, solveList);

    if (solveList.size() == 1 && (optimalPath.rating == 10000 || optimalPath.rating == -10000))
    {
        textOutPathInfo(optimalPath);
        transTable.clear();
        transTableSpecial.clear();

        return solveList[0].index;
    }


    for (global_currentMaxDepth = minAlphaBetaDepth;
        global_currentMaxDepth < maxAlphaBetaDepth;
        global_currentMaxDepth += 1/*, maxVCFDepth += 2, maxVCTDepth += 1*/)
    {
        if (duration_cast<seconds>(std::chrono::system_clock::now() - global_startSearchTime).count() >= maxSearchTime / 3)
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

    return optimalPath.path[0];
}


OptimalPath GoSearchEngine::makeSolveList(ChessBoard* board, vector<StepCandidateItem>& solveList)
{
    uint8_t side = util::otherside(board->getLastStep().getColor());

    PieceInfo otherhighest = board->getHighestInfo(util::otherside(side));
    PieceInfo selfhighest = board->getHighestInfo(side);

    OptimalPath optimalPath;
    optimalPath.startStep = startStep.step;
    optimalPath.endStep = startStep.step;
    optimalPath.rating = 0;

    if (selfhighest.chessmode == CHESSTYPE_5)
    {
        optimalPath.path.push_back(selfhighest.index);
        optimalPath.endStep += 1;
        optimalPath.rating = 10000;
        solveList.emplace_back(selfhighest.index, 10000);
    }
    else if (otherhighest.chessmode == CHESSTYPE_5)//敌方马上5连
    {
        optimalPath.path.push_back(otherhighest.index);
        optimalPath.endStep += 1;
        optimalPath.rating = 0;
        solveList.emplace_back(otherhighest.index, 0);
    }
    else if (util::hasfourkill(selfhighest.chessmode))//我方有4杀
    {
        optimalPath.path.push_back(selfhighest.index);
        optimalPath.endStep += 1;
        optimalPath.rating = 10000;
        solveList.emplace_back(selfhighest.index, 10000);
    }
    else if (doVCFSearch(board, side, optimalPath) == VCXRESULT_TRUE)
    {
        solveList.emplace_back(optimalPath.path[0], 10000);
    }
    else if (util::hasfourkill(otherhighest.chessmode))//敌方有4杀
    {
        getFourkillDefendSteps(board, otherhighest.index, solveList);
        //getVCFAtackSteps(board, solveList);
    }
    else if (doVCTSearch(board, side, optimalPath) == VCXRESULT_TRUE)
    {
        solveList.emplace_back(optimalPath.path[0], 10000);
    }
    else if (otherhighest.chessmode == CHESSTYPE_33)
    {
        getFourkillDefendSteps(board, otherhighest.index, solveList);
        //getVCFAtackSteps(board, solveList);
    }
    else
    {
        getNormalSteps(board, solveList);
    }

    if (solveList.empty())
    {
        optimalPath.rating = -10000;
        optimalPath.endStep++;
        optimalPath.path.push_back(otherhighest.index);
        solveList.emplace_back(otherhighest.index, -10000);
    }

    return optimalPath;
}


OptimalPath GoSearchEngine::solveBoard(ChessBoard* board, vector<StepCandidateItem>& solveList)
{
    int alpha = INT_MIN, beta = INT_MAX;
    OptimalPath optimalPath;
    optimalPath.startStep = startStep.step;
    optimalPath.endStep = startStep.step;
    optimalPath.rating = INT_MIN;
    uint8_t side = util::otherside(board->getLastStep().getColor());

    PieceInfo otherhighest = board->getHighestInfo(util::otherside(side));
    PieceInfo selfhighest = board->getHighestInfo(side);

    for (auto it = solveList.begin(); it != solveList.end(); ++it)
    {
        textSearchList(solveList, it->index, optimalPath.path.empty() ? it->index : optimalPath.path[0]);
        ChessBoard tempboard = *board;
        tempboard.move(it->index);

        OptimalPath tempPath;
        tempPath.startStep = board->getLastStep().step;
        tempPath.path.push_back(it->index);
        tempPath.endStep = board->getLastStep().step + 1;

        if (board->getChessType(it->index, side) == CHESSTYPE_BAN)
        {
            tempPath.rating = -(side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5));
        }
        else
        {
            doAlphaBetaSearch(&tempboard, alpha, beta, tempPath);
        }

        if (tempPath.path.size() == 1)
        {
            tempPath.rating = tempboard.getGlobalEvaluate(getAISide());
            if (side == getAISide())
            {
                tempPath.rating = -tempPath.rating;
            }
        }
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

    if (optimalPath.rating == -util::type2score(CHESSTYPE_5))
    {
        uint8_t struggleindex;
        if (doStruggleSearch(board, side, struggleindex))
        {
            OptimalPath tempPath;
            tempPath.startStep = board->getLastStep().step;
            tempPath.path.push_back(struggleindex);
            tempPath.endStep = board->getLastStep().step + 1;
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
    }

    textOutSearchInfo(optimalPath);
    return optimalPath;
}


void GoSearchEngine::doAlphaBetaSearchWrapper(ChessBoard* board, int alpha, int beta, OptimalPath& optimalPath)
{
    TransTableData data;
    if (getTransTable(board->getBoardHash().z32key, data))
    {
        if (data.checkHash == board->getBoardHash().z64key)
        {
            if ((data.isEnd() || data.endStep >= startStep.step + global_currentMaxDepth))
            {
                transTableStat.hit++;
                optimalPath.rating = data.value;
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
    //if (optimalPath.endStep > board->getLastStep().step + 1)//下一步就结束了的没必要写进置换表
    {
        data.checkHash = board->getBoardHash().z64key;
        data.endStep = optimalPath.endStep;
        data.value = optimalPath.rating;
        putTransTable(board->getBoardHash().z32key, data);
    }

}

void GoSearchEngine::doAlphaBetaSearch(ChessBoard* board, int alpha, int beta, OptimalPath& optimalPath)
{
    uint8_t steps = board->getLastStep().step;
    steps = steps;
    uint8_t side = util::otherside(board->getLastStep().getColor());

    if (board->getHighestInfo(side).chessmode == CHESSTYPE_5)
    {
        optimalPath.rating = side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
        optimalPath.path.push_back(board->getHighestInfo(side).index);
        optimalPath.endStep = board->getLastStep().step + 1;
        return;
    }

    //超时
    if (global_isOverTime || duration_cast<seconds>(std::chrono::system_clock::now() - global_startSearchTime).count() > maxSearchTime)
    {
        global_isOverTime = true;
        return;
    }

    if (board->getLastStep().step - startStep.step >= global_currentMaxDepth)
    {
        if (doVCFSearch(board, side, optimalPath) == VCXRESULT_TRUE)
        {
            return;
        }
        /*if (doVCTSearch(board, side, optimalPath) == VCXRESULT_TRUE)
        {
            return;
        }*/
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
            optimalPath.rating = -(side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5));
            return;
        }
        moves.emplace_back(otherhighest.index, 10);
    }
    else if (util::hasfourkill(selfhighest.chessmode))//我方有4杀
    {
        moves.emplace_back(selfhighest.index, 10);
    }
    else if (doVCFSearch(board, side, optimalPath) == VCXRESULT_TRUE)
    {
        return;
    }
    else if (util::hasfourkill(otherhighest.chessmode))//敌方有4杀
    {
        getFourkillDefendSteps(board, otherhighest.index, moves);
        //getVCFAtackSteps(board, moves, false);
    }
    else if (doVCTSearch(board, side, optimalPath) == VCXRESULT_TRUE)
    {
        return;
    }
    else if (otherhighest.chessmode == CHESSTYPE_33)
    {
        getFourkillDefendSteps(board, otherhighest.index, moves);
        //getVCFAtackSteps(board, moves, false);
    }
    else
    {
        getNormalSteps(board, moves);
    }

    if (moves.empty())
    {
        moves.emplace_back(otherhighest.index, -10000);
    }

    OptimalPath bestPath;
    if (side != startStep.getColor())//build AI
    {
        bestPath.rating = INT_MIN;
    }
    else
    {
        bestPath.rating = INT_MAX;
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
            tempPath.rating = -(side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5));
        }
        else
        {
            doAlphaBetaSearchWrapper(&tempboard, alpha, beta, tempPath);
            if (tempPath.endStep == board->getLastStep().step + 1)
            {
                tempPath.rating = tempboard.getGlobalEvaluate(getAISide());
                if (side == getAISide())
                {
                    tempPath.rating = -tempPath.rating;
                }
            }
        }


        if (side != startStep.getColor())//build AI
        {
            if (
                tempPath.rating > bestPath.rating
                || (tempPath.rating == bestPath.rating && tempPath.endStep < bestPath.endStep)
                )
            {
                bestPath.rating = tempPath.rating;
                bestPath.startStep = tempPath.startStep;
                bestPath.path.swap(tempPath.path);
                bestPath.endStep = tempPath.endStep;
            }

            if (tempPath.rating > beta || tempPath.rating == util::type2score(CHESSTYPE_5))//beta cut
            {
                break;
            }
            if (tempPath.rating > alpha)//update alpha
            {
                alpha = tempPath.rating;
            }
        }
        else // build player
        {
            if (
                tempPath.rating < bestPath.rating
                || (tempPath.rating == bestPath.rating && tempPath.endStep > bestPath.endStep)
                )
            {
                bestPath.rating = tempPath.rating;
                bestPath.startStep = tempPath.startStep;
                bestPath.path.swap(tempPath.path);
                bestPath.endStep = tempPath.endStep;
            }

            if (tempPath.rating < alpha || tempPath.rating == -util::type2score(CHESSTYPE_5))//alpha cut
            {
                break;
            }
            if (tempPath.rating < beta)//update beta
            {
                beta = tempPath.rating;
            }
        }
    }

    //挣扎
    if (util::hasfourkill(otherhighest.chessmode) || otherhighest.chessmode == CHESSTYPE_33)
    {
        uint8_t struggleindex;
        if ((side != startStep.getColor() && bestPath.rating == -util::type2score(CHESSTYPE_5))
            || (side == startStep.getColor() && bestPath.rating == util::type2score(CHESSTYPE_5)))
        {
            if (doStruggleSearch(board, side, struggleindex))
            {
                ChessBoard tempboard = *board;
                tempboard.move(struggleindex);

                OptimalPath tempPath;
                tempPath.startStep = board->getLastStep().step;
                tempPath.path.push_back(struggleindex);
                tempPath.endStep = board->getLastStep().step + 1;

                if (board->getChessType(struggleindex, side) == CHESSTYPE_BAN)
                {
                    tempPath.rating = -(side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5));
                }
                else
                {
                    doAlphaBetaSearchWrapper(&tempboard, alpha, beta, tempPath);
                    if (tempPath.endStep == board->getLastStep().step + 1)
                    {
                        tempPath.rating = tempboard.getGlobalEvaluate(getAISide());
                        if (side == getAISide())
                        {
                            tempPath.rating = -tempPath.rating;
                        }
                    }
                }


                if (side != startStep.getColor())//build AI
                {
                    if (
                        tempPath.rating > bestPath.rating
                        || (tempPath.rating == bestPath.rating && tempPath.endStep < bestPath.endStep)
                        )
                    {
                        bestPath.rating = tempPath.rating;
                        bestPath.startStep = tempPath.startStep;
                        bestPath.path.swap(tempPath.path);
                        bestPath.endStep = tempPath.endStep;
                    }

                    if (tempPath.rating > beta || tempPath.rating == util::type2score(CHESSTYPE_5))//beta cut
                    {

                    }
                    else if (tempPath.rating > alpha)//update alpha
                    {
                        alpha = tempPath.rating;
                    }
                }
                else // build player
                {
                    if (
                        tempPath.rating < bestPath.rating
                        || (tempPath.rating == bestPath.rating && tempPath.endStep > bestPath.endStep)
                        )
                    {
                        bestPath.rating = tempPath.rating;
                        bestPath.startStep = tempPath.startStep;
                        bestPath.path.swap(tempPath.path);
                        bestPath.endStep = tempPath.endStep;
                    }

                    if (tempPath.rating < alpha || tempPath.rating == -util::type2score(CHESSTYPE_5))//alpha cut
                    {

                    }
                    else if (tempPath.rating < beta)//update beta
                    {
                        beta = tempPath.rating;
                    }
                }
            }
        }
    }

    if (!bestPath.path.empty())
    {
        optimalPath.rating = bestPath.rating;
        optimalPath.cat(bestPath);
    }
}


void getNormalSteps1(ChessBoard* board, vector<StepCandidateItem>& childs)
{
    uint8_t side = util::otherside(board->getLastStep().getColor());

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
        if (selfp < CHESSTYPE_33)
        {
            for (int d = 0; d < DIRECTION4_COUNT; ++d)
            {
                priority += chesstypes[board->pieces_layer2[index][d][side]].atackPriority;
            }
        }
        else
        {
            priority += chesstypes[selfp].atackPriority;
            
        }

        if (otherp < CHESSTYPE_33)
        {
            for (int d = 0; d < DIRECTION4_COUNT; ++d)
            {
                priority += chesstypes[board->pieces_layer2[index][d][util::otherside(side)]].defendPriority;
            }
        }
        else
        {
            priority += chesstypes[otherp].defendPriority;
        }

        if (util::inLocalArea(index, board->getLastStep().index, LOCAL_SEARCH_RANGE))
        {
            priority += 2;
        }
        childs.emplace_back(index, priority);
    }
    std::sort(childs.begin(), childs.end(), CandidateItemCmp);
    if (childs.size() > 1)
    {
        std::sort(childs.begin(), childs.end(), CandidateItemCmp);
        for (std::vector<StepCandidateItem>::iterator it = childs.begin(); it != childs.end(); it++)
        {
            if (it->priority <= childs.begin()->priority / 2)
            {
                childs.erase(it, childs.end());
                break;
            }
        }
    }

    /*for (std::vector<StepCandidateItem>::iterator it = childs.begin(); it != childs.end(); it++)
    {
    if (util::inLocalArea(it->index, board->getLastStep().index, LOCAL_SEARCH_RANGE))
    {
    it->priority += it->priority / 2;
    }
    }
    std::sort(childs.begin(), childs.end(), CandidateItemCmp);*/

    //if (childs.size() > MAX_CHILD_NUM)
    //{
    //    childs.erase(childs.begin() + MAX_CHILD_NUM, childs.end());//只保留10个
    //}
}

void getNormalSteps2(ChessBoard* board, vector<StepCandidateItem>& childs)
{
    uint8_t side = util::otherside(board->getLastStep().getColor());

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
            if (selfp > otherp)
            {
                for (int d = 0; d < DIRECTION4_COUNT; ++d)
                {
                    priority += chesstypes[board->pieces_layer2[index][d][side]].atackPriority;
                    priority += chesstypes[board->pieces_layer2[index][d][util::otherside(side)]].defendPriority/2;
                }
            }
            else
            {
                for (int d = 0; d < DIRECTION4_COUNT; ++d)
                {
                    priority += chesstypes[board->pieces_layer2[index][d][util::otherside(side)]].defendPriority;
                    priority += chesstypes[board->pieces_layer2[index][d][side]].atackPriority/2;
                }
            }
            
        }
        else
        {
            priority += chesstypes[selfp].atackPriority > chesstypes[otherp].defendPriority ? chesstypes[selfp].atackPriority : chesstypes[otherp].defendPriority;
        }

        if (util::inLocalArea(index, board->getLastStep().index, LOCAL_SEARCH_RANGE))
        {
            priority += 2;
        }
        childs.emplace_back(index, priority);
    }
    std::sort(childs.begin(), childs.end(), CandidateItemCmp);
    if (childs.size() > 1)
    {
        std::sort(childs.begin(), childs.end(), CandidateItemCmp);
        for (std::vector<StepCandidateItem>::iterator it = childs.begin(); it != childs.end(); it++)
        {
            if (it->priority <= childs.begin()->priority / 2)
            {
                childs.erase(it, childs.end());
                break;
            }
        }
    }
}

void GoSearchEngine::getNormalSteps(ChessBoard* board, vector<StepCandidateItem>& childs)
{
    getNormalSteps1(board, childs);

}


void GoSearchEngine::getFourkillDefendSteps(ChessBoard* board, uint8_t index, vector<StepCandidateItem>& moves)
{
    uint8_t side = util::otherside(board->getLastStep().getColor());
    uint8_t defendType = board->getChessType(index, board->getLastStep().getColor());
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
        while (board->nextPosition(r, c, 1, n)) //如果不超出边界
        {
            if (board->getState(r, c) == PIECE_BLANK)
            {
                blankCount++;
                tempType = board->getChessType(r, c, util::otherside(side));
                if (tempType > CHESSTYPE_D3P)
                {
                    if (board->getThreat(r, c, side) == CHESSTYPE_BAN)//被禁手了
                    {
                        continue;
                    }
                    ChessBoard tempboard = *board;
                    tempboard.move(r, c);
                    if (tempboard.getHighestInfo(util::otherside(side)).chessmode < defendType)
                    {
                        if (board->getChessType(r, c, side) != CHESSTYPE_BAN)
                        {
                            moves.emplace_back(util::xy2index(r, c), 10);
                        }
                    }
                    else
                    {
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
                    moves.emplace_back(index, (int)(board->getRelatedFactor(index, side) * 2) + 2);
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
                    moves.emplace_back(index, (int)(board->getRelatedFactor(index, side) * 2) + 2);
                }
            }
            else
            {
                if (util::hasdead4(board->getChessType(index, side)))
                {
                    moves.emplace_back(index, (int)(board->getRelatedFactor(index, side) * 2));
                }
            }
        }

    }
    std::sort(moves.begin(), moves.end(), CandidateItemCmp);
}

uint8_t GoSearchEngine::doVCFSearchWrapper(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, bool global)
{
    TransTableDataSpecial data;
    if (getTransTableSpecial(board->getBoardHash().z32key, data))
    {
        if (data.checkHash == board->getBoardHash().z64key)
        {
            if ((data.VCFflag != VCXRESULT_NOSEARCH && data.VCFflag != VCXRESULT_UNSURE)
                || (data.VCFflag == VCXRESULT_UNSURE && startStep.step + maxVCFDepth == data.VCFEndStep))
            {
                transTableStat.hit++;
                if (data.VCFflag == VCXRESULT_TRUE)
                {
                    optimalPath.endStep = data.VCFEndStep;
                }
                return data.VCFflag;
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
    uint8_t flag = doVCFSearch(board, side, optimalPath, global);
    //if (optimalPath.endStep > board->getLastStep().step + 1)//下一步就结束了的没必要写进置换表
    {
        data.checkHash = board->getBoardHash().z64key;
        data.VCFflag = flag;
        data.VCFEndStep = optimalPath.endStep;
        if (data.VCFflag == VCXRESULT_UNSURE) data.VCFEndStep = startStep.step + maxVCFDepth;
        putTransTableSpecial(board->getBoardHash().z32key, data);
    }
    return flag;
}

uint8_t GoSearchEngine::doVCFSearch(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, bool global)
{
    uint8_t lastindex = board->getLastStep().index;
    uint8_t laststep = board->getLastStep().step;
    if (board->getHighestInfo(side).chessmode == CHESSTYPE_5)
    {
        optimalPath.path.push_back(board->getHighestInfo(side).index);
        optimalPath.endStep = board->getLastStep().step + 1;
        optimalPath.rating = side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
        return VCXRESULT_TRUE;
    }

    if (struggleFlag == 0 && board->getLastStep().step - startStep.step >= maxVCFDepth)
    {
        return VCXRESULT_UNSURE;
    }

    if (struggleFlag > 0 && board->getLastStep().step - startStep.step >= maxVCFDepth + 10)
    {
        return VCXRESULT_UNSURE;
    }

    if (global_isOverTime || duration_cast<seconds>(std::chrono::system_clock::now() - global_startSearchTime).count() > maxSearchTime)
    {
        global_isOverTime = true;
        return VCXRESULT_UNSURE;
    }

    vector<StepCandidateItem> moves;
    getVCFAtackSteps(board, moves, global);

    for (auto item : moves)
    {
        ChessBoard tempboard = *board;
        tempboard.move(item.index);//冲四

        OptimalPath tempPath;
        tempPath.startStep = optimalPath.endStep;
        tempPath.endStep = optimalPath.endStep;
        tempPath.push(item.index);

        if (tempboard.getHighestInfo(util::otherside(side)).chessmode == CHESSTYPE_5)
        {
            continue;
        }

        if (tempboard.getHighestInfo(side).chessmode != CHESSTYPE_5)//5连是禁手
        {
            continue;
        }

        if (tempboard.getChessType(tempboard.getHighestInfo(side).index, util::otherside(side)) == CHESSTYPE_BAN)//敌方触发禁手，VCF成功
        {
            optimalPath.rating = side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
            optimalPath.cat(tempPath);
            return VCXRESULT_TRUE;
        }
        tempPath.push(tempboard.getHighestInfo(side).index);//防五连
        tempboard.move(tempboard.getHighestInfo(side).index);

        if (doVCFSearchWrapper(&tempboard, side, tempPath, false) == VCXRESULT_TRUE)
        {
            optimalPath.rating = side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
            optimalPath.cat(tempPath);
            return VCXRESULT_TRUE;
        }
    }

    return VCXRESULT_FALSE;
}

uint8_t GoSearchEngine::doVCTSearchWrapper(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, bool global)
{
    TransTableDataSpecial data;
    if (getTransTableSpecial(board->getBoardHash().z32key, data))
    {
        if (data.checkHash == board->getBoardHash().z64key)
        {
            if ((data.VCTflag != VCXRESULT_NOSEARCH && data.VCTflag != VCXRESULT_UNSURE)
                || (data.VCTflag == VCXRESULT_UNSURE && startStep.step + maxVCTDepth == data.VCTEndStep))
            {
                transTableStat.hit++;
                if (data.VCTflag == VCXRESULT_TRUE)
                {
                    optimalPath.endStep = data.VCTEndStep;
                }
                return data.VCTflag;
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
    uint8_t flag = doVCTSearch(board, side, optimalPath, global);
    //if (optimalPath.endStep > board->getLastStep().step + 1)//下一步就结束了的没必要写进置换表
    {
        data.checkHash = board->getBoardHash().z64key;
        data.VCTflag = flag;
        data.VCTEndStep = optimalPath.endStep;
        if (data.VCTflag == VCXRESULT_UNSURE) data.VCTEndStep = startStep.step + maxVCTDepth;
        putTransTableSpecial(board->getBoardHash().z32key, data);
    }
    return flag;
}


uint8_t GoSearchEngine::doVCTSearch(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, bool global)
{
    uint8_t laststep = board->getLastStep().step;
    uint8_t lastindex = board->getLastStep().index;
    vector<StepCandidateItem> moves;
    if (board->getHighestInfo(side).chessmode == CHESSTYPE_5)
    {
        optimalPath.path.push_back(board->getHighestInfo(side).index);
        optimalPath.endStep += 1;
        optimalPath.rating = side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
        return VCXRESULT_TRUE;
    }
    else if (board->getHighestInfo(util::otherside(side)).chessmode == CHESSTYPE_5)
    {
        ChessBoard tempboard = *board;
        tempboard.move(board->getHighestInfo(util::otherside(side)).index);
        if (util::hasfourkill(tempboard.getHighestInfo(side).chessmode))
        {
            moves.emplace_back(board->getHighestInfo(util::otherside(side)).index, 10);
            goto startSearch;
        }
        else
        {
            return VCXRESULT_FALSE;
        }
    }

    if (struggleFlag == 0 && board->getLastStep().step - startStep.step >= maxVCTDepth)
    {
        return VCXRESULT_UNSURE;
    }

    if (struggleFlag > 0 && board->getLastStep().step - startStep.step >= maxVCTDepth + 5)
    {
        return VCXRESULT_UNSURE;
    }

    if (global_isOverTime || duration_cast<seconds>(std::chrono::system_clock::now() - global_startSearchTime).count() > maxSearchTime)
    {
        global_isOverTime = true;
        return VCXRESULT_UNSURE;
    }

    if (doVCFSearch(board, side, optimalPath) == VCXRESULT_TRUE)
    {
        return VCXRESULT_TRUE;
    }
    else
    {
        getVCTAtackSteps(board, moves, global);
    }

startSearch:
    for (auto item : moves)
    {
        ChessBoard tempboard = *board;
        tempboard.move(item.index);

        OptimalPath tempPath;
        tempPath.startStep = optimalPath.endStep;
        tempPath.endStep = optimalPath.endStep + 1;
        tempPath.path.push_back(item.index);

        if (tempboard.getHighestInfo((side)).chessmode == CHESSTYPE_5)//冲四
        {
            if (tempboard.getHighestInfo(util::otherside(side)).chessmode == CHESSTYPE_5)
            {
                continue;
            }

            if (tempboard.getChessType(tempboard.getHighestInfo(side).index, util::otherside(side)) == CHESSTYPE_BAN)//敌方触发禁手，VCF成功
            {
                optimalPath.rating = side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
                optimalPath.cat(tempPath);
                return VCXRESULT_TRUE;
            }
            tempPath.push(tempboard.getHighestInfo(side).index);//防五连
            tempboard.move(tempboard.getHighestInfo(side).index);

            if (doVCTSearchWrapper(&tempboard, side, tempPath, false) == VCXRESULT_TRUE)
            {
                optimalPath.rating = side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
                optimalPath.cat(tempPath);
                return VCXRESULT_TRUE;
            }
            continue;
        }

        if (doVCFSearch(&tempboard, util::otherside(side), tempPath) == VCXRESULT_TRUE)
        {
            continue;
        }

        if (!util::hasfourkill(tempboard.getHighestInfo(side).chessmode))//防假活三，连环禁手
        {
            continue;
        }
        vector<StepCandidateItem> defendmoves;
        getFourkillDefendSteps(&tempboard, tempboard.getHighestInfo(side).index, defendmoves);
        //getVCFAtackSteps(&tempboard, defendmoves, false);
        bool flag = true;
        OptimalPath tempPath2;
        tempPath2.startStep = tempPath.endStep;
        for (auto defend : defendmoves)
        {
            tempPath2.endStep = tempPath.endStep;
            tempPath2.path.clear();
            ChessBoard tempboard2 = tempboard;
            tempboard2.move(defend.index);

            tempPath2.push(defend.index);
            if (doVCTSearchWrapper(&tempboard2, side, tempPath2, false) != VCXRESULT_TRUE)
            {
                flag = false;
                break;
            }
        }
        if (flag)
        {
            struggleFlag++;
            uint8_t struggleindex;
            if (doStruggleSearch(&tempboard, util::otherside(side), struggleindex))
            {
                struggleFlag--;
                continue;
            }
            struggleFlag--;
            tempPath.cat(tempPath2);
            optimalPath.cat(tempPath);
            optimalPath.rating = side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
            return VCXRESULT_TRUE;
        }
    }
    return VCXRESULT_FALSE;
}

bool GoSearchEngine::doStruggleSearch(ChessBoard* board, uint8_t side, uint8_t &nextstep)
{
    uint8_t laststep = board->lastStep.step;
    vector<StepCandidateItem> moves;
    getVCFAtackSteps(board, moves, false);
    for (auto move : moves)
    {
        ChessBoard tempboard = *board;
        tempboard.move(move.index);//冲四
        if (tempboard.getHighestInfo(util::otherside(side)).chessmode == CHESSTYPE_5)
        {
            continue;
        }
        if (tempboard.getHighestInfo(side).chessmode != CHESSTYPE_5)//5连是禁手
        {
            continue;
        }
        tempboard.move(tempboard.getHighestInfo(side).index);//防五连
        vector<StepCandidateItem> defendmoves;
        getFourkillDefendSteps(&tempboard, tempboard.getHighestInfo(util::otherside(side)).index, defendmoves);

        bool flag = true;

        for (auto defend : defendmoves)
        {
            OptimalPath tempPath2;
            tempPath2.startStep = tempboard.lastStep.step;
            tempPath2.endStep = tempboard.lastStep.step;

            ChessBoard tempboard2 = tempboard;
            tempboard2.move(defend.index);
            tempPath2.push(defend.index);
            if (doVCFSearchWrapper(&tempboard2, util::otherside(side), tempPath2) == VCXRESULT_FALSE)
            {
                tempPath2.startStep = tempboard.lastStep.step;
                tempPath2.endStep = tempboard.lastStep.step;
                tempPath2.path.clear();
                tempPath2.push(defend.index);
                if (doVCTSearchWrapper(&tempboard2, util::otherside(side), tempPath2) == VCXRESULT_FALSE)//要确定挣扎成功，不能承认因为步数不够导致的成功
                {
                    nextstep = move.index;
                    return true;
                }
            }
        }
        if (doStruggleSearch(&tempboard, side, nextstep))
        {
            nextstep = move.index;
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
                moves.emplace_back(index, (int)(board->getRelatedFactor(index, side) * 2) + 2);
            }
            else
            {
                moves.emplace_back(index, (int)(board->getRelatedFactor(index, side) * 2));
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
            uint8_t tempindex;
            ChessBoard tempboard;
            for (int n : direction)
            {
                int r = util::getrow(index), c = util::getcol(index);
                int blankCount = 0, chessCount = 0;
                while (ChessBoard::nextPosition(r, c, 1, n)) //如果不超出边界
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
                        tempboard.moveTemporary(tempindex);
                        if (util::hasfourkill(tempboard.getChessType(index, side)))
                        {
                            if (util::inLocalArea(tempindex, board->getLastStep().index, LOCAL_SEARCH_RANGE))
                            {
                                moves.emplace_back(tempindex, (int)(board->getRelatedFactor(tempindex, side) * 2) + 2);
                            }
                            else
                            {
                                moves.emplace_back(tempindex, (int)(board->getRelatedFactor(tempindex, side) * 2));
                            }
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
    getVCFAtackSteps(board, moves, global);

    std::sort(moves.begin(), moves.end(), CandidateItemCmp);
    if (moves.size() > MAX_CHILD_NUM)
    {
        moves.erase(moves.begin() + MAX_CHILD_NUM, moves.end());//只保留10个
    }
}