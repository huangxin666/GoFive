#include "GoSearch.h"
#include "ThreadPool.h"
#include "DBSearch.h"
#include <cassert>


//#define ENABLE_PV 
//#define ENABLE_NULLMOVE
#define GOSEARCH_DEBUG
mutex GoSearchEngine::message_queue_lock;
queue<string> GoSearchEngine::message_queue;

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
    transTable.setMaxMemory((maxMemoryBytes) / 2);
    transTableVCX.setMaxMemory((maxMemoryBytes) / 2);
    enableDebug = setting.enableDebug;
    maxAlphaBetaDepth = setting.maxAlphaBetaDepth;
    minAlphaBetaDepth = setting.minAlphaBetaDepth;
    VCFExpandDepth = setting.VCFExpandDepth;
    VCTExpandDepth = setting.VCTExpandDepth;
    useTransTable = setting.useTransTable;
    useDBSearch = setting.useDBSearch;
    useMultiThread = setting.multithread;
    rule = setting.ban;
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

    s.str("");
    s << "DBSearchNode:" << DBSearchNodeCount;
    sendMessage(s.str());
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
        MovePath temp(startStep.step);
        //find_winning_move = true;
        //if (findWinningMove(board, temp))
        //{
        //    textOutIterativeInfo(temp);
        //    bestPath = temp;
        //    break;
        //}
        //find_winning_move = false;
        temp = selectBestMove(board, bestStep);
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
        uint8_t highest = board->getHighestType(startStep.getState());
        ForEachPosition
        {
            if (!board->canMove(pos.row,pos.col)) continue;
            if (board->getChessType(pos,startStep.getState()) == highest)
            {
                bestPath.path.push_back(pos);
                break;
            }
        }
    }

    return bestPath.path[0];
}

static int base_alpha = INT_MIN, base_beta = INT_MAX;
static MovePath optimalPath(0);

void GoSearchEngine::solveBoardForEachThread(PVSearchData data)
{

}

MovePath GoSearchEngine::selectBestMove(ChessBoard* board, StepCandidateItem& bestStep)
{
    base_alpha = INT_MIN, base_beta = INT_MAX;
    optimalPath.startStep = startStep.step;
    optimalPath.endStep = startStep.step;
    optimalPath.path.clear();
    optimalPath.rating = INT_MIN;
    size_t searchUpper = 0;

    uint8_t side = board->getLastStep().getOtherSide();

    //PieceInfo otherhighest = board->getHighestInfo(Util::otherside(side));
    //PieceInfo selfhighest = board->getHighestInfo(side);
    MovePath VCXPath(board->getLastStep().step);

    vector<StepCandidateItem> solveList;
    if (board->hasChessType(side, CHESSTYPE_5))
    {
        ForEachPosition
        {
            if (!board->canMove(pos.row,pos.col)) continue;
            if (board->getChessType(pos,side) == CHESSTYPE_5)
            {
                optimalPath.push(pos);
                optimalPath.rating = 10000;
                bestStep = StepCandidateItem(pos, 10000);
                break;
            }
        }
        return optimalPath;
    }
    else if (board->hasChessType(Util::otherside(side), CHESSTYPE_5))//敌方马上5连
    {
        ForEachPosition
        {
            if (!board->canMove(pos.row,pos.col)) continue;
            if (board->getChessType(pos,Util::otherside(side)) == CHESSTYPE_5)
            {
                if (board->getChessType(pos, side) == CHESSTYPE_BAN)
                {
                    optimalPath.rating = -CHESSTYPE_5_SCORE;
                }
                else
                {
                    optimalPath.rating = board->getGlobalEvaluate(getAISide());
                }
                optimalPath.push(pos);
                bestStep = StepCandidateItem(pos, 10000);
                break;
            }
        }
        return optimalPath;
    }
    else if (doVCXExpand(board, VCXPath, NULL, useTransTable, true) == VCXRESULT_SUCCESS)
    {
        bestStep = StepCandidateItem(VCXPath.path[0], 10000);
        VCXPath.rating = CHESSTYPE_5_SCORE;
        return VCXPath;
    }
    else
    {
        //ChessBoard tempBoard = *board;
        //tempBoard.moveNull();
        //if (doVCXExpand(&tempBoard, VCXPath, NULL, false, true) == VCXRESULT_SUCCESS)
        //{
        //    set<Position> related;
        //    getRelatedSetFromWinningSequence(&tempBoard, related, VCXPath);
        //    for (auto it = related.begin(); it != related.end(); ++it)
        //    {
        //        //if (board->getChessType(*it, Util::otherside(side)) > CHESSTYPE_D3)
        //        solveList.emplace_back(*it, 0);
        //    }
        //    searchUpper = solveList.size();
        //}
        //else
        {
            searchUpper = board->getNormalCandidates(solveList, false, false);
        }
        //searchUpper = board->getNormalCandidates(solveList, false, false);

        if (searchUpper == 0) solveList.size();

        if (bestStep.pos.valid())
        {
            //优先搜索置换表中记录的上一个迭代的最好着法
            for (size_t i = 0; i < searchUpper; ++i)
            {
                if (solveList[i].pos == bestStep.pos)
                {
                    solveList[i].priority = 10000;
                    std::sort(solveList.begin(), solveList.begin() + searchUpper, CandidateItemCmp);
                    break;
                }
            }
        }
    }

    if (solveList.empty())
    {
        return optimalPath;
    }


    bool foundPV = false;
    size_t index = 0;

    for (int search_time = 0; search_time < 2; ++search_time)
    {
        //first search
        for (; index < searchUpper; ++index)
        {
            if (search_time == 1 && !Util::inRect(solveList[index].pos.row, solveList[index].pos.col, startStep.pos.row, startStep.pos.col, 5))
            {
                continue;
            }

            if (useMultiThread && foundPV)
            {
                PVSearchData data(this, solveList.begin() + index);
                if (search_time == 1) data.struggle = true;
                ThreadPool::getInstance()->run(bind(solveBoardForEachThread, data));
                continue;
            }

            MovePath tempPath(board->getLastStep().step);
            tempPath.push(solveList[index].pos);
            ChessBoard currentBoard = *board;
            currentBoard.move(solveList[index].pos, rule);
#ifdef ENABLE_PV
            if (foundPV)
            {
                //假设当前是最好的，没有任何其他的会比当前的PV好（大于alpha）
                doAlphaBetaSearch(&currentBoard, currentAlphaBetaDepth - 1, base_alpha, base_alpha + 1, tempPath, startStep.pos, useTransTable);//极小窗口剪裁 
                if (tempPath.rating > base_alpha && tempPath.rating < base_beta)//使用完整窗口
                {
                    tempPath.path.clear();
                    tempPath.push(solveList[index].pos);
                    doAlphaBetaSearch(&currentBoard, currentAlphaBetaDepth - 1, base_alpha, base_beta, tempPath, startStep.pos, useTransTable);
                }

            }
            else
#endif
            {
                doAlphaBetaSearch(&currentBoard, currentAlphaBetaDepth - 1, base_alpha, base_beta, tempPath, startStep.pos, useTransTable);
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
                if (rule != FREESTYLE && tempPath.rating > -CHESSTYPE_5_SCORE)
                {
                    //检查敌人是否能VCT
                    MovePath VCTPath(board->getLastStep().step);
                    VCTPath.push(solveList[index].pos);
                    if (doVCXExpand(&currentBoard, VCTPath, NULL, useTransTable, true) == VCXRESULT_SUCCESS)
                    {
                        if (optimalPath.rating < -CHESSTYPE_5_SCORE)
                        {
                            optimalPath = VCTPath;
                            optimalPath.rating = -CHESSTYPE_5_SCORE;
                            base_alpha = -CHESSTYPE_5_SCORE;
                            foundPV = true;
                        }
                        else if (optimalPath.rating == -CHESSTYPE_5_SCORE && VCTPath.endStep > optimalPath.endStep)
                        {
                            optimalPath = VCTPath;
                            optimalPath.rating = -CHESSTYPE_5_SCORE;
                        }
                        tempPath = VCTPath;
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

            //if (enableDebug)
            //{
            //    textForTest(tempPath, solveList[index].priority);
            //}

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

        if (searchUpper < solveList.size() && optimalPath.rating == -CHESSTYPE_5_SCORE)
        {
            searchUpper = solveList.size();
            continue;
        }
        else
        {
            break;
        }
    }

    bestStep = StepCandidateItem(optimalPath.path[0], 0);
    return optimalPath;
}

bool GoSearchEngine::findWinningMove(ChessBoard* board, MovePath& optimalPath)
{
    int base_alpha = INT_MIN, base_beta = INT_MAX;
    optimalPath.startStep = startStep.step;
    optimalPath.endStep = startStep.step;
    optimalPath.path.clear();
    optimalPath.rating = INT_MIN;
    size_t searchUpper = 0;

    uint8_t side = board->getLastStep().getOtherSide();

    MovePath VCXPath(board->getLastStep().step);

    vector<StepCandidateItem> solveList;
    if (board->hasChessType(side, CHESSTYPE_5))
    {
        ForEachPosition
        {
            if (!board->canMove(pos.row,pos.col)) continue;
            if (board->getChessType(pos,side) == CHESSTYPE_5)
            {
                optimalPath.push(pos);
                optimalPath.rating = 10000;
                return true;
            }
        }
        return false;
    }
    else if (board->hasChessType(Util::otherside(side), CHESSTYPE_5))//敌方马上5连
    {
        return false;
    }
    else if (doVCXExpand(board, VCXPath, NULL, useTransTable, true) == VCXRESULT_SUCCESS)
    {
        optimalPath = VCXPath;
        optimalPath.rating = CHESSTYPE_5_SCORE;
        return true;
    }
    else
    {

        searchUpper = board->getNormalCandidates(solveList, true, true);

        if (searchUpper == 0) solveList.size();
    }

    if (solveList.empty())
    {
        return false;
    }

    int fix_depth = currentAlphaBetaDepth/* / 2 + 1*/;

    for (size_t index = 0; index < searchUpper; ++index)
    {
        MovePath tempPath(board->getLastStep().step);
        tempPath.push(solveList[index].pos);
        ChessBoard currentBoard = *board;
        currentBoard.move(solveList[index].pos, rule);

        doAlphaBetaSearch(&currentBoard, fix_depth - 1, base_alpha, base_beta, tempPath, startStep.pos, false);
        if (tempPath.rating == CHESSTYPE_5_SCORE)
        {
            optimalPath = tempPath;
            return true;
        }
        //处理超时
        if (global_isOverTime)
        {
            if (optimalPath.rating == INT_MIN)
            {
                optimalPath = tempPath;
            }
            return false;
        }

        if (tempPath.rating > optimalPath.rating)
        {
            base_alpha = tempPath.rating;
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

    return optimalPath.rating == CHESSTYPE_5_SCORE ? true : false;
}

void GoSearchEngine::doAlphaBetaSearch(ChessBoard* board, int depth, int alpha, int beta, MovePath& optimalPath, Position lastlastPos, bool useTransTable)
{
    //for multithread
    //if (alpha < base_alpha) alpha = base_alpha;
    //if (beta > base_beta) beta = base_beta;
    //

    uint8_t side = board->getLastStep().getOtherSide();
    uint8_t otherside = board->getLastStep().getState();
    Position lastpos = board->getLastStep().pos;
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

    uint8_t move_index = 0;
    size_t searchUpper = 0;
    MovePath VCXPath(board->getLastStep().step);
    //set<Position> reletedset;
    vector<StepCandidateItem> moves;

    MovePath bestPath(board->getLastStep().step);
    bestPath.rating = isPlayerSide(side) ? INT_MAX : INT_MIN;

    if (board->hasChessType(side, CHESSTYPE_5))
    {
        optimalPath.rating = isPlayerSide(side) ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
        return;
    }
    else if (depth <= 0)
    {
        optimalPath.rating = board->getGlobalEvaluate(getAISide(), AIweight);
        return;
    }
    else if (global_isOverTime || duration_cast<milliseconds>(std::chrono::system_clock::now() - startSearchTime).count() > maxStepTimeMs)//超时
    {
        optimalPath.rating = -CHESSTYPE_5_SCORE + 111;
        global_isOverTime = true;
        return;
    }
    else if (board->hasChessType(otherside, CHESSTYPE_5))//防5连
    {
        ForEachPosition
        {
            if (board->canMove(pos) && board->getChessType(pos, otherside) == CHESSTYPE_5)
            {
                if (board->getChessType(pos, side) == CHESSTYPE_BAN)//触发禁手，otherside赢了
                {
                    optimalPath.rating = isPlayerSide(side) ? CHESSTYPE_5_SCORE : -CHESSTYPE_5_SCORE;
                    return;
                }
                else
                {
                    moves.emplace_back(pos, 10000);
                    searchUpper = moves.size();
                }
                break;
            }
        }
    }
    else if (/*(find_winning_move || side == getPlayerSide()) && */doVCXExpand(board, VCXPath, NULL, useTransTable, false) == VCXRESULT_SUCCESS)
    {
        optimalPath.cat(VCXPath);
        optimalPath.rating = isPlayerSide(side) ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
        return;
    }
    else
    {
        if (find_winning_move) searchUpper = board->getNormalCandidates(moves, side == getAISide() ? true : false, find_winning_move);
        else searchUpper = board->getNormalCandidates(moves, false, find_winning_move);
    }


    if (has_best_pos)
    {
        //优先搜索置换表中记录的上一个迭代的最好着法
        for (size_t i = 0; i < searchUpper; ++i)
        {
            if (moves[i].pos == data.bestStep)
            {
                moves[i].priority = 10000;
                std::sort(moves.begin(), moves.begin() + searchUpper, CandidateItemCmp);
                break;
            }
        }
    }


    bool foundPV = false;
    data.type = TRANSTYPE_EXACT;

    if (continue_flag && data.continue_index < searchUpper)
    {
        move_index = data.continue_index;
        bestPath.rating = data.value;
        bestPath.push(data.bestStep);
        bestPath.endStep = data.depth + startStep.step;
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


    for (int search_time = 0; search_time < 2; ++search_time)
    {
        //first search
        for (; move_index < searchUpper; ++move_index)
        {
            if (search_time == 1 && !Util::inRect(moves[move_index].pos.row, moves[move_index].pos.col, lastpos.row, lastpos.col, 5))
            {
                continue;
            }

            MovePath tempPath(board->getLastStep().step);
            tempPath.push(moves[move_index].pos);
            ChessBoard currentBoard = *board;
            currentBoard.move(moves[move_index].pos, rule);

            //剪枝
            if (isPlayerSide(side))//build player
            {
#ifdef ENABLE_PV
                if (foundPV)
                {
                    doAlphaBetaSearch(&currentBoard, depth - 1, beta - 1, beta, tempPath, lastpos, useTransTable);//极小窗口剪裁
                    //假设当前是最好的，没有任何其他的会比当前的PV好（小于beta）
                    if (tempPath.rating < beta && tempPath.rating > alpha)//失败，使用完整窗口
                    {
                        tempPath.path.clear();
                        tempPath.push(moves[move_index].pos);
                        doAlphaBetaSearch(&currentBoard, depth - 1, alpha, beta, tempPath, lastpos, useTransTable);
                    }
                }
                else
#endif // ENABLE_PV
                {
                    doAlphaBetaSearch(&currentBoard, depth - 1, alpha, beta, tempPath, lastpos, useTransTable);
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
                    if (search_time == 1) move_index = (uint8_t)moves.size();
                    break;
                }
            }
            else // build AI
            {
#ifdef ENABLE_PV
                if (foundPV)
                {
                    doAlphaBetaSearch(&currentBoard, depth - 1, alpha, alpha + 1, tempPath, lastpos, useTransTable);//极小窗口剪裁
                    //假设当前是最好的，没有任何其他的会比当前的PV好（大于alpha）
                    if (tempPath.rating > alpha && tempPath.rating < beta)
                    {
                        tempPath.path.clear();
                        tempPath.push(moves[move_index].pos);
                        doAlphaBetaSearch(&currentBoard, depth - 1, alpha, beta, tempPath, lastpos, useTransTable);
                    }
                }
                else
#endif // ENABLE_PV
                {
                    doAlphaBetaSearch(&currentBoard, depth - 1, alpha, beta, tempPath, lastpos, useTransTable);
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
                    if (search_time == 1) move_index = (uint8_t)moves.size();
                    break;
                }
            }
        }
        if (searchUpper < moves.size()
            && ((isPlayerSide(side) && bestPath.rating == CHESSTYPE_5_SCORE) || (!isPlayerSide(side) && bestPath.rating == -CHESSTYPE_5_SCORE)))
        {
            //二次搜索
            searchUpper = moves.size();
            continue;
        }
        break;
    }


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

VCXRESULT GoSearchEngine::doVCXExpand(ChessBoard* board, MovePath& optimalPath, Position* center, bool useTransTable, bool VCTExpand)
{

    if (rule == FREESTYLE && useDBSearch)
    {
        TransTableVCXData data;
        //if (useTransTable)
        {
            if (transTableVCX.get(board->getBoardHash().hash_key, data))
            {
                if (data.checkHash == board->getBoardHash().check_key)
                {
                    optimalPath.endStep += data.VCTdepth;
                    return data.VCTflag;
                }
            }
        }
        DBSearch::node_count = 0;
        DBSearch dbs(board, rule, 2);
        bool ret = dbs.doDBSearch(optimalPath.path);
        DBSearchNodeCount += DBSearch::node_count;
        data.VCTflag = VCXRESULT_FAIL;
        if (ret)
        {
            vector<DBMetaOperator> sequence;
            data.VCTflag = VCXRESULT_SUCCESS;
            data.VCTdepth = optimalPath.path.size();
            optimalPath.endStep = optimalPath.startStep + (uint16_t)optimalPath.path.size();
        }

        //if (useTransTable)
        {
            data.checkHash = board->getBoardHash().check_key;
            transTableVCX.insert(board->getBoardHash().hash_key, data);
        }
        return data.VCTflag;
    }
    else
    {
        MovePath VCFPath(board->getLastStep().step);
        MovePath VCTPath(board->getLastStep().step);
        if (doVCFSearchWrapper(board, getVCFDepth(board->getLastStep().step), VCFPath, NULL, true) == VCXRESULT_SUCCESS)
        {
            optimalPath.cat(VCFPath);
            return VCXRESULT_SUCCESS;
        }
        else if ((VCTExpand || rule == FREESTYLE) && doVCTSearchWrapper(board, getVCTDepth(board->getLastStep().step), VCTPath, NULL, true) == VCXRESULT_SUCCESS)
        {
            optimalPath.cat(VCTPath);
            return VCXRESULT_SUCCESS;
        }
    }
    return VCXRESULT_FAIL;
}

void GoSearchEngine::getNormalRelatedSet(ChessBoard* board, set<Position>& reletedset, MovePath& optimalPath)
{
    uint8_t defendside = board->getLastStep().getOtherSide();
    uint8_t atackside = board->getLastStep().getState();//laststep的进攻成功，现在要找防守步

    vector<Position> path;
    path.push_back(board->getLastStep().pos);
    path.insert(path.end(), optimalPath.path.begin(), optimalPath.path.end());

    ChessBoard tempboard = *board;
    for (size_t i = 0; i < path.size(); i++)
    {
        //tempboard.move(path[i]);
        reletedset.insert(path[i]);
        tempboard.getAtackReletedPos(reletedset, path[i], atackside);//相关点是对于进攻而言的，防守策略根据进攻的相关点去防守
        tempboard.getDefendReletedPos(reletedset, path[i], atackside);//相关点是对于进攻而言的，防守策略根据进攻的相关点去防守
        i++;
        if (i < path.size())
        {
            reletedset.insert(path[i]);
        }
    }
}

void GoSearchEngine::getRelatedSetFromWinningSequence(ChessBoard* board, set<Position>& reletedset, MovePath& optimalPath)
{
    uint8_t defendside = board->getLastStep().getOtherSide();
    uint8_t atackside = board->getLastStep().getState();//laststep的进攻成功，现在要找防守步

    ChessBoard tempboard = *board;
    for (size_t i = 0; i < optimalPath.path.size(); i++)
    {
        reletedset.insert(optimalPath.path[i]);
        tempboard.getAtackReletedPos(reletedset, optimalPath.path[i], atackside);//相关点是对于进攻而言的，防守策略根据进攻的相关点去防守
        tempboard.getDefendReletedPos(reletedset, optimalPath.path[i], atackside);//相关点是对于进攻而言的，防守策略根据进攻的相关点去防守
        i++;
        if (i < optimalPath.path.size())
        {
            reletedset.insert(optimalPath.path[i]);
        }
    }
}

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
    if (board->hasChessType(side, CHESSTYPE_5))
    {
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
    board->getVCFCandidates(moves, center);
    std::sort(moves.begin(), moves.end(), CandidateItemCmp);

    for (auto item : moves)
    {
        ChessBoard tempboard = *board;
        uint8_t direction = tempboard.getChessDirection(item.pos, side);
        uint8_t atackType = tempboard.getLayer2(item.pos.row, item.pos.col, side, direction);
        tempboard.move(item.pos, rule);//冲四

        MovePath tempPath(board->getLastStep().step);
        tempPath.push(item.pos);

        if (tempboard.hasChessType(Util::otherside(side), CHESSTYPE_5))
        {
            continue;
        }

        if (!tempboard.hasChessType(side, CHESSTYPE_5))//5连是禁手
        {
            continue;
        }

        vector<Position> reply;
        tempboard.getThreatReplies(item.pos, atackType, direction, reply);
        if (reply.empty())
        {
            optimalPath.rating = side == startStep.getState() ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
            optimalPath.cat(tempPath);
            return VCXRESULT_SUCCESS;
        }

        if (tempboard.getChessType(reply[0], Util::otherside(side)) == CHESSTYPE_BAN)//敌方触发禁手，VCF成功
        {
            optimalPath.rating = side == startStep.getState() ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
            optimalPath.cat(tempPath);
            return VCXRESULT_SUCCESS;
        }
        tempPath.push(reply[0]);//防五连
        tempboard.move(reply[0], rule);

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

        uint8_t result = doVCFSearch(&tempboard, depth - 2, tempPath, &item.pos, useTransTable);
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
    if (board->hasChessType(side, CHESSTYPE_5))
    {
        optimalPath.rating = side == startStep.getState() ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
        return VCXRESULT_SUCCESS;
    }
    else if (board->hasChessType(Util::otherside(side), CHESSTYPE_5))
    {
        ForEachPosition
        {
            if (!board->canMove(pos.row,pos.col)) continue;
            if (board->getChessType(pos,Util::otherside(side)) == CHESSTYPE_5)
            {
                if (board->getChessType(pos, side) == CHESSTYPE_BAN)
                {
                    return VCXRESULT_FAIL;
                }
                defendfive = true;
                moves.emplace_back(pos, 10);
                break;
            }
        }
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
    else if (doVCFSearch(board, depth + getVCFDepth(0) - getVCTDepth(0), VCFPath, center, useTransTable) == VCXRESULT_SUCCESS)
    {
        optimalPath.cat(VCFPath);
        optimalPath.rating = VCFPath.rating;
        return VCXRESULT_SUCCESS;
    }
    else
    {
        board->getVCTCandidates(moves, center);
        board->getVCFCandidates(moves, center);
        std::sort(moves.begin(), moves.end(), CandidateItemCmp);
    }

    bool unsure = false;//默认是false 只有要一个unsure 整个都是unsure

    VCXRESULT tempresult;
    for (auto item : moves)
    {
        ChessBoard tempboard = *board;
        uint8_t direction = tempboard.getChessDirection(item.pos, side);
        uint8_t atackType = tempboard.getLayer2(item.pos.row, item.pos.col, side, direction);
        tempboard.move(item.pos, rule);
        bool isVCF;//本步是否是冲四
        if (tempboard.getHighestType(side) < CHESSTYPE_33)//无法造成VCT威胁 & //防假活三，连环禁手
        {
            continue;
        }

        if (tempboard.hasChessType(Util::otherside(side), CHESSTYPE_5))//失败，对方有5连
        {
            continue;
        }

        MovePath tempPath(board->getLastStep().step);
        tempPath.push(item.pos);

        vector<StepCandidateItem> defendmoves;
        

        if (Util::hasdead4(atackType))
        {
            vector<Position> reply;
            tempboard.getThreatReplies(item.pos, atackType, direction, reply);
            if (reply.empty())
            {
                optimalPath.rating = side == startStep.getState() ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
                optimalPath.cat(tempPath);
                return VCXRESULT_SUCCESS;
            }

            if (tempboard.getChessType(reply[0], Util::otherside(side)) == CHESSTYPE_BAN)//敌方触发禁手，VCF成功
            {
                optimalPath.rating = side == startStep.getState() ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
                optimalPath.cat(tempPath);
                return VCXRESULT_SUCCESS;
            }
            isVCF = true;
            defendmoves.emplace_back(reply[0], 10000);
        }
        else if (Util::isalive3(atackType))
        {
            MovePath tempPathVCF(tempboard.getLastStep().step);
            tempresult = doVCFSearchWrapper(&tempboard, depth - 1 + getVCFDepth(0) - getVCTDepth(0), tempPathVCF, NULL, useTransTable);
            if (tempresult == VCXRESULT_SUCCESS)
            {
                continue;
            }

            isVCF = false;

            vector<Position> reply;
            tempboard.getThreatReplies(item.pos, atackType, direction, reply);
            if (reply.empty())
            {
                optimalPath.rating = side == startStep.getState() ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
                optimalPath.cat(tempPath);
                return VCXRESULT_SUCCESS;
            }

            for (auto r : reply)
            {
                defendmoves.emplace_back(r, 100);
            }
        }
        else 
        {
            continue;
            if (defendfive)
            {
                uint8_t threat = tempboard.getHighestType(side);
                Position threat_pos;
                ForEachPosition
                {
                    if (!tempboard.canMove(pos.row,pos.col)) continue;
                    if (tempboard.getChessType(pos, side) == threat)
                    {
                    tempboard.getFourkillDefendCandidates(pos, defendmoves, rule);
                    }
                }
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
                tempboard2.move(defend.pos, rule);
                tempPathDefend.push(defend.pos);

                tempresult = doVCTSearch(&tempboard2, depth - 2, tempPathDefend, defendfive ? center : &item.pos, useTransTable);

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
                    tempboard2.move(next, rule);
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
    board->getVCFCandidates(moves, struggleset);
    for (auto move : moves)
    {
        ChessBoard tempboard = *board;
        tempboard.move(move.pos, rule);
        MovePath tempPath(tempboard.lastStep.step);
        tempPath.push(move.pos);

        if (tempboard.hasChessType(Util::otherside(side), CHESSTYPE_5))
        {
            continue;
        }

        uint8_t result = VCXRESULT_SUCCESS;
        if (tempboard.hasChessType(side, CHESSTYPE_5))
        {
            result = doVCTSearch(&tempboard, depth + 1, tempPath, center, useTransTable);
        }
        //else if (tempboard.getHighestInfo(side).chesstype > CHESSTYPE_D4P)
        //{
        //    result = doVCTSearchWrapper(&tempboard, depth - 1, tempPath, &atackset, useTransTable);
        //}
        //if (result == VCXRESULT_FALSE)
        //{
        //    return true;
        //}
        if (result != VCXRESULT_SUCCESS)
        {
            return true;
        }
    }
    return false;
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