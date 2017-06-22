#include "Game.h"
#include "TrieTree.h"

string AIEngine::textOut;

Game::Game()
{
    stepList.reserve(225);
    srand(unsigned int(time(0)));
}

Game::~Game()
{

}

bool Game::initTrieTree()
{
    return TrieTreeNode::getInstance()->buildTrieTree();
}

bool Game::initAIHelper(int num)
{
    //AIGameTree::setThreadPoolSize(num);
    ChessBoard::initZobrist();
    ChessBoard::initChessModeHashTable();
    return true;
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

void Game::doNextStep(int row, int col)
{
    uint8_t side;
    if (stepList.empty())
    {
        side = PIECE_BLACK;
    }
    else
    {
        side = util::otherside(stepList.back().getColor());
    }
    uint8_t chessMode = currentBoard->getChessType(row, col, side);
    currentBoard->move(util::xy2index(row, col));
    stepList.push_back(ChessStep(row, col, uint8_t(stepList.size()) + 1, chessMode, side == PIECE_BLACK ? true : false));
    updateGameState();
}

void Game::stepBack()
{
    if (stepList.size() > 0)
    {
        ChessStep step = stepList.back();
        stepList.pop_back();
        currentBoard->unmove(step.index, stepList.empty()? ChessStep(0,0,0,false) : stepList.back());
        updateGameState();
    }
}

void Game::doNextStepByAI(uint8_t level, bool ban, AIParameter setting)
{
    Position pos = getNextStepByAI(level, ban, setting);
    doNextStep(pos.row, pos.col);
}

Position Game::getNextStepByAI(uint8_t level, bool ban, AIParameter setting)
{
    if (stepList.empty())
    {
        return Position{ 7,7 };
    }
    if (ai != NULL)
    {
        delete ai;
    }

    if (level == 1)
    {
        ai = new AIWalker();
    }
    else if (level == 2)
    {
        ai = new AIWalker();
    }
    else if (level == 3 || level == 4)
    {
        ai = new AIGameTree();
    }
    else if (level == 5)
    {
        ai = new AIGoSearch();
    }
    ChessBoard *board = new ChessBoard();
    *board = *currentBoard;
    AISettings set = { setting.maxSearchDepth, setting.maxSearchTimeMs,setting.multiThread };
    Position pos = ai->getNextStep(board, set, stepList.back(), util::otherside(stepList.back().getColor()), level, ban);

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
        stringstream ss;
        ss << currentBoard->getGlobalEvaluate(PIECE_BLACK) << "|" << currentBoard->getGlobalEvaluate(PIECE_WHITE);
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
