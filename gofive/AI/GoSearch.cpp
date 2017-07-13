#include "GoSearch.h"
#include "utils.h"


#define GOSEARCH_DEBUG

HashStat GoSearchEngine::transTableStat;
string GoSearchEngine::textout;

TransTableMap GoSearchEngine::transTable;
shared_mutex GoSearchEngine::transTableLock;
TransTableMapSpecial GoSearchEngine::transTableSpecial;


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
    //optimalPath����Ϊ��
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
    global_isOverTime = false;
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
        global_currentMaxDepth += 1, maxVCFDepth += 2/*, maxVCTDepth += 1*/)
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

        //�ѳɶ��ֵĲ���Ҫ����������
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
        optimalPath.path.push_back(board->getHighestInfo(startStep.getColor()).index);
    }

    return optimalPath.path[0];
}


OptimalPath GoSearchEngine::makeSolveList(ChessBoard* board, vector<StepCandidateItem>& solveList)
{
    uint8_t side = util::otherside(board->getLastStep().getColor());

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
    else if (otherhighest.chesstype == CHESSTYPE_5)//�з�����5��
    {
        optimalPath.push(otherhighest.index);
        optimalPath.rating = 0;
        solveList.emplace_back(otherhighest.index, 0);
    }
    else if (doVCFSearch(board, side, optimalPath, NULL) == VCXRESULT_TRUE)
    {
        solveList.emplace_back(optimalPath.path[0], 10000);
    }
    else if (util::isfourkill(otherhighest.chesstype))//�з���4ɱ
    {
        getFourkillDefendSteps(board, otherhighest.index, solveList);
    }
    else if (doVCTSearch(board, side, optimalPath, NULL) == VCXRESULT_TRUE)
    {
        solveList.emplace_back(optimalPath.path[0], 10000);
    }
    else
    {
        getNormalSteps(board, solveList);
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
    uint8_t side = util::otherside(board->getLastStep().getColor());

    PieceInfo otherhighest = board->getHighestInfo(util::otherside(side));
    PieceInfo selfhighest = board->getHighestInfo(side);

    for (auto it = solveList.begin(); it != solveList.end(); ++it)
    {
        textSearchList(solveList, it->index, optimalPath.path.empty() ? it->index : optimalPath.path[0]);
        ChessBoard tempboard = *board;
        tempboard.move(it->index);

        OptimalPath tempPath(board->getLastStep().step);
        tempPath.push(it->index);

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
        }
        //����ʱ
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
        struggleFlag++;
        if (doVTCStruggleSearch(board, side, struggleindex))
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
    }

    textOutSearchInfo(optimalPath);
    return optimalPath;
}


void GoSearchEngine::doAlphaBetaSearchWrapper(ChessBoard* board, int alpha, int beta, OptimalPath& optimalPath)
{
    TransTableData data;
    if (getTransTable(board->getBoardHash().z64key, data))
    {
        if (data.checkHash == board->getBoardHash().z32key)
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
    doAlphaBetaSearch(board, alpha, beta, optimalPath);
    //if (optimalPath.endStep > board->getLastStep().step + 1)//��һ���ͽ����˵�û��Ҫд���û���
    {
        data.checkHash = board->getBoardHash().z32key;
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
        putTransTable(board->getBoardHash().z64key, data);
    }

}

void GoSearchEngine::doAlphaBetaSearch(ChessBoard* board, int alpha, int beta, OptimalPath& optimalPath)
{
    uint8_t steps = board->getLastStep().step;
    steps = steps;
    uint8_t side = util::otherside(board->getLastStep().getColor());
    OptimalPath VCFPath(board->getLastStep().step);
    OptimalPath VCTPath(board->getLastStep().step);

    if (board->getHighestInfo(side).chesstype == CHESSTYPE_5)
    {
        optimalPath.rating = side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
        optimalPath.push(board->getHighestInfo(side).index);
        return;
    }

    //��ʱ
    if (global_isOverTime || duration_cast<milliseconds>(std::chrono::system_clock::now() - global_startSearchTime).count() > maxSearchTimeMs)
    {
        global_isOverTime = true;
        return;
    }

    if (board->getLastStep().step - startStep.step >= global_currentMaxDepth)
    {
        return;
    }

    PieceInfo otherhighest = board->getHighestInfo(util::otherside(side));
    PieceInfo selfhighest = board->getHighestInfo(side);

    vector<StepCandidateItem> moves;

    if (otherhighest.chesstype == CHESSTYPE_5)//�з�����5��
    {
        if (board->getChessType(otherhighest.index, side) == CHESSTYPE_BAN)
        {
            optimalPath.push(otherhighest.index);
            optimalPath.rating = -(side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5));
            return;
        }
        moves.emplace_back(otherhighest.index, 10);
    }
    else if (doVCFSearch(board, side, VCFPath, NULL) == VCXRESULT_TRUE)
    {
        optimalPath.cat(VCFPath);
        optimalPath.rating = VCFPath.rating;
        return;
    }
    else if (util::isfourkill(otherhighest.chesstype))//�з���4ɱ
    {
        getFourkillDefendSteps(board, otherhighest.index, moves);
    }
    else if (doVCTSearch(board, side, VCTPath, NULL) == VCXRESULT_TRUE)
    {
        optimalPath.cat(VCTPath);
        optimalPath.rating = VCTPath.rating;
        return;
    }
    else
    {
        getNormalSteps(board, moves);
    }

    if (moves.empty())
    {
        moves.emplace_back(otherhighest.index, -10000);
    }

    OptimalPath bestPath(board->getLastStep().step);
    bestPath.rating = side == startStep.getColor() ? INT_MAX : INT_MIN;

    for (size_t i = 0; i < moves.size(); ++i)
    {
        ChessBoard tempboard = *board;
        tempboard.move(moves[i].index);

        OptimalPath tempPath(board->getLastStep().step);
        tempPath.push(moves[i].index);

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
            }
        }


        if (side != startStep.getColor())//build AI
        {
            if (tempPath.rating > bestPath.rating
                || (tempPath.rating == bestPath.rating && tempPath.endStep < bestPath.endStep))
            {
                bestPath = tempPath;
            }

            if (tempPath.rating > beta /*|| tempPath.rating == util::type2score(CHESSTYPE_5)*/)//beta cut
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
            if (tempPath.rating < bestPath.rating
                || (tempPath.rating == bestPath.rating && tempPath.endStep < bestPath.endStep))
            {
                bestPath = tempPath;
            }

            if (tempPath.rating < alpha /*|| tempPath.rating == -util::type2score(CHESSTYPE_5)*/)//alpha cut
            {
                break;
            }
            if (tempPath.rating < beta)//update beta
            {
                beta = tempPath.rating;
            }
        }
    }

    //����

    uint8_t struggleindex;
    if ((side != startStep.getColor() && bestPath.rating == -util::type2score(CHESSTYPE_5))
        || (side == startStep.getColor() && bestPath.rating == util::type2score(CHESSTYPE_5)))
    {
        struggleFlag++;
        if (doVTCStruggleSearch(board, side, struggleindex))
        {
            struggleFlag--;
            ChessBoard tempboard = *board;
            tempboard.move(struggleindex);

            OptimalPath tempPath(board->getLastStep().step);
            tempPath.push(struggleindex);

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
                }
            }


            if (side != startStep.getColor())//build AI
            {
                if (
                    tempPath.rating > bestPath.rating
                    || (tempPath.rating == bestPath.rating && tempPath.endStep < bestPath.endStep)
                    )
                {
                    bestPath = tempPath;
                }
            }
            else // build player
            {
                if (
                    tempPath.rating < bestPath.rating
                    || (tempPath.rating == bestPath.rating && tempPath.endStep < bestPath.endStep)
                    )
                {
                    bestPath = tempPath;
                }
            }
        }
        else
        {
            struggleFlag--;
        }
    }

    optimalPath.cat(bestPath);
    optimalPath.rating = bestPath.rating;

}

bool GoSearchEngine::doNormalStruggleSearch(ChessBoard* board, uint8_t side, uint8_t &nextstep)
{
    uint8_t laststep = board->lastStep.step;
    vector<StepCandidateItem> moves;
    getVCFAtackSteps(board, moves, NULL);
    for (auto move : moves)
    {

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
        if (priority <= 0)
        {
            continue;
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
    //    childs.erase(childs.begin() + MAX_CHILD_NUM, childs.end());//ֻ����10��
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

    std::sort(childs.begin(), childs.end(), CandidateItemCmp);
    for (std::vector<StepCandidateItem>::iterator it = childs.begin(); it != childs.end(); it++)
    {
        if (it->priority <= childs.begin()->priority / 2)
        {
            childs.erase(it, childs.end());
            break;
        }
    }
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
    //    childs.erase(childs.begin() + MAX_CHILD_NUM, childs.end());//ֻ����10��
    //}
    std::sort(childs.begin(), childs.end(), CandidateItemCmp);
}

void GoSearchEngine::getNormalSteps(ChessBoard* board, vector<StepCandidateItem>& childs)
{
    getNormalSteps2(board, childs);

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
        while (board->nextPosition(r, c, 1, n)) //����������߽�
        {
            if (board->getState(r, c) == PIECE_BLANK)
            {
                blankCount++;
                tempType = board->getChessType(r, c, util::otherside(side));
                if (tempType > CHESSTYPE_0)
                {
                    if (board->getChessType(r, c, side) == CHESSTYPE_BAN)//��������
                    {
                        continue;
                    }
                    ChessBoard tempboard = *board;
                    tempboard.moveTemporary(util::xy2index(r, c));
                    /*if (tempboard.getChessType(index, board->getLastStep().getColor()) < defendType)*/
                    if (tempboard.getHighestInfo(board->getLastStep().getColor()).chesstype < defendType)
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
            if (data.VCFflag == VCXRESULT_NOSEARCH)//��δ����
            {
                transTableStat.miss++;
            }
            else
            {
                if (data.VCFflag == VCXRESULT_UNSURE && data.VCFDepth < (struggleFlag ? maxVCFDepth + extraVCXDepth : maxVCFDepth))//��Ҫ����
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
    if (optimalPath.endStep > board->getLastStep().step + 1)//��һ���ͽ����˵�û��Ҫд���û���
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
            if (data.VCTflag == VCXRESULT_NOSEARCH)//��δ����
            {
                transTableStat.miss++;
            }
            else
            {
                if (data.VCTflag == VCXRESULT_UNSURE && data.VCTDepth < (struggleFlag ? maxVCTDepth + extraVCXDepth : maxVCTDepth))//��Ҫ����
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
    if (optimalPath.endStep > board->getLastStep().step + 1)//��һ���ͽ����˵�û��Ҫд���û���
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
        optimalPath.rating = side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
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
        tempboard.move(item.index);//����

        OptimalPath tempPath(board->getLastStep().step);
        tempPath.push(item.index);

        if (tempboard.getHighestInfo(util::otherside(side)).chesstype == CHESSTYPE_5)
        {
            continue;
        }

        if (tempboard.getHighestInfo(side).chesstype != CHESSTYPE_5)//5���ǽ���
        {
            continue;
        }

        if (tempboard.getChessType(tempboard.getHighestInfo(side).index, util::otherside(side)) == CHESSTYPE_BAN)//�з��������֣�VCF�ɹ�
        {
            optimalPath.rating = side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
            optimalPath.cat(tempPath);
            return VCXRESULT_TRUE;
        }
        tempPath.push(tempboard.getHighestInfo(side).index);//������
        tempboard.move(tempboard.getHighestInfo(side).index);

        set<uint8_t> atackset;
        //if (reletedset != NULL)
        //{
        //    set<uint8_t> tempatackset;
        //    tempboard.getAtackReletedPos(tempatackset, item.index, side);
        //    util::myset_intersection(reletedset, &tempatackset, &atackset);
        //}
        //else
        {
            tempboard.getAtackReletedPos(atackset, item.index, side);
        }

        uint8_t result = doVCFSearchWrapper(&tempboard, side, tempPath, &atackset);
        if (result == VCXRESULT_TRUE)
        {
            optimalPath.rating = side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
            optimalPath.cat(tempPath);
            return VCXRESULT_TRUE;
        }

        //ֻҪ��һ��UNSURE����û��TRUE����ô�������UNSURE
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
        optimalPath.rating = side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
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
    else if (board->getLastStep().step - startStep.step > (struggleFlag ? maxVCTDepth + extraVCXDepth : maxVCTDepth))
    {
        return VCXRESULT_UNSURE;
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

        if (tempboard.getHighestInfo((side)).chesstype == CHESSTYPE_5)//����
        {
            if (tempboard.getHighestInfo(util::otherside(side)).chesstype == CHESSTYPE_5)
            {
                continue;
            }

            if (tempboard.getChessType(tempboard.getHighestInfo(side).index, util::otherside(side)) == CHESSTYPE_BAN)//�з��������֣�VCF�ɹ�
            {
                optimalPath.rating = side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
                optimalPath.cat(tempPath);
                return VCXRESULT_TRUE;
            }
            tempPath.push(tempboard.getHighestInfo(side).index);//������
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

            tempresult = doVCTSearchWrapper(&tempboard, side, tempPath, &atackset);
            if (tempresult == VCXRESULT_TRUE)
            {
                optimalPath.rating = side == startStep.getColor() ? -util::type2score(CHESSTYPE_5) : util::type2score(CHESSTYPE_5);
                optimalPath.cat(tempPath);
                return VCXRESULT_TRUE;
            }
            if (tempresult == VCXRESULT_UNSURE)
            {
                unsure_flag = true;
            }
            if (tempPath.endStep > bestPath.endStep)
            {
                bestPath.endStep = tempPath.endStep;
            }
            continue;
        }
        tempresult = doVCFSearch(&tempboard, util::otherside(side), tempPath, NULL);
        if (tempresult == VCXRESULT_TRUE)
        {
            continue;
        }
        if (tempresult == VCXRESULT_UNSURE)
        {
            unsure_flag = true;
        }

        if (!util::isfourkill(tempboard.getHighestInfo(side).chesstype))//���ٻ�������������
        {
            continue;
        }
        vector<StepCandidateItem> defendmoves;
        getFourkillDefendSteps(&tempboard, tempboard.getHighestInfo(side).index, defendmoves);
        //getVCFAtackSteps(&tempboard, defendmoves, false);
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
            if (tempPath.endStep > bestPath.endStep)
            {
                bestPath.endStep = tempPath.endStep;
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
            if (doVTCStruggleSearch(&tempboard, util::otherside(side), struggleindex))
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

bool GoSearchEngine::doVTCStruggleSearch(ChessBoard* board, uint8_t side, uint8_t &nextstep)
{
    uint8_t laststep = board->lastStep.step;
    vector<StepCandidateItem> moves;
    getVCFAtackSteps(board, moves, NULL);
    for (auto move : moves)
    {
        ChessBoard tempboard = *board;
        tempboard.move(move.index);//����
        if (tempboard.getHighestInfo(util::otherside(side)).chesstype == CHESSTYPE_5)
        {
            continue;
        }
        if (tempboard.getHighestInfo(side).chesstype != CHESSTYPE_5)//5���ǽ���
        {
            continue;
        }
        tempboard.move(tempboard.getHighestInfo(side).index);//������


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
            if (result != VCXRESULT_TRUE)//Ҫȷ�������ɹ������ܳ�����Ϊ�����������µĳɹ�
            {
                nextstep = move.index;
                return true;
            }
        }
        if (doVTCStruggleSearch(&tempboard, side, nextstep))
        {
            nextstep = move.index;
            return true;
        }

    }
    return false;
}


void GoSearchEngine::getVCFAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, set<uint8_t>* reletedset)
{
    uint8_t side = util::otherside(board->getLastStep().getColor());

    if (reletedset == NULL)
    {
        for (uint8_t index = 0; util::valid(index); ++index)
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

                if (util::inLocalArea(index, board->getLastStep().index, LOCAL_SEARCH_RANGE))
                {
                    moves.emplace_back(index, (int)(board->getRelatedFactor(index, side, true) * 3));
                }
                else
                {
                    moves.emplace_back(index, (int)(board->getRelatedFactor(index, side, true) * 2));
                }
            }
        }
    }
    else
    {
        for (set<uint8_t>::iterator it = reletedset->begin(); it != reletedset->end(); it++)
        {
            if (!board->canMove(*it))
            {
                continue;
            }

            if (util::hasdead4(board->getChessType(*it, side)))
            {
                if (util::isfourkill(board->getChessType(*it, side)))
                {
                    if (board->getChessType(*it, side) == CHESSTYPE_4)
                    {
                        moves.emplace_back(*it, 100);
                    }
                    else if (board->getChessType(*it, side) == CHESSTYPE_44)
                    {
                        moves.emplace_back(*it, 80);
                    }
                    else
                    {
                        moves.emplace_back(*it, 50);
                    }
                    continue;
                }
                moves.emplace_back(*it, (int)(board->getRelatedFactor(*it, side, true) * 2));
            }
        }
    }


    std::sort(moves.begin(), moves.end(), CandidateItemCmp);
}

void GoSearchEngine::getVCTAtackSteps(ChessBoard* board, vector<StepCandidateItem>& moves, set<uint8_t>* reletedset)
{
    uint8_t side = util::otherside(board->getLastStep().getColor());

    if (reletedset == NULL)
    {
        for (uint8_t index = 0; util::valid(index); ++index)
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
                if (util::inLocalArea(index, board->lastStep.index, LOCAL_SEARCH_RANGE))
                {
                    moves.emplace_back(index, (int)(board->getRelatedFactor(index, side, true) * 3));
                }
                else
                {
                    moves.emplace_back(index, (int)(board->getRelatedFactor(index, side, true) * 2));
                }
            }
            else if (util::isdead4(board->getChessType(index, side)))
            {
                if (util::inLocalArea(index, board->lastStep.index, LOCAL_SEARCH_RANGE))
                {
                    moves.emplace_back(index, (int)(board->getRelatedFactor(index, side, true) * 3));
                }
                else
                {
                    moves.emplace_back(index, (int)(board->getRelatedFactor(index, side, true) * 2));
                }

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
                    while (ChessBoard::nextPosition(r, c, 1, n)) //����������߽�
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
                            if (util::isfourkill(tempboard.getChessType(index, side)))
                            {
                                if (util::inLocalArea(tempindex, board->lastStep.index, LOCAL_SEARCH_RANGE))
                                {
                                    moves.emplace_back(tempindex, (int)(board->getRelatedFactor(tempindex, side, true) * 3));
                                }
                                else
                                {
                                    moves.emplace_back(tempindex, (int)(board->getRelatedFactor(tempindex, side, true) * 2));
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
    }
    else
    {
        for (auto index : *reletedset)
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
                    while (ChessBoard::nextPosition(r, c, 1, n)) //����������߽�
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
    }

    std::sort(moves.begin(), moves.end(), CandidateItemCmp);
    if (moves.size() > MAX_CHILD_NUM)
    {
        moves.erase(moves.begin() + MAX_CHILD_NUM, moves.end());//ֻ����10��
    }
}