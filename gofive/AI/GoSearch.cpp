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
    transTableStat = { 0,0,0,0 };
    this->board = board;
    this->startStep = board->lastStep;
    queue<string> initqueue;
    message_queue.swap(initqueue);
}

void GoSearchEngine::applySettings(AISettings setting)
{
    msgCallBack = setting.msgfunc;
    maxStepTimeMs = setting.maxStepTimeMs;
    restMatchTimeMs = setting.restMatchTimeMs;
    maxMemoryBytes = setting.maxMemoryBytes;
    transTable.setMaxMemory((maxMemoryBytes) / 3);
    transTableVCX.setMaxMemory((maxMemoryBytes) / 3 * 2);
    enableDebug = setting.enableDebug;
    maxAlphaBetaDepth = setting.maxAlphaBetaDepth;
    minAlphaBetaDepth = setting.minAlphaBetaDepth;
    VCFExpandDepth = setting.VCFExpandDepth;
    VCTExpandDepth = setting.VCTExpandDepth;
    useTransTable = setting.useTransTable;
    fullSearch = setting.fullSearch;
    useMultiThread = setting.multithread;
    ban = setting.ban;
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
    //message_queue.push(debugstr);
    msgCallBack(debugstr);
    message_queue_lock.unlock();
}

void GoSearchEngine::textOutIterativeInfo(MovePath& optimalPath)
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
    s << "rating:" << optimalPath.rating << ", next:" << (int)nextpos.row << "," << (int)nextpos.col << " bestpath:";
    for (auto pos : optimalPath.path)
    {
        s << "(" << (int)pos.row << "," << (int)pos.col << ") ";
    }
    sendMessage(s.str());
}

void GoSearchEngine::textOutResult(MovePath& optimalPath)
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
    s << "table:" << transTable.getTransTableSize() << (transTable.memoryValid() ? " " : "(full)") << " stable:" << transTableVCX.getTransTableSize() << (transTableVCX.memoryValid() ? " " : "(full)");
    sendMessage(s.str());
    s.str("");
    s << "hit:" << transTableStat.hit << " miss:" << transTableStat.miss << " clash:" << transTableStat.clash << " cover:" << transTableStat.cover;
    sendMessage(s.str());

    //for (int i = 0; i < 20; ++i)
    //{
    //    s.str("");
    //    s << i << ":" << VCXSuccessCount[i];
    //    sendMessage(s.str());
    //}
}

void GoSearchEngine::textForTest(MovePath& optimalPath, int priority)
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
    if (step < 8)
    {
        max_time = restMatchTimeMs / 20 < maxStepTimeMs ? restMatchTimeMs / 20 : maxStepTimeMs;
        suggest_time = max_time;
    }
    else if (step < 60)
    {
        if (restMatchTimeMs < maxStepTimeMs)
        {
            max_time = restMatchTimeMs / 10 * 2;
            suggest_time = restMatchTimeMs / 10;
        }
        else if (restMatchTimeMs / 30 < maxStepTimeMs / 3)
        {
            max_time = restMatchTimeMs / 15;
            suggest_time = restMatchTimeMs / 30;
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
            max_time = restMatchTimeMs / 10;
            suggest_time = restMatchTimeMs / 10;
        }
        else if (restMatchTimeMs / 3 < maxStepTimeMs)
        {
            max_time = maxStepTimeMs / 6;
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

    MovePath bestPath(0);
    currentAlphaBetaDepth = minAlphaBetaDepth;
    StepCandidateItem bestStep(Position(-1, -1), 0);
    while (true)
    {
        if (duration_cast<milliseconds>(std::chrono::system_clock::now() - this->startSearchTime).count() > suggest_time)
        {
            currentAlphaBetaDepth -= 1;
            break;
        }

        MovePath temp = solveBoard(board, bestStep);
        textOutIterativeInfo(temp);
        if (currentAlphaBetaDepth > minAlphaBetaDepth && global_isOverTime)
        {
            if (bestPath.rating < CHESSTYPE_5_SCORE && temp.rating == CHESSTYPE_5_SCORE)
            {
                bestPath = temp;
            }
            //else if (!temp.path.empty() && temp.rating > -1000)
            //{
            //    bestPath = temp;
            //}
            currentAlphaBetaDepth -= 1;
            break;
        }
        bestPath = temp;
        if (temp.rating >= CHESSTYPE_5_SCORE || temp.rating == -CHESSTYPE_5_SCORE)
        {
            break;
        }

        //已成定局的不需要继续搜索了
        if (bestStep.priority == 10000)
        {
            break;
        }

        if (startStep.step < 4)
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
    textOutResult(bestPath);

    if (bestPath.path.empty())
    {
        bestPath.path.push_back(board->getHighestInfo(startStep.getState()).pos);
    }

    return bestPath.path[0];
}

static int base_alpha = INT_MIN, base_beta = INT_MAX;
static MovePath optimalPath(0);

void GoSearchEngine::solveBoardForEachThread(PVSearchData data)
{
    MovePath tempPath(data.engine->board->getLastStep().step);
    tempPath.push(data.it->pos);

    bool deadfour = false;
    if (data.struggle && Util::hasdead4(data.engine->board->getChessType(data.it->pos, data.engine->board->getLastStep().getOtherSide())))
    {
        deadfour = true;
    }

    ChessBoard currentBoard = *(data.engine->board);
    currentBoard.move(data.it->pos, data.engine->ban);


    data.engine->doAlphaBetaSearch(&currentBoard, deadfour&&data.struggle ? data.engine->currentAlphaBetaDepth + 1 : data.engine->currentAlphaBetaDepth - 1,
        base_alpha, base_alpha + 1, tempPath, data.engine->useTransTable, false);
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

MovePath GoSearchEngine::solveBoard(ChessBoard* board, StepCandidateItem& bestStep)
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
    MovePath VCFPath(board->getLastStep().step);
    MovePath VCTPath(board->getLastStep().step);

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
    else if (doVCFSearch(board, getVCFDepth(board->getLastStep().step), VCFPath, NULL, useTransTable) == VCXRESULT_SUCCESS)
    {
        bestStep = StepCandidateItem(VCFPath.path[0], 10000);
        VCFPath.rating = CHESSTYPE_5_SCORE;
        return VCFPath;
    }
    else if (doVCTSearch(board, getVCTDepth(board->getLastStep().step), VCTPath, NULL, useTransTable) == VCXRESULT_SUCCESS)
    {
        bestStep = StepCandidateItem(VCTPath.path[0], 10000);
        VCTPath.rating = CHESSTYPE_5_SCORE;
        return VCTPath;
    }
    else
    {
        if (Util::isRealFourKill(otherhighest.chesstype))//敌方有 44 或者 4
        {
            getFourkillDefendCandidates(board, otherhighest.pos, solveList, ban);
            firstSearchUpper = solveList.size();
            getVCFCandidates(board, solveList, NULL);
            if (otherhighest.chesstype == CHESSTYPE_43)
            {
                getVCTCandidates(board, solveList, NULL);
            }
        }
        else
        {
            firstSearchUpper = getNormalCandidates(board, solveList, NULL, fullSearch);
        }

        if (firstSearchUpper == 0) solveList.size();

        if (bestStep.pos.valid())
        {
            //优先搜索置换表中记录的上一个迭代的最好着法
            for (size_t i = 0; i < firstSearchUpper; ++i)
            {
                if (solveList[i].pos == bestStep.pos)
                {
                    solveList[i].priority = 10000;
                    std::sort(solveList.begin(), solveList.begin() + firstSearchUpper, CandidateItemCmp);
                    break;
                }
            }
        }
    }

    if (solveList.empty())
    {
        optimalPath.rating = -10000;
        optimalPath.push(otherhighest.pos);
        bestStep = StepCandidateItem(otherhighest.pos, 10000);
        return optimalPath;
    }


    bool foundPV = false;
    size_t index = 0;

    //first search
    for (; index < firstSearchUpper; ++index)
    {
        if (useMultiThread && foundPV)
        {
            PVSearchData data(this, solveList.begin() + index);
            ThreadPool::getInstance()->run(bind(solveBoardForEachThread, data));
        }
        else
        {
            MovePath tempPath(board->getLastStep().step);
            tempPath.push(solveList[index].pos);
            ChessBoard currentBoard = *board;
            currentBoard.move(solveList[index].pos, ban);

            if (foundPV)
            {
                //假设当前是最好的，没有任何其他的会比当前的PV好（大于alpha）
                doAlphaBetaSearch(&currentBoard, currentAlphaBetaDepth - 1, base_alpha, base_alpha + 1, tempPath, useTransTable, false);//极小窗口剪裁 
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



            if (tempPath.rating > optimalPath.rating)
            {
                if (tempPath.rating > -CHESSTYPE_5_SCORE)
                {
                    //检查敌人是否能VCT
                    MovePath VCTPath(board->getLastStep().step);
                    VCTPath.push(solveList[index].pos);
                    if (doVCTSearch(&currentBoard, getVCTDepth(currentBoard.getLastStep().step), VCTPath, NULL, useTransTable) == VCXRESULT_SUCCESS)
                    {
                        if (optimalPath.rating < -CHESSTYPE_5_SCORE)
                        {
                            optimalPath = VCTPath;
                            base_alpha = -CHESSTYPE_5_SCORE;
                            foundPV = true;
                            tempPath = VCTPath;
                        }
                        else if (optimalPath.rating == -CHESSTYPE_5_SCORE && VCTPath.endStep > optimalPath.endStep)
                        {
                            optimalPath = VCTPath;
                            tempPath = VCTPath;
                        }
                    }
                    else
                    {
                        base_alpha = tempPath.rating;
                        optimalPath = tempPath;
                        foundPV = true;
                    }
                }
                else
                {
                    base_alpha = tempPath.rating;
                    optimalPath = tempPath;
                    foundPV = true;
                }
            }
            else if (tempPath.rating == optimalPath.rating)
            {
                if ((tempPath.rating == CHESSTYPE_5_SCORE && tempPath.endStep < optimalPath.endStep) ||
                    (tempPath.rating == -CHESSTYPE_5_SCORE && tempPath.endStep > optimalPath.endStep))
                {
                    optimalPath = tempPath;
                }
            }

            if (enableDebug)
            {
                textForTest(tempPath, solveList[index].priority);
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

    //second search
    if (firstSearchUpper < solveList.size() && optimalPath.rating == -CHESSTYPE_5_SCORE)
    {
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

            MovePath tempPath(board->getLastStep().step);
            tempPath.push(solveList[index].pos);
            ChessBoard currentBoard = *board;
            currentBoard.move(solveList[index].pos, ban);

            bool deadfour = false;
            if (Util::hasdead4(board->getChessType(solveList[index].pos, side)))
            {
                deadfour = true;
            }

            if (foundPV)
            {
                doAlphaBetaSearch(&currentBoard, deadfour ? currentAlphaBetaDepth + 1 : currentAlphaBetaDepth - 1, base_alpha, base_alpha + 1, tempPath, useTransTable, false);//0窗口剪裁
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
                bestStep = StepCandidateItem(optimalPath.path[0], 0);
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
                bestStep = StepCandidateItem(optimalPath.path[0], 0);
                return optimalPath;
            }
        }
    }

    bestStep = StepCandidateItem(optimalPath.path[0], 0);
    return optimalPath;
}

void GoSearchEngine::doAlphaBetaSearch(ChessBoard* board, int depth, int alpha, int beta, MovePath& optimalPath, bool useTransTable, bool deepSearch)
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
#define TRANSTABLE_HIT_FUNC transTableStat.hit++;optimalPath.rating = data.value;optimalPath.push(data.bestStep);optimalPath.endStep = data.depth + startStep.step;
    if (useTransTable)
    {
        if (transTable.get(board->getBoardHash().hash_key, data))
        {
            if (data.checkHash == board->getBoardHash().check_key)
            {
                if (data.value == -CHESSTYPE_5_SCORE || data.value == CHESSTYPE_5_SCORE)
                {
                    TRANSTABLE_HIT_FUNC
                        return;
                }
                else if (data.maxdepth < depth)
                {
                    transTableStat.cover++;
                    if (data.bestStep.valid())
                    {
                        has_best_pos = true;
                    }
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

    bool isdefendfive = false;
    uint8_t move_index = 0;
    size_t firstSearchUpper = 0;
    MovePath VCFPath(board->getLastStep().step);
    MovePath VCTPath(board->getLastStep().step);

    vector<StepCandidateItem> moves;

    MovePath bestPath(board->getLastStep().step);
    bestPath.rating = isPlayerSide(side) ? INT_MAX : INT_MIN;

    if (board->getHighestInfo(side).chesstype == CHESSTYPE_5)
    {
        optimalPath.rating = isPlayerSide(side) ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
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
            optimalPath.rating = board->getGlobalEvaluate(getAISide(), 100);//存疑
            return;
        }
        else
        {
            isdefendfive = true;
            moves.emplace_back(board->getHighestInfo(otherside).pos, 10);
            firstSearchUpper = moves.size();
        }
    }
    else if (global_isOverTime || duration_cast<milliseconds>(std::chrono::system_clock::now() - startSearchTime).count() > maxStepTimeMs)//超时
    {
        optimalPath.rating = -CHESSTYPE_5_SCORE + 111;
        global_isOverTime = true;
        return;
    }
    else if (depth <= 0)
    {
        //静态搜索拓展
        //if (doVCFSearch(board, getVCFDepth(board->getLastStep().step), VCFPath, NULL, useTransTable) == VCXRESULT_SUCCESS)//side赢了
        //{
        //    bestPath = VCFPath;
        //    goto end;
        //}
        //else
        {
            optimalPath.rating = board->getGlobalEvaluate(getAISide(), 100);
            return;
        }
    }
    else if (doVCFSearchWrapper(board, getVCFDepth(board->getLastStep().step), VCFPath, NULL, useTransTable) == VCXRESULT_SUCCESS)//side赢了
    {
        bestPath = VCFPath;
        bestPath.rating = isPlayerSide(side) ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
        goto end;
    }
    //else if (depth > currentAlphaBetaDepth / 2 && doVCTSearchWrapper(board, getVCTDepth(board->getLastStep().step), VCTPath, NULL, useTransTable) == VCXRESULT_SUCCESS)
    //{
    //    bestPath = VCTPath;
    //    bestPath.rating = isPlayerSide(side) ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
    //    goto end;
    //}
    else
    {
        if (Util::isRealFourKill(board->getHighestInfo(otherside).chesstype))//敌方有 44 或者 4
        {
            getFourkillDefendCandidates(board, board->getHighestInfo(otherside).pos, moves, ban);
            firstSearchUpper = moves.size();
            getVCFCandidates(board, moves, NULL);
            if (board->getHighestInfo(otherside).chesstype == CHESSTYPE_43)
            {
                getVCTCandidates(board, moves, NULL);
            }
        }
        else
        {
            firstSearchUpper = getNormalCandidates(board, moves, NULL, fullSearch);
        }

        if (firstSearchUpper == 0) firstSearchUpper = moves.size();

        if (has_best_pos)
        {
            //优先搜索置换表中记录的上一个迭代的最好着法
            for (size_t i = 0; i < firstSearchUpper; ++i)
            {
                if (moves[i].pos == data.bestStep)
                {
                    moves[i].priority = 10000;
                    std::sort(moves.begin(), moves.begin() + firstSearchUpper, CandidateItemCmp);
                    break;
                }
            }
        }
    }


    bool foundPV = false;
    data.type = TRANSTYPE_EXACT;

    if (continue_flag && data.continue_index < firstSearchUpper)
    {
        move_index = data.continue_index;
        bestPath.rating = data.value;
        bestPath.push(data.bestStep);
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

    //first search
    for (; move_index < firstSearchUpper; ++move_index)
    {
        MovePath tempPath(board->getLastStep().step);
        tempPath.push(moves[move_index].pos);
        ChessBoard currentBoard = *board;
        currentBoard.move(moves[move_index].pos, ban);

        //剪枝
        if (isPlayerSide(side))//build player
        {
            if (foundPV)
            {
                doAlphaBetaSearch(&currentBoard, depth - 1, beta - 1, beta, tempPath, useTransTable, false);//极小窗口剪裁
                //假设当前是最好的，没有任何其他的会比当前的PV好（小于beta）
                if (tempPath.rating < beta && tempPath.rating > alpha)//失败，使用完整窗口
                {
                    tempPath.path.clear();
                    tempPath.push(moves[move_index].pos);
                    doAlphaBetaSearch(&currentBoard, depth - 1, alpha, beta, tempPath, useTransTable, deepSearch);
                }
            }
            else
            {
                doAlphaBetaSearch(&currentBoard, depth - 1, alpha, beta, tempPath, useTransTable, deepSearch);
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
                doAlphaBetaSearch(&currentBoard, depth - 1, alpha, alpha + 1, tempPath, useTransTable, false);//极小窗口剪裁
                //假设当前是最好的，没有任何其他的会比当前的PV好（大于alpha）
                if (tempPath.rating > alpha && tempPath.rating < beta)
                {
                    tempPath.path.clear();
                    tempPath.push(moves[move_index].pos);
                    doAlphaBetaSearch(&currentBoard, depth - 1, alpha, beta, tempPath, useTransTable, deepSearch);
                }
            }
            else
            {
                doAlphaBetaSearch(&currentBoard, depth - 1, alpha, beta, tempPath, useTransTable, deepSearch);
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

            MovePath tempPath(board->getLastStep().step);
            tempPath.push(moves[move_index].pos);
            ChessBoard currentBoard = *board;
            currentBoard.move(moves[move_index].pos, ban);
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
                    //假设当前是最好的，没有任何其他的会比当前的PV好（小于beta）
                    doAlphaBetaSearch(&currentBoard, deadfour ? depth + 1 : depth - 1, beta - 1, beta, tempPath, useTransTable, false);//极小窗口剪裁
                    if (tempPath.rating < beta && tempPath.rating > alpha)//失败，使用完整窗口
                    {
                        tempPath.path.clear();
                        tempPath.push(moves[move_index].pos);
                        doAlphaBetaSearch(&currentBoard, deadfour ? depth + 1 : depth - 1, alpha, beta, tempPath, useTransTable, deepSearch);
                    }
                }
                else
                {
                    doAlphaBetaSearch(&currentBoard, deadfour ? depth + 1 : depth - 1, alpha, beta, tempPath, useTransTable, deepSearch);
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
                    doAlphaBetaSearch(&currentBoard, deadfour ? depth + 1 : depth - 1, alpha, alpha + 1, tempPath, useTransTable, false);//极小窗口剪裁
                    //假设当前是最好的，没有任何其他的会比当前的PV好（大于alpha）
                    if (tempPath.rating > alpha && tempPath.rating < beta)
                    {
                        tempPath.path.clear();
                        tempPath.push(moves[move_index].pos);
                        doAlphaBetaSearch(&currentBoard, deadfour ? depth + 1 : depth - 1, alpha, beta, tempPath, useTransTable, deepSearch);
                    }
                }
                else
                {
                    doAlphaBetaSearch(&currentBoard, deadfour ? depth + 1 : depth - 1, alpha, beta, tempPath, useTransTable, deepSearch);
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
        data.checkHash = board->getBoardHash().check_key;
        data.value = optimalPath.rating;
        data.depth = optimalPath.endStep - startStep.step;
        data.maxdepth = depth;
        data.continue_index = move_index;
        data.bestStep = bestPath.path.empty() ? Position(-1, -1) : bestPath.path[0];
        transTable.insert(board->getBoardHash().hash_key, data);
    }
    //end USE TransTable
}

void GoSearchEngine::getNormalRelatedSet(ChessBoard* board, set<Position>& reletedset, MovePath& optimalPath)
{
    uint8_t defendside = board->getLastStep().getOtherSide();
    uint8_t atackside = board->getLastStep().getState();//laststep的进攻成功，现在要找防守步

    vector<Position> path;
    path.push_back(board->getLastStep().pos);
    path.insert(path.end(), optimalPath.path.begin(), optimalPath.path.end());

    ChessBoard tempboard = *board;
    for (size_t i = 0; i < path.size() && i < 10; i++)
    {
        //tempboard.move(path[i]);
        reletedset.insert(path[i]);
        i++;
        if (i < path.size())
        {
            //tempboard.move(path[i]);
            tempboard.getDefendReletedPos(reletedset, path[i - 1], atackside);//相关点是对于进攻而言的，防守策略根据进攻的相关点去防守
            reletedset.insert(path[i]);
        }
    }
}

void getSimpleRelatedSet(ChessBoard* board, set<Position>& reletedset, MovePath& optimalPath)
{
    vector<Position> path;
    path.push_back(board->getLastStep().pos);
    path.insert(path.end(), optimalPath.path.begin(), optimalPath.path.end());

    size_t len = path.size();
    for (size_t i = 0; i < len; i++)
    {
        reletedset.insert(path[i]);
    }
}

size_t GoSearchEngine::getNormalCandidates(ChessBoard* board, vector<StepCandidateItem>& moves, Position* center, bool full_search)
{
    uint8_t side = board->getLastStep().getOtherSide();
    Position lastPos = board->getLastStep().pos;
    ForEachPosition
    //ForRectPosition(Util::generate_rect(lastPos.row, lastPos.col, 5))
    {
        if (!(board->canMove(pos) && board->useful(pos)))
        {
            continue;
        }

        uint8_t selftype = board->getChessType(pos, side);

        if (selftype == CHESSTYPE_BAN)
        {
            continue;
        }

        uint8_t otherp = board->getChessType(pos, Util::otherside(side));

        int atack = board->getRelatedFactor(pos, side), defend = board->getRelatedFactor(pos, Util::otherside(side), true);

        //if (!full_search && board->getLastStep().step < 10 && atack < 10 && otherp < CHESSTYPE_2)
        //{
        //    continue;
        //}

        //if ((Util::isdead4(selftype) /*|| Util::isalive3(selftype)*/) && atack < 20 && defend < 5)//会导致禁手陷阱无法触发，因为禁手陷阱一般都是始于“无意义”的冲四
        //{
        //    moves.emplace_back(pos, 0);
        //    continue;
        //}

        moves.emplace_back(pos, atack + defend);
    }

    std::sort(moves.begin(), moves.end(), CandidateItemCmp);

    if (!full_search)
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

    //uint8_t side = board->getLastStep().getOtherSide();
    //ForEachPosition
    //{
    //    if (!(board->canMove(pos) && board->useful(pos)))
    //    {
    //        continue;
    //    }

    //uint8_t selfp = board->getChessType(pos, side);

    //if (selfp == CHESSTYPE_BAN)
    //{
    //    continue;
    //}

    //uint8_t otherp = board->getChessType(pos, Util::otherside(side));

    //int atack = board->getRelatedFactor(pos, side), defend = board->getRelatedFactor(pos, Util::otherside(side), true);

    //if (!full_search && atack < 10 && defend == 0)
    //{
    //    continue;
    //}
    //if (!full_search && selfp == CHESSTYPE_D4 && atack < 20 && defend < 5)//会导致禁手陷阱无法触发，因为禁手陷阱一般都是始于“无意义”的冲四
    //{
    //    moves.emplace_back(pos, 0);
    //}
    //moves.emplace_back(pos, atack + defend);
    //}

    //std::sort(moves.begin(), moves.end(), CandidateItemCmp);

    //if (moves.size() > MAX_CHILD_NUM && !full_search)
    //{
    //    for (auto i = 0; i < moves.size(); ++i)
    //    {
    //        if (moves[i].priority < moves[0].priority / 3)
    //        {
    //            //moves.erase(moves.begin() + i, moves.end());
    //            return i;
    //        }
    //    }
    //}

    //return moves.size();

}

void GoSearchEngine::getALLFourkillDefendSteps(ChessBoard* board, vector<StepCandidateItem>& moves, bool is33)
{
    //uint8_t side = board->getLastStep().getState();
    //ForEachPosition
    //{
    //    if (is33)
    //    {
    //        if (board->getChessType(pos, side) == CHESSTYPE_33)
    //        {
    //            getFourkillDefendCandidates(board, pos, moves);
    //        }
    //    }
    //    else if (Util::isfourkill(board->getChessType(pos, side)))
    //    {
    //        getFourkillDefendCandidates(board,pos, moves);
    //    }
    //}

}

void GoSearchEngine::getFourkillDefendCandidates(ChessBoard* board, Position pos, vector<StepCandidateItem>& moves, GAME_RULE rule)
{
    //现在该防守方落子
    uint8_t defendside = board->getLastStep().getOtherSide();//防守方
    uint8_t atackside = board->getLastStep().getState();//进攻方
    uint8_t atackType = board->getChessType(pos, atackside);

    vector<uint8_t> direction;

    if (board->getChessType(pos, defendside) != CHESSTYPE_BAN)
    {
        moves.emplace_back(pos, 100);
    }

    if (atackType == CHESSTYPE_5)
    {
        return;
    }
    else if (atackType == CHESSTYPE_4)//两个进攻点__ooo__，两个防点/一个进攻点x_ooo__（有一边被堵），三个防点
    {
        for (int d = 0; d < DIRECTION4_COUNT; ++d)
        {
            if (board->getLayer2(pos.row, pos.col, atackside, d) == CHESSTYPE_4)
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
                    uint8_t tempType = board->getLayer2(temppos.row, temppos.col, atackside, n / 2);
                    if (tempType == CHESSTYPE_4)
                    {
                        defend_point_count++;
                        if (board->getChessType(temppos.row, temppos.col, defendside) != CHESSTYPE_BAN)
                        {
                            moves.emplace_back(temppos, 80);
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
            if (board->getLayer2(pos.row, pos.col, atackside, d) == CHESSTYPE_44)
            {
                direction.push_back(d * 2);
                direction.push_back(d * 2 + 1);
                break;
            }
            else if (Util::isdead4(board->getLayer2(pos.row, pos.col, atackside, d)))
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
            if (Util::isdead4(board->getLayer2(pos.row, pos.col, atackside, d)) || Util::isalive3(board->getLayer2(pos.row, pos.col, atackside, d)))
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
            if (Util::isalive3(board->getLayer2(pos.row, pos.col, atackside, d)))
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
                uint8_t tempType = board->getLayer2(temppos.row, temppos.col, atackside, n / 2);
                if (tempType > CHESSTYPE_0)
                {
                    if (board->getChessType(temppos.row, temppos.col, defendside) != CHESSTYPE_BAN)//被禁手了
                    {
                        ChessBoard tempboard = *board;
                        tempboard.move(temppos, rule);
                        if (tempboard.getChessType(pos, atackside) < atackType)
                        {
                            moves.emplace_back(temppos, 80);
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

//VCXRESULT GoSearchEngine::doVCFSearchWrapper(ChessBoard* board, int depth, MovePath& optimalPath, set<Position>* reletedset, bool useTransTable)
//{
//    if (!useTransTable)
//    {
//        return doVCFSearch(board, depth, optimalPath, reletedset, useTransTable);
//    }
//    TransTableVCXData data;
//    if (transTableVCX.get(board->getBoardHash().hash_key, data))
//    {
//        if (data.checkHash == board->getBoardHash().check_key)
//        {
//            if (data.VCFflag == VCXRESULT_NOSEARCH)//还未搜索
//            {
//                transTableStat.miss++;
//            }
//            else
//            {
//                if (data.VCFflag == VCXRESULT_UNSURE && data.VCFmaxdepth < depth)//需要更新
//                {
//                    transTableStat.cover++;
//                }
//                else
//                {
//                    transTableStat.hit++;
//                    //optimalPath.push(data.bestStep);
//                    optimalPath.endStep = data.VCFdepth + startStep.step;
//                    return data.VCFflag;
//                }
//            }
//        }
//        else
//        {
//            transTableStat.clash++;
//        }
//    }
//    else
//    {
//        transTableStat.miss++;
//        if (!transTableVCX.memoryValid())
//        {
//            return doVCFSearch(board, depth, optimalPath, reletedset, useTransTable);
//        }
//    }
//
//    VCXRESULT flag = doVCFSearch(board, depth, optimalPath, reletedset, useTransTable);
//
//    if (depth > 0 /*&& (reletedset == NULL || flag == VCXRESULT_SUCCESS)*/)
//    {
//        data.VCFflag = flag;
//        data.checkHash = board->getBoardHash().check_key;
//        data.VCFmaxdepth = depth;
//        data.VCFdepth = optimalPath.endStep - startStep.step;
//        transTableVCX.insert(board->getBoardHash().hash_key, data);
//    }
//    return flag;
//}
//
//VCXRESULT GoSearchEngine::doVCTSearchWrapper(ChessBoard* board, int depth, MovePath& optimalPath, set<Position>* reletedset, bool useTransTable)
//{
//    if (!useTransTable)
//    {
//        return doVCTSearch(board, depth, optimalPath, reletedset, useTransTable);
//    }
//
//    TransTableVCXData data;
//    if (transTableVCX.get(board->getBoardHash().hash_key, data))
//    {
//        if (data.checkHash == board->getBoardHash().check_key)
//        {
//            if (data.VCTflag == VCXRESULT_NOSEARCH)//还未搜索
//            {
//                transTableStat.miss++;
//            }
//            else
//            {
//                if (data.VCTflag == VCXRESULT_UNSURE  && data.VCTmaxdepth < depth)//需要更新
//                {
//                    transTableStat.cover++;
//                }
//                else
//                {
//                    transTableStat.hit++;
//                    optimalPath.endStep = data.VCTdepth + startStep.step;
//                    return data.VCTflag;
//                }
//            }
//        }
//        else
//        {
//            transTableStat.clash++;
//        }
//    }
//    else
//    {
//        transTableStat.miss++;
//        if (!transTableVCX.memoryValid())
//        {
//            return doVCTSearch(board, depth, optimalPath, reletedset, useTransTable);
//        }
//    }
//    VCXRESULT flag = doVCTSearch(board, depth, optimalPath, reletedset, useTransTable);
//    //可能是局部结果
//    if (depth > 0 /*&& (reletedset == NULL || flag == VCXRESULT_SUCCESS)*/)
//    {
//        data.checkHash = board->getBoardHash().check_key;
//        data.VCTflag = flag;
//        data.VCTmaxdepth = depth;
//        data.VCTdepth = optimalPath.endStep - startStep.step;
//        transTableVCX.insert(board->getBoardHash().hash_key, data);
//    }
//    return flag;
//}


VCXRESULT GoSearchEngine::doVCFSearchWrapper(ChessBoard* board, int depth, MovePath& optimalPath, Position* center, bool useTransTable)
{
    if (!useTransTable)
    {
        return doVCFSearch(board, depth, optimalPath, center, useTransTable);
    }
    TransTableVCXData data;
    if (transTableVCX.get(board->getBoardHash().hash_key, data))
    {
        if (data.checkHash == board->getBoardHash().check_key)
        {
            if (data.VCFflag == VCXRESULT_NOSEARCH)//还未搜索
            {
                transTableStat.miss++;
            }
            else
            {
                if (data.VCFflag == VCXRESULT_UNSURE && data.VCFmaxdepth < depth)//需要更新
                {
                    transTableStat.cover++;
                }
                else
                {
                    transTableStat.hit++;
                    //optimalPath.push(data.bestStep);
                    optimalPath.endStep = data.VCFdepth + startStep.step;
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
        if (!transTableVCX.memoryValid())
        {
            return doVCFSearch(board, depth, optimalPath, center, useTransTable);
        }
    }

    VCXRESULT flag = doVCFSearch(board, depth, optimalPath, center, useTransTable);

    if (depth > 0 /*&& (reletedset == NULL || flag == VCXRESULT_SUCCESS)*/)
    {
        data.VCFflag = flag;
        data.checkHash = board->getBoardHash().check_key;
        data.VCFmaxdepth = depth;
        data.VCFdepth = optimalPath.endStep - startStep.step;
        transTableVCX.insert(board->getBoardHash().hash_key, data);
    }
    return flag;
}

VCXRESULT GoSearchEngine::doVCTSearchWrapper(ChessBoard* board, int depth, MovePath& optimalPath, Position* center, bool useTransTable)
{
    if (!useTransTable)
    {
        return doVCTSearch(board, depth, optimalPath, center, useTransTable);
    }

    TransTableVCXData data;
    if (transTableVCX.get(board->getBoardHash().hash_key, data))
    {
        if (data.checkHash == board->getBoardHash().check_key)
        {
            if (data.VCTflag == VCXRESULT_NOSEARCH)//还未搜索
            {
                transTableStat.miss++;
            }
            else
            {
                if (data.VCTflag == VCXRESULT_UNSURE  && data.VCTmaxdepth < depth)//需要更新
                {
                    transTableStat.cover++;
                }
                else
                {
                    transTableStat.hit++;
                    optimalPath.endStep = data.VCTdepth + startStep.step;
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
        if (!transTableVCX.memoryValid())
        {
            return doVCTSearch(board, depth, optimalPath, center, useTransTable);
        }
    }
    VCXRESULT flag = doVCTSearch(board, depth, optimalPath, center, useTransTable);
    //可能是局部结果
    if (depth > 0 /*&& (reletedset == NULL || flag == VCXRESULT_SUCCESS)*/)
    {
        data.checkHash = board->getBoardHash().check_key;
        data.VCTflag = flag;
        data.VCTmaxdepth = depth;
        data.VCTdepth = optimalPath.endStep - startStep.step;
        transTableVCX.insert(board->getBoardHash().hash_key, data);
    }
    return flag;
}

VCXRESULT GoSearchEngine::doVCFSearch(ChessBoard* board, int depth, MovePath& optimalPath, Position* center, bool useTransTable)
{
    uint8_t side = board->getLastStep().getOtherSide();
    Position lastindex = board->getLastStep().pos;
    uint16_t laststep = board->getLastStep().step;
    if (board->getHighestInfo(side).chesstype == CHESSTYPE_5)
    {
        optimalPath.push(board->getHighestInfo(side).pos);
        optimalPath.rating = side == startStep.getState() ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
        return VCXRESULT_SUCCESS;
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
    getVCFCandidates(board, moves, center);
    std::sort(moves.begin(), moves.end(), CandidateItemCmp);

    for (auto item : moves)
    {
        ChessBoard tempboard = *board;
        tempboard.move(item.pos, ban);//冲四

        MovePath tempPath(board->getLastStep().step);
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
            return VCXRESULT_SUCCESS;
        }
        tempPath.push(tempboard.getHighestInfo(side).pos);//防五连
        tempboard.move(tempboard.getHighestInfo(side).pos, ban);

        //set<Position> atackset;
        ///*if (reletedset != NULL)
        //{
        //    set<uint8_t> tempatackset;
        //    tempboard.getAtackReletedPos(tempatackset, item.index, side);
        //    util::myset_intersection(reletedset, &tempatackset, &atackset);
        //}
        //else*/
        //{
        //    tempboard.getAtackReletedPos(atackset, item.pos, side);
        //}

        uint8_t result = doVCFSearchWrapper(&tempboard, depth - 2, tempPath, &item.pos, useTransTable);
        if (result == VCXRESULT_SUCCESS)
        {
            optimalPath.rating = side == startStep.getState() ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
            optimalPath.cat(tempPath);
            return VCXRESULT_SUCCESS;
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
        return VCXRESULT_FAIL;
    }

}

VCXRESULT GoSearchEngine::doVCTSearch(ChessBoard* board, int depth, MovePath& optimalPath, Position* center, bool useTransTable)
{
    bool defendfive = false;
    uint8_t side = board->getLastStep().getOtherSide();
    uint16_t laststep = board->getLastStep().step;
    Position lastindex = board->getLastStep().pos;
    MovePath VCFPath(board->getLastStep().step);
    vector<StepCandidateItem> moves;
    if (board->getHighestInfo(side).chesstype == CHESSTYPE_5)
    {
        optimalPath.push(board->getHighestInfo(side).pos);
        optimalPath.rating = side == startStep.getState() ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
        return VCXRESULT_SUCCESS;
    }
    else if (board->getHighestInfo(Util::otherside(side)).chesstype == CHESSTYPE_5)
    {
        if (board->getChessType(board->getHighestInfo(Util::otherside(side)).pos, side) == CHESSTYPE_BAN)
        {
            return VCXRESULT_FAIL;
        }
        defendfive = true;
        moves.emplace_back(board->getHighestInfo(Util::otherside(side)).pos, 10);
    }
    else if (global_isOverTime || duration_cast<milliseconds>(std::chrono::system_clock::now() - startSearchTime).count() > maxStepTimeMs)
    {
        global_isOverTime = true;
        return VCXRESULT_UNSURE;
    }
    else if (doVCFSearchWrapper(board, depth + getVCFDepth(0) - getVCTDepth(0), VCFPath, center, useTransTable) == VCXRESULT_SUCCESS)
    {
        optimalPath.cat(VCFPath);
        optimalPath.rating = VCFPath.rating;
        return VCXRESULT_SUCCESS;
    }
    else if (depth <= 0)
    {
        return VCXRESULT_UNSURE;
    }
    else
    {
        getVCTCandidates(board, moves, center);
        getVCFCandidates(board, moves, center);//优先VCF
        std::sort(moves.begin(), moves.end(), CandidateItemCmp);
    }

    bool unsure = false;//默认是false 只有要一个unsure 整个都是unsure

    VCXRESULT tempresult;
    for (auto item : moves)
    {
        ChessBoard tempboard = *board;
        tempboard.move(item.pos, ban);
        bool isVCF;//本步是否是冲四
        if (tempboard.getHighestInfo(side).chesstype < CHESSTYPE_33)//无法造成VCT威胁 & //防假活三，连环禁手
        {
            continue;
        }

        if (tempboard.getHighestInfo(Util::otherside(side)).chesstype == CHESSTYPE_5)//失败，对方有5连
        {
            continue;
        }

        MovePath tempPath(board->getLastStep().step);
        tempPath.push(item.pos);

        vector<StepCandidateItem> defendmoves;
        if (tempboard.getHighestInfo(side).chesstype == CHESSTYPE_5)
        {
            if (tempboard.getChessType(tempboard.getHighestInfo(side).pos, Util::otherside(side)) == CHESSTYPE_BAN)//敌方触发禁手，VCF成功
            {
                optimalPath.rating = side == startStep.getState() ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
                optimalPath.cat(tempPath);
                errorVCFSuccessInVCTCount++;
                return VCXRESULT_SUCCESS;
            }
            isVCF = true;
            defendmoves.emplace_back(tempboard.getHighestInfo(side).pos, 10000);
        }
        else
        {
            MovePath tempPathVCF(tempboard.getLastStep().step);
            tempresult = doVCFSearch(&tempboard, depth - 1 + getVCFDepth(0) - getVCTDepth(0), tempPathVCF, NULL, useTransTable);
            if (tempresult == VCXRESULT_SUCCESS)
            {
                continue;
            }

            isVCF = false;
            getFourkillDefendCandidates(&tempboard, tempboard.getHighestInfo(side).pos, defendmoves, ban);
            if (defendmoves.empty())
            {
                optimalPath.cat(tempPath);
                optimalPath.rating = side == startStep.getState() ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
                return VCXRESULT_SUCCESS;
            }
        }

        //set<Position> atackset;
        ///*if (reletedset != NULL)
        //{
        //set<uint8_t> tempatackset;
        //tempboard2.getAtackReletedPos(tempatackset, item.index, side);
        //util::myset_intersection(reletedset, &tempatackset, &atackset);
        //}
        //else*/
        //{
        //    tempboard.getAtackReletedPos(atackset, item.pos, side);
        //}

        VCXRESULT flag = VCXRESULT_UNSURE;

        MovePath tempPath2(tempboard.lastStep.step);
        //if (ChessBoard::ban)
        {
            for (auto defend : defendmoves)
            {
                MovePath tempPathDefend(tempboard.lastStep.step);
                ChessBoard tempboard2 = tempboard;
                tempboard2.move(defend.pos, ban);
                tempPathDefend.push(defend.pos);

                tempresult = doVCTSearchWrapper(&tempboard2, depth - 2, tempPathDefend, defendfive ? center : &item.pos, useTransTable);

                if (tempresult == VCXRESULT_SUCCESS)
                {
                    if (flag == VCXRESULT_SUCCESS)
                    {
                        //优先选步数长的
                        if (tempPath2.endStep < tempPathDefend.endStep)
                        {
                            tempPath2 = tempPathDefend;
                        }
                    }
                    else
                    {
                        flag = VCXRESULT_SUCCESS;
                        tempPath2 = tempPathDefend;
                    }
                }
                else
                {
                    flag = tempresult;
                    if (flag == VCXRESULT_UNSURE)
                    {
                        unsure = true;
                    }
                    break;
                }
            }
        }
        //else
        //{
        //    OptimalPath tempPathVCF(tempboard.getLastStep().step);
        //    tempresult = doVCFSearch(&tempboard, depth + getVCFDepth(0) - getVCTDepth(0) - 1, tempPathVCF, NULL, useTransTable);
        //    if (tempresult == VCXRESULT_TRUE)
        //    {
        //        continue;
        //    }
        //    tempPath2.endStep = tempPath.endStep;
        //    tempPath2.path.clear();
        //    tempPath2.push(Position(-1, -1));//special
        //    ChessBoard tempboard2 = tempboard;
        //    for (auto defend : defendmoves)
        //    {
        //        tempboard2.putchess(defend.pos.row, defend.pos.col, Util::otherside(side));
        //    }
        //    tempresult = doVCTSearchWrapper(&tempboard2, depth - 2, tempPath2, defendfive ? reletedset : &atackset, useTransTable);
        //    if (tempresult == VCXRESULT_UNSURE)
        //    {
        //        unsure_flag = true;
        //    }
        //    if (tempresult != VCXRESULT_TRUE)
        //    {
        //        flag = false;
        //    }
        //    needstruggle = false; //此时进行struggle是不明智的
        //}


        if (flag == VCXRESULT_SUCCESS)
        {
            if (!isVCF)//不是冲四才有挣扎的必要
            {
                set<Position> struggleset;

                if (tempPath2.endStep > tempPath2.path.size() + tempboard.lastStep.step && tempPath2.path.size() < 10)
                {
                    auto next = tempPath2.path[0];
                    ChessBoard tempboard2 = tempboard;
                    tempboard2.move(next, ban);
                    tempPath2.endStep = tempPath.endStep;
                    tempPath2.path.clear();
                    tempPath2.push(next);
                    tempresult = doVCTSearch(&tempboard2, depth - 2, tempPath2, &item.pos, false);
                }
                getNormalRelatedSet(&tempboard, struggleset, tempPath2);

                //tempboard.getAtackReletedPos(struggleset, item.pos, side);
                if (doVCTStruggleSearch(&tempboard, depth - 1, struggleset, defendfive ? center : &item.pos, useTransTable))
                {
                    continue;//挣扎成功
                }
            }

            //end struggle

            tempPath.cat(tempPath2);
            optimalPath.cat(tempPath);
            optimalPath.rating = side == startStep.getState() ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
            //vtx success
            return VCXRESULT_SUCCESS;
        }
    }

    if (unsure)
    {
        return VCXRESULT_UNSURE;
    }
    else
    {
        return VCXRESULT_FAIL;
    }
}

void GoSearchEngine::getPathFromTransTable(ChessBoard* board, MovePath& path)
{

}

bool GoSearchEngine::doVCTStruggleSearch(ChessBoard* board, int depth, set<Position>& struggleset, Position* center, bool useTransTable)
{
    if (depth - getVCTDepth(board->getLastStep().step) > 8)//连续4次struggle了
    {
        return false;
    }
    //if (board->getLastStep().step > startStep.step + currentAlphaBetaDepth)
    //{
    //    return false;
    //}
    uint8_t side = board->lastStep.getOtherSide();
    uint16_t laststep = board->lastStep.step;
    vector<StepCandidateItem> moves;
    getVCFCandidates(board, moves, struggleset);
    for (auto move : moves)
    {
        ChessBoard tempboard = *board;
        tempboard.move(move.pos, ban);
        MovePath tempPath(tempboard.lastStep.step);
        tempPath.push(move.pos);

        if (tempboard.getHighestInfo(Util::otherside(side)).chesstype == CHESSTYPE_5)
        {
            continue;
        }

        uint8_t result = VCXRESULT_SUCCESS;
        if (tempboard.getHighestInfo(side).chesstype == CHESSTYPE_5)
        {
            result = doVCTSearchWrapper(&tempboard, depth + 1, tempPath, center, useTransTable);
        }
        //else if (tempboard.getHighestInfo(side).chesstype > CHESSTYPE_D4P)
        //{
        //    result = doVCTSearchWrapper(&tempboard, depth - 1, tempPath, &atackset, useTransTable);
        //}
        //if (result == VCXRESULT_FALSE)
        //{
        //    return true;
        //}
        if (result < VCXRESULT_SUCCESS)
        {
            return true;
        }
    }
    return false;
}

void GoSearchEngine::getVCFCandidates(ChessBoard* board, vector<StepCandidateItem>& moves, set<Position>& reletedset)
{
    uint8_t side = Util::otherside(board->getLastStep().getState());
    for (auto pos : reletedset)
    {
        if (!board->canMove(pos))
        {
            continue;
        }
        if (board->getChessType(pos, side) == CHESSTYPE_4)
        {
            moves.emplace_back(pos, 1000);
        }
        else if (board->getChessType(pos, side) == CHESSTYPE_44)
        {
            moves.emplace_back(pos, 800);
        }
        else if (board->getChessType(pos, side) == CHESSTYPE_43)
        {
            moves.emplace_back(pos, 500);
        }
        else if (Util::isdead4(board->getChessType(pos, side)))
        {
            moves.emplace_back(pos, board->getRelatedFactor(pos, side));
        }
    }
}

void GoSearchEngine::getVCFCandidates(ChessBoard* board, vector<StepCandidateItem>& moves, Position* center)
{
    uint8_t side = Util::otherside(board->getLastStep().getState());

    if (center == NULL)
    {
        ForEachPosition
        {
            if (!board->canMove(pos))
            {
                continue;
            }
            if (board->getChessType(pos, side) == CHESSTYPE_4)
            {
                moves.emplace_back(pos, 1000);
            }
            else if (board->getChessType(pos, side) == CHESSTYPE_44)
            {
                moves.emplace_back(pos, 800);
            }
            else if (board->getChessType(pos, side) == CHESSTYPE_43)
            {
                moves.emplace_back(pos, 500);
            }
            else if (Util::isdead4(board->getChessType(pos, side)))
            {
                moves.emplace_back(pos, board->getRelatedFactor(pos, side));
            }
        }
    }
    else
    {
        ForRectPosition(Util::generate_rect(center->row, center->col, 5))
        {
            if (!board->canMove(pos))
            {
                continue;
            }
            if (board->getChessType(pos, side) == CHESSTYPE_4)
            {
                moves.emplace_back(pos, 1000);
            }
            else if (board->getChessType(pos, side) == CHESSTYPE_44)
            {
                moves.emplace_back(pos, 800);
            }
            else if (board->getChessType(pos, side) == CHESSTYPE_43)
            {
                moves.emplace_back(pos, 500);
            }
            else if (Util::isdead4(board->getChessType(pos, side)))
            {
                moves.emplace_back(pos, board->getRelatedFactor(pos, side));
            }
        }
    }

    //std::sort(moves.begin() + begin_index, moves.end(), CandidateItemCmp);
}

void GoSearchEngine::getVCTCandidates(ChessBoard* board, vector<StepCandidateItem>& moves, Position* center)
{
    uint8_t side = Util::otherside(board->getLastStep().getState());
    size_t begin_index = moves.size();

    if (center == NULL)
    {
        ForEachPosition
        {
            if (!board->canMove(pos))
            {
                continue;
            }
            if (board->getChessType(pos, side) == CHESSTYPE_33)
            {
                moves.emplace_back(pos, 400);
                continue;
            }
            if (Util::isalive3(board->getChessType(pos, side)))
            {
                moves.emplace_back(pos, board->getRelatedFactor(pos, side));
            }
        }
    }
    else
    {
        ForRectPosition(Util::generate_rect(center->row, center->col, 5))
        {
            if (!board->canMove(pos))
            {
                continue;
            }

            if (board->getChessType(pos, side) == CHESSTYPE_33)
            {
                moves.emplace_back(pos, 400);
                continue;
            }

            if (Util::isalive3(board->getChessType(pos, side)))
            {
                moves.emplace_back(pos, board->getRelatedFactor(pos, side));
            }

        }
    }

    //std::sort(moves.begin() + begin_index, moves.end(), CandidateItemCmp);
}


//#define EXTRA_VCT_CHESSTYPE

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