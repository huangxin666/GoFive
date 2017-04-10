#include "stdafx.h"
#include "GoFive.h"
#include "ChildView.h"
#include "DlgSettings.h"
#include "GameTree.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CChildView

static CWinThread* AIWorkThread;

CChildView::CChildView()
{
    currentPos.enable = false;
    oldPos.enable = false;
    game = new Game();
    if (!game->initTrieTree())
    {
        MessageBox(_T("��ʼ���ֵ���ʧ�ܣ�"), _T("error"), MB_OK);
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

    myProgressStatic.Create(_T("AI˼���У�"), WS_CHILD | SS_CENTER,
        CRect(BROARD_X / 2 - 150, BLANK + BROARD_Y, BROARD_X / 2 - 50, BLANK + BROARD_Y + 20), this);

    infoStatic.Create(_T(""), WS_CHILD | SS_CENTER | WS_VISIBLE, CRect(BLANK, 0, (BROARD_X + BLANK), BLANK), this);

    debugStatic.Create(_T("debug"), WS_CHILD | SS_CENTER | WS_VISIBLE, CRect(BROARD_X + BLANK * 2, BLANK, (BROARD_X + BLANK * 2) + 150, BLANK + 100), this);

    font.CreatePointFont(110 * DEFAULT_DPI / dpiX, _T("΢���ź�"), NULL);

    myProgressStatic.SetFont(&font);
    infoStatic.SetFont(&font);
    debugStatic.SetFont(&font);
    updateInfoStatic();
}

void CChildView::updateInfoStatic()
{
    CString info;
    if (!game->playerToPlayer)
    {
        info.AppendFormat(_T("��ң�%s    ���֣�%s    AI�ȼ���"), game->playerSide == 1 ? _T("����") : _T("����"),
            game->isBan() ? _T("��") : _T("��"));
        switch (game->AIlevel)
        {
        case 1:
            info.AppendFormat(_T("�ͼ�    ������ȣ�2"));
            break;
        case 2:
            info.AppendFormat(_T("�м�    ������ȣ�4"));
            break;
        case 3:
            info.AppendFormat(_T("�߼�    ������ȣ�%d"), game->parameter.caculateSteps * 2);
            break;
        case 4:
            info.AppendFormat(_T("��ʦ    ������ȣ�%d"), game->parameter.caculateSteps * 2);
            break;
        default:
            info += "δ֪";
            break;
        }
    }
    else
    {
        info.Append(_T("���˶�ս"));
    }

    infoStatic.SetWindowTextW(info);
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
                DrawChess(&dcMemory, game->stepList);
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

void CChildView::DrawChess(CDC* pDC, const std::vector<ChessStep> &stepList)
{
    BITMAP bm;
    CDC ImageDC;
    CBitmap ForeBMP;
    CBitmap *pOldImageBMP;
    ChessStep p;
    for (UINT i = 0; i < stepList.size(); ++i)
    {
        CString str;
        str.Format(TEXT("%d"), i + 1);
        p = stepList[i];
        ImageDC.CreateCompatibleDC(pDC);
        ForeBMP.LoadBitmap(p.getColor() == 1 ? IDB_CHESS_BLACK : IDB_CHESS_WHITE);
        ForeBMP.GetBitmap(&bm);
        pOldImageBMP = ImageDC.SelectObject(&ForeBMP);
        TransparentBlt(pDC->GetSafeHdc(), 2 + BLANK + p.col * 35, 4 + BLANK + p.row * 35, 36, 36,
            ImageDC.GetSafeHdc(), 0, 0, bm.bmWidth, bm.bmHeight, p.getColor() == 1 ? RGB(255, 255, 255) : RGB(50, 100, 100));
        if (game->showStep)
        {
            pDC->SetBkMode(TRANSPARENT);
            pDC->SetTextColor(p.getColor() == 1 ? RGB(255, 255, 255) : RGB(0, 0, 0));
            pDC->TextOut(14 + BLANK + p.col * 35, 14 + BLANK + p.row * 35, str);
        }
        ImageDC.SelectObject(pOldImageBMP);
        ForeBMP.DeleteObject();
        ImageDC.DeleteDC();
    }
    //������
    if (!stepList.empty())
    {
        p = stepList.back();//��ȡ��ǰ����
        ImageDC.CreateCompatibleDC(pDC);
        ForeBMP.LoadBitmap(p.getColor() == 1 ? IDB_CHESS_BLACK_FOCUS : IDB_CHESS_WHITE_FOCUS);
        ForeBMP.GetBitmap(&bm);
        pOldImageBMP = ImageDC.SelectObject(&ForeBMP);
        TransparentBlt(pDC->GetSafeHdc(), 2 + BLANK + p.col * 35, 4 + BLANK + p.row * 35, 36, 36,
            ImageDC.GetSafeHdc(), 0, 0, bm.bmWidth, bm.bmHeight, p.getColor() == 1 ? RGB(255, 255, 255) : RGB(50, 100, 100));
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
        MessageBox(_T("��������ʤ����"), _T(""), MB_OK);
    else if (state == GAME_STATE_WHITEWIN)
        MessageBox(_T("��������ʤ����"), _T(""), MB_OK);
    else if (state == GAME_STATE_DRAW)
        MessageBox(_T("�;֣�"), _T(""), MB_OK);
    else if (state == GAME_STATE_BLACKBAN)
        MessageBox(_T("������֣�����ʤ��"), _T(""), MB_OK);
}

void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
    CClientDC dc(this);
    CRect rcBroard(BLANK, BLANK, BROARD_X + BLANK, BROARD_Y + BLANK);
    if (game->gameState == GAME_STATE_RUN) {
        int col = (point.x - 2 - BLANK) / 35;
        int row = (point.y - 4 - BLANK) / 35;
        if (col < 15 && row < 15 && point.x >= 2 + BLANK&&point.y >= 4 + BLANK) {
            if (game->getPieceState(row, col) == 0) {
                currentPos = { row, col, true };
                SetClassLong(this->GetSafeHwnd(),
                    GCL_HCURSOR,
                    (LONG)LoadCursor(NULL, IDC_HAND));
            }
            else if (game->getPieceState(row, col) != 0) {
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
    if (rcBroard.PtInRect(point) && game->gameState == GAME_STATE_RUN)
    {
        int col = (point.x - 2 - BLANK) / 35;
        int row = (point.y - 4 - BLANK) / 35;
        if (game->getPieceState(row, col) == 0 && row < 15 && col < 15 && point.x >= 2 + BLANK&&point.y >= 4 + BLANK)
        {
            //���Ӳ���
            int side = game->playerSide;
            char str[40] = { '\0' };
            game->playerWork(row, col);
            game->getChessMode(str, row, col, side);
            debugStatic.SetWindowTextW(CString(str));
            currentPos.enable = false;
            oldPos = currentPos;
            SetClassLong(this->GetSafeHwnd(), GCL_HCURSOR, (LONG)LoadCursor(NULL, IDC_NO));
            InvalidateRect(rcBroard, false);
            checkVictory(game->gameState);
            AIWork();
        }
    }

    CWnd::OnLButtonDown(nFlags, point);
}

void CChildView::AIWork()
{
    if (game->gameState == GAME_STATE_RUN)
    {
        if (!game->playerToPlayer)
        {
            //AIworkinfo = new AIWorkInfo;
            //AIworkinfo->game = game;
            game->setGameState(GAME_STATE_WAIT);
            startProgress();
            //AIWorkThread = AfxBeginThread(AIWorkThreadFunc, AIworkinfo);
            AIWorkThread = AfxBeginThread(AIWorkThreadFunc, game);
        }

    }
}

UINT CChildView::AIWorkThreadFunc(LPVOID lpParam)
{
    srand(unsigned int(time(0)));
    Game* game = (Game*)lpParam;
    GameTreeNode::maxTaskNum = 0;
    game->AIWork();
    //delete pInfo;
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

void CChildView::OnTimer(UINT_PTR nIDEvent)
{
    if (1 == nIDEvent)
    {
        if (game->gameState == GAME_STATE_WAIT)
            myProgress.StepIt();
        else
        {
            endProgress();
            InvalidateRect(CRect(0 + BLANK, 0 + BLANK, BROARD_X + BLANK, BROARD_Y + BLANK), FALSE);
            checkVictory(game->gameState);
            //CString s;
            //s.AppendFormat(_T("%d"), GameTreeNode::maxTaskNum);
            //debugStatic.SetWindowTextW(s);
            CString s;
            s.AppendFormat(_T("hit: %llu \n miss:%llu \n clash:%llu \n"), GameTreeNode::hash_hit, GameTreeNode::hash_miss, GameTreeNode::hash_clash);
            if (GameTreeNode::resultFlag == AIRESULTFLAG_NEARWIN)
            {
                s.AppendFormat(_T("������������Ҫ���ˣ�"));
            }
            else if (GameTreeNode::resultFlag == AIRESULTFLAG_FAIL)
            {
                s.AppendFormat(_T("��ţ���ҷ���"));
            }
            else if (GameTreeNode::resultFlag == AIRESULTFLAG_TAUNT)
            {
                s.AppendFormat(_T("�������ˣ�û�õ�"));
            }
            debugStatic.SetWindowTextW(s);
        }

    }
    CWnd::OnTimer(nIDEvent);
}

void CChildView::OnStepback()
{
    if (game->gameState != GAME_STATE_WAIT)
    {
        game->stepBack();
        Invalidate(FALSE);
    }
}

void CChildView::OnStart()
{
    if (game->gameState != GAME_STATE_WAIT)
    {
        game->initGame();
        currentPos.enable = false;
        oldPos.enable = false;
        Invalidate(FALSE);
    }
}

void CChildView::OnAIhelp()
{
    if (game->gameState == GAME_STATE_RUN)
    {
        Position pos = game->getNextStepByAI(game->HelpLevel);
        game->playerWork(pos.row, pos.col);
        Invalidate(FALSE);
        checkVictory(game->gameState);
        if (game->gameState == GAME_STATE_RUN)
        {
            AIWork();
        }
    }
}

void CChildView::OnFirsthand()
{
    if (game->gameState != GAME_STATE_WAIT)
    {
        game->playerToPlayer = false;
        game->changeSide(1);
        updateInfoStatic();
        Invalidate(FALSE);
    }
}

void CChildView::OnUpdateFirsthand(CCmdUI *pCmdUI)
{
    if (game->playerSide == 1 && !game->playerToPlayer)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}

void CChildView::OnSecondhand()
{
    if (game->gameState != GAME_STATE_WAIT)
    {
        game->playerToPlayer =false;
        game->changeSide(-1);
        updateInfoStatic();
        Invalidate(FALSE);
    }
}

void CChildView::OnUpdateSecondhand(CCmdUI *pCmdUI)
{
    if (game->playerSide == -1 && !game->playerToPlayer)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}

void CChildView::OnPlayertoplayer()
{
    if (game->gameState != GAME_STATE_WAIT)
    {
        game->playerToPlayer = true;
        updateInfoStatic();
    }
}


void CChildView::OnUpdatePlayertoplayer(CCmdUI *pCmdUI)
{
    if (game->playerToPlayer)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}

void CChildView::OnSave()
{
    if (game->gameState != GAME_STATE_WAIT)
    {
        CString		PathName;
        CString		path_and_fileName;

        UpdateData(TRUE);

        PathName = _T("ChessBoard");

        CString szFilter = _T("ChessBoard Files(*.cshx)|*.cshx||");

        CFileDialog	fdlg(FALSE, _T("cshx"), PathName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter);

        if (IDOK != fdlg.DoModal()) return;
        path_and_fileName = fdlg.GetPathName();   //path_and_fileName��Ϊ�ļ�����·��
        game->saveBoard(path_and_fileName);
        UpdateData(FALSE);
    }
}

void CChildView::OnLoad()
{
    if (game->gameState != GAME_STATE_WAIT)
    {
        // Create dialog to open multiple files.
        CRect rcBroard(0 + BLANK, 0 + BLANK, BROARD_X + BLANK, BROARD_Y + BLANK);
        CString filePath;
        CString szFilter = _T("ChessBoard Files(*.cshx)|*.cshx||");
        UpdateData(TRUE);
        CFileDialog  fdlg(TRUE, _T("cshx"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter);
        if (IDOK != fdlg.DoModal()) return;
        filePath = fdlg.GetPathName();   // filePath��Ϊ���򿪵��ļ���·��  
        UpdateData(FALSE);
        game->loadBoard(filePath);
        checkVictory(game->gameState);
        updateInfoStatic();
        Invalidate();
    }
}

void CChildView::OnHelpPrimary()
{
    if (game->gameState != GAME_STATE_WAIT)
    {
        game->HelpLevel = (1);
    }
}


void CChildView::OnHelpSecondry()
{
    if (game->gameState != GAME_STATE_WAIT)
    {
        game->HelpLevel = (2);
    }
}


void CChildView::OnHelpAdvanced()
{
    if (game->gameState != GAME_STATE_WAIT)
    {
        game->HelpLevel = (3);
    }
}


void CChildView::OnUpdateHelpPrimary(CCmdUI *pCmdUI)
{
    if (game->HelpLevel == 1)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}


void CChildView::OnUpdateHelpSecondry(CCmdUI *pCmdUI)
{
    if (game->HelpLevel == 2)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}


void CChildView::OnUpdateHelpAdvanced(CCmdUI *pCmdUI)
{
    if (game->HelpLevel == 3)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}


void CChildView::OnAIPrimary()
{
    if (game->gameState != GAME_STATE_WAIT)
    {
        game->AIlevel = 1;
        game->setBan(false);
        updateInfoStatic();
    }
}

void CChildView::OnUpdateAIPrimary(CCmdUI *pCmdUI)
{
    if (game->AIlevel == 1)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}

void CChildView::OnAISecondry()
{
    if (game->gameState != GAME_STATE_WAIT)
    {
        game->AIlevel = 2;
        game->setBan(true);
        updateInfoStatic();
    }
}

void CChildView::OnUpdateAISecondry(CCmdUI *pCmdUI)
{
    if (game->AIlevel == 2)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
    /*pCmdUI->Enable(false);*/
}

void CChildView::OnAIAdvanced()
{
    if (game->gameState != GAME_STATE_WAIT)
    {
        game->AIlevel = 3;
        game->setBan(true);
        updateInfoStatic();
    }
}

void CChildView::OnUpdateAIAdvanced(CCmdUI *pCmdUI)
{
    if (game->AIlevel == 3)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}

void CChildView::OnAIMaster()
{
    if (game->gameState != GAME_STATE_WAIT)
    {
        game->AIlevel = 4;
        game->setBan(true);
        updateInfoStatic();
    }
}


void CChildView::OnUpdateAIMaster(CCmdUI *pCmdUI)
{
    if (game->AIlevel == 4)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}

void CChildView::OnDebug()
{
    /*Invalidate();*/
    //string result = ChessBoard::searchTrieTree->testSearch();
    
    debugStatic.SetWindowTextW(game->debug(1));

    /*MessageBox(debug, _T("������Ϣ"), MB_OK);*/
}

void CChildView::OnSettings()
{
    if (game->gameState != GAME_STATE_WAIT)
    {
        DlgSettings dlg;
        dlg.uStep = game->parameter.caculateSteps;
        dlg.algType = TrieTreeNode::algType;
        if (dlg.DoModal() == IDOK)
        {
            game->parameter.caculateSteps = dlg.uStep;
            TrieTreeNode::algType = dlg.algType;
            updateInfoStatic();
        }
    }
}


void CChildView::OnMultithread()
{
    if (game->gameState != GAME_STATE_WAIT)
    {
        game->parameter.multithread = !game->parameter.multithread;
        updateInfoStatic();
    } 
}

void CChildView::OnUpdateMultithread(CCmdUI *pCmdUI)
{
    if (game->parameter.multithread)
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
    game->showStep = !game->showStep;
    Invalidate(FALSE);
}


void CChildView::OnUpdateShowStep(CCmdUI *pCmdUI)
{
    if (game->showStep)
        pCmdUI->SetCheck(true);
    else
        pCmdUI->SetCheck(false);
}


BOOL CChildView::OnEraseBkgnd(CDC* pDC)
{
    //���α���ˢ��
    return TRUE;
}
