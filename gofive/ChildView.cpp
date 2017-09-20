#include "stdafx.h"
#include "GoFive.h"
#include "ChildView.h"
#include "DlgSettings.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CChildView

static CWinThread* AIWorkThread;

static CChildView* viewhandle = NULL;

CChildView::CChildView() : showStep(false), waitAI(false), onAIHelp(false)
{
    currentPos.enable = false;
    oldPos.enable = false;
    gameMode = GAME_MODE::PLAYER_FIRST;
    viewhandle = this;
    settings.msgfunc = msgCallBack;
    settings.ban = true;
    settings.enableAtack = true;
    settings.maxSearchDepth = 12;
    settings.maxStepTimeMs = 30000;
    settings.restMatchTimeMs = UINT32_MAX;
    settings.maxMemoryBytes = 350000000;
    helpEngine = AIGAMETREE;
    helpLevel = AILEVEL_INTERMEDIATE;

    AIEngine = AIGOSEARCH;
    settings.defaultGoSearch(AILEVEL_UNLIMITED);
    settings.enableDebug = true;

    game = new Game();
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
    if (!game->initAIHelper(thread_num))
    {
        MessageBox(_T("��ʼ���ֵ���ʧ�ܣ�"), _T("error"), MB_OK);
    }
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
    ON_COMMAND(ID_AI_GOSEARCH, &CChildView::OnAIGosearch)
    ON_UPDATE_COMMAND_UI(ID_AI_GOSEARCH, &CChildView::OnUpdateAIGosearch)
    ON_COMMAND(ID_HELP_MASTER, &CChildView::OnHelpMaster)
    ON_UPDATE_COMMAND_UI(ID_HELP_MASTER, &CChildView::OnUpdateHelpMaster)
    ON_WM_CTLCOLOR()
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

    myProgressStatic.Create(_T("AI˼���У�"), WS_CHILD | SS_CENTER,
        CRect(BROARD_X / 2 - 150, BLANK + BROARD_Y, BROARD_X / 2 - 50, BLANK + BROARD_Y + 20), this);

    infoStatic.Create(_T(""), WS_CHILD | SS_CENTER | WS_VISIBLE, CRect(BLANK + 2, 2, (BROARD_X + BLANK - 2), BLANK - 2), this);
    debugRect.SetRect(BROARD_X + BLANK + 10, BLANK, (BROARD_X + BLANK * 2) + 370, BLANK + 520);
    debugStatic.Create(WS_CHILD | WS_VISIBLE | ES_AUTOVSCROLL | ES_MULTILINE | ES_WANTRETURN, debugRect, this, 66);
    debugStatic.ShowScrollBar(SB_VERT, TRUE);
    font.CreatePointFont(110 * DEFAULT_DPI / dpiX, _T("΢���ź�"), NULL);

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
        info.Append(_T("���˶�ս"));
    }
    else if (gameMode == GAME_MODE::NO_PLAYER)
    {

    }
    else
    {
        info.AppendFormat(_T("��ң�%s    ���֣�%s    AI�ȼ���"), gameMode == GAME_MODE::PLAYER_FIRST ? _T("����") : _T("����"), settings.ban ? _T("��") : _T("��"));
        switch (AIEngine)
        {
        case AISIMPLE:
            info.AppendFormat(_T("�ͼ�"));
            break;
        case AIGAMETREE:
            if (!settings.extraSearch)
            {
                info.AppendFormat(_T("�м�"));
            }
            else
            {
                info.AppendFormat(_T("�߼�"));
            }
            break;
        case AIGOSEARCH:
            info.AppendFormat(_T("��ʦ"));
            break;
        default:
            info += "δ֪";
            break;
        }
    }

    infoStatic.SetWindowTextW(info);
    InvalidateRect(FALSE);
}

void CChildView::OnPaint()
{
    CPaintDC dc(this);    // ������Ļ��ʾ���豸
    CDC dcMemory;  // �ڴ��豸
    CBitmap bitmap;
    CRect m_rcClient;
    GetClientRect(&m_rcClient);
    if (!dc.IsPrinting())
    {
        // ��dc�豸����
        if (dcMemory.CreateCompatibleDC(&dc))
        {
            // ʹ��bitmap��ʵ����ʾ���豸����
            if (bitmap.CreateCompatibleBitmap(&dc, m_rcClient.right, m_rcClient.bottom))
            {
                // �ڴ��豸ѡ�������λͼ
                //���Ʊ�����
                dcMemory.SelectObject(&bitmap);
                DrawBack(&dcMemory);
                DrawChessBoard(&dcMemory);
                DrawMouseFocus(&dcMemory);
                DrawChess(&dcMemory);
                // ���ڴ��豸�����ݿ�����ʵ����Ļ��ʾ���豸
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

    for (size_t i = 0; i < game->getStepsCount(); ++i)
    {
        CString str;
        str.Format(TEXT("%d"), i + 1);
        p = game->getStep(i);
        ImageDC.CreateCompatibleDC(pDC);
        ForeBMP.LoadBitmap(p.getState() == PIECE_BLACK ? IDB_CHESS_BLACK : IDB_CHESS_WHITE);
        ForeBMP.GetBitmap(&bm);
        pOldImageBMP = ImageDC.SelectObject(&ForeBMP);
        TransparentBlt(pDC->GetSafeHdc(), 2 + BLANK + p.getCol() * 35, 4 + BLANK + p.getRow() * 35, 36, 36,
            ImageDC.GetSafeHdc(), 0, 0, bm.bmWidth, bm.bmHeight, p.getState() == PIECE_BLACK ? RGB(255, 255, 255) : RGB(50, 100, 100));
        if (showStep)
        {
            pDC->SetBkMode(TRANSPARENT);
            pDC->SetTextColor(p.getState() == PIECE_BLACK ? RGB(255, 255, 255) : RGB(0, 0, 0));
            pDC->DrawTextW(str, &CRect(8 + BLANK + p.getCol() * 35, 16 + BLANK + p.getRow() * 35, 32 + BLANK + p.getCol() * 35, 28 + BLANK + p.getRow() * 35), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        ImageDC.SelectObject(pOldImageBMP);
        ForeBMP.DeleteObject();
        ImageDC.DeleteDC();
    }
    //������
    if (game->getStepsCount() != 0)
    {
        p = game->getLastStep();//��ȡ��ǰ����
        ImageDC.CreateCompatibleDC(pDC);
        ForeBMP.LoadBitmap(p.getState() == PIECE_BLACK ? IDB_CHESS_BLACK_FOCUS : IDB_CHESS_WHITE_FOCUS);
        ForeBMP.GetBitmap(&bm);
        pOldImageBMP = ImageDC.SelectObject(&ForeBMP);
        TransparentBlt(pDC->GetSafeHdc(), 2 + BLANK + p.getCol() * 35, 4 + BLANK + p.getRow() * 35, 36, 36,
            ImageDC.GetSafeHdc(), 0, 0, bm.bmWidth, bm.bmHeight, p.getState() == PIECE_BLACK ? RGB(255, 255, 255) : RGB(50, 100, 100));
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
        MessageBox(_T("��������ʤ����"), _T(""), MB_OK);
    else if (state == GAME_STATE_WHITEWIN)
        MessageBox(_T("��������ʤ����"), _T(""), MB_OK);
    else if (state == GAME_STATE_DRAW)
        MessageBox(_T("�;֣�"), _T(""), MB_OK);
    else if (state == GAME_STATE_BLACKBAN)
        MessageBox(_T("������֣�����ʤ��"), _T(""), MB_OK);
    else
        return false;

    return true;
}

void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
    CClientDC dc(this);
    CRect rcBroard(BLANK, BLANK, BROARD_X + BLANK, BROARD_Y + BLANK);
    if (game->getGameState() == GAME_STATE_RUN)
    {
        int col = (point.x - 2 - BLANK) / 35;
        int row = (point.y - 4 - BLANK) / 35;
        if (col < 15 && row < 15 && point.x >= 2 + BLANK&&point.y >= 4 + BLANK)
        {
            if (game->getPieceState(row, col) == PIECE_BLANK && !waitAI)
            {
                currentPos = { row, col, true };
                SetClassLongPtr(this->GetSafeHwnd(),
                    GCLP_HCURSOR,
                    (LONG_PTR)LoadCursor(NULL, IDC_HAND));
            }
            else
            {
                currentPos.enable = false;

                SetClassLongPtr(this->GetSafeHwnd(),
                    GCLP_HCURSOR,
                    (LONG_PTR)LoadCursor(NULL, IDC_NO));
            }
        }
        else
        {
            currentPos.enable = false;
            SetClassLongPtr(this->GetSafeHwnd(),
                GCLP_HCURSOR,
                (LONG_PTR)LoadCursor(NULL, IDC_ARROW));
        }
    }
    else
    {
        currentPos.enable = false;
        SetClassLongPtr(this->GetSafeHwnd(),
            GCLP_HCURSOR,
            (LONG_PTR)LoadCursor(NULL, IDC_ARROW));
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
            //���Ӳ���
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
                side = game->getStepsCount() == 0 ? PIECE_BLACK : Util::otherside(game->getLastStep().getState());
            }

            game->doNextStep(row, col, settings.ban);

            currentPos.enable = false;
            oldPos = currentPos;
            SetClassLongPtr(this->GetSafeHwnd(), GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, IDC_NO));
            InvalidateRect(rcBroard, false);
            checkVictory(game->getGameState());
            AIWork(false);
        }
    }

    CWnd::OnLButtonDown(nFlags, point);
}

void CChildView::AIWork(bool ishelp)
{
    if (game->getGameState() == GAME_STATE_RUN)
    {
        if (gameMode != GAME_MODE::NO_AI)
        {
            waitAI = true;
            startProgress();
            AIWorkThreadData* data = new AIWorkThreadData;
            data->view = this;
            if (ishelp)
            {
                data->setting = settings;
                data->engine = helpEngine;
                if (helpEngine == AIGAMETREE)
                {
                    if (helpLevel == AILEVEL_PRIMARY)
                    {
                        data->setting.extraSearch = false;
                    }
                    else
                    {
                        data->setting.extraSearch = true;
                    }
                }
            }
            else
            {
                data->setting = settings;
                data->engine = AIEngine;
            }

            AIWorkThread = AfxBeginThread(AIWorkThreadFunc, (void*)data);
        }

    }
}

UINT CChildView::AIWorkThreadFunc(LPVOID lpParam)
{
    srand(unsigned int(time(0)));
    AIWorkThreadData* data = (AIWorkThreadData*)lpParam;
    data->setting.startTimeMs = system_clock::to_time_t(system_clock::now());
    data->view->game->doNextStepByAI(data->engine, data->setting);
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


void CChildView::appendDebugEdit(CString &str)
{
    if (str.IsEmpty())
    {
        return;
    }
    int pos = debugStatic.GetScrollPos(SB_VERT);
    CString s;
    debugStatic.GetWindowTextW(s);
    if (s.GetLength() > 1024000)
    {
        s = s.Right(512000);
        debugStatic.SetWindowTextW(s);
    }


    int nLength = debugStatic.GetWindowTextLength();
    debugStatic.SetSel(nLength, nLength);
    debugStatic.ReplaceSel(str);
    debugStatic.SetSel(debugStatic.GetWindowTextLength(), debugStatic.GetWindowTextLength());
    


    debugStatic.LineScroll(debugStatic.GetLineCount());

    //debugStatic.LineScroll(pos);

    //debugStatic.SetWindowTextW(str);
    /*str.Append(_T("\r\n"));
    int nLength = debugStatic.SendMessage(WM_GETTEXTLENGTH);
    debugStatic.SetSel(nLength, nLength);
    debugStatic.ReplaceSel(str);*/
}


void CChildView::msgCallBack(string &msg)
{
    CString s;
    s.AppendFormat(_T("%s \r\n"), CString(msg.c_str()));
    viewhandle->appendDebugEdit(s);
}

void CChildView::OnTimer(UINT_PTR nIDEvent)
{
    if (1 == nIDEvent)
    {
        if (waitAI)
        {
            myProgress.StepIt();
            //string msg;
            //CString s;
            //while (game->getAITextOut(msg))
            //{
            //    s.AppendFormat(_T("%s \r\n"), CString(msg.c_str()));
            //}
            //appendDebugEdit(s);
        }
        else//����
        {
            endProgress();
            InvalidateRect(CRect(0 + BLANK, 0 + BLANK, BROARD_X + BLANK, BROARD_Y + BLANK), FALSE);
            checkVictory(game->getGameState());

            //string msg;
            //CString s;
            //while (game->getAITextOut(msg))
            //{
            //    s.AppendFormat(_T("%s \r\n"), CString(msg.c_str()));
            //}
            //appendDebugEdit(s);

            if (onAIHelp)
            {
                onAIHelp = false;
                if (game->getGameState() == GAME_STATE_RUN)
                {
                    AIWork(false);
                }
            }
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
                if (game->getLastStep().getState() == PIECE_BLACK)
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
                if (game->getLastStep().getState() != PIECE_BLACK)
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
        if (gameMode == AI_FIRST)
        {
            AIWork(true);
        }
        Invalidate(FALSE);
    }
}

void CChildView::OnAIhelp()
{
    if (game->getGameState() == GAME_STATE_RUN && !waitAI)
    {

        onAIHelp = true;
        AIWork(true);

    }
}

void CChildView::OnFirsthand()
{
    gameMode = GAME_MODE::PLAYER_FIRST;
    updateInfoStatic();
    if (game->getGameState() == GAME_STATE_RUN && game->getStepsCount() > 0 && game->getLastStep().getState() == PIECE_BLACK)
    {
        AIWork(true);
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
    if (game->getGameState() == GAME_STATE_RUN && (game->getLastStep().getState() != PIECE_BLACK || game->getStepsCount() == 0))
    {
        AIWork(true);
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
        path_and_fileName = fdlg.GetPathName();   //path_and_fileName��Ϊ�ļ�����·��

        CFile oFile(path_and_fileName, CFile::
            modeCreate | CFile::modeWrite);
        CArchive oar(&oFile, CArchive::store);
        //д��汾��
        CString version;
        if (!GetMyProcessVer(version))
        {
            version = _T("0.0.0.0");
        }
        oar << version;
        //д��stepList
        for (UINT i = 0; i < game->getStepsCount(); ++i)
        {
            oar << (byte)game->getStep(i).step << (byte)game->getStep(i).getRow() << (byte)game->getStep(i).getCol() << (bool)(game->getStep(i).getState() == PIECE_BLACK);
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
        CString szFilter = _T("ALL Files(*.*)|*.*|ChessBoard Files(*.cshx)|*.cshx|Piskvorky Files(*.psq)|*.psq||");
        UpdateData(TRUE);
        CFileDialog  fdlg(TRUE, _T("cshx"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter);
        if (IDOK != fdlg.DoModal()) return;
        filePath = fdlg.GetPathName();   // filePath��Ϊ���򿪵��ļ���·��  
        UpdateData(FALSE);

        CStdioFile oFile;
        CFileException fileException;
        if (!oFile.Open(filePath, CStdioFile::modeRead, &fileException))
        {
            return;
        }

        CString str;
        if (oFile.ReadString(str) && str.Find(_T("Piskvorky")) >= 0)
        {
            game->initGame();
            while (oFile.ReadString(str))
            {
                int index = str.Find(',');
                if (index < 0)
                {
                    break;
                }
                stringstream ss;
                ss << (CStringA)str;
                int row,col;
                char temp;
                ss >> row >> temp >> col;
                game->doNextStep(row, col, settings.ban);
                if (checkVictory(game->getGameState()))
                {
                    break;
                }
            }  
            oFile.Close();
        }
        else
        {
            oFile.Close();
            CStdioFile oFile2;
            CFileException fileException2;
            if (!oFile2.Open(filePath, CStdioFile::modeRead, &fileException2))
            {
                return;
            }
            CArchive oar(&oFile2, CArchive::load);
            //����汾��
            CString version;
            oar >> version;
            //��ʼ������
            game->initGame();
            //����stepList
            byte step, row, col;
            bool black;
            while (!oar.IsBufferEmpty())
            {
                oar >> step >> row >> col >> black;
                game->doNextStep(row, col, settings.ban);
                if (checkVictory(game->getGameState()))
                {
                    break;
                }
            }
            oar.Close();
            oFile2.Close();
        }

       

        if (game->getStepsCount() == 0)
        {
            gameMode = GAME_MODE::PLAYER_FIRST;
        }
        else
        {
            gameMode = game->getLastStep().getState() == PIECE_BLACK ? GAME_MODE::AI_FIRST : GAME_MODE::PLAYER_FIRST;
        }

        updateInfoStatic();
        Invalidate();
    }
}

BOOL GetMyProcessVer(CString& strver)   //����ȡ���Լ��İ汾��   
{
    TCHAR strfile[MAX_PATH];
    GetModuleFileName(NULL, strfile, sizeof(strfile));  //����ȡ���Լ����ļ���   

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
    helpEngine = AISIMPLE;
}


void CChildView::OnHelpSecondry()
{
    helpEngine = AIGAMETREE;
    helpLevel = AILEVEL_PRIMARY;
}


void CChildView::OnHelpAdvanced()
{
    helpEngine = AIGAMETREE;
    helpLevel = AILEVEL_INTERMEDIATE;
}


void CChildView::OnUpdateHelpPrimary(CCmdUI *pCmdUI)
{
    if (helpEngine == AISIMPLE)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}


void CChildView::OnUpdateHelpSecondry(CCmdUI *pCmdUI)
{
    if (helpEngine == AIGAMETREE && helpLevel == AILEVEL_PRIMARY)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}


void CChildView::OnUpdateHelpAdvanced(CCmdUI *pCmdUI)
{
    if (helpEngine == AIGAMETREE && helpLevel == AILEVEL_INTERMEDIATE)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}



void CChildView::OnHelpMaster()
{
    helpEngine = AIGOSEARCH;
}


void CChildView::OnUpdateHelpMaster(CCmdUI *pCmdUI)
{
    if (helpEngine == AIGOSEARCH)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}


void CChildView::OnAIPrimary()
{
    AIEngine = AISIMPLE;
    settings.ban = false;
    updateInfoStatic();
}

void CChildView::OnUpdateAIPrimary(CCmdUI *pCmdUI)
{
    if (AIEngine == AISIMPLE)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}

void CChildView::OnAISecondry()
{
    AIEngine = AIGAMETREE;
    settings.extraSearch = false;
    settings.ban = true;
    updateInfoStatic();
}

void CChildView::OnUpdateAISecondry(CCmdUI *pCmdUI)
{
    if (AIEngine == AIGAMETREE && !settings.extraSearch)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
    /*pCmdUI->Enable(false);*/
}

void CChildView::OnAIAdvanced()
{
    AIEngine = AIGAMETREE;
    settings.extraSearch = true;
    settings.ban = true;
    updateInfoStatic();
}

void CChildView::OnUpdateAIAdvanced(CCmdUI *pCmdUI)
{
    if (AIEngine == AIGAMETREE && settings.extraSearch)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}

void CChildView::OnAIMaster()
{
    AIEngine = AIGOSEARCH;
    settings.ban = true;
    settings.defaultGoSearch(AILEVEL_UNLIMITED);
    updateInfoStatic();
}


void CChildView::OnUpdateAIMaster(CCmdUI *pCmdUI)
{
    if (AIEngine == AIGOSEARCH)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}



void CChildView::OnAIGosearch()
{
    return;
    updateInfoStatic();
}


void CChildView::OnUpdateAIGosearch(CCmdUI *pCmdUI)
{
    return;
    if (AIEngine == AIGOSEARCH)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}

void CChildView::OnDebug()
{
    /*Invalidate();*/
    //string result = ChessBoard::searchTrieTree->testSearch();
    CString s(game->debug(2).c_str());
    appendDebugEdit(s);
    /*MessageBox(debug, _T("������Ϣ"), MB_OK);*/
}

void CChildView::OnSettings()
{
    DlgSettings dlg;
    dlg.uStep = settings.maxSearchDepth;
    dlg.maxmemsize = settings.maxMemoryBytes;
    dlg.maxTime = settings.maxStepTimeMs / 1000;
    dlg.mindepth = settings.minAlphaBetaDepth;
    dlg.maxdepth = settings.maxAlphaBetaDepth;
    dlg.vcf_expend = settings.VCFExpandDepth;
    dlg.vct_expend = settings.VCTExpandDepth;
    dlg.useTransTable = settings.useTransTable ? TRUE : FALSE;
    dlg.fullSearch = settings.fullSearch ? TRUE : FALSE;
    if (dlg.DoModal() == IDOK)
    {
        settings.maxSearchDepth = dlg.uStep;
        settings.maxMemoryBytes = dlg.maxmemsize;
        settings.maxStepTimeMs = dlg.maxTime * 1000;
        settings.minAlphaBetaDepth = dlg.mindepth;
        settings.maxAlphaBetaDepth = dlg.maxdepth;
        settings.useTransTable = dlg.useTransTable == TRUE ? true : false;
        settings.VCFExpandDepth = dlg.vcf_expend;
        settings.VCTExpandDepth = dlg.vct_expend;
        settings.fullSearch = dlg.fullSearch == TRUE ? true : false;
        updateInfoStatic();
    }
}


void CChildView::OnMultithread()
{
    settings.multithread = !settings.multithread;
    //settings.enableAtack = !settings.enableAtack;
    updateInfoStatic();
}

void CChildView::OnUpdateMultithread(CCmdUI *pCmdUI)
{
    if (settings.multithread)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}


void CChildView::OnBan()
{
    settings.ban = !settings.ban;
    updateInfoStatic();
}


void CChildView::OnUpdateBan(CCmdUI *pCmdUI)
{
    if (settings.ban)
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
    //���α���ˢ��
    return TRUE;
}

HBRUSH CChildView::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CWnd::OnCtlColor(pDC, pWnd, nCtlColor);

    if (pWnd->GetDlgCtrlID() == 66)//debug��͸��
    {
        return hbr;
    }

    if (nCtlColor == CTLCOLOR_STATIC)
    {
        pDC->SetBkMode(TRANSPARENT);
        return (HBRUSH)::GetStockObject(NULL_BRUSH);
    }
    else if (nCtlColor == CTLCOLOR_EDIT)
    {

    }

    return hbr;
}
