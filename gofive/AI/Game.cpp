#include "Game.h"
#include "TrieTree.h"
#include "ThreadPool.h"
#include "DBSearch.h"
#include "DBSearchPlus.h"
#include "PNSearch.h"
#include "AIConfig.h"

bool Util::needBreak = false;
Game::Game()
{
    srand(unsigned int(time(0)));
}

Game::~Game()
{

}

bool Game::initAIHelper(int num)
{
    if (TrieTreeNode::getInstance()->buildTrieTree())//build tree 一定要在initChessModeHashTable之前
    {
        ThreadPool::getInstance()->start(num);
        ChessBoard::initStaticHelper();
        return true;
    }

    return false;
}

int Game::getPieceState(int row, int col)
{
    return currentBoard->getState(Position(row, col));
}

void Game::stopSearching()
{
    Util::needBreak = true;
}

void Game::updateGameState()
{
    if (stepList.empty())
    {
        gameState = GAME_STATE_RUN;
        return;
    }

    if (stepList.size() == Util::BoardSize * Util::BoardSize)
    {
        gameState = GAME_STATE_DRAW;
        return;
    }
    else
    {
        int mode = stepList.back().chessType;
        if (mode == CHESSTYPE_BAN)
        {
            gameState = GAME_STATE_BLACKBAN;
        }
        else if (mode == CHESSTYPE_5)
        {
            if (stepList.back().state == PIECE_BLACK)
            {
                gameState = GAME_STATE_BLACKWIN;
            }
            else
            {
                gameState = GAME_STATE_WHITEWIN;
            }
        }
        else
        {
            gameState = GAME_STATE_RUN;
        }
    }
}

void Game::setGameState(uint8_t state)
{
    gameState = state;
}

void Game::initGame(uint8_t boardSize)
{
    Util::setBoardSize(boardSize);
    stepList.reserve(boardSize * boardSize);
    gameState = GAME_STATE_RUN;
    if (currentBoard)
    {
        delete currentBoard;
    }
    currentBoard = new ChessBoard();
    currentBoard->initBoard();
    stepList.clear();
    //printTable(7);
}

void Game::putChess(int row, int col, uint8_t side, GAME_RULE ban)
{
    uint8_t chessMode = currentBoard->getChessType(Position(row, col), side);
    currentBoard->move(Position(row, col), side, ban);
    stepList.push_back(ChessStep(row, col, uint8_t(stepList.size()) + 1, chessMode, side == PIECE_BLACK ? true : false));
    updateGameState();
}

void Game::doNextStep(int row, int col, GAME_RULE ban)
{
    currentBoard->move(Position(row, col), ban);
    stepList.push_back(currentBoard->getLastStep());
    updateGameState();
}

void Game::stepBack(GAME_RULE ban)
{
    if (stepList.size() > 0)
    {
        ChessStep step = stepList.back();
        stepList.pop_back();
        currentBoard->unmove(step.pos, stepList.empty() ? ChessStep() : stepList.back(), ban);
        updateGameState();
    }
}

void Game::doNextStepByAI(AIENGINE type)
{
    Position pos = getNextStepByAI(type);
    doNextStep(pos.row, pos.col, AIConfig::getInstance()->rule);
}

Position Game::getNextStepByAI(AIENGINE AIType)
{
    if (stepList.empty())
    {
        return OpenEngine::getOpen1(currentBoard);
    }
    else if (stepList.size() == 1 && OpenEngine::checkOpen2(currentBoard))
    {
        return OpenEngine::getOpen2(currentBoard);
    }

    if (ai != NULL)
    {
        delete ai;
    }

    if (AIType == AISIMPLE)
    {
        ai = new AISimple();
    }
    else if (AIType == AIGOSEARCH)
    {
        ai = new AIGoSearch();
        //setting.defaultGoSearch(AILEVEL_UNLIMITED);
    }
    ChessBoard *board = new ChessBoard();
    *board = *currentBoard;

    Position pos = ai->getNextStep(board, AIConfig::getInstance()->startTimeMs);

    delete board;

    return pos;
}

string Game::debug(int mode)
{
    if (mode == 1)
    {
        return TrieTreeNode::getInstance()->testSearch();
    }
    else if (mode == 2)
    {
        string s;
        currentBoard->printGlobalEvaluate(s);
        fstream of("debug.txt", ios::out);
        of << s;
        of.close();
        return s;
    }
    else if (mode == 3)
    {
        fstream of("debug.txt", ios::out);
        string globalinfo;
        currentBoard->printGlobalEvaluate(globalinfo);
        of << globalinfo;
        of.close();
        stringstream ss;

        ss << "TrieTree_Test:" << TrieTreeNode::getInstance()->testSearch() << "\r\n";

        ss << "Evaluate: black:" << currentBoard->getGlobalEvaluate(PIECE_BLACK) << "\r\n";

        vector<pair<Position, int>> moves;

        AIGoSearch::getMoveList(currentBoard, moves, 1, true);
        ss << "normal:[";
        for (auto move : moves)
        {
            ss << "(" << (int)move.first.row << "," << (int)move.first.col << "|" << (int)move.second << ") ";
        }
        ss << "]\r\n";
        moves.clear();

        AIGoSearch::getMoveList(currentBoard, moves, 2, true);
        ss << "threat:[";
        for (auto move : moves)
        {
            ss << "(" << (int)move.first.row << "," << (int)move.first.col << "|" << (int)move.second << ") ";
        }
        ss << "]\r\n";
        moves.clear();

        DBSearch dbs(currentBoard, FREESTYLE, 2);
        vector<Position> sequence;
        dbs.doDBSearch(&sequence);
        dbs.printWholeTree();

        int count = dbs.getWinningSequenceCount();
        ss << "DBSearch tree print out to dbsearch_tree.txt \r\n";

        for (auto move : sequence)
        {
            ss << "(" << (int)move.row << "," << (int)move.col << "),";
        }

        ss << count << "\r\n";

        return ss.str();
    }
    else if (mode == 4)
    {
        time_point<system_clock> starttime = system_clock::now();
        stringstream ss;
        PNSearch pn(currentBoard, AIConfig::getInstance()->rule);
        pn.setMaxDepth(AIConfig::getInstance()->pnMaxDepth);
        pn.start();
        string result = (pn.getResult() == PROVEN || pn.getResult() == DISPROVEN) ? (pn.getResult() == PROVEN ? string("success") : string("failed")) : string("unknown");
        ss << result << " " << pn.getNodeCount() << " hit:" << pn.hit << " miss:" << pn.miss << " DBNode:" << pn.DBNodeCount << "\r\n";
        vector<Position> list;
        pn.getSequence(list);
        for (auto move : list)
        {
            ss << "(" << (int)move.row << "," << (int)move.col << "),";
        }
        ss << " time:" << duration_cast<milliseconds>(system_clock::now() - starttime).count() << "ms\r\n";
        return ss.str();
    }
    else if (mode == 5)
    {
        DBSearchPlus::node_count = 0;
        DBSearchPlus dbs(currentBoard, AIConfig::getInstance()->rule, 2, true);
        vector<Position> sequence;
        bool ret = dbs.doDBSearchPlus(sequence);
        return ret ? string("success") : string("fail");
    }

    return string("debug");
}

void Game::printTable(uint8_t len)
{
    stringstream ss;
    fstream of("debug.txt", ios::out);
    uint32_t size = 1;
    /*for (uint8_t i = 0; i < len; ++i)
    {
        size *= 2;
    }
    ss << (int)len << " " << size << "\n";
    for (uint8_t i = 0; i < size; ++i)
    {
        ss << (int)i << "\t";
        for (uint8_t j = 0; j < len; ++j)
        {
            ss << (int)ChessBoard::chessModeHashTable[len][i*len + j] << " ";
        }
        ss << "\n";
    }*/
    of << ss.str();
    of.close();
}
