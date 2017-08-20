#include "GoSearch.h"
#include "ThreadPool.h"

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

void GoSearchEngine::initSearchEngine(ChessBoard* board)
{
    GoSearchEngine::transTableStat = { 0,0,0 };
    this->board = board;
    this->startStep = board->lastStep;
    textout.clear();
}

void GoSearchEngine::applySettings(
    uint32_t max_searchtime_ms,
    uint32_t rest_match_time_ms,
    int min_depth,
    int max_depth,
    int vcf_expand,
    int vct_expand,
    bool enable_debug,
    bool use_transtable,
    bool full_search
)
{
    maxStepTimeMs = max_searchtime_ms;
    restMatchTimeMs = rest_match_time_ms;
    enableDebug = enable_debug;
    maxAlphaBetaDepth = max_depth;
    minAlphaBetaDepth = min_depth;
    VCFExpandDepth = vcf_expand;
    VCTExpandDepth = vct_expand;
    useTransTable = use_transtable;
    fullSearch = full_search;
}

void GoSearchEngine::textOutSearchInfo(OptimalPath& optimalPath)
{
    stringstream s;
    if (optimalPath.path.size() == 0)
    {
        textout = "no path";
        return;
    }
    Position nextpos = optimalPath.path[0];
    s << "depth:" << currentAlphaBetaDepth << "-" << (int)(optimalPath.endStep - startStep.step) << ", ";
    s << "time:" << duration_cast<milliseconds>(system_clock::now() - startSearchTime).count() << "ms, ";
    s << "rating:" << optimalPath.rating << ", next:" << (int)nextpos.row << "," << (int)nextpos.col << "\r\n";
    textold += texttemp + s.str();
    texttemp = "";
    textout = textold;
}

void GoSearchEngine::textOutPathInfo(OptimalPath& optimalPath, uint32_t suggest_time)
{
    //optimalPath可能为空
    stringstream s;
    s << "rating:" << optimalPath.rating << " depth:" << currentAlphaBetaDepth << "-" << (int)(optimalPath.endStep - startStep.step) << " bestpath:";
    for (auto pos : optimalPath.path)
    {
        s << "(" << (int)pos.row << "," << (int)pos.col << ") ";
    }
    s << "\r\n";
    s << "maxtime:" << maxStepTimeMs << "ms suggest:" << suggest_time << "ms\r\n";
    s << "table:" << transTable.getTransTableSize() << " stable:" << transTable.getTransTableVCXSize() << "\r\n";
    textold += s.str();
    textout = textold;
}

void GoSearchEngine::textSearchList(vector<StepCandidateItem>& moves, Position current, Position best)
{
    stringstream s;
    s << "current:" << (int)current.row << "," << (int)current.col << "\r\n";
    s << "best:" << (int)best.row << "," << (int)best.col << "\r\n";

    s << "list:[";
    for (auto move : moves)
    {
        s << "(" << (int)move.pos.row << "," << (int)move.pos.col << "|" << (int)move.priority << ") ";
    }
    s << "]\r\n";

    textout = textold + texttemp + s.str();
}

void GoSearchEngine::textForTest(OptimalPath& optimalPath, int priority)
{
    stringstream s;
    //s << "current:" << (int)Util::getrow(currentindex) << "," << (int)Util::getcol(currentindex) << " rating:" << rating << " priority:" << priority << "\r\n";

    s << "current:" << (int)(optimalPath.path[0].row) << "," << (int)(optimalPath.path[0].col) << " rating:" << optimalPath.rating << " priority:" << priority;
    /*s << "\r\nbestpath:";
    for (auto index : optimalPath.path)
    {
        s << "(" << (int)index.row << "," << (int)index.col << ") ";
    }*/
    s << "\r\n";
    texttemp += s.str();
    textout = textold + texttemp;
}

void GoSearchEngine::allocatedTime(uint32_t& max_time, uint32_t&suggest_time)
{
    int step = startStep.step;
    if (step < 5)
    {
        max_time = restMatchTimeMs / 20 < maxStepTimeMs ? restMatchTimeMs / 20 : maxStepTimeMs;
        suggest_time = max_time;
    }
    else if (step < 50)
    {
        if (restMatchTimeMs < maxStepTimeMs)
        {
            max_time = restMatchTimeMs;
            suggest_time = restMatchTimeMs / 25;
        }
        else if (restMatchTimeMs / 25 < maxStepTimeMs / 3)
        {
            max_time = maxStepTimeMs / 2;
            suggest_time = (restMatchTimeMs / 25);
        }
        else
        {
            max_time = maxStepTimeMs;
            suggest_time = maxStepTimeMs / 3;
        }
    }
    else
    {
        if (restMatchTimeMs < maxStepTimeMs)
        {
            max_time = restMatchTimeMs;
            suggest_time = restMatchTimeMs / 10;
        }
        else if (restMatchTimeMs / 50 < maxStepTimeMs / 5)
        {
            max_time = maxStepTimeMs / 3;
            suggest_time = (restMatchTimeMs / 50);
        }
        else
        {
            max_time = maxStepTimeMs;
            suggest_time = maxStepTimeMs / 5;
        }
    }
}


Position GoSearchEngine::getBestStep(uint64_t startSearchTime)
{
    this->startSearchTime = system_clock::from_time_t(startSearchTime);

    //if (board->getLastStep().step < 4)//前5步增加alphabeta的步数，减少VCT步数，利于抢占局面
    //{
    //    /*maxVCTDepth -= 2;
    //    maxVCFDepth -= 4;*/
    //    VCTExpandDepth = 0;
    //}
    uint32_t max_time, suggest_time;
    allocatedTime(max_time, suggest_time);
    maxStepTimeMs = max_time;

    currentAlphaBetaDepth = minAlphaBetaDepth;
    vector<StepCandidateItem> solveList;
    OptimalPath optimalPath = makeSolveList(board, solveList);

    if (solveList.size() == 1 && (solveList[0].priority == 10000 || solveList[0].priority == -10000))
    {
        textOutPathInfo(optimalPath, suggest_time);

        return solveList[0].pos;
    }



    for (int count = 0;;
        count++
        //, VCFExpandDepth += 2
        //, maxVCTDepth += count % 2 == 0 ? 2 : 0
        )
    {
        if (duration_cast<milliseconds>(std::chrono::system_clock::now() - this->startSearchTime).count() > suggest_time)
        {
            currentAlphaBetaDepth--;
            break;
        }

        OptimalPath temp = solveBoard(board, solveList);
        std::sort(solveList.begin(), solveList.end(), CandidateItemCmp);
        if (currentAlphaBetaDepth > minAlphaBetaDepth && global_isOverTime)
        {
            currentAlphaBetaDepth--;
            break;
        }
        optimalPath = temp;
        if (temp.rating >= CHESSTYPE_5_SCORE || temp.rating <= -CHESSTYPE_5_SCORE)
        {
            //break;
        }

        //已成定局的不需要继续搜索了
        if (solveList.size() <= 1)
        {
            break;
        }
        else
        {
            if (solveList[0].priority > -10000 && solveList[1].priority == -10000)
            {
                break;
            }
        }
        /*for (size_t i = 0; i < solveList.size(); ++i)
        {
            if (solveList[i].priority == -10000 && i > 0)
            {
                solveList.erase(solveList.begin() + i, solveList.end());
                break;
            }
        }*/
        if (solveList.empty())
        {
            break;
        }

        if (currentAlphaBetaDepth == maxAlphaBetaDepth)
        {
            break;
        }
        else
        {
            currentAlphaBetaDepth++;
        }
    }
    textOutPathInfo(optimalPath, suggest_time);

    if (optimalPath.path.empty())
    {
        optimalPath.path.push_back(board->getHighestInfo(startStep.getState()).pos);
    }

    return optimalPath.path[0];
}


OptimalPath GoSearchEngine::makeSolveList(ChessBoard* board, vector<StepCandidateItem>& solveList)
{
    uint8_t side = Util::otherside(board->getLastStep().getState());

    PieceInfo otherhighest = board->getHighestInfo(Util::otherside(side));
    PieceInfo selfhighest = board->getHighestInfo(side);

    OptimalPath optimalPath(startStep.step);
    optimalPath.endStep = startStep.step;
    optimalPath.rating = 0;

    if (selfhighest.chesstype == CHESSTYPE_5)
    {
        optimalPath.push(selfhighest.pos);
        optimalPath.rating = 10000;
        solveList.emplace_back(selfhighest.pos, 10000);
    }
    else if (otherhighest.chesstype == CHESSTYPE_5)//敌方马上5连
    {
        if (board->getChessType(otherhighest.pos, side) == CHESSTYPE_BAN)
        {
            optimalPath.rating = -10000;
            solveList.emplace_back(otherhighest.pos, 10000);
        }
        else
        {
            optimalPath.push(otherhighest.pos);
            board->move(otherhighest.pos);
            optimalPath.rating = board->getGlobalEvaluate(getAISide());
            solveList.emplace_back(otherhighest.pos, 10000);
        }
    }
    else if (doVCFSearch(board, getVCFDepth(board->getLastStep().step), optimalPath, NULL, useTransTable) == VCXRESULT_TRUE)
    {
        solveList.emplace_back(optimalPath.path[0], 10000);
    }
    else if (Util::isfourkill(otherhighest.chesstype))//敌方有4杀
    {
        getFourkillDefendSteps(board, otherhighest.pos, solveList);
    }
    else if ((useMultiThread ? doVCTSearchWithMultiThread(board, getVCTDepth(board->getLastStep().step), optimalPath, useTransTable)
        : doVCTSearch(board, getVCTDepth(board->getLastStep().step), optimalPath, NULL, useTransTable)) == VCXRESULT_TRUE)
    {
        solveList.emplace_back(optimalPath.path[0], 10000);
    }
    else if (otherhighest.chesstype == CHESSTYPE_33)//敌方有33
    {
        getFourkillDefendSteps(board, otherhighest.pos, solveList);
    }
    else
    {
        getNormalSteps(board, solveList, NULL, fullSearch);
    }

    if (solveList.empty())
    {
        optimalPath.rating = -10000;
        optimalPath.push(otherhighest.pos);
        solveList.emplace_back(otherhighest.pos, 10000);
    }

    return optimalPath;
}


OptimalPath GoSearchEngine::solveBoard(ChessBoard* board, vector<StepCandidateItem>& solveList)
{
    int alpha = INT_MIN, beta = INT_MAX;
    OptimalPath optimalPath(startStep.step);
    optimalPath.endStep = startStep.step;
    optimalPath.rating = INT_MIN;

    bool foundPV = false;
    for (auto it = solveList.begin(); it != solveList.end(); ++it)
    {
        textSearchList(solveList, it->pos, optimalPath.path.empty() ? it->pos : optimalPath.path[0]);

        OptimalPath tempPath(board->getLastStep().step);
        tempPath.push(it->pos);
        ChessBoard currentBoard = *board;
        currentBoard.move(it->pos);

        if (foundPV)
        {
            doAlphaBetaSearch(&currentBoard, currentAlphaBetaDepth - 1, alpha, alpha + 1, tempPath, useTransTable);//0窗口剪裁
            //假设当前是最好的，没有任何其他的会比当前的PV好（大于alpha）                                                                                   
            if (tempPath.rating > alpha && tempPath.rating < beta)//使用完整窗口
            {
                tempPath.path.clear();
                tempPath.push(it->pos);
                doAlphaBetaSearch(&currentBoard, currentAlphaBetaDepth - 1, alpha, beta, tempPath, useTransTable);
            }
        }
        else
        {
            doAlphaBetaSearch(&currentBoard, currentAlphaBetaDepth - 1, alpha, beta, tempPath, useTransTable);
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

        if (enableDebug)
        {
            textForTest(tempPath, it->priority);
        }

        it->priority = tempPath.rating;

        if (tempPath.rating > alpha)
        {
            alpha = tempPath.rating;
            foundPV = true;
        }

        if (tempPath.rating > optimalPath.rating)
        {
            optimalPath = tempPath;
        }
        else if (tempPath.rating == optimalPath.rating)
        {
            if ((tempPath.rating == CHESSTYPE_5_SCORE && tempPath.endStep < optimalPath.endStep) ||
                (tempPath.rating == -CHESSTYPE_5_SCORE && tempPath.endStep > optimalPath.endStep))
            {
                optimalPath = tempPath;
            }
        }
    }

    if (optimalPath.rating == -CHESSTYPE_5_SCORE)
    {
        set<Position> reletedset;
        getNormalRelatedSet(board, reletedset, optimalPath);
        OptimalPath tempPath(board->getLastStep().step);
        if (doNormalStruggleSearch(board, currentAlphaBetaDepth, alpha, beta, reletedset, tempPath, &solveList, useTransTable))
        {
            optimalPath = tempPath;
        }
    }

    textOutSearchInfo(optimalPath);
    return optimalPath;
}

void GoSearchEngine::doAlphaBetaSearch(ChessBoard* board, int depth, int alpha, int beta, OptimalPath& optimalPath, bool useTransTable)
{
    uint8_t side = board->getLastStep().getOtherSide();
    uint8_t otherside = board->getLastStep().getState();
    Position lastindex = board->getLastStep().pos;
    int laststep = board->getLastStep().step;
    bool continue_flag = false;
    //USE TransTable
    TransTableData data;
    if (useTransTable)
    {
        if (transTable.getTransTable(board->getBoardHash().z64key, data))
        {
            if (data.checkHash == board->getBoardHash().z32key)
            {
                if (data.depth < depth)
                {
                    transTableStat.cover++;
                    data.continue_index = 0;
                }
                else
                {
                    if (isPlayerSide(side))
                    {
                        if (data.value < alpha)
                        {
                            transTableStat.hit++;
                            optimalPath.rating = data.value;
                            optimalPath.endStep = data.endStep;
                            return;
                        }
                        else//data.value >= alpha
                        {
                            if (data.type == TRANSTYPE_EXACT)
                            {
                                transTableStat.hit++;
                                optimalPath.rating = data.value;
                                optimalPath.endStep = data.endStep;
                                return;
                            }
                            else if (data.type == TRANSTYPE_LOWER)
                            {
                                transTableStat.cover++;
                                continue_flag = true;
                            }
                        }
                    }
                    else
                    {
                        if (data.value > beta)
                        {
                            transTableStat.hit++;
                            optimalPath.rating = data.value;
                            optimalPath.endStep = data.endStep;
                            return;
                        }
                        else
                        {
                            if (data.type == TRANSTYPE_EXACT)
                            {
                                transTableStat.hit++;
                                optimalPath.rating = data.value;
                                optimalPath.endStep = data.endStep;
                                return;
                            }
                            else if (data.type == TRANSTYPE_UPPER)
                            {
                                transTableStat.cover++;
                                continue_flag = true;
                            }
                        }
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
    }
    //end USE TransTable
    size_t i = data.continue_index;

    OptimalPath VCFPath(board->getLastStep().step);
    OptimalPath VCTPath(board->getLastStep().step);

    vector<StepCandidateItem> moves;
    OptimalPath bestPath(board->getLastStep().step);
    bestPath.rating = isPlayerSide(side) ? INT_MAX : INT_MIN;

    if (board->getHighestInfo(side).chesstype == CHESSTYPE_5)
    {
        optimalPath.rating = isPlayerSide(side) ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
        //optimalPath.push(board->getHighestInfo(side).index);
        return;
    }
    else if (board->getHighestInfo(otherside).chesstype == CHESSTYPE_5)//防5连
    {
        if (board->getChessType(board->getHighestInfo(otherside).pos, side) == CHESSTYPE_BAN)//触发禁手，otherside赢了
        {
            //optimalPath.push(board->getHighestInfo(otherside).index);
            optimalPath.rating = isPlayerSide(side) ? CHESSTYPE_5_SCORE : -CHESSTYPE_5_SCORE;
            return;
        }
        if (depth <= 0)
        {
            //optimalPath.push(board->getHighestInfo(otherside).index);
            //TO MAKESURE
            optimalPath.rating = board->getGlobalEvaluate(getAISide());//存疑
            return;
        }
        else
        {
            moves.emplace_back(board->getHighestInfo(otherside).pos, 10);
        }
    }
    else if (global_isOverTime || duration_cast<milliseconds>(std::chrono::system_clock::now() - startSearchTime).count() > maxStepTimeMs)//超时
    {
        global_isOverTime = true;
        return;
    }
    else if (depth <= 0)
    {
        //静态搜索拓展
        if (doVCFSearch(board, getVCFDepth(board->getLastStep().step), VCFPath, NULL, useTransTable) == VCXRESULT_TRUE)//side赢了
        {
            bestPath = VCFPath;
            goto end;
        }
        else
        {
            optimalPath.rating = board->getGlobalEvaluate(getAISide());
            return;
        }
    }
    else if (doVCFSearch(board, getVCFDepth(board->getLastStep().step), VCFPath, NULL, useTransTable) == VCXRESULT_TRUE)//side赢了
    {
        bestPath = VCFPath;
        goto end;
    }
    else if (Util::isfourkill(board->getHighestInfo(otherside).chesstype))//防4杀
    {
        getFourkillDefendSteps(board, board->getHighestInfo(otherside).pos, moves);
        if (moves.empty())
        {
            moves.emplace_back(board->getHighestInfo(otherside).pos, -10000);
        }
    }
    else if ((useMultiThread ? doVCTSearchWithMultiThread(board, getVCTDepth(board->getLastStep().step), VCTPath, useTransTable)
        : doVCTSearch(board, getVCTDepth(board->getLastStep().step), VCTPath, NULL, useTransTable)) == VCXRESULT_TRUE)
    {
        bestPath = VCTPath;
        goto end;
    }

    else if (board->getHighestInfo(otherside).chesstype == CHESSTYPE_33)//敌方有33
    {
        getFourkillDefendSteps(board, board->getHighestInfo(otherside).pos, moves);
        if (moves.empty())
        {
            moves.emplace_back(board->getHighestInfo(otherside).pos, -10000);
        }
    }
    else
    {
        set<Position> myset;
        getNormalSteps(board, moves, myset.empty() ? NULL : &myset, fullSearch);
        if (moves.empty())
        {
            moves.emplace_back(board->getHighestInfo(otherside).pos, -10000);
        }
    }

    bool foundPV = false;

    data.type = TRANSTYPE_EXACT;

    //空着剪裁，只对黑方
    //if (continue_flag)
    //{
    //    bestPath.rating = data.value;
    //    bestPath.endStep = data.endStep;
    //}
    //else
    //{
    //    if (side == PIECE_BLACK)
    //    {
    //        ChessBoard tempboard = *board;
    //        OptimalPath tempPath(board->getLastStep().step);
    //        tempPath.push(board->getLastStep().index);
    //        tempboard.moveNull();
    //        if (isPlayerSide(side))//build player
    //        {
    //            doAlphaBetaSearch(&tempboard, depth - 1, beta - 1, beta, tempPath, false);//0窗口剪裁
    //            if (tempPath.rating < alpha)//alpha cut
    //            {
    //                bestPath.rating = tempPath.rating;
    //                bestPath.endStep = tempPath.endStep;
    //                data.type = TRANSTYPE_LOWER;
    //                goto end;
    //            }
    //        }
    //        else
    //        {
    //            doAlphaBetaSearch(&tempboard, depth - 1, alpha, alpha + 1, tempPath, false);//0窗口剪裁
    //            if (tempPath.rating > beta)//beta cut
    //            {
    //                bestPath.rating = tempPath.rating;
    //                bestPath.endStep = tempPath.endStep;
    //                data.type = TRANSTYPE_UPPER;
    //                goto end;
    //            }
    //        }
    //    }
    //}

    for (; i < moves.size(); ++i)
    {
        OptimalPath tempPath(board->getLastStep().step);
        tempPath.push(moves[i].pos);
        ChessBoard currentBoard = *board;
        currentBoard.move(moves[i].pos);

        //剪枝
        if (isPlayerSide(side))//build player
        {
            if (foundPV)
            {
                doAlphaBetaSearch(&currentBoard, depth - 1, beta - 1, beta, tempPath, useTransTable);//0窗口剪裁
                //假设当前是最好的，没有任何其他的会比当前的PV好（小于beta）
                if (tempPath.rating < beta && tempPath.rating > alpha)//失败，使用完整窗口
                {
                    tempPath.path.clear();
                    tempPath.push(moves[i].pos);
                    doAlphaBetaSearch(&currentBoard, depth - 1, alpha, beta, tempPath, useTransTable);
                }
            }
            else
            {
                doAlphaBetaSearch(&currentBoard, depth - 1, alpha, beta, tempPath, useTransTable);
            }

            if (tempPath.rating < bestPath.rating)
            {
                bestPath = tempPath;
            }
            else if (tempPath.rating == bestPath.rating)
            {
                if (tempPath.rating == -CHESSTYPE_5_SCORE && tempPath.endStep < bestPath.endStep)//赢了，尽量快
                {
                    bestPath = tempPath;
                }
                else if (tempPath.rating == CHESSTYPE_5_SCORE && tempPath.endStep > bestPath.endStep)//必输，尽量拖
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
            if (foundPV)
            {
                doAlphaBetaSearch(&currentBoard, depth - 1, alpha, alpha + 1, tempPath, useTransTable);//0窗口剪裁
                //假设当前是最好的，没有任何其他的会比当前的PV好（大于alpha）
                if (tempPath.rating > alpha && tempPath.rating < beta)
                {
                    tempPath.path.clear();
                    tempPath.push(moves[i].pos);
                    doAlphaBetaSearch(&currentBoard, depth - 1, alpha, beta, tempPath, useTransTable);
                }
            }
            else
            {
                doAlphaBetaSearch(&currentBoard, depth - 1, alpha, beta, tempPath, useTransTable);
            }

            if (tempPath.rating > bestPath.rating
                || (tempPath.rating == bestPath.rating && tempPath.endStep < bestPath.endStep))
            {
                bestPath = tempPath;
            }
            else if (tempPath.rating == bestPath.rating)
            {
                if (tempPath.rating == CHESSTYPE_5_SCORE && tempPath.endStep < bestPath.endStep)//赢了，尽量快
                {
                    bestPath = tempPath;
                }
                else if (tempPath.rating == -CHESSTYPE_5_SCORE && tempPath.endStep > bestPath.endStep)//必输，尽量拖
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


    if (board->getHighestInfo(Util::otherside(side)).chesstype != CHESSTYPE_5)//如果是要防5连，就没必要挣扎了
    {
        //挣扎
        if ((isPlayerSide(side) && bestPath.rating == CHESSTYPE_5_SCORE)
            || (!isPlayerSide(side) && bestPath.rating == -CHESSTYPE_5_SCORE))
        {
            set<Position> reletedset;
            getNormalRelatedSet(board, reletedset, bestPath);
            OptimalPath tempPath(board->getLastStep().step);
            if (doNormalStruggleSearch(board, depth, alpha, beta, reletedset, tempPath, NULL, useTransTable))
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
    if (useTransTable)
    {
        data.checkHash = board->getBoardHash().z32key;
        data.endStep = optimalPath.endStep;
        data.value = optimalPath.rating;
        data.depth = depth;
        data.continue_index = (uint8_t)i;
        transTable.putTransTable(board->getBoardHash().z64key, data);
    }
    //end USE TransTable
}

bool GoSearchEngine::doNormalStruggleSearch(ChessBoard* board, int depth, int alpha, int beta, set<Position>& reletedset, OptimalPath& optimalPath, vector<StepCandidateItem>* solveList, bool useTransTable)
{
    uint8_t laststep = board->lastStep.step;
    uint8_t side = board->getLastStep().getOtherSide();
    vector<StepCandidateItem> moves;
    getNormalDefendSteps(board, moves, &reletedset);
    optimalPath.rating = isPlayerSide(side) ? CHESSTYPE_5_SCORE : -CHESSTYPE_5_SCORE;
    for (auto move : moves)
    {
        OptimalPath tempPath(board->getLastStep().step);
        tempPath.push(move.pos);
        ChessBoard currentBoard = *board;
        currentBoard.move(move.pos);
        if (Util::hasdead4(board->getChessType(move.pos, side)))
        {
            doAlphaBetaSearch(&currentBoard, depth + 1, alpha, beta, tempPath, useTransTable);
        }
        else
        {
            doAlphaBetaSearch(&currentBoard, depth - 1, alpha, beta, tempPath, useTransTable);
        }

        if (!isPlayerSide(side))//AI不想输(-util::type2score(CHESSTYPE_5))
        {
            if (tempPath.rating > optimalPath.rating)
            {
                optimalPath = tempPath;
                alpha = tempPath.rating;
                if (solveList != NULL)
                {
                    solveList->emplace_back(move.pos, tempPath.rating);
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
                    solveList->emplace_back(move.pos, tempPath.rating);
                }
            }
        }
    }

    if ((!isPlayerSide(side) && optimalPath.rating > -CHESSTYPE_5_SCORE)
        || (isPlayerSide(side) && optimalPath.rating < CHESSTYPE_5_SCORE))
    {
        return true;
    }
    return false;
}



void GoSearchEngine::getNormalRelatedSet(ChessBoard* board, set<Position>& reletedset, OptimalPath& optimalPath)
{
    //uint8_t defendside = board->getLastStep().getOtherSide();
    uint8_t atackside = board->getLastStep().getState();//laststep的进攻成功，现在要找防守步
    Position lastindex = board->getLastStep().pos;
    vector<Position> path;
    path.push_back(lastindex);
    path.insert(path.end(), optimalPath.path.begin(), optimalPath.path.end());

    ChessBoard tempboard = *board;
    for (size_t i = 0; i < path.size() && i < 10; i++)
    {
        tempboard.move(optimalPath.path[i]);
        reletedset.insert(optimalPath.path[i]);

        i++;
        if (i < optimalPath.path.size())
        {
            tempboard.move(optimalPath.path[i]);
            tempboard.getAtackReletedPos(reletedset, optimalPath.path[i - 1], atackside);//相关点是对于进攻而言的，防守策略根据进攻的相关点去防守
            reletedset.insert(optimalPath.path[i]);
        }
    }
}

void getNormalSteps1(ChessBoard* board, vector<StepCandidateItem>& childs, bool fullSearch)
{
    //优先进攻
    uint8_t side = board->getLastStep().getOtherSide();

    ForEachPosition
    {
        if (!(board->canMove(pos) && board->useful(pos)))
        {
            continue;
        }

        uint8_t selfp = board->getChessType(pos, side);

        if (selfp == CHESSTYPE_BAN)
        {
            continue;
        }

        uint8_t otherp = board->getChessType(pos, Util::otherside(side));

        double atack = board->getRelatedFactor(pos, side), defend = board->getRelatedFactor(pos, Util::otherside(side), true);

        if (atack < 1.0 && defend < 0.5)
        {
            continue;
        }
        if (selfp == CHESSTYPE_D4 && atack < 2.0 && defend < 0.5)//会导致禁手陷阱无法触发，因为禁手陷阱一般都是始于“无意义”的冲四
        {
            continue;
        }
        childs.emplace_back(pos, (int)((atack + defend) * 10));
    }

    std::sort(childs.begin(), childs.end(), CandidateItemCmp);

    if (childs.size() > MAX_CHILD_NUM && !fullSearch)
    {
        for (auto it = childs.begin(); it != childs.end(); it++)
        {
            if (it->priority < childs[0].priority / 2)
            {
                childs.erase(it, childs.end());
                break;
            }
        }
    }

    //if (childs.size() > MAX_CHILD_NUM && !fullSearch)
    //{
    //    int threshold = childs[MAX_CHILD_NUM - 1].priority;
    //    for (auto it = childs.begin() + MAX_CHILD_NUM; it != childs.end(); it++)
    //    {
    //        if (it->priority < threshold)
    //        {
    //            childs.erase(it, childs.end());
    //            break;
    //        }
    //    }
    //    //childs.erase(childs.begin() + MAX_CHILD_NUM, childs.end());//只保留10个
    //}
}

void GoSearchEngine::getNormalSteps(ChessBoard* board, vector<StepCandidateItem>& childs, set<Position>* reletedset, bool full_search)
{
    getNormalSteps1(board, childs, full_search);
}

void GoSearchEngine::getNormalDefendSteps(ChessBoard* board, vector<StepCandidateItem>& moves, set<Position>* reletedset)
{
    uint8_t side = board->getLastStep().getOtherSide();
    set<Position>* range;
    if (reletedset == NULL)
    {
        range = &Util::board_range;
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
            //if (board->getLastStep().step - startStep.step < global_currentMaxAlphaBetaDepth - 2)
            {
                moves.emplace_back(index, 10);
            }
            continue;
        }
        uint8_t otherp = board->getChessType(index, Util::otherside(side));
        double atack = board->getRelatedFactor(index, side), defend = board->getRelatedFactor(index, Util::otherside(side), true);
        moves.emplace_back(index, (int)(defend + atack / 2));
    }
    std::sort(moves.begin(), moves.end(), CandidateItemCmp);
}


void GoSearchEngine::getFourkillDefendSteps(ChessBoard* board, Position pos, vector<StepCandidateItem>& moves)
{
    //现在该防守方落子
    uint8_t defendside = board->getLastStep().getOtherSide();//防守方
    uint8_t atackside = board->getLastStep().getState();//进攻方
    uint8_t atackType = board->getChessType(pos, atackside);

    vector<uint8_t> direction;

    if (board->getChessType(pos, defendside) != CHESSTYPE_BAN)
    {
        moves.emplace_back(pos, 10);
    }

    if (atackType == CHESSTYPE_4)//两个进攻点__ooo__，两个防点/一个进攻点x_ooo__（有一边被堵），三个防点
    {
        for (int d = 0; d < DIRECTION4_COUNT; ++d)
        {
            if (board->pieces_layer2[pos.row][pos.col][d][atackside] == CHESSTYPE_4)
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
            Position temppos = pos;
            int blankCount = 0, chessCount = 0;
            while (temppos.displace8(1, n)) //如果不超出边界
            {
                if (board->getState(temppos.row, temppos.col) == PIECE_BLANK)
                {
                    blankCount++;
                    uint8_t tempType = board->pieces_layer2[temppos.row][temppos.col][n / 2][atackside];
                    if (tempType == CHESSTYPE_4)
                    {
                        defend_point_count++;
                        if (board->getChessType(temppos.row, temppos.col, defendside) != CHESSTYPE_BAN)
                        {
                            moves.emplace_back(temppos, 8);
                        }
                    }
                }
                else if (board->getState(temppos.row, temppos.col) == defendside)
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
            if (board->pieces_layer2[pos.row][pos.col][d][atackside] == CHESSTYPE_44)
            {
                direction.push_back(d * 2);
                direction.push_back(d * 2 + 1);
                break;
            }
            else if (Util::isdead4(board->pieces_layer2[pos.row][pos.col][d][atackside]))
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
            if (Util::isdead4(board->pieces_layer2[pos.row][pos.col][d][atackside]) || Util::isalive3(board->pieces_layer2[pos.row][pos.col][d][atackside]))
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
            if (Util::isalive3(board->pieces_layer2[pos.row][pos.col][d][atackside]))
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
        Position temppos = pos;
        int blankCount = 0, chessCount = 0;
        while (temppos.displace8(1, n)) //如果不超出边界
        {
            if (board->getState(temppos.row, temppos.col) == PIECE_BLANK)
            {
                blankCount++;
                uint8_t tempType = board->pieces_layer2[temppos.row][temppos.col][n / 2][atackside];
                if (tempType > CHESSTYPE_0)
                {
                    if (board->getChessType(temppos.row, temppos.col, defendside) != CHESSTYPE_BAN)//被禁手了
                    {
                        ChessBoard tempboard = *board;
                        tempboard.move(temppos.row, temppos.col);
                        //if (tempboard.getHighestInfo(board->getLastStep().getSide()).chesstype < defendType)
                        if (tempboard.getChessType(pos, atackside) < atackType)
                        {
                            moves.emplace_back(temppos, 8);
                        }
                    }
                }
            }
            else if (board->getState(temppos.row, temppos.col) == defendside)
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

VCXRESULT GoSearchEngine::doVCFSearchWrapper(ChessBoard* board, int depth, OptimalPath& optimalPath, set<Position>* reletedset, bool useTransTable)
{
    if (!useTransTable)
    {
        return doVCFSearch(board, depth, optimalPath, reletedset, useTransTable);
    }

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
                if (data.VCFflag == VCXRESULT_UNSURE && data.VCFDepth < depth)//需要更新
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
    VCXRESULT flag = doVCFSearch(board, depth, optimalPath, reletedset, useTransTable);
    //if (reletedset == NULL || flag != VCXRESULT_FALSE)
    if (depth > 0)
    {
        data.checkHash = board->getBoardHash().z32key;
        data.VCFflag = flag;
        data.VCFEndStep = optimalPath.endStep;
        data.VCFDepth = depth;
        transTable.putTransTableVCX(board->getBoardHash().z64key, data);
    }
    return flag;
}

VCXRESULT GoSearchEngine::doVCTSearchWrapper(ChessBoard* board, int depth, OptimalPath& optimalPath, set<Position>* reletedset, bool useTransTable)
{
    if (!useTransTable)
    {
        return doVCTSearch(board, depth, optimalPath, reletedset, useTransTable);
    }

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
                if ((data.VCTflag == VCXRESULT_UNSURE /*|| data.VCTflag == VCXRESULT_TRUE*/) && data.VCTDepth < depth)//需要更新
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
    VCXRESULT flag = doVCTSearch(board, depth, optimalPath, reletedset, useTransTable);
    //if (reletedset == NULL || flag == VCXRESULT_FALSE)
    if (depth > 0)
    {
        data.checkHash = board->getBoardHash().z32key;
        data.VCTflag = flag;
        data.VCTEndStep = optimalPath.endStep;
        data.VCTDepth = depth;
        transTable.putTransTableVCX(board->getBoardHash().z64key, data);
    }
    return flag;
}

VCXRESULT GoSearchEngine::doVCFSearch(ChessBoard* board, int depth, OptimalPath& optimalPath, set<Position>* reletedset, bool useTransTable)
{
    uint8_t side = board->getLastStep().getOtherSide();
    Position lastindex = board->getLastStep().pos;
    uint8_t laststep = board->getLastStep().step;
    if (board->getHighestInfo(side).chesstype == CHESSTYPE_5)
    {
        optimalPath.push(board->getHighestInfo(side).pos);
        optimalPath.rating = side == startStep.getState() ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
        return VCXRESULT_TRUE;
    }
    else if (depth <= 0)
    {
        return VCXRESULT_UNSURE;
    }
    else if (global_isOverTime || duration_cast<milliseconds>(std::chrono::system_clock::now() - startSearchTime).count() > maxStepTimeMs)
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
        tempboard.move(item.pos);//冲四

        OptimalPath tempPath(board->getLastStep().step);
        tempPath.push(item.pos);

        if (tempboard.getHighestInfo(Util::otherside(side)).chesstype == CHESSTYPE_5)
        {
            continue;
        }

        if (tempboard.getHighestInfo(side).chesstype != CHESSTYPE_5)//5连是禁手
        {
            continue;
        }

        if (tempboard.getChessType(tempboard.getHighestInfo(side).pos, Util::otherside(side)) == CHESSTYPE_BAN)//敌方触发禁手，VCF成功
        {
            optimalPath.rating = side == startStep.getState() ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
            optimalPath.cat(tempPath);
            return VCXRESULT_TRUE;
        }
        tempPath.push(tempboard.getHighestInfo(side).pos);//防五连
        tempboard.move(tempboard.getHighestInfo(side).pos);

        set<Position> atackset;
        /*if (reletedset != NULL)
        {
            set<uint8_t> tempatackset;
            tempboard.getAtackReletedPos(tempatackset, item.index, side);
            util::myset_intersection(reletedset, &tempatackset, &atackset);
        }
        else*/
        {
            tempboard.getAtackReletedPos(atackset, item.pos, side);
        }

        uint8_t result = doVCFSearchWrapper(&tempboard, depth - 2, tempPath, &atackset, useTransTable);
        if (result == VCXRESULT_TRUE)
        {
            optimalPath.rating = side == startStep.getState() ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
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

static bool thread_unsure_flag = false;
static bool search_success_flag = false;
static OptimalPath resultPath(0);

void GoSearchEngine::doVCTSearchForEachThread(VCTData *data)
{
    uint8_t side = data->board->getLastStep().getOtherSide();
    ChessBoard tempboard = *(data->board);
    tempboard.move(data->nextpos);

    if (tempboard.getHighestInfo(side).chesstype != CHESSTYPE_5
        && (!Util::isfourkill(tempboard.getHighestInfo(side).chesstype)))//无法造成VCT威胁
    {
        delete data;
        return;
    }

    OptimalPath tempPath(data->board->getLastStep().step);
    tempPath.push(data->nextpos);

    vector<StepCandidateItem> defendmoves;

    if (tempboard.getHighestInfo(side).chesstype == CHESSTYPE_5)//冲四
    {
        if (tempboard.getHighestInfo(Util::otherside(side)).chesstype == CHESSTYPE_5)//失败，对方有5连
        {
            delete data;
            return;
        }
        if (tempboard.getChessType(tempboard.getHighestInfo(side).pos, Util::otherside(side)) == CHESSTYPE_BAN)//敌方触发禁手，VCF成功
        {
            search_success_flag = true;
            resultPath = tempPath;
            delete data;
            return;
        }
        defendmoves.emplace_back(tempboard.getHighestInfo(side).pos, 10);
    }
    else //活三
    {
        OptimalPath tempPath2(tempboard.getLastStep().step);
        VCXRESULT tempresult = data->engine->doVCFSearch(&tempboard, data->depth + data->engine->VCFExpandDepth - data->engine->VCTExpandDepth - 1, tempPath2, NULL, data->engine->useTransTable);
        if (tempresult == VCXRESULT_TRUE)
        {
            delete data;
            return;
        }
        if (tempresult == VCXRESULT_UNSURE)
        {
            thread_unsure_flag = true;
        }
        if (!Util::isfourkill(tempboard.getHighestInfo(side).chesstype))//防假活三，连环禁手
        {
            delete data;
            return;
        }

        getFourkillDefendSteps(&tempboard, tempboard.getHighestInfo(side).pos, defendmoves);
    }


    bool flag = true;
    OptimalPath tempPath2(tempboard.lastStep.step);
    for (auto defend : defendmoves)
    {
        tempPath2.endStep = tempPath.endStep;
        tempPath2.path.clear();
        ChessBoard tempboard2 = tempboard;
        tempboard2.move(defend.pos);

        tempPath2.push(defend.pos);

        set<Position> atackset;
        /*if (reletedset != NULL)
        {
        set<uint8_t> tempatackset;
        tempboard2.getAtackReletedPos(tempatackset, item.index, side);
        util::myset_intersection(reletedset, &tempatackset, &atackset);
        }
        else*/
        {
            tempboard2.getAtackReletedPos(atackset, data->nextpos, side);
        }

        VCXRESULT tempresult = data->engine->doVCTSearchWrapper(&tempboard2, data->depth - 2, tempPath2, &atackset, data->engine->useTransTable);

        if (tempresult == VCXRESULT_UNSURE)
        {
            thread_unsure_flag = true;
        }
        if (tempresult != VCXRESULT_TRUE)
        {
            flag = false;
            break;
        }
    }
    if (flag)
    {
        Position struggleindex = data->nextpos;
        if (data->engine->doVCTStruggleSearch(&tempboard, data->depth - 1, struggleindex, data->engine->useTransTable))
        {
            delete data;
            return;
        }

        tempPath.cat(tempPath2);
        resultPath = tempPath;
    }

    delete data;
    return;
}

VCXRESULT GoSearchEngine::doVCTSearchWithMultiThread(ChessBoard* board, int depth, OptimalPath& optimalPath, bool useTransTable)
{
    uint8_t side = board->getLastStep().getOtherSide();
    uint8_t laststep = board->getLastStep().step;
    Position lastindex = board->getLastStep().pos;
    OptimalPath VCFPath(board->getLastStep().step);
    vector<StepCandidateItem> moves;
    if (board->getHighestInfo(side).chesstype == CHESSTYPE_5)
    {
        optimalPath.push(board->getHighestInfo(side).pos);
        optimalPath.rating = side == startStep.getState() ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
        return VCXRESULT_TRUE;
    }
    else if (board->getHighestInfo(Util::otherside(side)).chesstype == CHESSTYPE_5)
    {
        if (board->getChessType(board->getHighestInfo(Util::otherside(side)).pos, side) == CHESSTYPE_BAN)
        {
            return VCXRESULT_FALSE;
        }
        moves.emplace_back(board->getHighestInfo(Util::otherside(side)).pos, 10);
    }
    else if (global_isOverTime || duration_cast<milliseconds>(std::chrono::system_clock::now() - startSearchTime).count() > maxStepTimeMs)
    {
        global_isOverTime = true;
        return VCXRESULT_UNSURE;
    }
    else if (doVCFSearch(board, depth + VCFExpandDepth - VCTExpandDepth, VCFPath, NULL, useTransTable) == VCXRESULT_TRUE)
    {
        optimalPath.cat(VCFPath);
        optimalPath.rating = VCFPath.rating;
        return VCXRESULT_TRUE;
    }
    else if (depth <= 0)
    {
        return VCXRESULT_UNSURE;
    }
    else
    {
        getVCTAtackSteps(board, moves, NULL);
    }


    thread_unsure_flag = false;
    search_success_flag = false;
    for (auto item : moves)
    {
        VCTData *data = new VCTData(this, board, item.pos, depth);
        ThreadPool::getInstance()->run(bind(doVCTSearchForEachThread, data));
    }
    ThreadPool::getInstance()->wait();

    if (search_success_flag)
    {
        optimalPath.cat(resultPath);
        optimalPath.rating = side == startStep.getState() ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
        search_success_flag = false;
        return VCXRESULT_TRUE;
    }

    if (thread_unsure_flag)
    {
        return VCXRESULT_UNSURE;
    }
    else
    {
        return VCXRESULT_FALSE;
    }
}

VCXRESULT GoSearchEngine::doVCTSearch(ChessBoard* board, int depth, OptimalPath& optimalPath, set<Position>* reletedset, bool useTransTable)
{
    uint8_t side = board->getLastStep().getOtherSide();
    uint8_t laststep = board->getLastStep().step;
    Position lastindex = board->getLastStep().pos;
    OptimalPath VCFPath(board->getLastStep().step);
    vector<StepCandidateItem> moves;
    if (board->getHighestInfo(side).chesstype == CHESSTYPE_5)
    {
        optimalPath.push(board->getHighestInfo(side).pos);
        optimalPath.rating = side == startStep.getState() ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
        return VCXRESULT_TRUE;
    }
    else if (board->getHighestInfo(Util::otherside(side)).chesstype == CHESSTYPE_5)
    {
        if (board->getChessType(board->getHighestInfo(Util::otherside(side)).pos, side) == CHESSTYPE_BAN)
        {
            return VCXRESULT_FALSE;
        }
        moves.emplace_back(board->getHighestInfo(Util::otherside(side)).pos, 10);
    }
    else if (search_success_flag || global_isOverTime || duration_cast<milliseconds>(std::chrono::system_clock::now() - startSearchTime).count() > maxStepTimeMs)
    {
        global_isOverTime = true;
        return VCXRESULT_UNSURE;
    }
    else if (doVCFSearch(board, depth + VCFExpandDepth - VCTExpandDepth, VCFPath, NULL, useTransTable) == VCXRESULT_TRUE)
    {
        optimalPath.cat(VCFPath);
        optimalPath.rating = VCFPath.rating;
        return VCXRESULT_TRUE;
    }
    else if (depth <= 0)
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
        tempboard.move(item.pos);

        if (tempboard.getHighestInfo(side).chesstype != CHESSTYPE_5
            && (!Util::isfourkill(tempboard.getHighestInfo(side).chesstype)))//无法造成VCT威胁
        {
            continue;
        }

        OptimalPath tempPath(board->getLastStep().step);
        tempPath.push(item.pos);

        vector<StepCandidateItem> defendmoves;

        if (tempboard.getHighestInfo(side).chesstype == CHESSTYPE_5)//冲四
        {
            if (tempboard.getHighestInfo(Util::otherside(side)).chesstype == CHESSTYPE_5)//失败，对方有5连
            {
                continue;
            }
            if (tempboard.getChessType(tempboard.getHighestInfo(side).pos, Util::otherside(side)) == CHESSTYPE_BAN)//敌方触发禁手，VCF成功
            {
                optimalPath.rating = side == startStep.getState() ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
                optimalPath.cat(tempPath);
                return VCXRESULT_TRUE;
            }
            defendmoves.emplace_back(tempboard.getHighestInfo(side).pos, 10);
        }
        else //活三
        {
            OptimalPath tempPath2(tempboard.getLastStep().step);
            tempresult = doVCFSearch(&tempboard, depth + VCFExpandDepth - VCTExpandDepth - 1, tempPath2, NULL, useTransTable);
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

            getFourkillDefendSteps(&tempboard, tempboard.getHighestInfo(side).pos, defendmoves);
        }


        bool flag = true;
        OptimalPath tempPath2(tempboard.lastStep.step);
        for (auto defend : defendmoves)
        {
            tempPath2.endStep = tempPath.endStep;
            tempPath2.path.clear();
            ChessBoard tempboard2 = tempboard;
            tempboard2.move(defend.pos);

            tempPath2.push(defend.pos);

            set<Position> atackset;
            /*if (reletedset != NULL)
            {
                set<uint8_t> tempatackset;
                tempboard2.getAtackReletedPos(tempatackset, item.index, side);
                util::myset_intersection(reletedset, &tempatackset, &atackset);
            }
            else*/
            {
                tempboard2.getAtackReletedPos(atackset, item.pos, side);
            }

            tempresult = doVCTSearchWrapper(&tempboard2, depth - 2, tempPath2, &atackset, useTransTable);

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
            Position struggleindex = item.pos;
            if (doVCTStruggleSearch(&tempboard, depth - 1, struggleindex, useTransTable))
            {
                continue;
            }

            tempPath.cat(tempPath2);
            optimalPath.cat(tempPath);
            optimalPath.rating = side == startStep.getState() ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
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

bool GoSearchEngine::doVCTStruggleSearch(ChessBoard* board, int depth, Position &nextstep, bool useTransTable)
{
    uint8_t side = board->lastStep.getOtherSide();
    uint8_t laststep = board->lastStep.step;
    vector<StepCandidateItem> moves;
    set<Position> atackset;
    board->getAtackReletedPos(atackset, nextstep, board->lastStep.getState());
    getVCFAtackSteps(board, moves, &atackset);
    if (board->lastStep.chessType < CHESSTYPE_D4)//特殊处理，可能引入新bug
    {
        for (auto index : atackset)
        {
            if (Util::isalive3(board->getChessType(index, side)))
            {
                moves.emplace_back(index, 8);
            }
        }
    }
    for (auto move : moves)
    {
        ChessBoard tempboard = *board;
        tempboard.move(move.pos);
        OptimalPath tempPath(tempboard.lastStep.step);
        tempPath.push(move.pos);
        uint8_t result;
        if (Util::hasdead4(board->getChessType(move.pos, side)))
        {
            result = doVCTSearchWrapper(&tempboard, depth + 1, tempPath, &atackset, useTransTable);
        }
        else
        {
            result = doVCTSearchWrapper(&tempboard, depth - 1, tempPath, &atackset, useTransTable);
        }

        if (result != VCXRESULT_TRUE)
        {
            nextstep = move.pos;
            return true;
        }
    }
    return false;
}


void GoSearchEngine::getVCFAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, set<Position>* reletedset)
{
    uint8_t side = Util::otherside(board->getLastStep().getState());

    set<Position>* range;
    if (reletedset == NULL)
    {
        range = &Util::board_range;
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
            double atack = board->getRelatedFactor(index, side), defend = board->getRelatedFactor(index, Util::otherside(side), true);
            moves.emplace_back(index, (int)((atack + defend) * 10));
        }
    }

    std::sort(moves.begin(), moves.end(), CandidateItemCmp);
}

void GoSearchEngine::getVCTAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, set<Position>* reletedset)
{
    uint8_t side = Util::otherside(board->getLastStep().getState());

    set<Position>* range;
    if (reletedset == NULL)
    {
        range = &Util::board_range;
    }
    else
    {
        range = reletedset;
    }

    //vector<StepCandidateItem> VCFmoves;//冲四全部搜索，不裁剪，VCT的才裁剪
    for (auto pos : *range)
    {
        if (!board->canMove(pos))
        {
            continue;
        }

        if (board->getChessType(pos, side) == CHESSTYPE_4)
        {
            moves.emplace_back(pos, 1000);
            continue;
        }
        else if (board->getChessType(pos, side) == CHESSTYPE_44)
        {
            moves.emplace_back(pos, 800);
            continue;
        }
        else if (board->getChessType(pos, side) == CHESSTYPE_43)
        {
            moves.emplace_back(pos, 500);
            continue;
        }
        else if (board->getChessType(pos, side) == CHESSTYPE_33)
        {
            moves.emplace_back(pos, 400);
            continue;
        }

        if (Util::isalive3(board->getChessType(pos, side)))
        {
            moves.emplace_back(pos, (int)(board->getRelatedFactor(pos, side) * 10));
        }
        else if (Util::isdead4(board->getChessType(pos, side)))
        {
            moves.emplace_back(pos, (int)(board->getRelatedFactor(pos, side) * 10));

            ChessBoard tempboard;
            for (uint8_t n = 0; n < DIRECTION8::DIRECTION8_COUNT; ++n)
            {
                if (Util::isdead4(board->pieces_layer2[pos.row][pos.col][n / 2][side]))
                {
                    continue;
                }
                Position temppos = pos;
                int blankCount = 0, chessCount = 0;
                while (temppos.displace8(1, n)) //如果不超出边界
                {

                    if (board->getState(temppos.row, temppos.col) == PIECE_BLANK)
                    {
                        blankCount++;
                        if (Util::isalive3(board->getChessType(temppos, side)))
                        {
                            break;
                        }
                        tempboard = *board;
                        tempboard.move(temppos.row, temppos.col);
                        if (Util::isfourkill(tempboard.getChessType(pos, side)))
                        {
                            moves.emplace_back(temppos, (int)(board->getRelatedFactor(temppos, side) * 10));
                        }
                    }
                    else if (board->getState(temppos) == Util::otherside(side))
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
        else if (board->getChessType(pos, side) == CHESSTYPE_D3)
        {
            uint8_t direction = board->getChessDirection(pos, side);
            //特殊棋型，在同一条线上的双四
            //o?o?!?o  || o?o!??o || oo?!??oo
            Position pos(pos);
            Position temppos1 = pos.getNextPosition(direction, 1), temppos2 = pos.getNextPosition(direction, -1);
            if (!temppos1.valid() || !temppos2.valid())
            {
                continue;
            }

            if (board->getState(temppos1) == PIECE_BLANK && board->getState(temppos2) == PIECE_BLANK)
            {
                //o?o?!?o || oo?!??oo
                temppos1 = pos.getNextPosition(direction, 2), temppos2 = pos.getNextPosition(direction, -2);
                if (!temppos1.valid() || !temppos2.valid())
                {
                    continue;
                }
                if (board->getState(temppos1) == side && board->getState(temppos2) == side)
                {
                    //o?!?o?o
                    temppos1 = pos.getNextPosition(direction, 3), temppos2 = pos.getNextPosition(direction, 4);
                    if (temppos1.valid() && temppos2.valid())
                    {
                        if (board->getState(temppos1) == PIECE_BLANK && board->getState(temppos2) == side)
                        {
                            moves.emplace_back(pos, (int)(board->getRelatedFactor(pos, side) * 10));
                            continue;
                        }
                    }
                    //o?o?!?o
                    temppos1 = pos.getNextPosition(direction, -3), temppos2 = pos.getNextPosition(direction, -4);
                    if (temppos1.valid() && temppos2.valid())
                    {
                        if (board->getState(temppos1) == PIECE_BLANK && board->getState(temppos2) == side)
                        {
                            moves.emplace_back(pos, (int)(board->getRelatedFactor(pos, side) * 10));
                            continue;
                        }
                    }
                }
                else
                {
                    //  oo?!??oo
                    // oo??!?oo

                    if (board->getState(temppos1) == Util::otherside(side) || board->getState(temppos2) == Util::otherside(side))
                    {
                        continue;
                    }
                    if (board->getState(temppos1) != PIECE_BLANK && board->getState(temppos2) != PIECE_BLANK)
                    {
                        continue;
                    }

                    temppos1 = pos.getNextPosition(direction, 3), temppos2 = pos.getNextPosition(direction, -3);
                    if (!temppos1.valid() || !temppos2.valid())
                    {
                        continue;
                    }
                    if (board->getState(temppos1) == side && board->getState(temppos2) == side)
                    {
                        temppos1 = pos.getNextPosition(direction, 4), temppos2 = pos.getNextPosition(direction, -4);
                        if (temppos1.valid() && board->getState(temppos1) == side)
                        {
                            moves.emplace_back(pos, (int)(board->getRelatedFactor(pos, side) * 10));
                            continue;
                        }
                        if (temppos2.valid() && board->getState(temppos2) == side)
                        {
                            moves.emplace_back(pos, (int)(board->getRelatedFactor(pos, side) * 10));
                            continue;
                        }
                    }
                }
            }
            else
            {
                //o?o!??o 
                if (board->getState(temppos1) == Util::otherside(side) || board->getState(temppos2) == Util::otherside(side))
                {
                    continue;
                }
                if (board->getState(temppos1) != PIECE_BLANK && board->getState(temppos2) != PIECE_BLANK)
                {
                    continue;
                }
                temppos1 = pos.getNextPosition(direction, 2), temppos2 = pos.getNextPosition(direction, -2);
                if (!temppos1.valid() || !temppos2.valid())
                {
                    continue;
                }
                if (board->getState(temppos1) == PIECE_BLANK && board->getState(temppos2) == PIECE_BLANK)
                {
                    temppos1 = pos.getNextPosition(direction, 3), temppos2 = pos.getNextPosition(direction, -3);
                    if (!temppos1.valid() || !temppos2.valid())
                    {
                        continue;
                    }
                    if (board->getState(temppos1) == side && board->getState(temppos2) == side)
                    {
                        moves.emplace_back(pos, (int)(board->getRelatedFactor(pos, side) * 10));
                        continue;
                    }
                }
            }
        }
    }

    std::sort(moves.begin(), moves.end(), CandidateItemCmp);
    //此处剪裁会造成棋力大幅下降
    //if (moves.size() > 8)
    //{
    //    moves.erase(moves.begin() + 8, moves.end());//如果此处只保留10个，会导致搜索不全，禁手胜利会感知不到，解决方案：冲四全部搜索（VCFmoves），不裁剪，VCT的才裁剪
    //}
    //moves.insert(moves.end(), VCFmoves.begin(), VCFmoves.end());
    //std::sort(moves.begin(), moves.end(), CandidateItemCmp);
}