#include "stdafx.h"
#include "Game.h"
#include "AIGameTree.h"
#include "AIWalker.h"

Game::Game() : AIlevel(4), HelpLevel(1), playerSide(1), currentBoard(NULL), playerToPlayer(false), showStep(false)
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

bool Game::initAIHelper(int num)
{
    AIGameTree::setThreadPoolSize(num);
    ChessBoard::initZobrist();
    if (num < 4)
    {
        parameter.caculateSteps = 4;
    }
    return true;
}

int Game::getPieceState(int row, int col)
{
    return currentBoard->getPiece(row, col).state;
}

void Game::changeSide(int side)
{
    if (playerSide != side)
    {
        playerSide = side;
    }
}

bool Game::isBan()
{
    return parameter.ban;
}

void Game::setBan(bool b)
{
    if (gameState != GAME_STATE_WAIT)
    {
        parameter.ban = b;
        ChessBoard::setBan(b);
    }
}

void Game::updateGameState()
{
    if (!checkVictory())
    {
        if (stepList.size() == 225)
        {
            gameState = GAME_STATE_DRAW;
        }
        else
        {
            gameState = GAME_STATE_RUN;
        }
    }
}

void Game::setGameState(int state)
{
    gameState = state;
}

AIRESULTFLAG Game::getForecastStatus()
{
    return AIGameTree::getResultFlag();
}

HashStat Game::getTransTableStat()
{
    return AIGameTree::getTransTableHashStat();
}

void Game::initGame()
{
    gameState = GAME_STATE_RUN;
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
        AIWork(AIlevel, -playerSide);
    }
}

bool Game::checkVictory()
{
    if (stepList.empty())
        return false;
    int state = currentBoard->getLastPiece().state;
    int score = currentBoard->getLastStepScores(true);
    if (parameter.ban&&state == 1 && score < 0)//禁手判断
    {
        gameState = GAME_STATE_BLACKBAN;
        return true;
    }
    if (score >= SCORE_5_CONTINUE) {
        if (state == 1) {
            gameState = GAME_STATE_BLACKWIN;
        }
        else if (state == -1) {
            gameState = GAME_STATE_WHITEWIN;
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
            if (stepList.empty()) step.step = 0;
            else step = stepList.back();
            currentBoard->lastStep = (step);
            currentBoard->initHotArea();
            if (gameState != GAME_STATE_RUN)  gameState = GAME_STATE_RUN;
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
            if (stepList.empty()) step.step = 0;
            else step = stepList.back();
            currentBoard->lastStep = (step);
            currentBoard->initHotArea();
            if (gameState != GAME_STATE_RUN) gameState = GAME_STATE_RUN;
        }
    }
}

void Game::playerWork(int row, int col)
{
    currentBoard->doNextStep(row, col, playerSide);
    stepList.push_back(ChessStep(uint8_t(stepList.size()) + 1, row, col, playerSide == 1 ? true : false));
    updateGameState();
    if (playerToPlayer) playerSide = -playerSide;
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
    else if (level == 3 || level == 4)
    {
        ai = new AIGameTree();
    }
    ChessBoard *board = new ChessBoard();
    *board = *currentBoard;
    parameter.level = level;

    Position pos = ai->getNextStep(board, parameter);

    delete board;
    delete ai;

    return pos;
}

void Game::AIWork(int level, int side)
{
    Position pos = getNextStepByAI(level);
    //棋子操作
    currentBoard->doNextStep(pos.row, pos.col, side);
    stepList.push_back(ChessStep(uint8_t(stepList.size()) + 1, pos.row, pos.col, side == 1 ? true : false));
    updateGameState();
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
        oar << (byte)stepList[i].step << (byte)stepList[i].row << (byte)stepList[i].col << (bool)stepList[i].black;
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
        currentBoard->initHotArea();
    }

    updateGameState();

    if (stepList.empty())
        playerSide = 1;
    else if (gameState == GAME_STATE_RUN)
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
