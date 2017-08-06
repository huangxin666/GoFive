#include "Game.h"
#include "TrieTree.h"
#include "ThreadPool.h"

string AIEngine::textOut;

Game::Game()
{
    stepList.reserve(225);
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
        ChessBoard::initZobrist();
        ChessBoard::initChessModeHashTable();
        return true;
    }

    return false;
}

int Game::getPieceState(int row, int col)
{
    return currentBoard->getState(row, col);
}

string Game::getDebugString()
{
    return AIEngine::textOut;
}

void Game::updateGameState()
{
    if (stepList.empty())
    {
        gameState = GAME_STATE_RUN;
        return;
    }

    if (stepList.size() == 225)
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
            if (stepList.back().black)
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

void Game::initGame()
{
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

void Game::doNextStep(int row, int col, bool ban)
{
    uint8_t side;
    if (stepList.empty())
    {
        side = PIECE_BLACK;
    }
    else
    {
        side = Util::otherside(stepList.back().getSide());
    }
    ChessBoard::setBan(ban);
    uint8_t chessMode = currentBoard->getChessType(row, col, side);
    currentBoard->move(Util::xy2index(row, col));
    stepList.push_back(ChessStep(row, col, uint8_t(stepList.size()) + 1, chessMode, side == PIECE_BLACK ? true : false));
    updateGameState();
}

void Game::stepBack()
{
    if (stepList.size() > 0)
    {
        ChessStep step = stepList.back();
        stepList.pop_back();
        currentBoard->unmove(step.index, stepList.empty() ? ChessStep(0, 0, 0, false) : stepList.back());
        updateGameState();
    }
}

void Game::doNextStepByAI(AIENGINE type, AISettings setting)
{
    Position pos = getNextStepByAI(type, setting);
    doNextStep(pos.row, pos.col, setting.ban);
}

Position Game::getNextStepByAI(AIENGINE AIType, AISettings setting)
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

    if (AIType == AIWALKER_ATACK)
    {
        ai = new AIWalker(1);
    }
    else if (AIType == AIWALKER_DEFEND)
    {
        ai = new AIWalker(2);
    }
    else if (AIType == AIGAMETREE)
    {
        ai = new AIGameTree();
    }
    else if (AIType == AIGOSEARCH)
    {
        ai = new AIGoSearch();
        //setting.defaultGoSearch(AILEVEL_UNLIMITED);
    }
    ChessBoard *board = new ChessBoard();
    *board = *currentBoard;
    ai->applyAISettings(setting);
    Position pos = ai->getNextStep(board, system_clock::to_time_t(system_clock::now()));

    delete board;

    return pos;
}


string Game::getChessMode(int row, int col, int state)
{
    uint32_t chess[4] = { 0 };
    string s;
    currentBoard->formatChess2Int(chess, row, col, state);
    for (int i = 0; i < 4; i++)
    {
        SearchResult result = TrieTreeNode::getInstance()->search(chess[i]);
        if (result.chessMode > -1)
        {
            s += string(chessMode[result.chessMode].pat) + "\r\n";
        }
    }
    return s;
}



string Game::debug(int mode)
{
    if (mode == 1)
    {
        return TrieTreeNode::getInstance()->testSearch();
    }
    else if (mode == 2)
    {
        fstream of("debug.txt", ios::out);
        string globalinfo;
        currentBoard->printGlobalEvaluate(globalinfo);
        of << globalinfo;
        of.close();
        stringstream ss;

        ss << "TrieTree_Test:" << TrieTreeNode::getInstance()->testSearch() << "\r\n";

        ss << "Evaluate: black:" << currentBoard->getGlobalEvaluate(PIECE_BLACK) << "\r\n";

        vector<pair<uint8_t, int>> moves;

        AIGoSearch::getMoveList(currentBoard, moves, 1, true);
        ss << "normal:[";
        for (auto move : moves)
        {
            ss << "(" << (int)Util::getrow(move.first) << "," << (int)Util::getcol(move.first) << "|" << (int)move.second << ") ";
        }
        ss << "]\r\n";
        moves.clear();

        AIGoSearch::getMoveList(currentBoard, moves, 2, true);
        ss << "vct:[";
        for (auto move : moves)
        {
            ss << "(" << (int)Util::getrow(move.first) << "," << (int)Util::getcol(move.first) << "|" << (int)move.second << ") ";
        }
        ss << "]\r\n";
        moves.clear();

        AIGoSearch::getMoveList(currentBoard, moves, 3, true);
        ss << "vcf:[";
        for (auto move : moves)
        {
            ss << "(" << (int)Util::getrow(move.first) << "," << (int)Util::getcol(move.first) << "|" << (int)move.second << ") ";
        }
        ss << "]\r\n";
        moves.clear();
        return ss.str();
    }

    return string("debug");
}

void Game::printTable(uint8_t len)
{
    stringstream ss;
    fstream of("debug.txt", ios::out);
    uint32_t size = 1;
    for (uint8_t i = 0; i < len; ++i)
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
    }
    of << ss.str();
    of.close();
}
