
// ChildView.cpp : CChildView 类的实现
//

#include "stdafx.h"
#include "GoFive.h"
#include "ChildView.h"
#include "DlgSettings.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CChildView

static CWinThread* AIWorkThread;

struct AIWorkInfo
{
    Game *game;
};

static AIWorkInfo *AIworkinfo;

static UINT AIWorkThreadFunc(LPVOID lpParam);

static int threadFlag = 0;

CChildView::CChildView()
{
    currentPos.enable = false;
    oldPos.enable = false;
    game = new Game();
    debugCount = 0;
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
END_MESSAGE_MAP()



// CChildView 消息处理程序

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
                DrawChess(&dcMemory, game->getStepList());
                DrawProgress(&dcMemory);
                // 将内存设备的内容拷贝到实际屏幕显示的设备
                dc.BitBlt(m_rcClient.left, m_rcClient.top, m_rcClient.right, m_rcClient.bottom, &dcMemory, 0, 0, SRCCOPY);
                bitmap.DeleteObject();
            }
        }
    }
}

void CChildView::DrawProgress(CDC * pDC)
{

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

void CChildView::DrawChess(CDC* pDC, const std::vector<STEP> &stepList)
{
    BITMAP bm;
    CDC ImageDC;
    CBitmap ForeBMP;
    CBitmap *pOldImageBMP;
    STEP p;
    for (UINT i = 0; i < stepList.size(); ++i)
    {
        CString str;
        str.Format(TEXT("%d"), i + 1);
        p = stepList[i];
        ImageDC.CreateCompatibleDC(pDC);
        ForeBMP.LoadBitmap(p.isBlack ? IDB_CHESS_BLACK : IDB_CHESS_WHITE);
        ForeBMP.GetBitmap(&bm);
        pOldImageBMP = ImageDC.SelectObject(&ForeBMP);
        TransparentBlt(pDC->GetSafeHdc(), 2 + BLANK + p.uCol * 35, 4 + BLANK + p.uRow * 35, 36, 36,
            ImageDC.GetSafeHdc(), 0, 0, bm.bmWidth, bm.bmHeight, p.isBlack ? RGB(255, 255, 255) : RGB(50, 100, 100));
        if (game->isShowStep())
        {
            pDC->SetBkMode(TRANSPARENT);
            pDC->SetTextColor(p.isBlack ? RGB(255, 255, 255) : RGB(0, 0, 0));
            pDC->TextOut(14 + BLANK + p.uCol * 35, 14 + BLANK + p.uRow * 35, str);
        }
        ImageDC.SelectObject(pOldImageBMP);
        ForeBMP.DeleteObject();
        ImageDC.DeleteDC();
    }
    //画焦点
    if (!stepList.empty())
    {
        p = stepList.back();//获取当前棋子
        ImageDC.CreateCompatibleDC(pDC);
        ForeBMP.LoadBitmap(p.isBlack ? IDB_CHESS_BLACK_FOCUS : IDB_CHESS_WHITE_FOCUS);
        ForeBMP.GetBitmap(&bm);
        pOldImageBMP = ImageDC.SelectObject(&ForeBMP);
        TransparentBlt(pDC->GetSafeHdc(), 2 + BLANK + p.uCol * 35, 4 + BLANK + p.uRow * 35, 36, 36,
            ImageDC.GetSafeHdc(), 0, 0, bm.bmWidth, bm.bmHeight, p.isBlack ? RGB(255, 255, 255) : RGB(50, 100, 100));
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

void CChildView::checkVictory(int state)
{
    if (state == GAME_STATE_BLACKWIN)
        MessageBox(_T("黑棋五连胜利！"), _T(""), MB_OK);
    else if (state == GAME_STATE_WHITEWIN)
        MessageBox(_T("白棋五连胜利！"), _T(""), MB_OK);
    else if (state == GAME_STATE_DRAW)
        MessageBox(_T("和局！"), _T(""), MB_OK);
    else if (state == GAME_STATE_BLACKBAN)
        MessageBox(_T("黑棋禁手，白棋胜！"), _T(""), MB_OK);
}

void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
    CClientDC dc(this);
    CRect rcBroard(BLANK, BLANK, BROARD_X + BLANK, BROARD_Y + BLANK);
    if (game->getGameState() == GAME_STATE_RUN) {
        int col = (point.x - 2 - BLANK) / 35;
        int row = (point.y - 4 - BLANK) / 35;
        if (col < 15 && row < 15 && point.x >= 2 + BLANK&&point.y >= 4 + BLANK) {
            if (game->getPiece(row, col).getState() == 0) {
                currentPos = { row, col, true };
                SetClassLong(this->GetSafeHwnd(),
                    GCL_HCURSOR,
                    (LONG)LoadCursor(NULL, IDC_HAND));
            }
            else if (game->getPiece(row, col).getState() != 0) {
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

void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
    CClientDC dc(this);
    CRect rcBroard(0 + BLANK, 0 + BLANK, BROARD_X + BLANK, BROARD_Y + BLANK);
    if (rcBroard.PtInRect(point) && game->getGameState() == GAME_STATE_RUN)
    {
        int col = (point.x - 2 - BLANK) / 35;
        int row = (point.y - 4 - BLANK) / 35;
        if (game->getPiece(row, col).getState() == 0 && row < 15 && col < 15 && point.x >= 2 + BLANK&&point.y >= 4 + BLANK)
        {
            //棋子操作
            int side = game->getPlayerSide();
            char str[40] = { '\0' };
            game->playerWork(row, col);
            game->getChessMode(str, row, col, side);
            debugStatic.SetWindowTextW(CString(str));
            currentPos.enable = false;
            oldPos = currentPos;
            SetClassLong(this->GetSafeHwnd(), GCL_HCURSOR, (LONG)LoadCursor(NULL, IDC_NO));
            InvalidateRect(rcBroard, false);
            checkVictory(game->getGameState());
            AIWork();
        }
    }

    CWnd::OnLButtonDown(nFlags, point);
}

void CChildView::AIWork()
{
    if (game->getGameState() == GAME_STATE_RUN)
    {
        if (!game->isPlayerToPlayer())
        {
            AIworkinfo = new AIWorkInfo;
            AIworkinfo->game = game;
            game->setGameState(GAME_STATE_WAIT);
            startProgress();
            AIWorkThread = AfxBeginThread(AIWorkThreadFunc, AIworkinfo);
            /*	game->AIWork();
            InvalidateRect(rcBroard, FALSE);
            checkVictory(game->getGameState());*/
        }

    }
}

UINT AIWorkThreadFunc(LPVOID lpParam)
{
    srand(unsigned int(time(0)));
    AIWorkInfo* pInfo = (AIWorkInfo*)lpParam;
    Game *game = pInfo->game;
    game->AIWork(false);
    delete pInfo;
    return 0;
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

    infoStatic.Create(_T(""), WS_CHILD | SS_CENTER | WS_VISIBLE, CRect(BLANK, 0, (BROARD_X + BLANK), BLANK), this);

    debugStatic.Create(_T("debug"), WS_CHILD | SS_CENTER | WS_VISIBLE, CRect(BROARD_X + BLANK*2, BLANK, (BROARD_X + BLANK * 2)+100, BLANK+100), this);

    font.CreatePointFont(110 * DEFAULT_DPI / dpiX, _T("微软雅黑"), NULL);

    myProgressStatic.SetFont(&font);
    infoStatic.SetFont(&font);
    debugStatic.SetFont(&font);
    updateInfoStatic();
}

void CChildView::updateInfoStatic()
{
    CString info;
    if (!game->isPlayerToPlayer())
    {
        info.AppendFormat(_T("玩家：%s    禁手：%s    AI等级："), game->getPlayerSide() == 1 ? _T("先手") : _T("后手"),
            game->isBan() ? _T("有") : _T("无"));
        switch (game->getAIlevel())
        {
        case 1:
            info += "低级";
            break;
        case 2:
            info += "中级";
            break;
        case 3:
            info += "高级";
            break;
        default:
            info += "未知";
            break;
        }
    }
    else
    {
        info.Append(_T("人人对战"));
    }

    infoStatic.SetWindowTextW(info);
}

void CChildView::startProgress()
{
    myProgress.ShowWindow(SW_SHOWNA);
    myProgressStatic.ShowWindow(SW_SHOWNA);
    myProgress.SetPos(0);
    SetTimer(1, 200, NULL);
}

void CChildView::endProgress()
{
    KillTimer(1);
    myProgress.ShowWindow(SW_HIDE);
    myProgressStatic.ShowWindow(SW_HIDE);
}

void CChildView::OnTimer(UINT_PTR nIDEvent)
{
    if (1 == nIDEvent)
    {
        if (game->getGameState() == GAME_STATE_WAIT)
            myProgress.StepIt();
        else
        {
            endProgress();
            InvalidateRect(CRect(0 + BLANK, 0 + BLANK, BROARD_X + BLANK, BROARD_Y + BLANK), FALSE);
            checkVictory(game->getGameState());
        }

    }
    CWnd::OnTimer(nIDEvent);
}

void CChildView::OnStepback()
{
    if (game->getGameState() != GAME_STATE_WAIT)
    {
        game->stepBack();
        Invalidate(FALSE);
    }
}

void CChildView::OnStart()
{
    if (game->getGameState() != GAME_STATE_WAIT)
    {
        game->init();
        currentPos.enable = false;
        oldPos.enable = false;
        Invalidate(FALSE);
    }
}

void CChildView::OnAIhelp()
{
    if (game->getGameState() == GAME_STATE_RUN)
    {
        game->AIWork(true);
        Invalidate(FALSE);
        checkVictory(game->getGameState());
        if (game->getGameState() == GAME_STATE_RUN)
        {
            AIWork();
        }
    }
}

void CChildView::OnFirsthand()
{
    if (game->getGameState() != GAME_STATE_WAIT)
    {
        game->setPlayerToPlayer(false);
        game->changeSide(1);
        updateInfoStatic();
        Invalidate(FALSE);
    }
}

void CChildView::OnUpdateFirsthand(CCmdUI *pCmdUI)
{
    if (game->getPlayerSide() == 1 && !game->isPlayerToPlayer())
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}

void CChildView::OnSecondhand()
{
    if (game->getGameState() != GAME_STATE_WAIT)
    {
        game->setPlayerToPlayer(false);
        game->changeSide(-1);
        updateInfoStatic();
        Invalidate(FALSE);
    }
}

void CChildView::OnUpdateSecondhand(CCmdUI *pCmdUI)
{
    if (game->getPlayerSide() == -1 && !game->isPlayerToPlayer())
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}

void CChildView::OnPlayertoplayer()
{
    if (game->getGameState() != GAME_STATE_WAIT)
    {
        game->setPlayerToPlayer(true);
        updateInfoStatic();
    }
}


void CChildView::OnUpdatePlayertoplayer(CCmdUI *pCmdUI)
{
    if (game->isPlayerToPlayer())
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}

void CChildView::OnSave()
{
    if (game->getGameState() != GAME_STATE_WAIT)
    {
        CString		PathName;
        CString		path_and_fileName;

        UpdateData(TRUE);

        PathName = _T("ChessBoard");

        CString szFilter = _T("ChessBoard Files(*.cshx)|*.cshx||");

        CFileDialog	fdlg(FALSE, _T("cshx"), PathName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter);

        if (IDOK != fdlg.DoModal()) return;
        path_and_fileName = fdlg.GetPathName();   //path_and_fileName即为文件保存路径
        game->saveBoard(path_and_fileName);
        UpdateData(FALSE);
    }
}

void CChildView::OnLoad()
{
    if (game->getGameState() != GAME_STATE_WAIT)
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
        game->loadBoard(filePath);
        checkVictory(game->getGameState());
        updateInfoStatic();
        Invalidate();
    }
}

void CChildView::OnHelpPrimary()
{
    if (game->getGameState() != GAME_STATE_WAIT)
    {
        game->setHelpLevel(1);
    }
}


void CChildView::OnHelpSecondry()
{
    if (game->getGameState() != GAME_STATE_WAIT)
    {
        game->setHelpLevel(2);
    }
}


void CChildView::OnHelpAdvanced()
{
    if (game->getGameState() != GAME_STATE_WAIT)
    {
        game->setHelpLevel(3);
    }
}


void CChildView::OnUpdateHelpPrimary(CCmdUI *pCmdUI)
{
    if (game->getHelpLevel() == 1)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}


void CChildView::OnUpdateHelpSecondry(CCmdUI *pCmdUI)
{
    if (game->getHelpLevel() == 2)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}


void CChildView::OnUpdateHelpAdvanced(CCmdUI *pCmdUI)
{
    if (game->getHelpLevel() == 3)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}


void CChildView::OnAIPrimary()
{
    game->setAIlevel(1);
    updateInfoStatic();
}

void CChildView::OnUpdateAIPrimary(CCmdUI *pCmdUI)
{
    if (game->getAIlevel() == 1)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}

void CChildView::OnAISecondry()
{
    game->setAIlevel(2);
    updateInfoStatic();
}

void CChildView::OnUpdateAISecondry(CCmdUI *pCmdUI)
{
    if (game->getAIlevel() == 2)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
    /*pCmdUI->Enable(false);*/
}

void CChildView::OnRButtonDown(UINT nFlags, CPoint point)
{
    OnStepback();
    CWnd::OnRButtonDown(nFlags, point);
}

void CChildView::OnAIAdvanced()
{
    game->setAIlevel(3);
    updateInfoStatic();
}

void CChildView::OnUpdateAIAdvanced(CCmdUI *pCmdUI)
{
    if (game->getAIlevel() == 3)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}

void CChildView::OnDebug()
{
    CString debug = game->debug(1);
    /*Invalidate();*/


    /*MessageBox(debug, _T("调试信息"), MB_OK);*/
}

void CChildView::OnSettings()
{
    if (game->getGameState() != GAME_STATE_WAIT)
    {
        DlgSettings dlg;
        dlg.uStep = game->getCaculateStep();
        if (dlg.DoModal() == IDOK)
        {
            game->setCaculateStep(dlg.uStep);
            updateInfoStatic();
        }
    }
}


void CChildView::OnMultithread()
{
    game->setMultithread(!game->isMultithread());
    updateInfoStatic();
}

void CChildView::OnUpdateMultithread(CCmdUI *pCmdUI)
{
    if (game->isMultithread())
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}


void CChildView::OnBan()
{
    game->setBan(!game->isBan());
    updateInfoStatic();
}


void CChildView::OnUpdateBan(CCmdUI *pCmdUI)
{
    if (game->isBan())
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}


void CChildView::OnShowStep()
{
    game->setShowStep(!game->isShowStep());
    Invalidate(FALSE);
}


void CChildView::OnUpdateShowStep(CCmdUI *pCmdUI)
{
    if (game->isShowStep())
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}
