#include "stdafx.h"
#include "GoFive.h"
#include "ChildView.h"
#include "DlgSettings.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CChildView

static CWinThread* AIWorkThread;

CChildView::CChildView() :AIlevel(1), HelpLevel(1), showStep(false), caculateSteps(6), multithread(true), ban(true), waitAI(false)
{
    currentPos.enable = false;
    oldPos.enable = false;
    gameMode = GAME_MODE::PLAYER_FIRST;
    game = new Game();
    if (!game->initTrieTree())
    {
        MessageBox(_T("初始化字典树失败！"), _T("error"), MB_OK);
    }
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    int thread_num;
    if (si.dwNumberOfProcessors > 4)
    {
        thread_num = si.dwNumberOfProcessors - 1;
    }
    else
    {
        thread_num = si.dwNumberOfProcessors;
    }
    game->initAIHelper(thread_num);
    game->initGame();
}

CChildView::~CChildView()
{
    delete game;
}


BEGIN_MESSAGE_MAP(CChildView, CWnd)
    ON_WM_PAINT()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONDOWN()
    ON_COMMAND(ID_STEPBACK, &CChildView::OnStepback)
    ON_COMMAND(ID_START, &CChildView::OnStart)

    ON_COMMAND(ID_FIRSTHAND, &CChildView::OnFirsthand)
    ON_UPDATE_COMMAND_UI(ID_FIRSTHAND, &CChildView::OnUpdateFirsthand)
    ON_COMMAND(ID_SECONDHAND, &CChildView::OnSecondhand)
    ON_UPDATE_COMMAND_UI(ID_SECONDHAND, &CChildView::OnUpdateSecondhand)
    ON_COMMAND(ID_AI_PRIMARY, &CChildView::OnAIPrimary)
    ON_UPDATE_COMMAND_UI(ID_AI_PRIMARY, &CChildView::OnUpdateAIPrimary)
    ON_COMMAND(ID_AI_SECONDRY, &CChildView::OnAISecondry)
    ON_UPDATE_COMMAND_UI(ID_AI_SECONDRY, &CChildView::OnUpdateAISecondry)
    ON_WM_RBUTTONDOWN()
    ON_COMMAND(ID_AI_ADVANCED, &CChildView::OnAIAdvanced)
    ON_UPDATE_COMMAND_UI(ID_AI_ADVANCED, &CChildView::OnUpdateAIAdvanced)
    ON_COMMAND(ID_SAVE, &CChildView::OnSave)
    ON_COMMAND(ID_LOAD, &CChildView::OnLoad)
    ON_COMMAND(ID_HELP_PRIMARY, &CChildView::OnHelpPrimary)
    ON_COMMAND(ID_HELP_SECONDRY, &CChildView::OnHelpSecondry)
    ON_COMMAND(ID_HELP_ADVANCED, &CChildView::OnHelpAdvanced)
    ON_UPDATE_COMMAND_UI(ID_HELP_PRIMARY, &CChildView::OnUpdateHelpPrimary)
    ON_UPDATE_COMMAND_UI(ID_HELP_SECONDRY, &CChildView::OnUpdateHelpSecondry)
    ON_UPDATE_COMMAND_UI(ID_HELP_ADVANCED, &CChildView::OnUpdateHelpAdvanced)
    ON_COMMAND(ID_AIHELP, &CChildView::OnAIhelp)
    ON_COMMAND(ID_DEBUG, &CChildView::OnDebug)
    ON_COMMAND(ID_PLAYERTOPLAYER, &CChildView::OnPlayertoplayer)
    ON_UPDATE_COMMAND_UI(ID_PLAYERTOPLAYER, &CChildView::OnUpdatePlayertoplayer)
    ON_WM_TIMER()
    ON_COMMAND(ID_SETTINGS, &CChildView::OnSettings)
    ON_COMMAND(ID_MULTITHREAD, &CChildView::OnMultithread)
    ON_UPDATE_COMMAND_UI(ID_MULTITHREAD, &CChildView::OnUpdateMultithread)
    ON_COMMAND(ID_BAN, &CChildView::OnBan)
    ON_UPDATE_COMMAND_UI(ID_BAN, &CChildView::OnUpdateBan)
    ON_COMMAND(ID_SHOWSTEP, &CChildView::OnShowStep)
    ON_UPDATE_COMMAND_UI(ID_SHOWSTEP, &CChildView::OnUpdateShowStep)
    ON_WM_ERASEBKGND()
    ON_COMMAND(ID_AI_MASTER, &CChildView::OnAIMaster)
    ON_UPDATE_COMMAND_UI(ID_AI_MASTER, &CChildView::OnUpdateAIMaster)
END_MESSAGE_MAP()



BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CWnd::PreCreateWindow(cs))
        return FALSE;

    cs.dwExStyle |= WS_EX_CLIENTEDGE;
    cs.style &= ~WS_BORDER;
    cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
        ::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), NULL);

    return TRUE;
}

void CChildView::init()
{
    CDC* dc = GetDC();
    int dpiX = GetDeviceCaps(dc->GetSafeHdc(), LOGPIXELSX);
    int dpiY = GetDeviceCaps(dc->GetSafeHdc(), LOGPIXELSY);
    ReleaseDC(dc);

    myProgress.Create(WS_CHILD | PBS_SMOOTH,
        CRect(BROARD_X / 2 - 50, BLANK + BROARD_Y, BROARD_X / 2 + BLANK + 50, BLANK + BROARD_Y + 20), this, 1);

    myProgressStatic.Create(_T("AI思考中："), WS_CHILD | SS_CENTER,
        CRect(BROARD_X / 2 - 150, BLANK + BROARD_Y, BROARD_X / 2 - 50, BLANK + BROARD_Y + 20), this);

    infoStatic.Create(_T(""), WS_CHILD | SS_CENTER | WS_VISIBLE, CRect(BLANK + 2, 2, (BROARD_X + BLANK - 2), BLANK - 2), this);

    debugStatic.Create(WS_CHILD | WS_VISIBLE| ES_AUTOVSCROLL | ES_MULTILINE | ES_WANTRETURN, CRect(BROARD_X + BLANK + 10, BLANK, (BROARD_X + BLANK * 2) + 370, BLANK + 520), this, 66);

    font.CreatePointFont(110 * DEFAULT_DPI / dpiX, _T("微软雅黑"), NULL);

    myProgressStatic.SetFont(&font);
    infoStatic.SetFont(&font);
    debugStatic.SetFont(&font);
    debugStatic.SetReadOnly(TRUE);
 
    updateInfoStatic();
}

void CChildView::updateInfoStatic()
{
    CString info;
    if (gameMode == GAME_MODE::NO_AI)
    {
        info.Append(_T("人人对战"));
    }
    else if (gameMode == GAME_MODE::NO_PLAYER)
    {

    }
    else
    {
        info.AppendFormat(_T("玩家：%s    禁手：%s    AI等级："), gameMode == GAME_MODE::PLAYER_FIRST ? _T("先手") : _T("后手"), ban ? _T("有") : _T("无"));
        switch (AIlevel)
        {
        case 1:
            info.AppendFormat(_T("低级    搜索深度：2"));
            break;
        case 2:
            info.AppendFormat(_T("中级    搜索深度：4"));
            break;
        case 3:
            info.AppendFormat(_T("高级    搜索深度：%d"), caculateSteps * 2);
            break;
        case 4:
            info.AppendFormat(_T("大师    搜索深度：%d"), caculateSteps * 2);
            break;
        default:
            info += "未知";
            break;
        }
    }

    infoStatic.SetWindowTextW(info);
}

void CChildView::OnPaint()
{
    CPaintDC dc(this);    // 用以屏幕显示的设备
    CDC dcMemory;  // 内存设备
    CBitmap bitmap;
    CRect m_rcClient;
    GetClientRect(&m_rcClient);
    if (!dc.IsPrinting())
    {
        // 与dc设备兼容
        if (dcMemory.CreateCompatibleDC(&dc))
        {
            // 使得bitmap与实际显示的设备兼容
            if (bitmap.CreateCompatibleBitmap(&dc, m_rcClient.right, m_rcClient.bottom))
            {
                // 内存设备选择物件－位图
                //绘制背景框
                dcMemory.SelectObject(&bitmap);
                DrawBack(&dcMemory);
                DrawChessBoard(&dcMemory);
                DrawMouseFocus(&dcMemory);
                DrawChess(&dcMemory);
                // 将内存设备的内容拷贝到实际屏幕显示的设备
                dc.BitBlt(m_rcClient.left, m_rcClient.top, m_rcClient.right, m_rcClient.bottom, &dcMemory, 0, 0, SRCCOPY);
                bitmap.DeleteObject();
            }
        }
    }
}

void CChildView::DrawBack(CDC *pDC)
{
    CDC dcMemory;
    CRect rect;
    GetClientRect(&rect);
    dcMemory.CreateCompatibleDC(pDC);
    CBitmap bmpBackground;
    bmpBackground.LoadBitmap(IDB_BACKGROUND);
    dcMemory.SelectObject(&bmpBackground);
    pDC->StretchBlt(0, 0, rect.Width(), rect.Height(), &dcMemory, 0, 0, rect.Width(), rect.Height(), SRCCOPY);
}

void CChildView::DrawChessBoard(CDC *pDC)
{
    CDC dcMemory;
    CRect rect;
    GetClientRect(&rect);
    dcMemory.CreateCompatibleDC(pDC);
    CBitmap bmpBackground;
    bmpBackground.LoadBitmap(IDB_CHESSBOARD);
    dcMemory.SelectObject(&bmpBackground);
    pDC->StretchBlt(BLANK, BLANK, BROARD_X, BROARD_Y, &dcMemory, 0, 0, BROARD_X, BROARD_Y, SRCCOPY);
}

void CChildView::DrawChess(CDC* pDC)
{
    BITMAP bm;
    CDC ImageDC;
    CBitmap ForeBMP;
    CBitmap *pOldImageBMP;
    ChessStep p;

    for (UINT i = 0; i < game->getStepsCount(); ++i)
    {
        CString str;
        str.Format(TEXT("%d"), i + 1);
        p = game->getStep(i);
        ImageDC.CreateCompatibleDC(pDC);
        ForeBMP.LoadBitmap(p.getColor() == PIECE_BLACK ? IDB_CHESS_BLACK : IDB_CHESS_WHITE);
        ForeBMP.GetBitmap(&bm);
        pOldImageBMP = ImageDC.SelectObject(&ForeBMP);
        TransparentBlt(pDC->GetSafeHdc(), 2 + BLANK + p.getCol() * 35, 4 + BLANK + p.getRow() * 35, 36, 36,
            ImageDC.GetSafeHdc(), 0, 0, bm.bmWidth, bm.bmHeight, p.getColor() == PIECE_BLACK ? RGB(255, 255, 255) : RGB(50, 100, 100));
        if (showStep)
        {
            pDC->SetBkMode(TRANSPARENT);
            pDC->SetTextColor(p.getColor() == PIECE_BLACK ? RGB(255, 255, 255) : RGB(0, 0, 0));
            pDC->TextOut(14 + BLANK + p.getCol() * 35, 14 + BLANK + p.getRow() * 35, str);
        }
        ImageDC.SelectObject(pOldImageBMP);
        ForeBMP.DeleteObject();
        ImageDC.DeleteDC();
    }
    //画焦点
    if (game->getStepsCount() != 0)
    {
        p = game->getLastStep();//获取当前棋子
        ImageDC.CreateCompatibleDC(pDC);
        ForeBMP.LoadBitmap(p.getColor() == PIECE_BLACK ? IDB_CHESS_BLACK_FOCUS : IDB_CHESS_WHITE_FOCUS);
        ForeBMP.GetBitmap(&bm);
        pOldImageBMP = ImageDC.SelectObject(&ForeBMP);
        TransparentBlt(pDC->GetSafeHdc(), 2 + BLANK + p.getCol() * 35, 4 + BLANK + p.getRow() * 35, 36, 36,
            ImageDC.GetSafeHdc(), 0, 0, bm.bmWidth, bm.bmHeight, p.getColor() == PIECE_BLACK ? RGB(255, 255, 255) : RGB(50, 100, 100));
        ImageDC.SelectObject(pOldImageBMP);
        ForeBMP.DeleteObject();
        ImageDC.DeleteDC();
    }
}

void CChildView::DrawMouseFocus(CDC * pDC)
{
    if (currentPos.enable)
    {
        BITMAP bm;
        CDC ImageDC;
        ImageDC.CreateCompatibleDC(pDC);
        CBitmap ForeBMP;
        ForeBMP.LoadBitmap(IDB_MOUSE_FOCUS);
        ForeBMP.GetBitmap(&bm);
        CBitmap *pOldImageBMP = ImageDC.SelectObject(&ForeBMP);
        TransparentBlt(pDC->GetSafeHdc(), 2 + BLANK + currentPos.col * 35, 4 + BLANK + currentPos.row * 35, 36, 36,
            ImageDC.GetSafeHdc(), 0, 0, bm.bmWidth, bm.bmHeight, RGB(255, 255, 255));
        ImageDC.SelectObject(pOldImageBMP);
        ForeBMP.DeleteObject();
        ImageDC.DeleteDC();
    }
}

bool CChildView::checkVictory(int state)
{
    if (state == GAME_STATE_BLACKWIN)
        MessageBox(_T("黑棋五连胜利！"), _T(""), MB_OK);
    else if (state == GAME_STATE_WHITEWIN)
        MessageBox(_T("白棋五连胜利！"), _T(""), MB_OK);
    else if (state == GAME_STATE_DRAW)
        MessageBox(_T("和局！"), _T(""), MB_OK);
    else if (state == GAME_STATE_BLACKBAN)
        MessageBox(_T("黑棋禁手，白棋胜！"), _T(""), MB_OK);
    else
        return false;

    return true;
}

void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
    CClientDC dc(this);
    CRect rcBroard(BLANK, BLANK, BROARD_X + BLANK, BROARD_Y + BLANK);
    if (game->getGameState() == GAME_STATE_RUN) {
        int col = (point.x - 2 - BLANK) / 35;
        int row = (point.y - 4 - BLANK) / 35;
        if (col < 15 && row < 15 && point.x >= 2 + BLANK&&point.y >= 4 + BLANK) {
            if (game->getPieceState(row, col) == PIECE_BLANK) {
                currentPos = { row, col, true };
                SetClassLong(this->GetSafeHwnd(),
                    GCL_HCURSOR,
                    (LONG)LoadCursor(NULL, IDC_HAND));
            }
            else if (game->getPieceState(row, col) != PIECE_BLANK) {
                currentPos.enable = false;
                SetClassLong(this->GetSafeHwnd(),
                    GCL_HCURSOR,
                    (LONG)LoadCursor(NULL, IDC_NO));
            }
        }
        else {
            currentPos.enable = false;
            SetClassLong(this->GetSafeHwnd(),
                GCL_HCURSOR,
                (LONG)LoadCursor(NULL, IDC_ARROW));
        }
    }
    else {
        currentPos.enable = false;
        SetClassLong(this->GetSafeHwnd(),
            GCL_HCURSOR,
            (LONG)LoadCursor(NULL, IDC_ARROW));
    }

    if (currentPos.enable && !(oldPos == currentPos)) {
        InvalidateRect(CRect(2 + BLANK + currentPos.col * 35, 4 + BLANK + currentPos.row * 35,
            38 + BLANK + currentPos.col * 35, 40 + BLANK + currentPos.row * 35), FALSE);
    }
    if (oldPos.enable && !(oldPos == currentPos)) {
        InvalidateRect(CRect(2 + BLANK + oldPos.col * 35, 4 + BLANK + oldPos.row * 35,
            38 + BLANK + oldPos.col * 35, 40 + BLANK + oldPos.row * 35), FALSE);
    }
    oldPos = currentPos;
    CWnd::OnMouseMove(nFlags, point);
}

void CChildView::OnRButtonDown(UINT nFlags, CPoint point)
{
    OnStepback();
    CWnd::OnRButtonDown(nFlags, point);
}

void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
    CClientDC dc(this);
    CRect rcBroard(0 + BLANK, 0 + BLANK, BROARD_X + BLANK, BROARD_Y + BLANK);
    if (rcBroard.PtInRect(point) && game->getGameState() == GAME_STATE_RUN && !waitAI)
    {
        int col = (point.x - 2 - BLANK) / 35;
        int row = (point.y - 4 - BLANK) / 35;
        if (game->getPieceState(row, col) == PIECE_BLANK && row < 15 && col < 15 && point.x >= 2 + BLANK&&point.y >= 4 + BLANK)
        {
            //棋子操作
            int side = 0;
            if (gameMode == GAME_MODE::PLAYER_FIRST)
            {
                side = PIECE_BLACK;
            }
            else if (gameMode == GAME_MODE::AI_FIRST)
            {
                side = PIECE_WHITE;
            }
            else if (gameMode == GAME_MODE::NO_AI)
            {
                side = game->getStepsCount() == 0 ? PIECE_BLACK : util::otherside(game->getLastStep().getColor());
            }

            game->doNextStep(row, col);
            debugStatic.SetWindowTextW(CString(game->getChessMode(row, col, side).c_str()));
            currentPos.enable = false;
            oldPos = currentPos;
            SetClassLong(this->GetSafeHwnd(), GCL_HCURSOR, (LONG)LoadCursor(NULL, IDC_NO));
            InvalidateRect(rcBroard, false);
            checkVictory(game->getGameState());
            AIWork(AIlevel);
        }
    }

    CWnd::OnLButtonDown(nFlags, point);
}

void CChildView::AIWork(uint8_t level)
{
    if (game->getGameState() == GAME_STATE_RUN)
    {
        if (gameMode != GAME_MODE::NO_AI)
        {
            waitAI = true;
            startProgress();
            AIWorkThreadData* data = new AIWorkThreadData;
            data->view = this;
            data->level = level;
            AIWorkThread = AfxBeginThread(AIWorkThreadFunc, (void*)data);
        }

    }
}

UINT CChildView::AIWorkThreadFunc(LPVOID lpParam)
{
    srand(unsigned int(time(0)));
    AIWorkThreadData* data = (AIWorkThreadData*)lpParam;
    AIParameter settings;
    settings.multiThread = data->view->multithread;
    settings.maxSearchDepth = data->view->caculateSteps;
    data->view->game->doNextStepByAI(data->level, data->view->ban, settings);
    data->view->waitAI = false;
    return 0;
}

void CChildView::startProgress()
{
    myProgress.ShowWindow(SW_SHOWNA);
    myProgressStatic.ShowWindow(SW_SHOWNA);
    myProgress.SetPos(0);
    SetTimer(1, 100, NULL);
}

void CChildView::endProgress()
{
    KillTimer(1);
    myProgress.ShowWindow(SW_HIDE);
    myProgressStatic.ShowWindow(SW_HIDE);
}

static int time_counter = 0;

void CChildView::OnTimer(UINT_PTR nIDEvent)
{
    if (1 == nIDEvent)
    {
        if (waitAI)
        {
            myProgress.StepIt();
            time_counter++;
            CString s(game->getAITextOut().c_str());
            debugStatic.SetWindowTextW(s);
            debugStatic.LineScroll(debugStatic.GetLineCount());
        }
        else//结束
        {
            endProgress();
            InvalidateRect(CRect(0 + BLANK, 0 + BLANK, BROARD_X + BLANK, BROARD_Y + BLANK), FALSE);
            checkVictory(game->getGameState());

            CString s(game->getAITextOut().c_str());
            s.AppendFormat(_T("\n用时：%.1f"), float(time_counter) / 10);
            time_counter = 0;
            debugStatic.SetWindowTextW(s);
            debugStatic.LineScroll(debugStatic.GetLineCount());
        }

    }
    CWnd::OnTimer(nIDEvent);
}

void CChildView::OnStepback()
{
    if (!waitAI)
    {
        if (gameMode == GAME_MODE::NO_AI)
        {
            if (game->getStepsCount() > 0)
            {
                game->stepBack();
            }
        }
        else if (gameMode == GAME_MODE::PLAYER_FIRST)
        {
            if (game->getStepsCount() > 1)
            {
                game->stepBack();
                if (game->getLastStep().black)
                {
                    game->stepBack();
                }
            }
        }
        else if (gameMode == GAME_MODE::AI_FIRST)
        {
            if (game->getStepsCount() > 1)
            {
                game->stepBack();
                if (!game->getLastStep().black)
                {
                    game->stepBack();
                }
            }
        }
        Invalidate(FALSE);
    }
}

void CChildView::OnStart()
{
    if (!waitAI)
    {
        game->initGame();
        currentPos.enable = false;
        oldPos.enable = false;
        Invalidate(FALSE);
    }
}

void CChildView::OnAIhelp()
{
    if (game->getGameState() == GAME_STATE_RUN && !waitAI)
    {
        AIParameter settings;
        settings.multiThread = multithread;
        settings.maxSearchDepth = caculateSteps;
        game->doNextStepByAI(HelpLevel, ban, settings);
        Invalidate(FALSE);
        checkVictory(game->getGameState());

        if (game->getGameState() == GAME_STATE_RUN)
        {
            AIWork(AIlevel);
        }
    }
}

void CChildView::OnFirsthand()
{
    gameMode = GAME_MODE::PLAYER_FIRST;
    updateInfoStatic();
    if (game->getGameState() == GAME_STATE_RUN && game->getStepsCount() > 0 && game->getLastStep().black)
    {
        AIWork(HelpLevel);
    }

}

void CChildView::OnUpdateFirsthand(CCmdUI *pCmdUI)
{
    if (gameMode == GAME_MODE::PLAYER_FIRST)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}

void CChildView::OnSecondhand()
{
    gameMode = GAME_MODE::AI_FIRST;
    updateInfoStatic();
    if (game->getGameState() == GAME_STATE_RUN && (!game->getLastStep().black || game->getStepsCount() == 0))
    {
        AIWork(HelpLevel);
    }
}

void CChildView::OnUpdateSecondhand(CCmdUI *pCmdUI)
{
    if (gameMode == GAME_MODE::AI_FIRST)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}

void CChildView::OnPlayertoplayer()
{
    gameMode = GAME_MODE::NO_AI;
    updateInfoStatic();
}


void CChildView::OnUpdatePlayertoplayer(CCmdUI *pCmdUI)
{
    if (gameMode == GAME_MODE::NO_AI)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}

void CChildView::OnSave()
{
    if (!waitAI)
    {
        CString		PathName;
        CString		path_and_fileName;

        UpdateData(TRUE);

        PathName = _T("ChessBoard");

        CString szFilter = _T("ChessBoard Files(*.cshx)|*.cshx||");

        CFileDialog	fdlg(FALSE, _T("cshx"), PathName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter);

        if (IDOK != fdlg.DoModal()) return;
        path_and_fileName = fdlg.GetPathName();   //path_and_fileName即为文件保存路径

        CFile oFile(path_and_fileName, CFile::
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
        for (UINT i = 0; i < game->getStepsCount(); ++i)
        {
            oar << (byte)game->getStep(i).step << (byte)game->getStep(i).getRow() << (byte)game->getStep(i).getCol() << (bool)game->getStep(i).black;
        }
        oar.Close();
        oFile.Close();

        UpdateData(FALSE);
    }
}

void CChildView::OnLoad()
{
    if (!waitAI)
    {
        // Create dialog to open multiple files.
        CRect rcBroard(0 + BLANK, 0 + BLANK, BROARD_X + BLANK, BROARD_Y + BLANK);
        CString filePath;
        CString szFilter = _T("ChessBoard Files(*.cshx)|*.cshx||");
        UpdateData(TRUE);
        CFileDialog  fdlg(TRUE, _T("cshx"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter);
        if (IDOK != fdlg.DoModal()) return;
        filePath = fdlg.GetPathName();   // filePath即为所打开的文件的路径  
        UpdateData(FALSE);

        /*CFile oFile(path, CFile::modeRead);*/
        CFile oFile;
        CFileException fileException;
        if (!oFile.Open(filePath, CFile::modeRead, &fileException))
        {
            return;
        }

        CArchive oar(&oFile, CArchive::load);

        //读入版本号
        CString version;
        oar >> version;
        //初始化棋盘
        game->initGame();
        //读入stepList
        byte step, uRow, uCol; bool black;
        while (!oar.IsBufferEmpty())
        {
            oar >> step >> uRow >> uCol >> black;
            game->doNextStep(uRow, uCol);
            if (checkVictory(game->getGameState()))
            {
                break;
            }
        }

        if (game->getStepsCount() == 0)
        {
            gameMode = GAME_MODE::PLAYER_FIRST;
        }
        else
        {
            gameMode = game->getLastStep().getColor() == PIECE_BLACK ? GAME_MODE::AI_FIRST : GAME_MODE::PLAYER_FIRST;
        }

        oar.Close();
        oFile.Close();


        updateInfoStatic();
        Invalidate();
    }
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

void CChildView::OnHelpPrimary()
{
    HelpLevel = 1;
}


void CChildView::OnHelpSecondry()
{
    HelpLevel = 2;
}


void CChildView::OnHelpAdvanced()
{
    HelpLevel = 3;
}


void CChildView::OnUpdateHelpPrimary(CCmdUI *pCmdUI)
{
    if (HelpLevel == 1)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}


void CChildView::OnUpdateHelpSecondry(CCmdUI *pCmdUI)
{
    if (HelpLevel == 2)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}


void CChildView::OnUpdateHelpAdvanced(CCmdUI *pCmdUI)
{
    if (HelpLevel == 3)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}


void CChildView::OnAIPrimary()
{
    AIlevel = 1;
    ban = false;
    updateInfoStatic();
}

void CChildView::OnUpdateAIPrimary(CCmdUI *pCmdUI)
{
    if (AIlevel == 1)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}

void CChildView::OnAISecondry()
{
    AIlevel = 2;
    ban = true;
    updateInfoStatic();
}

void CChildView::OnUpdateAISecondry(CCmdUI *pCmdUI)
{
    if (AIlevel == 2)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
    /*pCmdUI->Enable(false);*/
}

void CChildView::OnAIAdvanced()
{
    AIlevel = 3;
    ban = true;
    updateInfoStatic();
}

void CChildView::OnUpdateAIAdvanced(CCmdUI *pCmdUI)
{
    if (AIlevel == 3)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}

void CChildView::OnAIMaster()
{
    AIlevel = 4;
    ban = true;
    updateInfoStatic();
}


void CChildView::OnUpdateAIMaster(CCmdUI *pCmdUI)
{
    if (AIlevel == 4)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}

void CChildView::OnDebug()
{
    /*Invalidate();*/
    //string result = ChessBoard::searchTrieTree->testSearch();

    debugStatic.SetWindowTextW(CString(game->debug(2).c_str()));

    /*MessageBox(debug, _T("调试信息"), MB_OK);*/
}

void CChildView::OnSettings()
{
    DlgSettings dlg;
    dlg.uStep = caculateSteps;
    dlg.algType = 1;
    if (dlg.DoModal() == IDOK)
    {
        caculateSteps = dlg.uStep;
        //TrieTreeNode::algType = dlg.algType;
        updateInfoStatic();
    }
}


void CChildView::OnMultithread()
{
    multithread = !multithread;
    updateInfoStatic();
}

void CChildView::OnUpdateMultithread(CCmdUI *pCmdUI)
{
    if (multithread)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}


void CChildView::OnBan()
{
    ban = !ban;
    updateInfoStatic();
}


void CChildView::OnUpdateBan(CCmdUI *pCmdUI)
{
    if (ban)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}


void CChildView::OnShowStep()
{
    showStep = !showStep;
    Invalidate(FALSE);
}


void CChildView::OnUpdateShowStep(CCmdUI *pCmdUI)
{
    if (showStep)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}


BOOL CChildView::OnEraseBkgnd(CDC* pDC)
{
    //屏蔽背景刷新
    return TRUE;
}
