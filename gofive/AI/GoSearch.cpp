#include "GoSearch.h"
#include "utils.h"


#define GOSEARCH_DEBUG

HashStat GoSearchEngine::transTableStat;
string GoSearchEngine::textout;

static set<csidx> board_range;


//setting
uint32_t GoSearchEngine::maxSearchTimeMs = 10000;
bool GoSearchEngine::fullUseOfTime = false;
bool GoSearchEngine::enableDebug = true;
int  GoSearchEngine::maxAlphaBetaDepth = 12;
int  GoSearchEngine::minAlphaBetaDepth = 4;
int  GoSearchEngine::maxVCFDepth = 20;//冲四
int  GoSearchEngine::maxVCTDepth = 12;//追三
int  GoSearchEngine::extraVCXDepth = 4;
//

bool CandidateItemCmp(const StepCandidateItem &a, const StepCandidateItem &b)
{
    return a.priority > b.priority;
}

GoSearchEngine::GoSearchEngine() :board(NULL)
{
    if (board_range.empty())
    {
        for (uint8_t index = 0; Util::valid(index); ++index)
        {
            board_range.insert(index);
        }
    }
}

GoSearchEngine::~GoSearchEngine()
{

}

void GoSearchEngine::initSearchEngine(ChessBoard* board)
{
    GoSearchEngine::transTableStat = { 0,0,0 };
    this->board = board;
    this->startStep = board->lastStep;
    textout.clear();
}

void GoSearchEngine::applySettings(uint32_t max_searchtime_ms, int min_depth, int max_depth, int vcf_expand, int vct_expand, int vcx_extra, bool enable_debug)
{
    maxSearchTimeMs = max_searchtime_ms;
    enableDebug = enable_debug;
    maxAlphaBetaDepth = max_depth;
    minAlphaBetaDepth = min_depth;
    maxVCFDepth = vcf_expand;
    maxVCTDepth = vct_expand;
    extraVCXDepth = vcx_extra;
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
    s << "depth:" << global_currentMaxAlphaBetaDepth << "-" << (int)(optimalPath.endStep - startStep.step) << ", ";
    s << "time:" << duration_cast<milliseconds>(system_clock::now() - global_startSearchTime).count() << "ms, ";
    s << "rating:" << optimalPath.rating << ", next:" << (int)Util::getrow(nextIndex) << "," << (int)Util::getcol(nextIndex) << "\r\n";
    textold += texttemp + s.str();
    texttemp = "";
    textout = textold;
}

void GoSearchEngine::textOutPathInfo(OptimalPath& optimalPath)
{
    //optimalPath可能为空
    stringstream s;
    s << "table:" << transTable.getTransTableSize() << " stable:" << transTable.getTransTableSpecialSize() << "\r\n";
    s << "rating:" << optimalPath.rating << " depth:" << global_currentMaxAlphaBetaDepth << "-" << (int)(optimalPath.endStep - startStep.step) << " bestpath:";
    for (auto index : optimalPath.path)
    {
        s << "(" << (int)Util::getrow(index) << "," << (int)Util::getcol(index) << ") ";
    }
    s << "\r\n";
    textold += s.str();
    textout = textold;
}

void GoSearchEngine::textSearchList(vector<StepCandidateItem>& moves, uint8_t currentindex, uint8_t best)
{
    stringstream s;
    s << "current:" << (int)Util::getrow(currentindex) << "," << (int)Util::getcol(currentindex) << "\r\n";
    s << "best:" << (int)Util::getrow(best) << "," << (int)Util::getcol(best) << "\r\n";

    s << "list:[";
    for (auto move : moves)
    {
        s << "(" << (int)Util::getrow(move.index) << "," << (int)Util::getcol(move.index) << "|" << (int)move.priority << ") ";
    }
    s << "]\r\n";

    textout = textold + texttemp + s.str();
}

void GoSearchEngine::textForTest(uint8_t currentindex, int rating, int priority)
{
    stringstream s;
    s << "current:" << (int)Util::getrow(currentindex) << "," << (int)Util::getcol(currentindex) << " rating:" << rating << " priority:" << priority << "\r\n";
    texttemp += s.str();
    textout = textold + texttemp;
}

uint8_t GoSearchEngine::getBestStep(uint64_t startSearchTime)
{
    this->global_startSearchTime = system_clock::from_time_t(startSearchTime);

    if (board->getLastStep().step < 5)//前5步增加alphabeta的步数，减少VCT步数，利于抢占局面
    {
        maxVCTDepth = 0;
    }

    global_currentMaxAlphaBetaDepth = minAlphaBetaDepth;
    vector<StepCandidateItem> solveList;
    OptimalPath optimalPath = makeSolveList(board, solveList);

    if (solveList.size() == 1 && (optimalPath.rating == 10000 || optimalPath.rating == -10000))
    {
        textOutPathInfo(optimalPath);

        return solveList[0].index;
    }

    for (int count = 0;
        global_currentMaxAlphaBetaDepth < maxAlphaBetaDepth;
        count++
        , global_currentMaxAlphaBetaDepth += 1
        , maxVCFDepth += 2
        //, maxVCTDepth += count % 2 == 0 ? 2 : 0
        )
    {
        if (fullUseOfTime)
        {
            if (duration_cast<milliseconds>(std::chrono::system_clock::now() - global_startSearchTime).count() > maxSearchTimeMs)
            {
                global_currentMaxAlphaBetaDepth--;
                break;
            }
        }
        else
        {
            if (duration_cast<milliseconds>(std::chrono::system_clock::now() - global_startSearchTime).count() > maxSearchTimeMs / 3)
            {
                global_currentMaxAlphaBetaDepth--;
                break;
            }
        }

        OptimalPath temp = solveBoard(board, solveList);
        std::sort(solveList.begin(), solveList.end(), CandidateItemCmp);
        if (global_currentMaxAlphaBetaDepth > minAlphaBetaDepth && global_isOverTime)
        {
            global_currentMaxAlphaBetaDepth--;
            break;
        }
        optimalPath = temp;
        if (temp.rating >= chesstypes[CHESSTYPE_5].rating || temp.rating <= -chesstypes[CHESSTYPE_5].rating)
        {
            //break;
        }

        //已成定局的不需要继续搜索了
        if (solveList.size() == 1)
        {
            break;
        }
        /*for (size_t i = 0; i < solveList.size(); ++i)
        {
            if (solveList[i].priority == -10000)
            {
                solveList.erase(solveList.begin() + i, solveList.end());
                break;
            }
        }*/
        if (solveList.empty())
        {
            break;
        }
    }
    textOutPathInfo(optimalPath);

    if (optimalPath.path.empty())
    {
        optimalPath.path.push_back(board->getHighestInfo(startStep.getSide()).index);
    }

    return optimalPath.path[0];
}


OptimalPath GoSearchEngine::makeSolveList(ChessBoard* board, vector<StepCandidateItem>& solveList)
{
    uint8_t side = Util::otherside(board->getLastStep().getSide());

    PieceInfo otherhighest = board->getHighestInfo(Util::otherside(side));
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
    else if (doVCFSearchWrapper(board, side, optimalPath, NULL) == VCXRESULT_TRUE)
    {
        solveList.emplace_back(optimalPath.path[0], 10000);
    }
    else if (Util::isfourkill(otherhighest.chesstype))//敌方有4杀
    {
        getFourkillDefendSteps(board, otherhighest.index, solveList);
    }
    else if (doVCTSearchWrapper(board, side, optimalPath, NULL) == VCXRESULT_TRUE)
    {
        solveList.emplace_back(optimalPath.path[0], 10000);
    }
    else if (otherhighest.chesstype == CHESSTYPE_33)//敌方有33
    {
        getFourkillDefendSteps(board, otherhighest.index, solveList);
    }
    else
    {
        getNormalSteps(board, solveList, NULL);
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
    uint8_t side = Util::otherside(board->getLastStep().getSide());

    PieceInfo otherhighest = board->getHighestInfo(Util::otherside(side));
    PieceInfo selfhighest = board->getHighestInfo(side);
    //空着搜索

    //
    for (auto it = solveList.begin(); it != solveList.end(); ++it)
    {
        textSearchList(solveList, it->index, optimalPath.path.empty() ? it->index : optimalPath.path[0]);

        OptimalPath tempPath(board->getLastStep().step);

        doAlphaBetaSearch(board, global_currentMaxAlphaBetaDepth, it->index, alpha, beta, tempPath);

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

        if (enableDebug)
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
        set<uint8_t> reletedset;
        OptimalPath temp(board->getLastStep().step);
        temp.push(board->getLastStep().index);
        temp.cat(optimalPath);
        getNormalRelatedSet(board, reletedset, temp);
        OptimalPath tempPath(board->getLastStep().step);
        if (doNormalStruggleSearch(board, global_currentMaxAlphaBetaDepth, alpha, beta, reletedset, tempPath, &solveList))
        {
            optimalPath = tempPath;
        }
    }

    textOutSearchInfo(optimalPath);
    return optimalPath;
}

void GoSearchEngine::doAlphaBetaSearch(ChessBoard* board, int depth, csidx index, int alpha, int beta, OptimalPath& optimalPath)
{
    uint8_t side = Util::otherside(board->getLastStep().getSide());
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
    depth -= 1;
    //USE TransTable
    TransTableData data;
    if (transTable.getTransTable(currentBoard.getBoardHash().z64key, data))
    {
        if (data.checkHash == currentBoard.getBoardHash().z32key)
        {
            if (data.depth < depth + currentBoard.getLastStep().step)
            {
                transTableStat.cover++;
            }
            else
            {
                if (data.type == TRANSTYPE_LOWER && isPlayerSide(otherside) && data.value >= alpha)
                {
                    transTableStat.cover++;
                }
                else if (data.type == TRANSTYPE_UPPER && (!isPlayerSide(otherside)) && data.value <= beta)
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
    else if (currentBoard.getHighestInfo(side).chesstype == CHESSTYPE_5)//防5连
    {
        if (currentBoard.getChessType(currentBoard.getHighestInfo(side).index, otherside) == CHESSTYPE_BAN)//敌人触发禁手，自己赢了
        {
            optimalPath.push(currentBoard.getHighestInfo(side).index);
            optimalPath.rating = isPlayerSide(side) ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
            return;
        }
        if (depth <= 0)
        {
            optimalPath.push(currentBoard.getHighestInfo(side).index);
            optimalPath.rating = currentBoard.getGlobalEvaluate(getAISide());
            return;
        }
        else
        {
            moves.emplace_back(currentBoard.getHighestInfo(side).index, 10);
        }
    }
    else if (global_isOverTime || duration_cast<milliseconds>(std::chrono::system_clock::now() - global_startSearchTime).count() > maxSearchTimeMs)//超时
    {
        global_isOverTime = true;
        return;
    }
    else if (depth <= 0)
    {
        if (doVCFSearch(&currentBoard, otherside, VCFPath, NULL) == VCXRESULT_TRUE)//敌人赢了
        {
            bestPath = VCFPath;
            goto end;
        }
        else
        {
            optimalPath.rating = currentBoard.getGlobalEvaluate(getAISide());
            return;
        }
    }
    else if (doVCFSearch(&currentBoard, otherside, VCFPath, NULL) == VCXRESULT_TRUE)//敌人赢了
    {
        bestPath = VCFPath;
        goto end;
    }
    else if (Util::isfourkill(currentBoard.getHighestInfo(side).chesstype))//防4杀
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
        goto end;
    }
    else if (currentBoard.getHighestInfo(side).chesstype == CHESSTYPE_33)//敌方有33
    {
        getFourkillDefendSteps(board, currentBoard.getHighestInfo(side).index, moves);
        if (moves.empty())
        {
            moves.emplace_back(currentBoard.getHighestInfo(side).index, -10000);
        }
    }
    else
    {
        set<uint8_t> myset;
        getNormalSteps(&currentBoard, moves, myset.empty() ? NULL : &myset);
        if (moves.empty())
        {
            moves.emplace_back(currentBoard.getHighestInfo(side).index, -10000);
        }
    }
    bool foundPV = false;
    data.type = TRANSTYPE_EXACT;
    for (size_t i = 0; i < moves.size(); ++i)
    {
        //剪枝
        if (isPlayerSide(otherside))//build player
        {
            OptimalPath tempPath(currentBoard.getLastStep().step);
            if (foundPV)
            {
                doAlphaBetaSearch(&currentBoard, depth, moves[i].index, beta - 1, beta, tempPath);//0窗口剪裁
                //假设当前是最好的，没有任何其他的会比当前的PV好（小于beta）
                if (tempPath.rating < beta && tempPath.rating > alpha)
                {
                    tempPath.path.clear();
                    doAlphaBetaSearch(&currentBoard, depth, moves[i].index, alpha, beta, tempPath);
                }
            }
            else
            {
                doAlphaBetaSearch(&currentBoard, depth, moves[i].index, alpha, beta, tempPath);
            }

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
                if (i < moves.size() - 1)
                {
                    data.type = TRANSTYPE_LOWER;
                }
                break;
            }
            if (tempPath.rating < beta)//update beta
            {
                beta = tempPath.rating;
                foundPV = true;
            }

        }
        else // build AI
        {
            OptimalPath tempPath(currentBoard.getLastStep().step);
            if (foundPV)
            {
                doAlphaBetaSearch(&currentBoard, depth, moves[i].index, alpha, alpha + 1, tempPath);//0窗口剪裁
                //假设当前是最好的，没有任何其他的会比当前的PV好（大于alpha）
                if (tempPath.rating > alpha && tempPath.rating < beta)
                {
                    tempPath.path.clear();
                    doAlphaBetaSearch(&currentBoard, depth, moves[i].index, alpha, beta, tempPath);
                }
            }
            else
            {
                doAlphaBetaSearch(&currentBoard, depth, moves[i].index, alpha, beta, tempPath);
            }

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
                if (i < moves.size() - 1)
                {
                    data.type = TRANSTYPE_UPPER;
                }
                break;
            }
            if (tempPath.rating > alpha)//update alpha
            {
                alpha = tempPath.rating;
                foundPV = true;
            }
        }
    }


    //if (currentBoard.getLastStep().step - startStep.step < global_currentMaxDepth - 2)//保证最后两步能正常进行
    {
        //挣扎
        if ((otherside != startStep.getSide() && bestPath.rating == -util::type2score(CHESSTYPE_5))
            || (otherside == startStep.getSide() && bestPath.rating == util::type2score(CHESSTYPE_5)))
        {
            set<uint8_t> reletedset;
            OptimalPath temp(board->getLastStep().step);
            temp.push(index);
            temp.cat(bestPath);
            getNormalRelatedSet(board, reletedset, temp);
            OptimalPath tempPath(currentBoard.getLastStep().step);
            if (doNormalStruggleSearch(&currentBoard, depth, alpha, beta, reletedset, tempPath, NULL))
            {
                bestPath = tempPath;
            }
        }
    }

end:
    optimalPath.cat(bestPath);
    optimalPath.rating = bestPath.rating;

    //USE TransTable
    //写入置换表
    data.checkHash = currentBoard.getBoardHash().z32key;
    data.endStep = optimalPath.endStep;
    data.value = optimalPath.rating;
    data.depth = depth + currentBoard.getLastStep().step;
    transTable.putTransTable(currentBoard.getBoardHash().z64key, data);
    //end USE TransTable
}

bool GoSearchEngine::doNormalStruggleSearch(ChessBoard* board, int depth, int alpha, int beta, set<uint8_t>& reletedset, OptimalPath& optimalPath, vector<StepCandidateItem>* solveList)
{
    uint8_t laststep = board->lastStep.step;
    vector<StepCandidateItem> moves;
    getNormalDefendSteps(board, moves, &reletedset);
    optimalPath.rating = board->getLastStep().getSide() == startStep.getSide() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
    for (auto move : moves)
    {
        OptimalPath tempPath(board->getLastStep().step);

        doAlphaBetaSearch(board, depth, move.index, alpha, beta, tempPath);

        if (board->getLastStep().getSide() == startStep.getSide())//AI不想输(-util::type2score(CHESSTYPE_5))
        {
            if (tempPath.rating > optimalPath.rating)
            {
                optimalPath = tempPath;
                alpha = tempPath.rating;
                if (solveList != NULL)
                {
                    solveList->emplace_back(move.index, tempPath.rating);
                }
            }
        }
        else//player不想输（util::type2score(CHESSTYPE_5)）
        {
            if (tempPath.rating < optimalPath.rating)
            {
                optimalPath = tempPath;
                beta = tempPath.rating;
                if (solveList != NULL)
                {
                    solveList->emplace_back(move.index, tempPath.rating);
                }
            }
        }
    }

    if ((board->getLastStep().getSide() == startStep.getSide() && optimalPath.rating > -util::type2score(CHESSTYPE_5))
        || (board->getLastStep().getSide() != startStep.getSide() && optimalPath.rating < util::type2score(CHESSTYPE_5)))
    {
        return true;
    }
    return false;
}



void GoSearchEngine::getNormalRelatedSet(ChessBoard* board, set<uint8_t>& reletedset, OptimalPath& optimalPath)
{
    ChessBoard tempboard = *board;
    uint8_t side = board->getLastStep().getOtherSide();
    for (size_t i = 0; i < optimalPath.path.size() && i < 10; i++)
    {
        tempboard.move(optimalPath.path[i]);
        reletedset.insert(optimalPath.path[i]);

        //Position pos(optimalPath.path[i]);
        //Position temppos;
        //for (int d = 0; d < DIRECTION4_COUNT; ++d)
        //{
        //    for (int i = 0, symbol = -1; i < 2; ++i, symbol = 1)//正反
        //    {
        //        temppos = pos.getNextPosition(d, symbol);
        //        if (temppos.valid() && tempboard.canMove(temppos.toIndex()))
        //        {
        //            reletedset.insert(temppos.toIndex());
        //        }
        //    }
        //}

        //util::myset_intersection(&tempset, &reletedset, &reletedset);
        i++;
        if (i < optimalPath.path.size())
        {
            tempboard.move(optimalPath.path[i]);
            tempboard.getAtackReletedPos(reletedset, optimalPath.path[i - 1], side);
            //reletedset.insert(optimalPath.path[i]);
        }
    }
}

void getNormalSteps1(ChessBoard* board, vector<StepCandidateItem>& childs)
{
    //优先进攻
    uint8_t side = Util::otherside(board->getLastStep().getSide());

    for (uint8_t index = 0; Util::valid(index); ++index)
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

        uint8_t otherp = board->getChessType(index, Util::otherside(side));

        int8_t priority = 0;

        if (selfp < CHESSTYPE_33 && otherp < CHESSTYPE_33)
        {
            if (chesstypes[selfp].atackPriority > chesstypes[otherp].defendFactor)
            {
                priority += chesstypes[board->pieces_layer3[index][side]].atackPriority;
                priority += chesstypes[board->pieces_layer3[index][Util::otherside(side)]].defendPriority / 2;
            }
            else
            {
                priority += chesstypes[board->pieces_layer3[index][Util::otherside(side)]].defendPriority;
                priority += chesstypes[board->pieces_layer3[index][side]].atackPriority / 2;
            }
        }
        else
        {
            priority += chesstypes[selfp].atackPriority > chesstypes[otherp].defendPriority ? chesstypes[selfp].atackPriority : chesstypes[otherp].defendPriority;
        }

        if (chesstypes[selfp].atackPriority == 0 && otherp < CHESSTYPE_2)
        {
            continue;
        }

        double atack = board->getRelatedFactor(index, side), defend = board->getRelatedFactor(index, Util::otherside(side), true);
        childs.emplace_back(index, (int)(chesstypes[selfp].atackPriority*atack * 2 + chesstypes[otherp].defendPriority*defend * 2));
    }

    std::sort(childs.begin(), childs.end(), CandidateItemCmp);
    /*for (std::vector<StepCandidateItem>::iterator it = childs.begin(); it != childs.end(); it++)
    {
        if (it->priority <= childs.begin()->priority / 2)
        {
            childs.erase(it, childs.end());
            break;
        }
    }*/
    if (childs.size() > MAX_CHILD_NUM)
    {
        int threshold = childs[MAX_CHILD_NUM - 1].priority;
        for (auto it = childs.begin() + MAX_CHILD_NUM; it != childs.end(); it++)
        {
            if (it->priority < threshold)
            {
                childs.erase(it, childs.end());
                break;
            }
        }
        //childs.erase(childs.begin() + MAX_CHILD_NUM, childs.end());//只保留10个
    }
}

void GoSearchEngine::getNormalSteps(ChessBoard* board, vector<StepCandidateItem>& childs, set<uint8_t>* reletedset)
{
    getNormalSteps1(board, childs);
    //getNormalSteps1(board, childs, reletedset);
}

void GoSearchEngine::getNormalDefendSteps(ChessBoard* board, vector<StepCandidateItem>& moves, set<uint8_t>* reletedset)
{
    uint8_t side = Util::otherside(board->getLastStep().getSide());
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
        if (!(board->canMove(index) && board->useful(index)))
        {
            continue;
        }
        if (board->getChessType(index, side) == CHESSTYPE_BAN)
        {
            continue;
        }
        if (Util::isdead4(board->getChessType(index, side)))
        {
            if (board->getLastStep().step - startStep.step < global_currentMaxAlphaBetaDepth - 2)
            {
                moves.emplace_back(index, 10);
            }
            continue;
        }
        uint8_t otherp = board->getChessType(index, Util::otherside(side));
        double atack = board->getRelatedFactor(index, side), defend = board->getRelatedFactor(index, Util::otherside(side), true);
        moves.emplace_back(index, (int)(defend * 2 + atack) + chesstypes[otherp].defendPriority / 2);
    }
    std::sort(moves.begin(), moves.end(), CandidateItemCmp);
}


void GoSearchEngine::getFourkillDefendSteps(ChessBoard* board, uint8_t index, vector<StepCandidateItem>& moves)
{
    //现在该防守方落子
    uint8_t defendside = board->getLastStep().getOtherSide();//防守方
    uint8_t atackside = board->getLastStep().getSide();//进攻方
    uint8_t atackType = board->getChessType(index, atackside);

    vector<uint8_t> direction;

    if (board->getChessType(index, defendside) != CHESSTYPE_BAN)
    {
        moves.emplace_back(index, 10);
    }

    if (atackType == CHESSTYPE_4)//两个进攻点__ooo__，两个防点/一个进攻点x_ooo__（有一边被堵），三个防点
    {
        for (int d = 0; d < DIRECTION4_COUNT; ++d)
        {
            if (board->pieces_layer2[index][d][atackside] == CHESSTYPE_4)
            {
                direction.push_back(d * 2);
                direction.push_back(d * 2 + 1);
                break;
            }
        }
        //判断是哪种棋型
        int defend_point_count = 1;
        for (auto n : direction)
        {
            int r = Util::getrow(index), c = Util::getcol(index);
            int blankCount = 0, chessCount = 0;
            while (Util::displace(r, c, 1, n)) //如果不超出边界
            {
                if (board->getState(r, c) == PIECE_BLANK)
                {
                    blankCount++;
                    uint8_t tempType = board->pieces_layer2[Util::xy2index(r, c)][n / 2][atackside];
                    if (tempType == CHESSTYPE_4)
                    {
                        defend_point_count++;
                        if (board->getChessType(r, c, defendside) != CHESSTYPE_BAN)
                        {
                            moves.emplace_back(Util::xy2index(r, c), 8);
                        }
                    }
                }
                else if (board->getState(r, c) == defendside)
                {
                    break;
                }
                else
                {
                    chessCount++;
                }
                if (blankCount == 1
                    || chessCount > 3)
                {
                    break;
                }
            }
        }
        if (defend_point_count > 1)//__ooo__的两个防点已找到
        {
            return;
        }
        //没找到，说明是x_ooo__类型，继续找
    }
    else if (atackType == CHESSTYPE_44)//一个攻点，三个防点
    {
        for (int d = 0; d < DIRECTION4_COUNT; ++d)
        {
            if (board->pieces_layer2[index][d][atackside] == CHESSTYPE_44)
            {
                direction.push_back(d * 2);
                direction.push_back(d * 2 + 1);
                break;
            }
            else if (Util::isdead4(board->pieces_layer2[index][d][atackside]))
            {
                direction.push_back(d * 2);
                direction.push_back(d * 2 + 1);
            }
        }
    }
    else if (atackType == CHESSTYPE_43)//一个攻点，四个防点
    {
        for (int d = 0; d < DIRECTION4_COUNT; ++d)
        {
            if (Util::isdead4(board->pieces_layer2[index][d][atackside]) || Util::isalive3(board->pieces_layer2[index][d][atackside]))
            {
                direction.push_back(d * 2);
                direction.push_back(d * 2 + 1);
            }
        }
    }
    else if (atackType == CHESSTYPE_33)//一个攻点，五个防点
    {
        for (int d = 0; d < DIRECTION4_COUNT; ++d)
        {
            if (Util::isalive3(board->pieces_layer2[index][d][atackside]))
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

    for (auto n : direction)
    {
        int r = Util::getrow(index), c = Util::getcol(index);
        int blankCount = 0, chessCount = 0;
        while (Util::displace(r, c, 1, n)) //如果不超出边界
        {
            if (board->getState(r, c) == PIECE_BLANK)
            {
                blankCount++;
                uint8_t tempType = board->pieces_layer2[Util::xy2index(r, c)][n / 2][atackside];
                if (tempType > CHESSTYPE_0)
                {
                    if (board->getChessType(r, c, defendside) != CHESSTYPE_BAN)//被禁手了
                    {
                        ChessBoard tempboard = *board;
                        tempboard.move(r, c);
                        //if (tempboard.getHighestInfo(board->getLastStep().getSide()).chesstype < defendType)
                        if (tempboard.getChessType(index, atackside) < atackType)
                        {
                            moves.emplace_back(Util::xy2index(r, c), 8);
                        }
                    }
                }
            }
            else if (board->getState(r, c) == defendside)
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

VCXRESULT GoSearchEngine::doVCFSearchWrapper(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, set<uint8_t>* reletedset)
{
    TransTableVCXData data;
    if (transTable.getTransTableVCX(board->getBoardHash().z64key, data))
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
                    if (data.VCFflag == VCXRESULT_TRUE)
                    {
                        optimalPath.endStep = data.VCFEndStep;
                    }
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
    VCXRESULT flag = doVCFSearch(board, side, optimalPath, reletedset);
    if (reletedset == NULL || flag != VCXRESULT_UNSURE)
    {
        data.checkHash = board->getBoardHash().z32key;
        data.VCFflag = flag;
        data.VCFEndStep = optimalPath.endStep;
        data.VCFDepth = (struggleFlag ? maxVCFDepth + extraVCXDepth : maxVCFDepth);
        transTable.putTransTableVCX(board->getBoardHash().z64key, data);
    }
    return flag;
}

VCXRESULT GoSearchEngine::doVCTSearchWrapper(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, set<uint8_t>* reletedset)
{
    TransTableVCXData data;
    if (transTable.getTransTableVCX(board->getBoardHash().z64key, data))
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
                    if (data.VCTflag == VCXRESULT_TRUE)
                    {
                        optimalPath.endStep = data.VCTEndStep;
                    }
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
    VCXRESULT flag = doVCTSearch(board, side, optimalPath, reletedset);
    if (reletedset == NULL || flag != VCXRESULT_UNSURE)
    {
        data.checkHash = board->getBoardHash().z32key;
        data.VCTflag = flag;
        data.VCTEndStep = optimalPath.endStep;
        data.VCTDepth = (struggleFlag ? maxVCTDepth + extraVCXDepth : maxVCTDepth);
        transTable.putTransTableVCX(board->getBoardHash().z64key, data);
    }
    return flag;
}

VCXRESULT GoSearchEngine::doVCFSearch(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, set<uint8_t>* reletedset)
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

    bool unsure_flag = false;
    vector<StepCandidateItem> moves;
    getVCFAtackSteps(board, moves, reletedset);

    for (auto item : moves)
    {
        ChessBoard tempboard = *board;
        tempboard.move(item.index);//冲四

        OptimalPath tempPath(board->getLastStep().step);
        tempPath.push(item.index);

        if (tempboard.getHighestInfo(Util::otherside(side)).chesstype == CHESSTYPE_5)
        {
            continue;
        }

        if (tempboard.getHighestInfo(side).chesstype != CHESSTYPE_5)//5连是禁手
        {
            continue;
        }

        if (tempboard.getChessType(tempboard.getHighestInfo(side).index, Util::otherside(side)) == CHESSTYPE_BAN)//敌方触发禁手，VCF成功
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
    }

    if (unsure_flag)
    {
        return VCXRESULT_UNSURE;
    }
    else
    {
        return VCXRESULT_FALSE;
    }

}

VCXRESULT GoSearchEngine::doVCTSearch(ChessBoard* board, uint8_t side, OptimalPath& optimalPath, set<uint8_t>* reletedset)
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
    else if (board->getHighestInfo(Util::otherside(side)).chesstype == CHESSTYPE_5)
    {
        ChessBoard tempboard = *board;
        tempboard.move(board->getHighestInfo(Util::otherside(side)).index);
        if (Util::isfourkill(tempboard.getHighestInfo(side).chesstype))
        {
            moves.emplace_back(board->getHighestInfo(Util::otherside(side)).index, 10);
        }
        else
        {
            //optimalPath.push(board->getHighestInfo(util::otherside(side)).index);
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
            if (tempboard.getHighestInfo(Util::otherside(side)).chesstype == CHESSTYPE_5)//失败，对方有5连
            {
                continue;
            }
            if (tempboard.getChessType(tempboard.getHighestInfo(side).index, Util::otherside(side)) == CHESSTYPE_BAN)//敌方触发禁手，VCF成功
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
            tempresult = doVCFSearch(&tempboard, Util::otherside(side), tempPath2, NULL);
            if (tempresult == VCXRESULT_TRUE)
            {
                continue;
            }
            if (tempresult == VCXRESULT_UNSURE)
            {
                unsure_flag = true;
            }
            if (!Util::isfourkill(tempboard.getHighestInfo(side).chesstype))//防假活三，连环禁手
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
            uint8_t struggleindex = item.index;
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
    if (board->lastStep.step - startStep.step > (struggleFlag ? maxVCTDepth + extraVCXDepth : maxVCTDepth) - 2)
    {
        return false;
    }
    uint8_t side = board->lastStep.getOtherSide();
    uint8_t laststep = board->lastStep.step;
    vector<StepCandidateItem> moves;
    set<uint8_t> atackset;
    board->getAtackReletedPos(atackset, nextstep, board->lastStep.getSide());
    getVCFAtackSteps(board, moves, &atackset);
    if (board->lastStep.chessType < CHESSTYPE_D4)//特殊处理，可能引入新bug
    {
        for (auto index : atackset)
        {
            if (Util::isalive3(board->getChessType(index, side)) && board->getChessType(index, board->lastStep.getSide()) > CHESSTYPE_D4P)
            {
                moves.emplace_back(index, 8);
            }
        }
    }
    for (auto move : moves)
    {
        ChessBoard tempboard = *board;
        if (Util::hasdead4(tempboard.getChessType(move.index, side)))
        {
            tempboard.move(move.index);//冲四
            if (tempboard.getHighestInfo(Util::otherside(side)).chesstype == CHESSTYPE_5)
            {
                continue;
            }
            if (tempboard.getHighestInfo(side).chesstype != CHESSTYPE_5)//5连是禁手
            {
                continue;
            }
            tempboard.move(tempboard.getHighestInfo(side).index);//防五连

            if (!Util::isfourkill(tempboard.getHighestInfo(Util::otherside(side)).chesstype))
            {
                nextstep = move.index;
                return true;
            }

            vector<StepCandidateItem> defendmoves;
            getFourkillDefendSteps(&tempboard, tempboard.getHighestInfo(Util::otherside(side)).index, defendmoves);

            for (auto defend : defendmoves)
            {
                OptimalPath tempPath(tempboard.lastStep.step);
                ChessBoard tempboard2 = tempboard;
                tempboard2.move(defend.index);
                tempPath.push(defend.index);
                uint8_t result = doVCTSearchWrapper(&tempboard2, Util::otherside(side), tempPath, &atackset);
                if (result != VCXRESULT_TRUE)//要确定挣扎成功，不能承认因为步数不够导致的成功
                {
                    nextstep = move.index;
                    return true;
                }
            }
        }
        else
        {
            tempboard.move(move.index);
            OptimalPath tempPath(tempboard.lastStep.step);
            tempPath.push(move.index);
            uint8_t result = doVCTSearchWrapper(&tempboard, Util::otherside(side), tempPath, &atackset);
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
    uint8_t side = Util::otherside(board->getLastStep().getSide());

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

        if (Util::hasdead4(board->getChessType(index, side)))
        {
            if (Util::isfourkill(board->getChessType(index, side)))
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

void GoSearchEngine::getVCTAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, set<csidx>* reletedset)
{
    uint8_t side = Util::otherside(board->getLastStep().getSide());

    set<csidx>* range;
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

        if (Util::isalive3(board->getChessType(index, side)))
        {
            moves.emplace_back(index, (int)(board->getRelatedFactor(index, side) * 2));
        }
        else if (Util::isdead4(board->getChessType(index, side)))
        {
            moves.emplace_back(index, (int)(board->getRelatedFactor(index, side) * 2));

            uint8_t tempindex;
            ChessBoard tempboard;
            for (uint8_t n = 0; n < DIRECTION8::DIRECTION8_COUNT; ++n)
            {
                if (Util::isdead4(board->pieces_layer2[index][n / 2][side]))
                {
                    continue;
                }
                int r = Util::getrow(index), c = Util::getcol(index);
                int blankCount = 0, chessCount = 0;
                while (Util::displace(r, c, 1, n)) //如果不超出边界
                {
                    tempindex = Util::xy2index(r, c);
                    if (board->getState(tempindex) == PIECE_BLANK)
                    {
                        blankCount++;
                        if (Util::isalive3(board->getChessType(tempindex, side)))
                        {
                            break;
                        }
                        tempboard = *board;
                        tempboard.move(tempindex);
                        if (Util::isfourkill(tempboard.getChessType(index, side)))
                        {
                            moves.emplace_back(tempindex, (int)(board->getRelatedFactor(tempindex, side) * 2));
                        }
                    }
                    else if (board->getState(tempindex) == Util::otherside(side))
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
        //moves.erase(moves.begin() + MAX_CHILD_NUM, moves.end());//只保留10个
    }
}