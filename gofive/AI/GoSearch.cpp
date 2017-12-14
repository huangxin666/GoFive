#include "GoSearch.h"
#include "ThreadPool.h"
#include "DBSearch.h"

#include "DBSearchPlus.h"
#include <cassert>

#define USE_NEGAMAX
#define ENABLE_PV 
//#define ENABLE_NULLMOVE
#define GOSEARCH_DEBUG
mutex GoSearchEngine::message_queue_lock;
//TransTableArray<TransTableData> GoSearchEngine::transTable;

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
}

void GoSearchEngine::applySettings(AISettings setting)
{
    msgCallBack = setting.msgfunc;
    maxStepTimeMs = setting.maxStepTimeMs;
    restMatchTimeMs = setting.restMatchTimeMs;
    maxMemoryBytes = setting.maxMemoryBytes;
    transTable.init((maxMemoryBytes) / 3);
    DBSearch::transTable.init(maxMemoryBytes / 3 * 2);
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
    s << "depth: " << currentAlphaBetaDepth << "-" << MaxDepth - startStep.step;
    s << " [" << (int)nextpos.row << "," << (int)nextpos.col << "]";
    s << " rating:" << optimalPath.rating;
    s << " time:" << duration_cast<milliseconds>(system_clock::now() - startSearchTime).count() << "ms";
    s << " node:" << node_count;
    s << " path:";
    for (auto pos : optimalPath.path)
    {
        s << "[" << (int)pos.row << "," << (int)pos.col << "]";
    }
    node_count_total += node_count;
    node_count = 0;
    sendMessage(s.str());
}

void GoSearchEngine::textOutResult(MovePath& optimalPath)
{
    stringstream s;
    //optimalPath可能为空
    if (optimalPath.path.size() == 0)
    {
        sendMessage(string("no path"));
        return;
    }
    else
    {
        Position nextpos = optimalPath.path[0];
        s << "depth:" << currentAlphaBetaDepth;
        s << " [" << (int)nextpos.row << "," << (int)nextpos.col << "]";
    }
    s << " complex:" << complexity << " ab:" << node_count_total << " leaf:" << leaf_node_count << " scout:" << node_count_scout << " quies:" << node_count_quies << " null:" << null_prune_success_count
        << " bestmove:" << hit_bestmove_count;
    sendMessage(s.str());
    s.str("");
    s << "hit:" << transTableStat.hit << " miss:" << transTableStat.miss << " clash:" << transTableStat.clash << " cover:" << transTableStat.cover;
    sendMessage(s.str());
    s.str("");
    s << "DBSearch count:" << dbsearch_count << " total:" << DBSearchNodeCount << " max:" << maxDBSearchNodeCount;
    sendMessage(s.str());
}

void GoSearchEngine::textForTest(MovePath& optimalPath, int priority)
{
    stringstream s;

    s << "current:" << (int)(optimalPath.path[0].row) << "," << (int)(optimalPath.path[0].col) << " rating:" << optimalPath.rating << " priority:" << priority;
    //s << "\r\nbestpath:";
    //for (auto index : optimalPath.path)
    //{
    //    s << "(" << (int)index.row << "," << (int)index.col << ") ";
    //}
    sendMessage(s.str());
}

void GoSearchEngine::textOutAllocateTime(uint32_t max_time, uint32_t suggest_time)
{
    stringstream s;
    s << "time max:" << maxStepTimeMs << "ms suggest:" << suggest_time << "ms";
    sendMessage(s.str());
}

void GoSearchEngine::allocatedTime(uint32_t& max_time, uint32_t&suggest_time)
{
    int step = startStep.step;
    if (step < 5)
    {
        max_time = restMatchTimeMs / 10 < maxStepTimeMs ? restMatchTimeMs / 10 : maxStepTimeMs;
        suggest_time = max_time / 3 * 2;
    }
    else if (step < 20)
    {
        if (restMatchTimeMs < maxStepTimeMs)
        {
            max_time = restMatchTimeMs / 8;
            suggest_time = restMatchTimeMs / 16;
        }
        else if (restMatchTimeMs / 8 < maxStepTimeMs)
        {
            max_time = restMatchTimeMs / 8;
            suggest_time = restMatchTimeMs / 24;
        }
        else
        {
            max_time = maxStepTimeMs;
            suggest_time = maxStepTimeMs / 3;
        }
    }
    else if (step < 60)
    {
        if (restMatchTimeMs < maxStepTimeMs)
        {
            max_time = restMatchTimeMs / 8;
            suggest_time = restMatchTimeMs / 24;
        }
        else if (restMatchTimeMs / 16 < maxStepTimeMs)
        {
            max_time = restMatchTimeMs / 16;
            suggest_time = restMatchTimeMs / 48;
        }
        else
        {
            max_time = maxStepTimeMs;
            suggest_time = maxStepTimeMs / 3;
        }
    }
    else
    {
        max_time = restMatchTimeMs / 10;
        suggest_time = restMatchTimeMs / 20;
    }
}

Position GoSearchEngine::getBestStep(uint64_t startSearchTime)
{
    Util::needBreak = false;
    this->startSearchTime = system_clock::from_time_t(startSearchTime);
    uint32_t max_time, suggest_time;
    allocatedTime(max_time, suggest_time);
    maxStepTimeMs = max_time;

    MovePath bestPath(startStep.step);
    currentAlphaBetaDepth = minAlphaBetaDepth;
    vector<StepCandidateItem> moveList;
    analysePosition(board, moveList, bestPath);
    if (moveList.empty())
    {
        uint8_t highest = board->getHighestType(startStep.state);
        ForEachMove(board)
        {
            if (board->getChessType(pos, startStep.state) == highest)
            {
                return pos;
            }
        }
    }
    else if (moveList.size() == 1)
    {
        textOutResult(bestPath);
        return moveList[0].pos;
    }

    while (true)
    {

        MaxDepth = startStep.step;
        MovePath temp(startStep.step);
        selectBestMove(board, moveList, temp);
        textOutIterativeInfo(temp);
        if (currentAlphaBetaDepth > minAlphaBetaDepth && Util::needBreak)
        {
            if (bestPath.rating < CHESSTYPE_5_SCORE && temp.rating == CHESSTYPE_5_SCORE)
            {
                bestPath = temp;
                std::stable_sort(moveList.begin(), moveList.end(), CandidateItemCmp);
            }
            else
            {
                currentAlphaBetaDepth -= 1;
            }
            break;
        }

        std::stable_sort(moveList.begin(), moveList.end(), CandidateItemCmp);
        bestPath = temp;
        if (temp.rating >= CHESSTYPE_5_SCORE || temp.rating == -CHESSTYPE_5_SCORE)
        {
            break;
        }

        //已成定局的不需要继续搜索了
        if (moveList[0].value == 10000)
        {
            break;
        }

        if (duration_cast<milliseconds>(std::chrono::system_clock::now() - this->startSearchTime).count() > suggest_time)
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
    textOutAllocateTime(max_time, suggest_time);
    textOutResult(bestPath);

    return moveList[0].pos;
}

void GoSearchEngine::analysePosition(ChessBoard* board, vector<StepCandidateItem>& moves, MovePath& path)
{
    uint8_t side = board->getLastStep().getOtherSide();
    MovePath VCXPath(board->getLastStep().step);
    if (board->hasChessType(side, CHESSTYPE_5))
    {
        ForEachMove(board)
        {
            if (board->getChessType(pos, side) == CHESSTYPE_5)
            {
                path.rating = CHESSTYPE_5_SCORE;
                path.push(pos);
                moves.emplace_back(pos, 10000, 0, CHESSTYPE_5);
                break;
            }
        }
        return;
    }
    else if (board->hasChessType(Util::otherside(side), CHESSTYPE_5))//敌方马上5连
    {
        ForEachMove(board)
        {
            if (board->getChessType(pos, Util::otherside(side)) == CHESSTYPE_5)
            {
                if (board->getChessType(pos, side) == CHESSTYPE_BAN)
                {
                    path.rating = -CHESSTYPE_5_SCORE;
                }
                else
                {
                    path.rating = board->getGlobalEvaluate(side);
                }
                path.push(pos);
                moves.emplace_back(pos, 10000, 0, board->getChessType(pos, side));
                break;
            }
        }
        return;
    }
    else if (doVCXExpand(board, VCXPath, false, false))
    {
        path = VCXPath;
        path.rating = CHESSTYPE_5_SCORE;
        moves.emplace_back(path.path[0], 10000, 0, 0);
        return;
    }
    else
    {
        //bool indanger = false;
        ////null scout
        //{
        //    MovePath tempPath(board->getLastStep().step);
        //    ChessBoard currentBoard = *board;
        //    currentBoard.moveNull();
        //    allowed_nullmove = false;
        //    if (doVCXExpand(&currentBoard, VCXPath, false, false))
        //    {
        //        //处于危险中
        //        indanger = true;
        //    }
        //}
        board->getNormalCandidates(moves, true);
        std::sort(moves.begin(), moves.end(), CandidateItemCmp);
    }
    if (moves.size() > 1)
    {
        MovePath VCXPath(board->getLastStep().step + 1);
        for (size_t index = 0; index < moves.size(); ++index)
        {
            ChessBoard currentBoard = *board;
            currentBoard.move(moves[index].pos, rule);
            VCXPath.path.clear();
            if (doVCXExpand(&currentBoard, VCXPath, useTransTable, false))
            {
                moves[index].value = -10000;
            }
        }
        std::sort(moves.begin(), moves.end(), CandidateItemCmp);
        if (moves[0].value == -10000)
        {
            moves.clear();
            return;
        }
        for (size_t index = 0; index < moves.size(); ++index)
        {
            if (moves[index].value == -10000)
            {
                moves.erase(moves.begin() + index, moves.end());
                if (moves.size() == 1) path.push(moves[0].pos);
                break;
            }
        }
    }

}

void GoSearchEngine::selectBestMove(ChessBoard* board, vector<StepCandidateItem>& moves, MovePath& path)
{
    int base_alpha = INT_MIN + 1, base_beta = INT_MAX;
    path.rating = INT_MIN + 1;

    bool foundPV = false;
    //first search
    for (size_t index = 0; index < moves.size(); ++index)
    {

        MovePath tempPath(board->getLastStep().step);
        tempPath.push(moves[index].pos);
        ChessBoard currentBoard = *board;
        currentBoard.move(moves[index].pos, rule);

        //extend
        int extend_base = 0;
        double depth_extra = 0;
        if (Util::hasdead4(moves[index].type)) { extend_base = 1; depth_extra = 1.0; }
        else if (Util::isalive3or33(moves[index].type)) { extend_base = 1; depth_extra = 0.5; }


#ifdef ENABLE_PV
        if (foundPV)
        {
            //假设当前是最好的，没有任何其他的会比当前的PV好（大于alpha）
#ifdef USE_NEGAMAX
            //不加depth_extra是牺牲算杀准确性增加控场能力
            doPVSearch(&currentBoard, tempPath, currentAlphaBetaDepth - 1/* + depth_extra*/, extend_base, -base_alpha - 1, -base_alpha, CUT_NODE, true, useTransTable);
            tempPath.rating = -tempPath.rating;
#else
            doABSearch(&currentBoard, tempPath, currentAlphaBetaDepth - 1, extend_base, base_alpha, base_alpha + 1, true, useTransTable);//极小窗口剪裁 
#endif // USE_NEGAMAX


            if (tempPath.rating > base_alpha && tempPath.rating < base_beta)//使用完整窗口
            {
                complexity++;
                tempPath.path.clear();
                tempPath.push(moves[index].pos);
                //
#ifdef USE_NEGAMAX
                doPVSearch(&currentBoard, tempPath, currentAlphaBetaDepth - 1 + depth_extra, extend_base, -base_beta, -base_alpha, PV_NODE, true, useTransTable);
                tempPath.rating = -tempPath.rating;
#else
                doABSearch(&currentBoard, tempPath, currentAlphaBetaDepth - 1, extend_base, base_alpha, base_beta, true, useTransTable);
#endif
            }

        }
        else
#endif
        {
#ifdef USE_NEGAMAX
            doPVSearch(&currentBoard, tempPath, currentAlphaBetaDepth - 1 + depth_extra, extend_base, -base_beta, -base_alpha, PV_NODE, true, useTransTable);
            tempPath.rating = -tempPath.rating;
#else
            doABSearch(&currentBoard, tempPath, currentAlphaBetaDepth - 1, extend_base, base_alpha, base_beta, true, useTransTable);
#endif
        }


        moves[index].value = tempPath.rating;
        //处理超时
        if (Util::needBreak)
        {
            if (path.rating == INT_MIN + 1)
            {
                path = tempPath;
            }
            return;
        }

        if (tempPath.rating > path.rating)
        {
            base_alpha = tempPath.rating;
            path = tempPath;
            foundPV = true;
        }

        //textForTest(tempPath, moves[index].value);
    }
}

void GoSearchEngine::doABSearch(ChessBoard* board, MovePath& optimalPath, int depth, int depth_extend, int alpha, int beta, bool enableVCT, bool useTransTable)
{
    node_count++;

    uint8_t side = board->getLastStep().getOtherSide();
    uint8_t otherside = board->getLastStep().state;
    int laststep = board->getLastStep().step;
    bool memoryValid = true;
    //USE TransTable
    bool has_best_pos = false;

    TransTableData data;

    if (useTransTable)
    {
#define TRANSTABLE_HIT_FUNC transTableStat.hit++;optimalPath.rating = data.value;optimalPath.push(data.bestStep);
        if (transTable.get(board->getBoardHash().hash_key, data))
        {
            if (data.checkHash == board->getBoardHash().check_key)
            {
                if (data.value == -CHESSTYPE_5_SCORE || data.value == CHESSTYPE_5_SCORE)
                {
                    TRANSTABLE_HIT_FUNC
                        return;
                }
                else if (data.age < currentAlphaBetaDepth)
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
                        if (data.value <= alpha)
                        {
                            transTableStat.hit++; optimalPath.rating = alpha; optimalPath.push(data.bestStep);
                            return;
                        }
                        else//data.value > alpha
                        {
                            transTableStat.cover++;
                            if (data.bestStep.valid())
                            {
                                has_best_pos = true;
                            }
                        }
                    }
                    else
                    {
                        if (data.value >= beta)
                        {
                            transTableStat.hit++; optimalPath.rating = beta; optimalPath.push(data.bestStep);
                            return;
                        }
                        else
                        {
                            transTableStat.cover++;
                            if (data.bestStep.valid())
                            {
                                has_best_pos = true;
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
    else if (Util::needBreak || duration_cast<milliseconds>(std::chrono::system_clock::now() - startSearchTime).count() > maxStepTimeMs)//超时
    {
        optimalPath.rating = -CHESSTYPE_5_SCORE + 1;
        Util::needBreak = true;
        return;
    }
    else if (board->hasChessType(otherside, CHESSTYPE_5))//防5连
    {
        ForEachMove(board)
        {
            if (board->getChessType(pos, otherside) == CHESSTYPE_5)
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
    else if (enableVCT && doVCXExpand(board, VCXPath, useTransTable, false))
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
    /*else if (depth > 5 && alpha + 1 < beta)
    {
        for (uint8_t move_index = 0; move_index < searchUpper; ++move_index)
        {
            MovePath tempPath(board->getLastStep().step);
            tempPath.push(moves[move_index].pos);
            ChessBoard currentBoard = *board;
            currentBoard.move(moves[move_index].pos, rule);

            doABSearch(&currentBoard, tempPath, depth / 2, 0, alpha, beta, false, false);
            moves[move_index].value = (isPlayerSide(side)) ? -tempPath.rating : tempPath.rating;
        }
        std::sort(moves.begin(), moves.begin() + searchUpper, CandidateItemCmp);
    }*/
    else
    {
        std::sort(moves.begin(), moves.end(), CandidateItemCmp);
    }


    bool foundPV = false;
    data.type = PV_NODE;

    for (uint8_t move_index = 0; move_index < searchUpper; ++move_index)
    {

        MovePath tempPath(board->getLastStep().step);
        tempPath.push(moves[move_index].pos);
        ChessBoard currentBoard = *board;
        currentBoard.move(moves[move_index].pos, rule);

        //extend
        int extend_base = 0;
        if (Util::hasdead4(moves[move_index].type)) extend_base += 2;
        else if (Util::isalive3or33(moves[move_index].type)) extend_base += 1;

        //剪枝
        if (isPlayerSide(side))//build player
        {
#ifdef ENABLE_PV
            if (foundPV)
            {
                doABSearch(&currentBoard, tempPath, depth - 1, 0, beta - 1, beta, enableVCT, useTransTable);//极小窗口剪裁
                //假设当前是最好的，没有任何其他的会比当前的PV好（小于beta）
                if (tempPath.rating < beta && tempPath.rating > alpha)//失败，使用完整窗口
                {
                    tempPath.path.clear();
                    tempPath.push(moves[move_index].pos);
                    doABSearch(&currentBoard, tempPath, depth - 1, depth_extend + extend_base, alpha, beta, enableVCT, useTransTable);
                }
            }
            else
#endif // ENABLE_PV
            {
                doABSearch(&currentBoard, tempPath, depth - 1, depth_extend + extend_base, alpha, beta, enableVCT, useTransTable);
            }

            if (tempPath.rating < bestPath.rating)
            {
                foundPV = true;
                bestPath = tempPath;
            }
            //else if (tempPath.rating == bestPath.rating)
            //{
            //    if (tempPath.rating == -CHESSTYPE_5_SCORE && tempPath.endStep < bestPath.endStep)//赢了，尽量快
            //    {
            //        bestPath = tempPath;
            //    }
            //    else if (tempPath.rating == CHESSTYPE_5_SCORE && tempPath.endStep > bestPath.endStep)//必输，尽量拖
            //    {
            //        bestPath = tempPath;
            //    }
            //}
            if (tempPath.rating < beta)//update beta
            {
                beta = tempPath.rating;
            }
            if (tempPath.rating <= alpha)//alpha cut
            {
                data.type = ALL_NODE;
                break;
            }
        }
        else // build AI
        {
#ifdef ENABLE_PV
            if (foundPV)
            {
                doABSearch(&currentBoard, tempPath, depth - 1, 0, alpha, alpha + 1, enableVCT, useTransTable);//极小窗口剪裁
                //假设当前是最好的，没有任何其他的会比当前的PV好（大于alpha）
                if (tempPath.rating > alpha && tempPath.rating < beta)
                {
                    tempPath.path.clear();
                    tempPath.push(moves[move_index].pos);
                    doABSearch(&currentBoard, tempPath, depth - 1, depth_extend + extend_base, alpha, beta, enableVCT, useTransTable);
                }
            }
            else
#endif // ENABLE_PV
            {
                doABSearch(&currentBoard, tempPath, depth - 1, depth_extend + extend_base, alpha, beta, enableVCT, useTransTable);
            }


            if (tempPath.rating > bestPath.rating)
            {
                foundPV = true;
                bestPath = tempPath;
            }
            //else if (tempPath.rating == bestPath.rating)
            //{
            //    if (tempPath.rating == CHESSTYPE_5_SCORE && tempPath.endStep < bestPath.endStep)//赢了，尽量快
            //    {
            //        bestPath = tempPath;
            //    }
            //    else if (tempPath.rating == -CHESSTYPE_5_SCORE && tempPath.endStep > bestPath.endStep)//必输，尽量拖
            //    {
            //        bestPath = tempPath;
            //    }
            //}
            if (tempPath.rating > alpha)//update alpha
            {
                alpha = tempPath.rating;

            }
            if (tempPath.rating >= beta)//beta cut
            {
                data.type = CUT_NODE;
                break;
            }
        }

    }


    optimalPath.cat(bestPath);
    if (data.type == PV_NODE) // no cut
    {
        optimalPath.rating = isPlayerSide(side) ? beta : alpha;
    }
    else
    {
        optimalPath.rating = isPlayerSide(side) ? alpha : beta;
    }


    //USE TransTable
    //写入置换表
    if (useTransTable && memoryValid)
    {
        data.checkHash = board->getBoardHash().check_key;
        data.value = bestPath.rating;
        data.depth = depth;
        data.age = currentAlphaBetaDepth;
        data.bestStep = bestPath.path.empty() ? Position(-1, -1) : bestPath.path[0];
        transTable.insert(board->getBoardHash().hash_key, data);
    }
    //end USE TransTable
}

void GoSearchEngine::doPVSearch(ChessBoard* board, MovePath& optimalPath, double depth, int depth_extend, int alpha, int beta, uint8_t predicted_type, bool enableVCT, bool useTransTable)
{
    node_count++;

    uint8_t side = board->getLastStep().getOtherSide();
    uint8_t otherside = board->getLastStep().state;
    int laststep = board->getLastStep().step;
    //USE TransTable
    bool has_best_pos = false;
    TransTableData data;

    if (useTransTable)
    {
        transTable.get(board->getBoardHash().hash_key, data);
        if (data.checkHash == board->getBoardHash().check_key)
        {
            if (data.age >= currentAlphaBetaDepth && data.depth >= depth)
            {
                if (data.type == PV_NODE)
                {
                    transTableStat.hit++; optimalPath.rating = data.value; optimalPath.push(data.bestStep);
                    return;
                }
                if (data.type == CUT_NODE && data.value >= beta)
                {
                    transTableStat.hit++; optimalPath.rating = beta; optimalPath.push(data.bestStep);
                    return;
                }
                if (data.type == ALL_NODE && data.value <= alpha)
                {
                    transTableStat.hit++; optimalPath.rating = alpha; optimalPath.push(data.bestStep);
                    return;
                }
            }
            if (data.bestStep.valid())
            {
                has_best_pos = true;
            }
        }
        transTableStat.cover++;
    }
    //end USE TransTable


    size_t searchUpper = 0;
    MovePath VCXPath(board->getLastStep().step);
    vector<StepCandidateItem> moves;

    MovePath bestPath(board->getLastStep().step);
    bestPath.rating = INT_MIN + 1;

    if (board->hasChessType(side, CHESSTYPE_5))
    {
        optimalPath.rating = CHESSTYPE_5_SCORE;
        return;
    }
    else if (depth < 1.0)
    {
        leaf_node_count++;
        //if (doVCXExpand(board, VCXPath, useTransTable, true))
        //{
        //    optimalPath.cat(VCXPath);
        //    optimalPath.rating = CHESSTYPE_5_SCORE;
        //}
        //else
        {
            optimalPath.rating = board->getGlobalEvaluate(side, AIweight);
            //optimalPath.rating = doQuiescentSearch(board, depth_extend, alpha, beta, enableVCT, false);
        }
        return;
    }
    else if (Util::needBreak || duration_cast<milliseconds>(std::chrono::system_clock::now() - startSearchTime).count() > maxStepTimeMs)//超时
    {
        optimalPath.rating = (isPlayerSide(side)) ? (CHESSTYPE_5_SCORE - 1000) : -CHESSTYPE_5_SCORE + 1000;
        Util::needBreak = true;
        return;
    }
    else if (board->hasChessType(otherside, CHESSTYPE_5))//防5连
    {
        ForEachMove(board)
        {
            if (board->getChessType(pos, otherside) == CHESSTYPE_5)
            {
                if (board->getChessType(pos, side) == CHESSTYPE_BAN)//触发禁手，otherside赢了
                {
                    optimalPath.rating = -CHESSTYPE_5_SCORE;
                    return;
                }
                else
                {
                    moves.emplace_back(pos, 10000, 0, board->getChessType(pos, side));
                    searchUpper = moves.size();
                }
                break;
            }
        }
    }
    else if (enableVCT && doVCXExpand(board, VCXPath, useTransTable, true))
    {
        optimalPath.cat(VCXPath);
        optimalPath.rating = CHESSTYPE_5_SCORE;
        return;
    }
    else
    {
        //null prune
        if (depth > 3 && allowed_nullmove /*&& predicted_type != PV_NODE*/)
        {
            MovePath tempPath(board->getLastStep().step);
            ChessBoard currentBoard = *board;
            currentBoard.moveNull();
            allowed_nullmove = false;
            doPVSearch(&currentBoard, tempPath, depth - 1 - 2, 0, -beta, -beta + 1, CUT_NODE, enableVCT, false);
            allowed_nullmove = true;
            if (-tempPath.rating >= beta)
            {
                null_prune_success_count++;
                optimalPath.rating = beta;
                return;
            }
        }
        //generate candidates
        searchUpper = board->getNormalCandidates(moves, true);
        if (searchUpper == 0) searchUpper = moves.size();
    }

    if (has_best_pos)
    {
        hit_bestmove_count++;
        //优先搜索置换表中记录的上一个迭代的最好着法
        for (size_t i = 0; i < searchUpper; ++i)
        {
            if (moves[i].pos == data.bestStep)
            {
                moves[i].value = 1000;
                break;
            }
        }
        std::sort(moves.begin(), moves.begin() + searchUpper, CandidateItemCmp);
    }
    else if (depth > 5 && /*predicted_type != ALL_NODE */predicted_type == PV_NODE)
    {
        for (uint8_t move_index = 0; move_index < searchUpper; ++move_index)
        {
            node_count_scout++;
            MovePath tempPath(board->getLastStep().step);
            tempPath.push(moves[move_index].pos);
            ChessBoard currentBoard = *board;
            currentBoard.move(moves[move_index].pos, rule);
            doPVSearch(&currentBoard, tempPath, depth / 2, 0, -beta, -alpha, predicted_type, enableVCT, useTransTable);
            moves[move_index].value = -tempPath.rating;
        }
        std::sort(moves.begin(), moves.begin() + searchUpper, CandidateItemCmp);
    }
    else
    {
        std::sort(moves.begin(), moves.end(), CandidateItemCmp);
    }


    bool foundPV = false;
    uint8_t type = ALL_NODE;
    for (uint8_t move_index = 0; move_index < searchUpper; ++move_index)
    {

        MovePath tempPath(board->getLastStep().step);
        tempPath.push(moves[move_index].pos);
        ChessBoard currentBoard = *board;
        currentBoard.move(moves[move_index].pos, rule);
        int extend_base = 0;
        double depth_extra = 0;
        if (Util::hasdead4(moves[move_index].type)) { extend_base = 1; depth_extra = 1.0; }
        else if (Util::isalive3or33(moves[move_index].type)) { extend_base = 1; depth_extra = 0.5; }
        //剪枝
#ifdef ENABLE_PV
        if (foundPV)
        {
            doPVSearch(&currentBoard, tempPath, depth - 1/* + depth_extra*/, depth_extend + extend_base, -alpha - 1, -alpha, predicted_type == CUT_NODE ? ALL_NODE : CUT_NODE, enableVCT, useTransTable);//极小窗口剪裁
            tempPath.rating = -tempPath.rating;
            if ((tempPath.rating > alpha && tempPath.rating < beta)
                /*|| (predicted_type == PV_NODE && )*/)
            {
                complexity++;
                tempPath.path.clear();
                tempPath.push(moves[move_index].pos);
                doPVSearch(&currentBoard, tempPath, depth - 1 + depth_extra, depth_extend + extend_base, -beta, -alpha, predicted_type, enableVCT, useTransTable);
                tempPath.rating = -tempPath.rating;
            }
        }
        else
#endif // ENABLE_PV
        {
            doPVSearch(&currentBoard, tempPath, depth - 1 + depth_extra, depth_extend + extend_base, -beta, -alpha, predicted_type == PV_NODE ? PV_NODE : (predicted_type == CUT_NODE ? ALL_NODE : CUT_NODE), enableVCT, useTransTable);
            tempPath.rating = -tempPath.rating;
        }


        if (tempPath.rating > bestPath.rating)
        {
            foundPV = true;
            bestPath = tempPath;
        }
        if (tempPath.rating > alpha)//update alpha
        {
            type = PV_NODE;
            alpha = tempPath.rating;
        }
        if (tempPath.rating >= beta)//beta cut
        {
            type = CUT_NODE;
            break;
        }
    }

    optimalPath.cat(bestPath);
    optimalPath.rating = type == CUT_NODE ? beta : (type == ALL_NODE ? alpha : bestPath.rating);
    //optimalPath.rating = bestPath.rating;

    //USE TransTable
    //写入置换表
    if (useTransTable)
    {
        data.checkHash = board->getBoardHash().check_key;
        data.value = optimalPath.rating;
        data.type = type;
        data.depth = (uint8_t)depth;
        data.age = currentAlphaBetaDepth;
        data.bestStep = bestPath.path.empty() ? Position(-1, -1) : bestPath.path[0];
        transTable.insert(board->getBoardHash().hash_key, data);
    }
    //end USE TransTable
}

int GoSearchEngine::doQuiescentSearch(ChessBoard* board, int depth, int alpha, int beta, bool enableVCT, bool scout)
{
    node_count_quies++;

    uint8_t side = board->getLastStep().getOtherSide();
    uint8_t otherside = board->getLastStep().state;
    int laststep = board->getLastStep().step;

    int rating = board->getGlobalEvaluate(side, AIweight);

    if (rating >= beta)
    {
        return beta;
    }
    if (rating > alpha)
    {
        alpha = rating;
    }

    if (board->hasChessType(side, CHESSTYPE_5))
    {
        return CHESSTYPE_5_SCORE;
    }
    else if (depth <= 0)
    {
        return rating;
    }

    size_t searchUpper = 0;
    MovePath VCXPath(board->getLastStep().step);
    vector<StepCandidateItem> moves;

    MovePath bestPath(board->getLastStep().step);
    bestPath.rating = INT_MIN + 1;

    if (board->hasChessType(otherside, CHESSTYPE_5))//防5连
    {
        ForEachMove(board)
        {
            if (board->getChessType(pos, otherside) == CHESSTYPE_5)
            {
                if (board->getChessType(pos, side) == CHESSTYPE_BAN)//触发禁手，otherside赢了
                {
                    return -CHESSTYPE_5_SCORE;
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
    //else if (doVCXExpand(board, VCXPath, true, true))
    //{
    //    return CHESSTYPE_5_SCORE;
    //}
    else
    {
        searchUpper = board->getUsefulCandidates(moves);
        if (searchUpper == 0) searchUpper = moves.size();
    }

    for (uint8_t move_index = 0; move_index < searchUpper; ++move_index)
    {
        ChessBoard currentBoard = *board;
        currentBoard.move(moves[move_index].pos, rule);
        int rating = -doQuiescentSearch(&currentBoard, depth - 1, -beta, -alpha, enableVCT, scout);

        if (rating >= beta)//beta cut
        {
            return beta;
        }
        if (rating > alpha)//update alpha
        {
            alpha = rating;
        }

    }

    return alpha;
}

bool GoSearchEngine::doVCXExpand(ChessBoard* board, MovePath& optimalPath, bool useTransTable, bool onlyVCF)
{
    if (useDBSearch)
    {
        dbsearch_count++;
        TransTableDBData data;
        if (useTransTable)
        {
            if (DBSearch::transTable.get(board->getBoardHash().hash_key, data))
            {
                if (data.checkHash == board->getBoardHash().check_key)
                {
                    if (data.result)
                    {
                        transTableStat.hit++;
                        return data.result;
                    }
                    else if (!data.onlyVCF || onlyVCF)
                    {
                        transTableStat.hit++;
                        return data.result;
                    }
                }
            }
        }
        DBSearch::node_count = 0;
        DBSearch dbs(board, rule, onlyVCF ? 1 : 2);
        bool ret = dbs.doDBSearch(&optimalPath.path);
        DBSearchNodeCount += DBSearch::node_count;
        maxDBSearchNodeCount = DBSearch::node_count > maxDBSearchNodeCount ? DBSearch::node_count : maxDBSearchNodeCount;
        MaxDepth = MaxDepth < (board->getLastStep().step + dbs.getMaxDepth() * 2) ? (board->getLastStep().step + dbs.getMaxDepth() * 2) : MaxDepth;
        if (ret)
        {
            optimalPath.endStep = optimalPath.startStep + (uint16_t)optimalPath.path.size();
        }
        if (useTransTable)
        {
            data.result = ret;
            data.onlyVCF = onlyVCF;
            data.checkHash = board->getBoardHash().check_key;
            DBSearch::transTable.insert(board->getBoardHash().hash_key, data);
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