#include "Game.h"
#include "ChessAI.h"

Game::Game() :currentBoard(NULL)
{
    stepList.reserve(225);
    srand(unsigned int(time(0)));
}

Game::~Game()
{

}

bool Game::initTrieTree()
{
    return ChessBoard::buildTrieTree();
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
        int mode = stepList.back().chessMode;
        if (mode == MODE_ADV_BAN)
        {
            gameState = GAME_STATE_BLACKBAN;
        }
        else if (mode == MODE_BASE_5)
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

AIRESULTFLAG Game::getForecastStatus()
{
    //return AIGameTree::getResultFlag();
    return AIRESULTFLAG();
}

HashStat Game::getTransTableStat()
{
   // return AIGameTree::getTransTableHashStat();
    return HashStat();
}

void Game::initGame()
{
    gameState = GAME_STATE_RUN;
    if (currentBoard)
    {
        delete currentBoard;
    }
    currentBoard = new ChessBoard();
    currentBoard->init_layer1();
    currentBoard->initHash();
    currentBoard->initRatings();
    stepList.clear();
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
    uint8_t chessMode = currentBoard->getThreat(row, col, side);
    currentBoard->move(util::xy2index(row, col), side);
    stepList.push_back(ChessStep(row, col, uint8_t(stepList.size()) + 1, chessMode, side == PIECE_BLACK ? true : false));
    updateGameState();
}

void Game::stepBack()
{
    if (stepList.size() > 0)
    {
        ChessStep step = stepList.back();
        stepList.pop_back();
        currentBoard->unmove(step.index);
        updateGameState();
    }
}

void Game::doNextStepByAI(uint8_t level, bool ban, AISettings setting)
{
    Position pos = getNextStepByAI(level, ban, setting);
    doNextStep(pos.row, pos.col);
}

Position Game::getNextStepByAI(uint8_t level, bool ban, AISettings setting)
{
    if (stepList.empty())
    {
        return Position{ 7,7 };
    }

    ChessAI *ai = NULL;
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
       /* ai = new AIGameTree();
        ai->applyAISettings(setting);*/
    }
    ChessBoard *board = new ChessBoard();
    *board = *currentBoard;

    Position pos = ai->getNextStep(board, util::otherside(stepList.back().getColor()), level, ban);

    delete board;
    delete ai;

    return pos;
}


string Game::getChessMode(int row, int col, int state)
{
    uint32_t chess[4] = { 0 };
    string s;
    currentBoard->formatChess2Int(chess, row, col, state);
    for (int i = 0; i < 4; i++)
    {
        SearchResult result = ChessBoard::searchTrieTree->search(chess[i]);
        if (result.chessMode > -1)
        {
            s += string(chessMode[result.chessMode].pat) + "\n";
        }
    }
    return s;
}



string Game::debug(int mode)
{
    if (mode == 1)
    {
        return ChessBoard::searchTrieTree->testSearch();
    }

    return string("debug");
}
