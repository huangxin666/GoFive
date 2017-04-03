#include "stdafx.h"
#include "Game.h"
#include "AIGameTree.h"
#include "AIWalker.h"
#include "ThreadPool.h"

Game::Game(): AIlevel(3), HelpLevel(1), playerSide(1), currentBoard(NULL), playerToPlayer(false), showStep(false)
{
    stepList.reserve(225);
    srand(unsigned int(time(0)));
    parameter.ban = true;
    parameter.multithread = true;
    parameter.caculateSteps = 6;
}

Game::~Game()
{
}

bool Game::initTrieTree()
{
    return ChessBoard::buildTrieTree();
}

bool Game::initThreadPool(int num)
{
    ThreadPool::num_thread = num;
    return true;
}

void Game::setShowStep(bool b)
{
    showStep = b;
}

bool Game::isShowStep()
{
    return showStep;
}

bool Game::isMultithread()
{
    return parameter.multithread;
}
void Game::setMultithread(bool s)
{
    if (uGameState != GAME_STATE_WAIT)
        parameter.multithread = s;
}

Piece &Game::getPiece(int row, int col)
{
    return currentBoard->getPiece(row, col);
}

const std::vector<ChessStep> &Game::getStepList()
{
    return stepList;
}

int Game::getGameState()
{
    return uGameState;
}
int Game::getPlayerSide()
{
    return playerSide;
}

bool Game::stepListIsEmpty()
{
    return stepList.empty();
}

void Game::setAIlevel(int level)
{
    if (uGameState != GAME_STATE_WAIT)
    {
        AIlevel = level;
    }
       
}
void Game::setHelpLevel(int level)
{
    if (uGameState != GAME_STATE_WAIT)
        HelpLevel = level;
}

int Game::getAIlevel()
{
    return AIlevel;
}
int Game::getHelpLevel()
{
    return HelpLevel;
}

bool Game::isPlayerToPlayer()
{
    return playerToPlayer;
}

void Game::setPlayerToPlayer(bool s)
{
    if (uGameState != GAME_STATE_WAIT)
        playerToPlayer = s;
}

void Game::changeSide(int side)
{
    if (playerSide != side)
    {
        playerSide = side;
        if (uGameState == GAME_STATE_RUN && !playerToPlayer)
        {
            AIWork(false);
        }
    }
}

void Game::setCaculateStep(UINT s)
{
    parameter.caculateSteps = s;
}
byte Game::getCaculateStep()
{
    return parameter.caculateSteps;
}

bool Game::isBan()
{
    return parameter.ban;
}

void Game::setBan(bool b)
{
    if (uGameState != GAME_STATE_WAIT)
    {
        parameter.ban = b;
        ChessBoard::setBan(b);
    }
}

void Game::updateGameState()
{
    if (!checkVictory())
    {
        if (stepList.size() == 225) {
            uGameState = GAME_STATE_DRAW;
        }
        else
            uGameState = GAME_STATE_RUN;
    }
}

void Game::setGameState(int state)
{
    uGameState = state;
}

void Game::initGame()
{
    uGameState = GAME_STATE_RUN;
    if (currentBoard)
    {
        delete currentBoard;
    }
    currentBoard = new ChessBoard();
    stepList.clear();

    if (playerToPlayer)
        playerSide = 1;
    if (!playerToPlayer&&playerSide == -1) {
        //棋子操作
        AIWork(false);
    }
}

BOOL Game::checkVictory()
{
    if (stepList.empty())
        return false;
    int state = currentBoard->getLastPiece().state;
    int score = currentBoard->getLastStepScores(true);
    if (parameter.ban&&state == 1 && score < 0)//禁手判断
    {
        uGameState = GAME_STATE_BLACKBAN;
        return true;
    }
    if (score >= SCORE_5_CONTINUE) {
        if (state == 1) {
            uGameState = GAME_STATE_BLACKWIN;
        }
        else if (state == -1) {
            uGameState = GAME_STATE_WHITEWIN;
        }
        return true;
    }
    return false;
}

void Game::stepBack()
{
    if (playerToPlayer)
    {
        if (stepList.size() > 0)
        {
            currentBoard->getPiece(stepList.back().row, stepList.back().col).state = (0);
            stepList.pop_back();
            playerSide = -playerSide;
            ChessStep step;
            if (stepList.empty())
                step.step = 0;
            else
                step = stepList.back();
            currentBoard->lastStep = (step);
            currentBoard->resetHotArea();
            if (uGameState != GAME_STATE_RUN)
                uGameState = GAME_STATE_RUN;
        }
    }
    else
    {
        if (stepList.size() > 1)
        {
            if (currentBoard->getLastPiece().state == playerSide)
            {
                currentBoard->getPiece(stepList.back().row, stepList.back().col).state = (0);
                stepList.pop_back();
            }
            else
            {
                currentBoard->getPiece(stepList.back().row, stepList.back().col).state = (0);
                stepList.pop_back();
                currentBoard->getPiece(stepList.back().row, stepList.back().col).state = (0);
                stepList.pop_back();
            }
            ChessStep step;
            if (stepList.empty())
                step.step = 0;
            else
                step = stepList.back();
            currentBoard->lastStep = (step);
            currentBoard->resetHotArea();
            if (uGameState != GAME_STATE_RUN)
                uGameState = GAME_STATE_RUN;
        }
    }
}

void Game::playerWork(int row, int col)
{
    currentBoard->doNextStep(row, col, playerSide);
    stepList.push_back(ChessStep(uint8_t(stepList.size()) + 1, row, col, playerSide == 1 ? true : false));
    updateGameState();
    if (playerToPlayer)
        playerSide = -playerSide;
}

Position Game::getNextStepByAI(byte level)
{
    ChessAI *ai = NULL;
    if (level == 1)
    {
        ai = new AIWalker();
    }
    else if (level == 2)
    {
        ai = new AIWalker();
    }
    else if (level == 3)
    {
        ai = new AIGameTree();
    }
    else
    {
        //error
    }
    ChessBoard *board = new ChessBoard();
    *board = *currentBoard;
    parameter.level = level;

    Position pos = ai->getNextStep(board, parameter);

    delete board;
    delete ai;

    return pos;
}

void Game::AIWork(bool isHelp)
{
    int stepColor = isHelp ? playerSide : -playerSide;
    int level = isHelp ? HelpLevel : AIlevel;
    Position pos = getNextStepByAI(level);
    //棋子操作
    currentBoard->doNextStep(pos.row, pos.col, stepColor);
    stepList.push_back(ChessStep(uint8_t(stepList.size()) + 1, pos.row, pos.col, stepColor == 1 ? true : false));
    updateGameState();
}

extern ChessModeData chessMode[TRIE_COUNT];

void Game::getChessMode(char *str, int row, int col, int state)
{
    char chess[4][FORMAT_LENGTH];
    string s;
    currentBoard->formatChess2String(chess, row, col, state);
    for (int i = 0; i < 4; i++)
    {
        SearchResult result = ChessBoard::searchTrieTree->search(chess[i]);
        if (result.chessMode > -1)
        {
            s += string(chessMode[result.chessMode].pat) + "\n";
        }
    }

    strcpy(str, s.c_str());
}

#pragma comment (lib, "Version.lib")
BOOL GetMyProcessVer(CString& strver)   //用来取得自己的版本号   
{
    TCHAR strfile[MAX_PATH];
    GetModuleFileName(NULL, strfile, sizeof(strfile));  //这里取得自己的文件名   

    DWORD dwVersize = 0;
    DWORD dwHandle = 0;

    dwVersize = GetFileVersionInfoSize(strfile, &dwHandle);
    if (dwVersize == 0)
    {
        return FALSE;
    }

    TCHAR szVerBuf[1024] = _T("");
    if (GetFileVersionInfo(strfile, 0, dwVersize, szVerBuf))
    {
        VS_FIXEDFILEINFO* pInfo;
        UINT nInfoLen;

        if (VerQueryValue(szVerBuf, _T("\\"), (LPVOID*)&pInfo, &nInfoLen))
        {
            strver.Format(_T("%d.%d.%d.%d"), HIWORD(pInfo->dwFileVersionMS),
                LOWORD(pInfo->dwFileVersionMS), HIWORD(pInfo->dwFileVersionLS),
                LOWORD(pInfo->dwFileVersionLS));

            return TRUE;
        }
    }
    return FALSE;
}

bool Game::saveBoard(CString path)
{
    CFile oFile(path, CFile::
        modeCreate | CFile::modeWrite);
    CArchive oar(&oFile, CArchive::store);
    //写入版本号
    CString version;
    if (!GetMyProcessVer(version))
    {
        version = _T("0.0.0.0");
    }
    oar << version;
    //写入stepList
    for (UINT i = 0; i < stepList.size(); ++i)
    {
        oar << stepList[i].step << stepList[i].row << stepList[i].col << stepList[i].black;
    }
    oar.Close();
    oFile.Close();
    return true;
}

bool Game::loadBoard(CString path)
{
    /*CFile oFile(path, CFile::modeRead);*/
    CFile oFile;
    CFileException fileException;
    if (!oFile.Open(path, CFile::modeRead, &fileException))
    {
        return false;
    }

    CArchive oar(&oFile, CArchive::load);

    //读入版本号
    CString version;
    oar >> version;
    //初始化棋盘
    if (currentBoard)
    {
        delete currentBoard;
    }
    currentBoard = new ChessBoard();
    stepList.clear();
    //读入stepList
    byte step, uRow, uCol; bool black;
    while (!oar.IsBufferEmpty())
    {
        oar >> step >> uRow >> uCol >> black;
        stepList.push_back(ChessStep(step, uRow, uCol, black));
    }

    for (UINT i = 0; i < stepList.size(); ++i)
    {
        currentBoard->getPiece(stepList[i].row, stepList[i].col).state = stepList[i].getColor();//black ? 1 : -1);
    }
    if (!stepList.empty())
    {
        currentBoard->lastStep = stepList.back();
        currentBoard->resetHotArea();
    }

    updateGameState();

    if (stepList.empty())
        playerSide = 1;
    else if (uGameState == GAME_STATE_RUN)
        playerSide = -currentBoard->getLastPiece().state;

    oar.Close();
    oFile.Close();
    return true;
}

CString Game::debug(int mode)
{
    if (mode == 1)
    {
        return CString(ChessBoard::searchTrieTree->testSearch().c_str());
    }
    else if (mode == 2)
    {
        return CString(currentBoard->toString().c_str());
    }
    return CString(_T("debug"));
}

