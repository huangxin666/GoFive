#include "GoSearch.h"
#include "ThreadPool.h"
#include "DBSearch.h"

#include "DBSearchPlus.h"
#include <cassert>


#define ENABLE_PV 
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
    enableDebug = setting.enableDebug;
    maxAlphaBetaDepth = setting.maxAlphaBetaDepth;
    minAlphaBetaDepth = setting.minAlphaBetaDepth;
    VCFExpandDepth = setting.VCFExpandDepth;
    VCTExpandDepth = setting.VCTExpandDepth;
    useTransTable = setting.useTransTable;
    useDBSearch = setting.useDBSearch;
    useMultiThread = setting.multithread;
    rule = setting.rule;
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
    s << " depth:" << currentAlphaBetaDepth << "-" << MaxDepth - startStep.step;
    s << " move:" << (int)nextpos.row << "," << (int)nextpos.col;
    s << " rating:" << optimalPath.rating;
    s << " time:" << duration_cast<milliseconds>(system_clock::now() - startSearchTime).count() << "ms";
    s << " path:";
    for (auto pos : optimalPath.path)
    {
        s << "[" << (int)pos.row << "," << (int)pos.col << "] ";
    }
    sendMessage(s.str());
}

void GoSearchEngine::textOutResult(MovePath& optimalPath)
{
    //optimalPath可能为空
    stringstream s;
    size_t dbtablesize = 0;
    for (int step = 1; step < currentAlphaBetaDepth; ++step)
    {
        dbtablesize += DBSearch::transTable[board->getLastStep().step + step].getTransTableSize();
    }
    s << "table:" << transTable.getTransTableSize() << (transTable.memoryValid() ? " " : "(full)") << " stable:" << dbtablesize;
    sendMessage(s.str());
    s.str("");
    s << "hit:" << transTableStat.hit << " miss:" << transTableStat.miss << " clash:" << transTableStat.clash << " cover:" << transTableStat.cover;
    sendMessage(s.str());

    s.str("");
    s << "DBSearchNode total:" << DBSearchNodeCount << " max:" << maxDBSearchNodeCount;
    sendMessage(s.str());
}

void GoSearchEngine::textForTest(MovePath& optimalPath, int priority)
{
    stringstream s;

    s << "current:" << (int)(optimalPath.path[0].row) << "," << (int)(optimalPath.path[0].col) << " rating:" << optimalPath.rating << " priority:" << priority;
    s << "\r\nbestpath:";
    for (auto index : optimalPath.path)
    {
        s << "(" << (int)index.row << "," << (int)index.col << ") ";
    }
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
    if (step < 5)
    {
        max_time = restMatchTimeMs / 10 < maxStepTimeMs ? restMatchTimeMs / 10 : maxStepTimeMs;
        suggest_time = max_time / 2;
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

void clearDBTranstable(int depth)
{
    for (int i = depth; i >= 0; --i)
    {
        DBSearch::transTable[i].clearTransTable();
    }
}

Position GoSearchEngine::getBestStep(uint64_t startSearchTime)
{
    this->startSearchTime = system_clock::from_time_t(startSearchTime);
    clearDBTranstable(board->getLastStep().step);
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
            break;
        }
        MaxDepth = startStep.step;
        MovePath temp(startStep.step);

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
        if (bestStep.value == 10000)
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
        uint8_t highest = board->getHighestType(startStep.state);
        ForEachPosition
        {
            if (!board->canMove(pos.row,pos.col)) continue;
            if (board->getChessType(pos,startStep.state) == highest)
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
    else if (doVCXExpand(board, VCXPath, useTransTable, true))
    {
        bestStep = StepCandidateItem(VCXPath.path[0], 10000);
        VCXPath.rating = CHESSTYPE_5_SCORE;
        return VCXPath;
    }
    else
    {

        searchUpper = board->getNormalCandidates(solveList, true);

        //searchUpper = board->getNormalCandidates(solveList, false, false);

        if (searchUpper == 0) solveList.size();

        if (bestStep.pos.valid())
        {
            //优先搜索置换表中记录的上一个迭代的最好着法
            for (size_t i = 0; i < searchUpper; ++i)
            {
                if (solveList[i].pos == bestStep.pos)
                {
                    solveList[i].value = 10000;
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
            if (search_time == 1)
            {
                if (solveList[index].value < 0) continue;
                else if (!Util::inRect(solveList[index].pos.row, solveList[index].pos.col, startStep.pos.row, startStep.pos.col, 5)) continue;
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
            //VCXPath.path.clear();
            //VCXPath.endStep = VCXPath.startStep = currentBoard.getLastStep().step;
            //if (doVCXExpand(&currentBoard, VCXPath, useTransTable, true))
            //{
            //    tempPath.cat(VCXPath);
            //    tempPath.rating = -CHESSTYPE_5_SCORE;
            //}
            //else
            {
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
                    int next_depth = currentAlphaBetaDepth - 1;
                    //if (Util::hasdead4(solveList[index].type)) next_depth += 2;
                    //else if (Util::isalive3or33(solveList[index].type)) next_depth += 1;
                    doAlphaBetaSearch(&currentBoard, next_depth, base_alpha, base_beta, tempPath, startStep.pos, useTransTable);
                }
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
                //if (rule != FREESTYLE && tempPath.rating > -CHESSTYPE_5_SCORE)
                //{
                //    //检查敌人是否能VCT
                //    MovePath VCTPath(board->getLastStep().step);
                //    VCTPath.push(solveList[index].pos);
                //    if (doVCXExpand(&currentBoard, VCTPath, NULL, useTransTable, true) == VCXRESULT_SUCCESS)
                //    {
                //        if (optimalPath.rating < -CHESSTYPE_5_SCORE)
                //        {
                //            optimalPath = VCTPath;
                //            optimalPath.rating = -CHESSTYPE_5_SCORE;
                //            base_alpha = -CHESSTYPE_5_SCORE;
                //            foundPV = true;
                //        }
                //        else if (optimalPath.rating == -CHESSTYPE_5_SCORE && VCTPath.endStep > optimalPath.endStep)
                //        {
                //            optimalPath = VCTPath;
                //            optimalPath.rating = -CHESSTYPE_5_SCORE;
                //        }
                //        tempPath = VCTPath;
                //    }
                //    else
                //    {
                //        base_alpha = tempPath.rating;
                //        optimalPath = tempPath;
                //        foundPV = true;
                //    }
                //}
                //else
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
            //    textForTest(tempPath, solveList[index].value);
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

void GoSearchEngine::doAlphaBetaSearch(ChessBoard* board, int depth, int alpha, int beta, MovePath& optimalPath, Position lastlastPos, bool useTransTable)
{
    //for multithread
    //if (alpha < base_alpha) alpha = base_alpha;
    //if (beta > base_beta) beta = base_beta;
    //

    uint8_t side = board->getLastStep().getOtherSide();
    uint8_t otherside = board->getLastStep().state;
    ChessStep lastpos = board->getLastStep();
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
    else if (doVCXExpand(board, VCXPath, useTransTable, depth > currentAlphaBetaDepth - 2))
    {
        optimalPath.cat(VCXPath);
        optimalPath.rating = isPlayerSide(side) ? -CHESSTYPE_5_SCORE : CHESSTYPE_5_SCORE;
        return;
    }
    else
    {
        searchUpper = board->getNormalCandidates(moves, !isPlayerSide(side));
        if (searchUpper == 0) searchUpper = moves.size();
    }


    if (has_best_pos)
    {
        //优先搜索置换表中记录的上一个迭代的最好着法
        for (size_t i = 0; i < searchUpper; ++i)
        {
            if (moves[i].pos == data.bestStep)
            {
                moves[i].value = 10000;
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
            if (search_time == 1)
            {
                if (!Util::inRect(moves[move_index].pos.row, moves[move_index].pos.col, lastpos.getRow(), lastpos.getCol(), 5)) continue;
                else if (moves[move_index].value < 0) continue;
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
                    doAlphaBetaSearch(&currentBoard, depth - 1, beta - 1, beta, tempPath, lastpos.pos, useTransTable);//极小窗口剪裁
                    //假设当前是最好的，没有任何其他的会比当前的PV好（小于beta）
                    if (tempPath.rating < beta && tempPath.rating > alpha)//失败，使用完整窗口
                    {
                        tempPath.path.clear();
                        tempPath.push(moves[move_index].pos);
                        doAlphaBetaSearch(&currentBoard, depth - 1, alpha, beta, tempPath, lastpos.pos, useTransTable);
                    }
                }
                else
#endif // ENABLE_PV
                {
                    int next_depth = depth - 1;
                    //if (Util::hasdead4(moves[move_index].type)) next_depth += 2;
                    //else if (Util::isalive3or33(moves[move_index].type)) next_depth += 1;
                    doAlphaBetaSearch(&currentBoard, next_depth, alpha, beta, tempPath, lastpos.pos, useTransTable);
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
                    doAlphaBetaSearch(&currentBoard, depth - 1, alpha, alpha + 1, tempPath, lastpos.pos, useTransTable);//极小窗口剪裁
                    //假设当前是最好的，没有任何其他的会比当前的PV好（大于alpha）
                    if (tempPath.rating > alpha && tempPath.rating < beta)
                    {
                        tempPath.path.clear();
                        tempPath.push(moves[move_index].pos);
                        doAlphaBetaSearch(&currentBoard, depth - 1, alpha, beta, tempPath, lastpos.pos, useTransTable);
                    }
                }
                else
#endif // ENABLE_PV
                {
                    int next_depth = depth - 1;
                    //if (Util::hasdead4(moves[move_index].type)) next_depth += 2;
                    //else if (Util::isalive3or33(moves[move_index].type)) next_depth += 1;
                    doAlphaBetaSearch(&currentBoard, next_depth, alpha, beta, tempPath, lastpos.pos, useTransTable);
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

bool GoSearchEngine::doVCXExpand(ChessBoard* board, MovePath& optimalPath, bool useTransTable, bool extend)
{
    if (useDBSearch /*&& (!extend)*/)
    {
        TransTableDBData data;
        //if (useTransTable)
        {
            if (DBSearch::transTable[board->getLastStep().step].get(board->getBoardHash().hash_key, data))
            {
                if (data.checkHash == board->getBoardHash().check_key)
                {
                    transTableStat.hit++;
                    return data.result;
                }
            }
        }
        DBSearch::node_count = 0;
        DBSearch dbs(board, rule, 2);
        bool ret = dbs.doDBSearch(optimalPath.path);
        DBSearchNodeCount += DBSearch::node_count;
        maxDBSearchNodeCount = DBSearch::node_count > maxDBSearchNodeCount ? DBSearch::node_count : maxDBSearchNodeCount;
        MaxDepth = MaxDepth < (board->getLastStep().step + dbs.getMaxDepth() * 2) ? (board->getLastStep().step + dbs.getMaxDepth() * 2) : MaxDepth;
        if (ret)
        {
            optimalPath.endStep = optimalPath.startStep + (uint16_t)optimalPath.path.size();
        }

        //if (useTransTable)
        {
            data.result = ret;
            data.checkHash = board->getBoardHash().check_key;
            DBSearch::transTable[board->getLastStep().step].insert(board->getBoardHash().hash_key, data);
        }
        return ret;
    }
    else
    {
        DBSearchPlus dbs(board, rule, 2, true);
        bool ret = dbs.doDBSearchPlus(optimalPath.path);
        if (ret)
        {
            optimalPath.endStep = optimalPath.startStep + (uint16_t)optimalPath.path.size();
        }
        return ret;
    }
    return false;
}

//else if (!(ChessBoard::rule && board->getLastStep().getOtherSide() == PIECE_BLACK)&&board->getChessType(pos, side) == CHESSTYPE_D3)
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