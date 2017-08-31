#include "GoSearch.h"
#include "ThreadPool.h"
#include <cassert>

#define GOSEARCH_DEBUG
mutex GoSearchEngine::message_queue_lock;
queue<string> GoSearchEngine::message_queue;

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
    transTableStat = { 0,0,0 };
    this->board = board;
    this->startStep = board->lastStep;
    queue<string> initqueue;
    message_queue.swap(initqueue);
}

void GoSearchEngine::applySettings(
    uint32_t max_searchtime_ms,
    uint32_t rest_match_time_ms,
    uint32_t max_memory_bytes,
    int min_depth,
    int max_depth,
    int vcf_expand,
    int vct_expand,
    bool enable_debug,
    bool use_transtable,
    bool full_search,
    bool use_multithread
)
{
    maxStepTimeMs = max_searchtime_ms;
    restMatchTimeMs = rest_match_time_ms;
    maxMemoryBytes = max_memory_bytes;
    transTable.setMaxMemory(max_memory_bytes);
    enableDebug = enable_debug;
    maxAlphaBetaDepth = max_depth;
    minAlphaBetaDepth = min_depth;
    VCFExpandDepth = vcf_expand;
    VCTExpandDepth = vct_expand;
    useTransTable = use_transtable;
    fullSearch = full_search;
    useMultiThread = use_multithread;
}

bool GoSearchEngine::getDebugMessage(string &debugstr)
{
    message_queue_lock.lock();
    if (message_queue.empty())
    {
        message_queue_lock.unlock();
        return false;
    }
    debugstr = message_queue.front();
    message_queue.pop();
    message_queue_lock.unlock();
    return true;
}

void GoSearchEngine::sendMessage(string &debugstr)
{
    message_queue_lock.lock();
    message_queue.push(debugstr);
    message_queue_lock.unlock();
}

void GoSearchEngine::textOutIterativeInfo(OptimalPath& optimalPath)
{
    stringstream s;
    if (optimalPath.path.size() == 0)
    {
        sendMessage(string("no path"));
        return;
    }
    Position nextpos = optimalPath.path[0];
    s << "depth:" << currentAlphaBetaDepth << "-" << (int)(optimalPath.endStep - startStep.step) << ", ";
    s << "time:" << duration_cast<milliseconds>(system_clock::now() - startSearchTime).count() << "ms, ";
    s << "rating:" << optimalPath.rating << ", next:" << (int)nextpos.row << "," << (int)nextpos.col;
    sendMessage(s.str());
}

void GoSearchEngine::textOutResult(OptimalPath& optimalPath, uint32_t suggest_time)
{
    //optimalPath可能为空
    stringstream s;
    s << "rating:" << optimalPath.rating << " depth:" << currentAlphaBetaDepth << "-" << (int)(optimalPath.endStep - startStep.step) << " bestpath:";
    for (auto pos : optimalPath.path)
    {
        s << "(" << (int)pos.row << "," << (int)pos.col << ") ";
    }
    sendMessage(s.str());
    s.str("");
    s << "table:" << transTable.getTransTableSize() << " stable:" << transTable.getTransTableVCXSize() << "\r\n";
    sendMessage(s.str());
    s.str("");
    s << "hit:" << transTableStat.hit << " miss:" << transTableStat.miss << " clash:" << transTableStat.clash << " cover:" << transTableStat.cover;
    sendMessage(s.str());
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
    sendMessage(s.str());
}

void GoSearchEngine::textOutAllocateTime(uint32_t max_time, uint32_t suggest_time)
{
    stringstream s;
    s << "maxtime:" << maxStepTimeMs << "ms suggest:" << suggest_time << "ms";
    sendMessage(s.str());
}

void GoSearchEngine::allocatedTime(uint32_t& max_time, uint32_t&suggest_time)
{
    int step = startStep.step;
    if (step < 4)
    {
        max_time = restMatchTimeMs / 20 < maxStepTimeMs ? restMatchTimeMs / 20 : maxStepTimeMs;
        suggest_time = max_time / 3 * 2;
    }
    else if (step < 60)
    {
        if (restMatchTimeMs < maxStepTimeMs)
        {
            max_time = restMatchTimeMs / ((61 - step) / 2) * 2;
            suggest_time = restMatchTimeMs / ((61 - step) / 2);
        }
        else if (restMatchTimeMs / ((61 - step) / 2) < maxStepTimeMs / 3)
        {
            max_time = restMatchTimeMs / ((61 - step) / 2) * 2;
            suggest_time = restMatchTimeMs / ((61 - step) / 2);
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
            suggest_time = restMatchTimeMs / 5;
        }
        else if (restMatchTimeMs / 10 < maxStepTimeMs / 5)
        {
            max_time = maxStepTimeMs / 3;
            suggest_time = (restMatchTimeMs / 10);
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

    uint32_t max_time, suggest_time;
    allocatedTime(max_time, suggest_time);
    maxStepTimeMs = max_time;
    textOutAllocateTime(max_time, suggest_time);
    if (board->getLastStep().step < 4)//前5步增加alphabeta的步数，减少VCT步数，利于抢占局面
    {
        /*maxVCTDepth -= 2;
        maxVCFDepth -= 4;*/
        VCTExpandDepth = 0;
    }

    OptimalPath bestPath(0);
    currentAlphaBetaDepth = minAlphaBetaDepth;
    StepCandidateItem bestStep(Position(-1, -1), 0);
    while (true)
    {
        if (duration_cast<milliseconds>(std::chrono::system_clock::now() - this->startSearchTime).count() > suggest_time)
        {
            currentAlphaBetaDepth -= 1;
            break;
        }

        OptimalPath temp = solveBoard(board, bestStep);
        textOutIterativeInfo(temp);
        if (currentAlphaBetaDepth > minAlphaBetaDepth && global_isOverTime)
        {
            currentAlphaBetaDepth -= 1;
            break;
        }
        bestPath = temp;
        if (temp.rating >= CHESSTYPE_5_SCORE || temp.rating <= -CHESSTYPE_5_SCORE)
        {
            //break;
        }

        //已成定局的不需要继续搜索了
        if (bestStep.priority == 10000)
        {
            break;
        }

        if (currentAlphaBetaDepth >= maxAlphaBetaDepth)
        {
            break;
        }
        else
        {
            currentAlphaBetaDepth += 1;
        }
    }
    textOutResult(bestPath, suggest_time);

    if (bestPath.path.empty())
    {
        bestPath.path.push_back(board->getHighestInfo(startStep.getState()).pos);
    }

    return bestPath.path[0];
}

static int base_alpha = INT_MIN, base_beta = INT_MAX;
static OptimalPath optimalPath(0);

void GoSearchEngine::solveBoardForEachThread(PVSearchData data)
{
    OptimalPath tempPath(data.engine->board->getLastStep().step);
    tempPath.push(data.it->pos);



    bool deadfour = false;
    if (data.struggle && Util::hasdead4(data.engine->board->getChessType(data.it->pos, data.engine->board->getLastStep().getOtherSide())))
    {
        deadfour = true;
    }

    ChessBoard currentBoard = *(data.engine->board);
    currentBoard.move(data.it->pos);


    data.engine->doAlphaBetaSearch(&currentBoard, deadfour&&data.struggle ? data.engine->currentAlphaBetaDepth + 1 : data.engine->currentAlphaBetaDepth - 1,
        base_alpha, base_alpha + 1, tempPath, data.engine->useTransTable);
    if (tempPath.rating > base_alpha && tempPath.rating < base_beta)//使用完整窗口
    {
        tempPath.path.clear();
        tempPath.push(data.it->pos);
        data.engine->doAlphaBetaSearch(&currentBoard, deadfour&&data.struggle ? data.engine->currentAlphaBetaDepth + 1 : data.engine->currentAlphaBetaDepth - 1,
            base_alpha, base_beta, tempPath, data.engine->useTransTable);
    }

    if (data.engine->global_isOverTime)
    {
        if (optimalPath.rating == INT_MIN)
        {
            optimalPath = tempPath;
        }
        return;
    }

    if (data.engine->enableDebug)
    {
        data.engine->textForTest(tempPath, data.it->priority);
    }

    data.it->priority = tempPath.rating;

    if (tempPath.rating > base_alpha)
    {
        base_alpha = tempPath.rating;
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

OptimalPath GoSearchEngine::solveBoard(ChessBoard* board, StepCandidateItem& bestStep)
{
    base_alpha = INT_MIN, base_beta = INT_MAX;
    optimalPath.startStep = startStep.step;
    optimalPath.endStep = startStep.step;
    optimalPath.path.clear();
    optimalPath.rating = INT_MIN;
    size_t firstSearchUpper = 0;

    uint8_t side = board->getLastStep().getOtherSide();

    PieceInfo otherhighest = board->getHighestInfo(Util::otherside(side));
    PieceInfo selfhighest = board->getHighestInfo(side);
    OptimalPath VCFPath(board->getLastStep().step);
    OptimalPath VCTPath(board->getLastStep().step);

    vector<StepCandidateItem> solveList;
    if (selfhighest.chesstype == CHESSTYPE_5)
    {
        optimalPath.push(selfhighest.pos);
        optimalPath.rating = 10000;
        bestStep = StepCandidateItem(selfhighest.pos, 10000);
        return optimalPath;
    }
    else if (otherhighest.chesstype == CHESSTYPE_5)//敌方马上5连
    {
        if (board->getChessType(otherhighest.pos, side) == CHESSTYPE_BAN)
        {
            optimalPath.rating = -10000;
        }
        else
        {
            optimalPath.rating = board->getGlobalEvaluate(getAISide());
        }
        optimalPath.push(otherhighest.pos);
        bestStep = StepCandidateItem(otherhighest.pos, 10000);
        return optimalPath;
    }
    else if (doVCFSearch(board, getVCFDepth(board->getLastStep().step), VCFPath, NULL, useTransTable) == VCXRESULT_TRUE)
    {
        bestStep = StepCandidateItem(VCFPath.path[0], 10000);
        return VCFPath;
    }
    else if (doVCTSearch(board, getVCTDepth(board->getLastStep().step), VCTPath, NULL, useTransTable) == VCXRESULT_TRUE)
    {
        bestStep = StepCandidateItem(VCTPath.path[0], 10000);
        return VCTPath;
    }
    else
    {
        if (Util::isfourkill(otherhighest.chesstype))//敌方有4杀
        {
            getFourkillDefendSteps(board, otherhighest.pos, solveList);
            firstSearchUpper = solveList.size();
            getVCFAtackSteps(board, solveList, NULL);
        }
        else if (otherhighest.chesstype == CHESSTYPE_33)
        {
            getFourkillDefendSteps(board, otherhighest.pos, solveList);
            firstSearchUpper = solveList.size();
            getVCTAtackSteps(board, solveList, NULL);
        }
        else
        {
            firstSearchUpper = getNormalSteps(board, solveList, NULL, fullSearch);
        }
    }

    if (solveList.empty())
    {
        optimalPath.rating = -10000;
        optimalPath.push(otherhighest.pos);
        bestStep = StepCandidateItem(otherhighest.pos, 10000);
        return optimalPath;
    }

    firstSearchUpper = firstSearchUpper == 0 ? solveList.size() : firstSearchUpper;
    bool foundPV = false;
    size_t index = 0;
    bool hasBest = false;
    //best Search
    if (bestStep.pos.valid() && bestStep.priority == 1)
    {
        hasBest = true;
        OptimalPath tempPath(board->getLastStep().step);
        tempPath.push(bestStep.pos);
        ChessBoard currentBoard = *board;
        currentBoard.move(bestStep.pos);
        doAlphaBetaSearch(&currentBoard, currentAlphaBetaDepth - 1, base_alpha, base_beta, tempPath, useTransTable);
        //处理超时
        if (global_isOverTime)
        {
            if (optimalPath.rating == INT_MIN)
            {
                optimalPath = tempPath;
            }
            bestStep = StepCandidateItem(optimalPath.path[0], 1);
            return optimalPath;
        }

        if (enableDebug)
        {
            textForTest(tempPath, bestStep.priority);
        }

        if (tempPath.rating > base_alpha)
        {
            base_alpha = tempPath.rating;
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

    //first search
    for (; index < firstSearchUpper; ++index)
    {
        if (hasBest && solveList[index].pos == bestStep.pos)
        {
            continue;
        }
        if (useMultiThread && foundPV)
        {
            PVSearchData data(this, solveList.begin() + index);
            ThreadPool::getInstance()->run(bind(solveBoardForEachThread, data));
        }
        else
        {

            OptimalPath tempPath(board->getLastStep().step);
            tempPath.push(solveList[index].pos);
            ChessBoard currentBoard = *board;
            currentBoard.move(solveList[index].pos);

            if (foundPV)
            {
                doAlphaBetaSearch(&currentBoard, currentAlphaBetaDepth - 1, base_alpha, base_alpha + 1, tempPath, useTransTable);//0窗口剪裁
                                                                                                                                 //假设当前是最好的，没有任何其他的会比当前的PV好（大于alpha）                                                                                   
                if (tempPath.rating > base_alpha && tempPath.rating < base_beta)//使用完整窗口
                {
                    tempPath.path.clear();
                    tempPath.push(solveList[index].pos);
                    doAlphaBetaSearch(&currentBoard, currentAlphaBetaDepth - 1, base_alpha, base_beta, tempPath, useTransTable);
                }

            }
            else
            {
                doAlphaBetaSearch(&currentBoard, currentAlphaBetaDepth - 1, base_alpha, base_beta, tempPath, useTransTable);
            }

            //处理超时
            if (global_isOverTime)
            {
                if (optimalPath.rating == INT_MIN)
                {
                    optimalPath = tempPath;
                }
                bestStep = StepCandidateItem(optimalPath.path[0], 1);
                return optimalPath;
            }

            if (enableDebug)
            {
                textForTest(tempPath, solveList[index].priority);
            }

            solveList[index].priority = tempPath.rating;

            if (tempPath.rating > base_alpha)
            {
                base_alpha = tempPath.rating;
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
    }
    if (useMultiThread)
    {
        ThreadPool::getInstance()->wait();
        if (global_isOverTime)
        {
            bestStep = StepCandidateItem(optimalPath.path[0], 1);
            return optimalPath;
        }
    }
    bool secondSearch = false;
    //second search
    if (firstSearchUpper < solveList.size() && optimalPath.rating == -CHESSTYPE_5_SCORE)
    {
        secondSearch = true;
        set<Position> reletedset;
        getNormalRelatedSet(board, reletedset, optimalPath);
        //二次搜索
        for (; index < solveList.size(); ++index)
        {
            if (reletedset.find(solveList[index].pos) == reletedset.end())//不在reletedset中
            {
                continue;
            }

            if (useMultiThread && foundPV)
            {
                PVSearchData data(this, solveList.begin() + index);
                data.struggle = true;
                ThreadPool::getInstance()->run(bind(solveBoardForEachThread, data));
                continue;
            }

            OptimalPath tempPath(board->getLastStep().step);
            tempPath.push(solveList[index].pos);
            ChessBoard currentBoard = *board;
            currentBoard.move(solveList[index].pos);

            bool deadfour = false;
            if (Util::hasdead4(board->getChessType(solveList[index].pos, side)))
            {
                deadfour = true;
            }

            if (foundPV)
            {
                doAlphaBetaSearch(&currentBoard, deadfour ? currentAlphaBetaDepth + 1 : currentAlphaBetaDepth - 1, base_alpha, base_alpha + 1, tempPath, useTransTable);//0窗口剪裁
                                                                                                                                    //假设当前是最好的，没有任何其他的会比当前的PV好（大于alpha）                                                                                   
                if (tempPath.rating > base_alpha && tempPath.rating < base_beta)//使用完整窗口
                {
                    tempPath.path.clear();
                    tempPath.push(solveList[index].pos);
                    doAlphaBetaSearch(&currentBoard, deadfour ? currentAlphaBetaDepth + 1 : currentAlphaBetaDepth - 1, base_alpha, base_beta, tempPath, useTransTable);
                }
            }
            else
            {
                doAlphaBetaSearch(&currentBoard, deadfour ? currentAlphaBetaDepth + 1 : currentAlphaBetaDepth - 1, base_alpha, base_beta, tempPath, useTransTable);
            }

            //处理超时
            if (global_isOverTime)
            {
                if (optimalPath.rating == INT_MIN)
                {
                    optimalPath = tempPath;
                }
                bestStep = StepCandidateItem(optimalPath.path[0], 2);
                return optimalPath;
            }

            if (enableDebug)
            {
                textForTest(tempPath, solveList[index].priority);
            }

            solveList[index].priority = tempPath.rating;

            if (tempPath.rating > base_alpha)
            {
                base_alpha = tempPath.rating;
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

        if (useMultiThread)
        {
            ThreadPool::getInstance()->wait();
            if (global_isOverTime)
            {
                bestStep = StepCandidateItem(optimalPath.path[0], 2);
                return optimalPath;
            }
        }
    }

    bestStep = StepCandidateItem(optimalPath.path[0], secondSearch ? 2 : 1);
    return optimalPath;
}

void GoSearchEngine::doAlphaBetaSearch(ChessBoard* board, int depth, int alpha, int beta, OptimalPath& optimalPath, bool useTransTable)
{
    //for multithread
    //if (alpha < base_alpha) alpha = base_alpha;
    //if (beta > base_beta) beta = base_beta;
    //

    uint8_t side = board->getLastStep().getOtherSide();
    uint8_t otherside = board->getLastStep().getState();
    Position lastindex = board->getLastStep().pos;
    int laststep = board->getLastStep().step;
    bool memoryValid = true;
    //USE TransTable
    bool continue_flag = false;
    bool has_best_pos = false;
    TransTableData data;
    if (useTransTable)
    {

#define TRANSTABLE_HIT_FUNC transTableStat.hit++;optimalPath.rating = data.value;optimalPath.push(data.bestStep);optimalPath.endStep = data.endStep;

        if (transTable.getTransTable(board->getBoardHash().z64key, data))
        {
            if (data.checkHash == board->getBoardHash().z32key)
            {
                if (data.value == -CHESSTYPE_5_SCORE || data.value == CHESSTYPE_5_SCORE)
                {
                    TRANSTABLE_HIT_FUNC
                        return;
                }
                else if (data.depth < depth)
                {
                    transTableStat.cover++;
                    has_best_pos = true;
                }
                else
                {
                    if (isPlayerSide(side))
                    {
                        if (data.value < alpha)
                        {
                            TRANSTABLE_HIT_FUNC
                                return;
                        }
                        else//data.value >= alpha
                        {
                            if (data.type == TRANSTYPE_EXACT)
                            {
                                TRANSTABLE_HIT_FUNC
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
                            TRANSTABLE_HIT_FUNC
                                return;
                        }
                        else
                        {
                            if (data.type == TRANSTYPE_EXACT)
                            {
                                TRANSTABLE_HIT_FUNC
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
            memoryValid = transTable.memoryValid();
            transTableStat.miss++;
        }
    }
    //end USE TransTable

    uint8_t move_index = 0;
    size_t firstSearchUpper = 0;
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
            optimalPath.rating = isPlayerSide(side) ? CHESSTYPE_5_SCORE : -CHESSTYPE_5_SCORE;
            return;
        }
        if (depth <= 0)
        {
            optimalPath.rating = board->getGlobalEvaluate(getAISide(), getAISide() == PIECE_BLACK ? 100 : 100);//存疑
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
            optimalPath.rating = board->getGlobalEvaluate(getAISide(), getAISide() == PIECE_BLACK ? 100 : 100);
            return;
        }
    }
    else if (doVCFSearch(board, getVCFDepth(board->getLastStep().step), VCFPath, NULL, useTransTable) == VCXRESULT_TRUE)//side赢了
    {
        bestPath = VCFPath;
        goto end;
    }
    else if (doVCTSearch(board, getVCTDepth(board->getLastStep().step), VCTPath, NULL, useTransTable) == VCXRESULT_TRUE)
    {
        bestPath = VCTPath;
        goto end;
    }
    else
    {
        if (Util::isfourkill(board->getHighestInfo(otherside).chesstype))//防4杀
        {
            getFourkillDefendSteps(board, board->getHighestInfo(otherside).pos, moves);
            firstSearchUpper = moves.size();
            getVCFAtackSteps(board, moves, NULL);
        }
        else if (board->getHighestInfo(otherside).chesstype == CHESSTYPE_33)
        {
            getFourkillDefendSteps(board, board->getHighestInfo(otherside).pos, moves);
            firstSearchUpper = moves.size();
            getVCTAtackSteps(board, moves, NULL);
        }
        else
        {
            firstSearchUpper = getNormalSteps(board, moves, NULL, fullSearch);
        }
    }
    if (firstSearchUpper == 0) firstSearchUpper = moves.size();

    bool foundPV = false;
    bool hasBest = false;
    data.type = TRANSTYPE_EXACT;

    if (continue_flag && data.continue_index < firstSearchUpper)
    {
        move_index = data.continue_index;
        bestPath.rating = data.value;
        bestPath.push(data.bestStep);
        bestPath.endStep = data.endStep;
        if (isPlayerSide(side))
        {
            if (data.value < beta)//update beta
            {
                beta = data.value;
                foundPV = true;
            }
        }
        else
        {
            if (data.value > alpha)//update alpha
            {
                alpha = data.value;
                foundPV = true;
            }
        }
    }

    //优先搜索置换表中记录的上一个迭代的最好着法
    //best search
    if (has_best_pos && data.continue_index <= firstSearchUpper)
    {
        hasBest = true;

        OptimalPath tempPath(board->getLastStep().step);
        tempPath.push(data.bestStep);
        ChessBoard currentBoard = *board;
        currentBoard.move(data.bestStep);

        //剪枝
        if (isPlayerSide(side))//build player
        {
            doAlphaBetaSearch(&currentBoard, depth - 1, alpha, beta, tempPath, useTransTable);

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
            if (tempPath.rating < beta)//update beta
            {
                beta = tempPath.rating;
                foundPV = true;
            }
            if (tempPath.rating < alpha)//alpha cut
            {
                data.type = TRANSTYPE_LOWER;
                goto end;
            }
        }
        else // build AI
        {

            doAlphaBetaSearch(&currentBoard, depth - 1, alpha, beta, tempPath, useTransTable);

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
            if (tempPath.rating > alpha)//update alpha
            {
                alpha = tempPath.rating;
                foundPV = true;
            }
            if (tempPath.rating > beta)//beta cut
            {
                data.type = TRANSTYPE_UPPER;
                goto end;
            }
        }
    }

    //first search
    for (; move_index < firstSearchUpper; ++move_index)
    {
        if (hasBest && data.bestStep == moves[move_index].pos)
        {
            continue;
        }
        OptimalPath tempPath(board->getLastStep().step);
        tempPath.push(moves[move_index].pos);
        ChessBoard currentBoard = *board;
        currentBoard.move(moves[move_index].pos);

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
                    tempPath.push(moves[move_index].pos);
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
            if (tempPath.rating < beta)//update beta
            {
                beta = tempPath.rating;
                foundPV = true;
            }
            if (tempPath.rating < alpha)//alpha cut
            {
                data.type = TRANSTYPE_LOWER;
                break;
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
                    tempPath.push(moves[move_index].pos);
                    doAlphaBetaSearch(&currentBoard, depth - 1, alpha, beta, tempPath, useTransTable);
                }
            }
            else
            {
                doAlphaBetaSearch(&currentBoard, depth - 1, alpha, beta, tempPath, useTransTable);
            }

            if (tempPath.rating > bestPath.rating)
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
            if (tempPath.rating > alpha)//update alpha
            {
                alpha = tempPath.rating;
                foundPV = true;
            }
            if (tempPath.rating > beta)//beta cut
            {
                data.type = TRANSTYPE_UPPER;
                break;
            }
        }
    }

    //二次搜索
    if (firstSearchUpper < moves.size()
        && ((isPlayerSide(side) && bestPath.rating == CHESSTYPE_5_SCORE) || (!isPlayerSide(side) && bestPath.rating == -CHESSTYPE_5_SCORE)))
    {
        set<Position> reletedset;
        getNormalRelatedSet(board, reletedset, bestPath);
        for (; move_index < moves.size(); ++move_index)
        {
            if (reletedset.find(moves[move_index].pos) == reletedset.end())//不在reletedset中
            {
                continue;
            }

            OptimalPath tempPath(board->getLastStep().step);
            tempPath.push(moves[move_index].pos);
            ChessBoard currentBoard = *board;
            currentBoard.move(moves[move_index].pos);
            bool deadfour = false;
            if (Util::hasdead4(board->getChessType(moves[move_index].pos, side)))
            {
                deadfour = true;
            }

            //剪枝
            if (isPlayerSide(side))//build player
            {
                if (foundPV)
                {
                    doAlphaBetaSearch(&currentBoard, deadfour ? depth + 1 : depth - 1, beta - 1, beta, tempPath, useTransTable);//0窗口剪裁
                                                                                                         //假设当前是最好的，没有任何其他的会比当前的PV好（小于beta）
                    if (tempPath.rating < beta && tempPath.rating > alpha)//失败，使用完整窗口
                    {
                        tempPath.path.clear();
                        tempPath.push(moves[move_index].pos);
                        doAlphaBetaSearch(&currentBoard, deadfour ? depth + 1 : depth - 1, alpha, beta, tempPath, useTransTable);
                    }
                }
                else
                {
                    doAlphaBetaSearch(&currentBoard, deadfour ? depth + 1 : depth - 1, alpha, beta, tempPath, useTransTable);
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
                if (tempPath.rating < beta)//update beta
                {
                    beta = tempPath.rating;
                    foundPV = true;
                }
                if (tempPath.rating < alpha)//alpha cut
                {
                    data.type = TRANSTYPE_LOWER;
                    move_index = (uint8_t)moves.size();
                    break;
                }
            }
            else // build AI
            {
                if (foundPV)
                {
                    doAlphaBetaSearch(&currentBoard, deadfour ? depth + 1 : depth - 1, alpha, alpha + 1, tempPath, useTransTable);//0窗口剪裁
                    //假设当前是最好的，没有任何其他的会比当前的PV好（大于alpha）
                    if (tempPath.rating > alpha && tempPath.rating < beta)
                    {
                        tempPath.path.clear();
                        tempPath.push(moves[move_index].pos);
                        doAlphaBetaSearch(&currentBoard, deadfour ? depth + 1 : depth - 1, alpha, beta, tempPath, useTransTable);
                    }
                }
                else
                {
                    doAlphaBetaSearch(&currentBoard, deadfour ? depth + 1 : depth - 1, alpha, beta, tempPath, useTransTable);
                }

                if (tempPath.rating > bestPath.rating)
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
                if (tempPath.rating > alpha)//update alpha
                {
                    alpha = tempPath.rating;
                    foundPV = true;
                }
                if (tempPath.rating > beta)//beta cut
                {
                    data.type = TRANSTYPE_UPPER;
                    move_index = (uint8_t)moves.size();
                    break;
                }
            }
        }
    }

end:
    optimalPath.cat(bestPath);
    optimalPath.rating = bestPath.rating;

    //USE TransTable
    //写入置换表
    if (useTransTable && memoryValid)
    {
        data.checkHash = board->getBoardHash().z32key;
        data.endStep = optimalPath.endStep;
        data.value = optimalPath.rating;
        data.depth = depth;
        data.continue_index = move_index;
        assert(!bestPath.path.empty());
        data.bestStep = bestPath.path[0];
        transTable.putTransTable(board->getBoardHash().z64key, data);
    }
    //end USE TransTable
}

void GoSearchEngine::getNormalRelatedSet(ChessBoard* board, set<Position>& reletedset, OptimalPath& optimalPath)
{
    //uint8_t defendside = board->getLastStep().getOtherSide();
    uint8_t atackside = board->getLastStep().getState();//laststep的进攻成功，现在要找防守步

    vector<Position> path;
    path.push_back(board->getLastStep().pos);
    path.insert(path.end(), optimalPath.path.begin(), optimalPath.path.end());

    ChessBoard tempboard = *board;
    for (size_t i = 0; i < path.size() && i < 10; i++)
    {
        tempboard.move(path[i]);
        reletedset.insert(path[i]);
        i++;
        if (i < path.size())
        {
            tempboard.move(path[i]);
            tempboard.getAtackReletedPos(reletedset, path[i - 1], atackside);//相关点是对于进攻而言的，防守策略根据进攻的相关点去防守
            reletedset.insert(path[i]);
        }
    }
}

size_t GoSearchEngine::getNormalSteps(ChessBoard* board, vector<StepCandidateItem>& moves, set<Position>* reletedset, bool full_search)
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

        int atack = board->getRelatedFactor(pos, side), defend = board->getRelatedFactor(pos, Util::otherside(side), true);

        if (!full_search && atack < 10 && defend == 0)
        {
            continue;
        }
        if (!full_search && selfp == CHESSTYPE_D4 && atack < 20 && defend < 5)//会导致禁手陷阱无法触发，因为禁手陷阱一般都是始于“无意义”的冲四
        {
            moves.emplace_back(pos, 0);
        }
        moves.emplace_back(pos, atack + defend);
    }

    std::sort(moves.begin(), moves.end(), CandidateItemCmp);

    if (moves.size() > MAX_CHILD_NUM && !full_search)
    {
        for (auto i = 0; i < moves.size(); ++i)
        {
            if (moves[i].priority < moves[0].priority / 3)
            {
                //moves.erase(moves.begin() + i, moves.end());
                return i;
            }
        }
    }

    return moves.size();
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

    if (atackType == CHESSTYPE_5)
    {
        return;
    }
    else if (atackType == CHESSTYPE_4)//两个进攻点__ooo__，两个防点/一个进攻点x_ooo__（有一边被堵），三个防点
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
                if ((data.VCFDepth == VCXRESULT_UNSURE /*|| data.VCFDepth == VCXRESULT_FALSE*/) && data.VCFDepth < depth)//需要更新
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
        if (!transTable.memoryVCXValid())
        {
            return doVCFSearch(board, depth, optimalPath, reletedset, useTransTable);
        }
    }
    VCXRESULT flag = doVCFSearch(board, depth, optimalPath, reletedset, useTransTable);

    if (depth > 0 /*&& (reletedset == NULL || flag== VCXRESULT_TRUE)*/)
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
            if (data.VCFflag == VCXRESULT_TRUE)
            {
                optimalPath.endStep = data.VCFEndStep;
                return data.VCFflag;
            }
            else if (data.VCTflag == VCXRESULT_NOSEARCH)//还未搜索
            {
                transTableStat.miss++;
            }
            else
            {
                if ((data.VCTflag == VCXRESULT_UNSURE /*|| data.VCTflag == VCXRESULT_FALSE*/) && data.VCTDepth < depth)//需要更新
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
        if (!transTable.memoryVCXValid())
        {
            return doVCTSearch(board, depth, optimalPath, reletedset, useTransTable);
        }
    }
    VCXRESULT flag = doVCTSearch(board, depth, optimalPath, reletedset, useTransTable);
    if (depth > 0 /*&& (reletedset == NULL || flag == VCXRESULT_TRUE)*/)
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

VCXRESULT GoSearchEngine::doVCTSearch(ChessBoard* board, int depth, OptimalPath& optimalPath, set<Position>* reletedset, bool useTransTable)
{
    bool defendfive = false;
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
        defendfive = true;
        moves.emplace_back(board->getHighestInfo(Util::otherside(side)).pos, 10);
    }
    else if (global_isOverTime || duration_cast<milliseconds>(std::chrono::system_clock::now() - startSearchTime).count() > maxStepTimeMs)
    {
        global_isOverTime = true;
        return VCXRESULT_UNSURE;
    }
    else if (doVCFSearch(board, depth + getVCFDepth(0) - getVCTDepth(0), VCFPath, NULL, useTransTable) == VCXRESULT_TRUE)
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

        if (tempboard.getHighestInfo(side).chesstype < CHESSTYPE_33)//无法造成VCT威胁 & //防假活三，连环禁手
        {
            continue;
        }

        if (tempboard.getHighestInfo(Util::otherside(side)).chesstype == CHESSTYPE_5)//失败，对方有5连
        {
            continue;
        }

        OptimalPath tempPath(board->getLastStep().step);
        tempPath.push(item.pos);

        if (tempboard.getHighestInfo(side).chesstype == CHESSTYPE_5)
        {
            if (tempboard.getChessType(tempboard.getHighestInfo(side).pos, Util::otherside(side)) == CHESSTYPE_BAN)//敌方触发禁手，VCF成功
            {
                optimalPath.rating = side == startStep.getState() ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
                optimalPath.cat(tempPath);
                return VCXRESULT_TRUE;
            }
        }
        else
        {
            OptimalPath tempPath2(tempboard.getLastStep().step);
            tempresult = doVCFSearch(&tempboard, depth + getVCFDepth(0) - getVCTDepth(0) - 1, tempPath2, NULL, useTransTable);
            if (tempresult == VCXRESULT_TRUE)
            {
                continue;
            }
            if (tempresult == VCXRESULT_UNSURE)
            {
                unsure_flag = true;
            }
        }

        vector<StepCandidateItem> defendmoves;
        getFourkillDefendSteps(&tempboard, tempboard.getHighestInfo(side).pos, defendmoves);


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

            tempresult = doVCTSearchWrapper(&tempboard2, depth - 2, tempPath2, defendfive ? reletedset : &atackset, useTransTable);

            if (tempresult == VCXRESULT_UNSURE)
            {
                unsure_flag = true;
            }
            if (tempresult != VCXRESULT_TRUE)
            {
                flag = false;
                break;
            }
            /*for (auto m : tempPath2.path)
            {
                struggleset.insert(m);
            }*/
        }
        if (flag)
        {
            //start struggle
            set<Position> struggleset;
            //getNormalRelatedSet(&tempboard, reletedset, tempPath2);
            tempboard.getAtackReletedPos(struggleset, item.pos, side);
            if (doVCTStruggleSearch(&tempboard, depth - 1, defendfive ? *reletedset : struggleset, useTransTable))
            {
                continue;
            }
            //end struggle

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

bool GoSearchEngine::doVCTStruggleSearch(ChessBoard* board, int depth, set<Position>& struggleset, bool useTransTable)
{
    if (depth < 6)
    {
        return false;
    }
    uint8_t side = board->lastStep.getOtherSide();
    uint8_t laststep = board->lastStep.step;
    vector<StepCandidateItem> moves;
    getVCFAtackSteps(board, moves, &struggleset);
    //if (board->lastStep.chessType < CHESSTYPE_J3)//特殊处理，可能引入新bug
    //{
    //    for (auto index : atackset)
    //    {
    //        if (Util::isalive3(board->getChessType(index, side)))
    //        {
    //            moves.emplace_back(index, 8);
    //        }
    //    }
    //}
    for (auto move : moves)
    {
        ChessBoard tempboard = *board;
        tempboard.move(move.pos);
        OptimalPath tempPath(tempboard.lastStep.step);
        tempPath.push(move.pos);

        if (tempboard.getHighestInfo(Util::otherside(side)).chesstype == CHESSTYPE_5)
        {
            continue;
        }

        uint8_t result = VCXRESULT_FALSE;
        if (tempboard.getHighestInfo(side).chesstype == CHESSTYPE_5)
        {
            result = doVCTSearchWrapper(&tempboard, depth + 1, tempPath, &struggleset, useTransTable);
        }
        //else if (tempboard.getHighestInfo(side).chesstype > CHESSTYPE_D4P)
        //{
        //    result = doVCTSearchWrapper(&tempboard, depth - 1, tempPath, &atackset, useTransTable);
        //}

        if (result != VCXRESULT_TRUE)
        {
            return true;
        }
    }
    return false;
}


void GoSearchEngine::getVCFAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, set<Position>* reletedset)
{
    uint8_t side = Util::otherside(board->getLastStep().getState());
    size_t begin_index = moves.size();
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
            int atack = board->getRelatedFactor(index, side), defend = board->getRelatedFactor(index, Util::otherside(side), true);
            moves.emplace_back(index, atack + defend);
        }
    }

    std::sort(moves.begin() + begin_index, moves.end(), CandidateItemCmp);
}

void GoSearchEngine::getVCTAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, set<Position>* reletedset)
{
    uint8_t side = Util::otherside(board->getLastStep().getState());
    size_t begin_index = moves.size();
    set<Position>* range;
    if (reletedset == NULL)
    {
        range = &Util::board_range;
    }
    else
    {
        range = reletedset;
    }

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
        //#define EXTRA_VCT_CHESSTYPE
        if (Util::isalive3(board->getChessType(pos, side)))
        {
            moves.emplace_back(pos, board->getRelatedFactor(pos, side));

#ifdef EXTRA_VCT_CHESSTYPE
            for (uint8_t n = 0; n < DIRECTION8::DIRECTION8_COUNT; ++n)
            {
                if (Util::isalive3(board->pieces_layer2[pos.row][pos.col][n / 2][side]))
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
                        if ((board->getChessType(temppos, side) == CHESSTYPE_D3))
                        {
                            ChessBoard tempboard = *board;
                            tempboard.move(temppos.row, temppos.col);
                            if (Util::isfourkill(tempboard.getChessType(pos, side)))
                            {
                                moves.emplace_back(temppos, (int)(board->getRelatedFactor(temppos, side) * 10));
                            }
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
#endif
        }
        else if (Util::isdead4(board->getChessType(pos, side)))
        {
            moves.emplace_back(pos, board->getRelatedFactor(pos, side));

#ifdef EXTRA_VCT_CHESSTYPE
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
                        ChessBoard tempboard = *board;
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
#endif
        }
        //else if (!(ChessBoard::ban && board->getLastStep().getOtherSide() == PIECE_BLACK)&&board->getChessType(pos, side) == CHESSTYPE_D3)
        //{
        //    uint8_t direction = board->getChessDirection(pos, side);
        //    //特殊棋型，在同一条线上的双四
        //    //o?o?!?o  || o?o!??o || oo?!??oo
        //    Position pos(pos);
        //    Position temppos1 = pos.getNextPosition(direction, 1), temppos2 = pos.getNextPosition(direction, -1);
        //    if (!temppos1.valid() || !temppos2.valid())
        //    {
        //        continue;
        //    }

        //    if (board->getState(temppos1) == PIECE_BLANK && board->getState(temppos2) == PIECE_BLANK)
        //    {
        //        //o?o?!?o || oo?!??oo
        //        temppos1 = pos.getNextPosition(direction, 2), temppos2 = pos.getNextPosition(direction, -2);
        //        if (!temppos1.valid() || !temppos2.valid())
        //        {
        //            continue;
        //        }
        //        if (board->getState(temppos1) == side && board->getState(temppos2) == side)
        //        {
        //            //o?!?o?o
        //            temppos1 = pos.getNextPosition(direction, 3), temppos2 = pos.getNextPosition(direction, 4);
        //            if (temppos1.valid() && temppos2.valid())
        //            {
        //                if (board->getState(temppos1) == PIECE_BLANK && board->getState(temppos2) == side)
        //                {
        //                    moves.emplace_back(pos, (int)(board->getRelatedFactor(pos, side) * 10));
        //                    continue;
        //                }
        //            }
        //            //o?o?!?o
        //            temppos1 = pos.getNextPosition(direction, -3), temppos2 = pos.getNextPosition(direction, -4);
        //            if (temppos1.valid() && temppos2.valid())
        //            {
        //                if (board->getState(temppos1) == PIECE_BLANK && board->getState(temppos2) == side)
        //                {
        //                    moves.emplace_back(pos, (int)(board->getRelatedFactor(pos, side) * 10));
        //                    continue;
        //                }
        //            }
        //        }
        //        else
        //        {
        //            //  oo?!??oo
        //            // oo??!?oo

        //            if (board->getState(temppos1) == Util::otherside(side) || board->getState(temppos2) == Util::otherside(side))
        //            {
        //                continue;
        //            }
        //            if (board->getState(temppos1) != PIECE_BLANK && board->getState(temppos2) != PIECE_BLANK)
        //            {
        //                continue;
        //            }

        //            temppos1 = pos.getNextPosition(direction, 3), temppos2 = pos.getNextPosition(direction, -3);
        //            if (!temppos1.valid() || !temppos2.valid())
        //            {
        //                continue;
        //            }
        //            if (board->getState(temppos1) == side && board->getState(temppos2) == side)
        //            {
        //                temppos1 = pos.getNextPosition(direction, 4), temppos2 = pos.getNextPosition(direction, -4);
        //                if (temppos1.valid() && board->getState(temppos1) == side)
        //                {
        //                    moves.emplace_back(pos, (int)(board->getRelatedFactor(pos, side) * 10));
        //                    continue;
        //                }
        //                if (temppos2.valid() && board->getState(temppos2) == side)
        //                {
        //                    moves.emplace_back(pos, (int)(board->getRelatedFactor(pos, side) * 10));
        //                    continue;
        //                }
        //            }
        //        }
        //    }
        //    else
        //    {
        //        //o?o!??o 
        //        if (board->getState(temppos1) == Util::otherside(side) || board->getState(temppos2) == Util::otherside(side))
        //        {
        //            continue;
        //        }
        //        if (board->getState(temppos1) != PIECE_BLANK && board->getState(temppos2) != PIECE_BLANK)
        //        {
        //            continue;
        //        }
        //        temppos1 = pos.getNextPosition(direction, 2), temppos2 = pos.getNextPosition(direction, -2);
        //        if (!temppos1.valid() || !temppos2.valid())
        //        {
        //            continue;
        //        }
        //        if (board->getState(temppos1) == PIECE_BLANK && board->getState(temppos2) == PIECE_BLANK)
        //        {
        //            temppos1 = pos.getNextPosition(direction, 3), temppos2 = pos.getNextPosition(direction, -3);
        //            if (!temppos1.valid() || !temppos2.valid())
        //            {
        //                continue;
        //            }
        //            if (board->getState(temppos1) == side && board->getState(temppos2) == side)
        //            {
        //                moves.emplace_back(pos, (int)(board->getRelatedFactor(pos, side) * 10));
        //                continue;
        //            }
        //        }
        //    }
        //}
    }

    std::sort(moves.begin() + begin_index, moves.end(), CandidateItemCmp);
    //此处剪裁会造成棋力大幅下降
    //if (moves.size() > 8)
    //{
    //    moves.erase(moves.begin() + 8, moves.end());//如果此处只保留10个，会导致搜索不全，禁手胜利会感知不到，解决方案：冲四全部搜索（VCFmoves），不裁剪，VCT的才裁剪
    //}
    //moves.insert(moves.end(), VCFmoves.begin(), VCFmoves.end());
    //std::sort(moves.begin(), moves.end(), CandidateItemCmp);
}