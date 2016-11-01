
// ChildView.cpp : CChildView 类的实现
//

#include "stdafx.h"
#include "fiveRenju.h"
#include "ChildView.h"
#include "DlgSetCaculateStep.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CChildView

static CWinThread* AIWorkThread;

struct AIWorkInfo
{
	Game *game;
	CChildView *childView;

};

static AIWorkInfo *AIworkinfo;

static UINT AIWorkThreadFunc(LPVOID lpParam);

static int threadFlag = 0;

CChildView::CChildView()
{
	CurrentPoint = 0;
	oldCurrentPoint = 0;
	game = new Game();		
}

CChildView::~CChildView()
{
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
	ON_COMMAND(ID_DOBAN, &CChildView::OnDoban)
	ON_UPDATE_COMMAND_UI(ID_DOBAN, &CChildView::OnUpdateDoban)
	ON_COMMAND(ID_UNBAN, &CChildView::OnUnban)
	ON_UPDATE_COMMAND_UI(ID_UNBAN, &CChildView::OnUpdateUnban)
	ON_COMMAND(ID_MULTITHREAD_OPEN, &CChildView::OnMultithreadOpen)
	ON_UPDATE_COMMAND_UI(ID_MULTITHREAD_OPEN, &CChildView::OnUpdateMultithreadOpen)
	ON_COMMAND(ID_MULTITHREAD_CLOSE, &CChildView::OnMultithreadClose)
	ON_UPDATE_COMMAND_UI(ID_MULTITHREAD_CLOSE, &CChildView::OnUpdateMultithreadClose)
	ON_WM_TIMER()
	ON_COMMAND(ID_SET_AISTEP, &CChildView::OnSetAIStep)
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
				DrawChess(&dcMemory, game->getCurrentBoard());
				DrawProgress(&dcMemory);
				// 将内存设备的内容拷贝到实际屏幕显示的设备
				dc.BitBlt(m_rcClient.left, m_rcClient.top, m_rcClient.right, m_rcClient.bottom, &dcMemory, 0, 0, SRCCOPY);
				bitmap.DeleteObject();
			}
		}
	}

	// 不要为绘制消息而调用 CWnd::OnPaint()
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

void CChildView::DrawChess(CDC* pDC, ChessBoard * currentBoard)
{
	BITMAP bm;
	CDC ImageDC;
	CBitmap ForeBMP;
	CBitmap *pOldImageBMP;
	for (int i = 0; i < BOARD_ROW_MAX; i++){
		for (int j = 0; j < BOARD_COL_MAX; j++){
			if (currentBoard->getPiece(i, j).getState() != STATE_EMPTY){		
				ImageDC.CreateCompatibleDC(pDC);
				if (currentBoard->getPiece(i, j).getState() == STATE_CHESS_BLACK){
					ForeBMP.LoadBitmap(IDB_CHESS_BLACK);
					ForeBMP.GetBitmap(&bm);
					pOldImageBMP = ImageDC.SelectObject(&ForeBMP);
					TransparentBlt(pDC->GetSafeHdc(), 2 + BLANK + j * 35, 4 + BLANK + i * 35, 36, 36,
						ImageDC.GetSafeHdc(), 0, 0, bm.bmWidth, bm.bmHeight, RGB(255, 255, 255));
					ImageDC.SelectObject(pOldImageBMP);
				}
				else{
					ForeBMP.LoadBitmap(IDB_CHESS_WHITE);
					ForeBMP.GetBitmap(&bm);
					pOldImageBMP = ImageDC.SelectObject(&ForeBMP);
					TransparentBlt(pDC->GetSafeHdc(), 2 + BLANK + j * 35, 4 + BLANK + i * 35, 36, 36,
						ImageDC.GetSafeHdc(), 0, 0, bm.bmWidth, bm.bmHeight, RGB(50, 100, 100));
					ImageDC.SelectObject(pOldImageBMP);
				}		
				ForeBMP.DeleteObject();
				ImageDC.DeleteDC();
			}
		}
	}
	//画焦点
	if (!game->stepListIsEmpty())
	{
		Piece p = currentBoard->getPiece();//获取当前棋子
		ImageDC.CreateCompatibleDC(pDC);
		if (p.getState() == STATE_CHESS_BLACK){
			ForeBMP.LoadBitmap(IDB_CHESS_BLACK_FOCUS);
			ForeBMP.GetBitmap(&bm);
			pOldImageBMP = ImageDC.SelectObject(&ForeBMP);
			TransparentBlt(pDC->GetSafeHdc(), 2 + BLANK + p.getCol() * 35, 4 + BLANK + p.getRow() * 35, 36, 36,
				ImageDC.GetSafeHdc(), 0, 0, bm.bmWidth, bm.bmHeight, RGB(255, 255, 255));
			ImageDC.SelectObject(pOldImageBMP);
		}
		else{
			ForeBMP.LoadBitmap(IDB_CHESS_WHITE_FOCUS);
			ForeBMP.GetBitmap(&bm);
			pOldImageBMP = ImageDC.SelectObject(&ForeBMP);
			TransparentBlt(pDC->GetSafeHdc(), 2 + BLANK + p.getCol() * 35, 4 + BLANK + p.getRow() * 35, 36, 36,
				ImageDC.GetSafeHdc(), 0, 0, bm.bmWidth, bm.bmHeight, RGB(50, 100, 100));
			ImageDC.SelectObject(pOldImageBMP);
		}
		ForeBMP.DeleteObject();
		ImageDC.DeleteDC();
	}
}

void CChildView::DrawMouseFocus(CDC * pDC)
{
	if (CurrentPoint)
	{
		BITMAP bm;
		CDC ImageDC;
		ImageDC.CreateCompatibleDC(pDC);
		CBitmap ForeBMP;
		ForeBMP.LoadBitmap(IDB_MOUSE_FOCUS);
		ForeBMP.GetBitmap(&bm);
		CBitmap *pOldImageBMP = ImageDC.SelectObject(&ForeBMP);
		TransparentBlt(pDC->GetSafeHdc(), 2 + BLANK + CurrentPoint->getCol() * 35, 4 + BLANK + CurrentPoint->getRow() * 35, 36, 36,
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
	if (game->getGameState() == GAME_STATE_RUN){
		int col = (point.x - 2 - BLANK) / 35;
		int row = (point.y - 4 - BLANK) / 35;
		if (col < 15 && row < 15 && point.x >= 2 + BLANK&&point.y >= 4 + BLANK){
			if (game->getCurrentBoard()->getPiece(row, col).getState() == 0){
				CurrentPoint = &game->getCurrentBoard()->getPiece(row, col);
				SetClassLong(this->GetSafeHwnd(),
					GCL_HCURSOR,
					(LONG)LoadCursor(NULL, IDC_HAND));
			}
			else if (game->getCurrentBoard()->getPiece(row, col).getState() != 0){
				CurrentPoint = 0;
				SetClassLong(this->GetSafeHwnd(),
					GCL_HCURSOR,
					(LONG)LoadCursor(NULL, IDC_NO));
			}
		}
		else{
			CurrentPoint = 0;
			SetClassLong(this->GetSafeHwnd(),
				GCL_HCURSOR,
				(LONG)LoadCursor(NULL, IDC_ARROW));
		}
	}
	else{
		CurrentPoint = 0;
		SetClassLong(this->GetSafeHwnd(),
			GCL_HCURSOR,
			(LONG)LoadCursor(NULL, IDC_ARROW));
	}
	if (CurrentPoint && (oldCurrentPoint != CurrentPoint)){
		InvalidateRect(CRect(2 + BLANK + CurrentPoint->getCol() * 35, 4 + BLANK + CurrentPoint->getRow() * 35,
			38 + BLANK + CurrentPoint->getCol() * 35, 40 + BLANK + CurrentPoint->getRow() * 35), FALSE);
	}
	if (oldCurrentPoint){
		InvalidateRect(CRect(2 + BLANK + oldCurrentPoint->getCol() * 35, 4 + BLANK + oldCurrentPoint->getRow() * 35,
			38 + BLANK + oldCurrentPoint->getCol() * 35, 40 + BLANK + oldCurrentPoint->getRow() * 35), FALSE);
	}
	oldCurrentPoint = CurrentPoint;
	CWnd::OnMouseMove(nFlags, point);
}

void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
	CClientDC dc(this);
	CRect rcBroard(0 + BLANK, 0 + BLANK, BROARD_X + BLANK, BROARD_Y + BLANK);
	if (rcBroard.PtInRect(point) && game->getGameState() == GAME_STATE_RUN){
		int col = (point.x - 2 - BLANK) / 35;
		int row = (point.y - 4 - BLANK) / 35;
		if (game->getCurrentBoard()->getPiece(row, col).getState() == 0 && row < 15 && col < 15 && point.x >= 2 + BLANK&&point.y >= 4 + BLANK)
		{
			//棋子操作
			game->playerWork(row, col);
			CurrentPoint = 0;
			SetClassLong(this->GetSafeHwnd(), GCL_HCURSOR, (LONG)LoadCursor(NULL, IDC_NO));
			InvalidateRect(rcBroard, FALSE);
			checkVictory(game->getGameState());
			if (game->getGameState() == GAME_STATE_RUN)
			{
				if (!game->isPlayerToPlayer())
				{
					game->setGameState(GAME_STATE_WAIT);
					AIworkinfo = new AIWorkInfo;
					AIworkinfo->game = game;
					AIworkinfo->childView = this;
					AIWorkThread = AfxBeginThread(AIWorkThreadFunc, AIworkinfo);
					startProgress();

				/*	game->AIWork();
					InvalidateRect(rcBroard, FALSE);
					checkVictory(game->getGameState());*/
				}
					
			}
			
		}
	}
	CWnd::OnLButtonDown(nFlags, point);
}

UINT AIWorkThreadFunc(LPVOID lpParam)
{
	srand(unsigned int(time(0)));
	AIWorkInfo* pInfo = (AIWorkInfo*)lpParam;
	Game *game = pInfo->game;
	CChildView *childView = pInfo->childView;
	game->AIWork();
	delete pInfo;
	return 0;
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
		CurrentPoint = 0;
		oldCurrentPoint = 0;
		Invalidate(FALSE);
	}
}

void CChildView::OnAIhelp()
{
	if (game->getGameState() != GAME_STATE_WAIT)
	{
		game->AIHelp();
		Invalidate(FALSE);
		checkVictory(game->getGameState());
	}
}

void CChildView::OnFirsthand()
{
	if (game->getGameState() != GAME_STATE_WAIT)
	{
		game->setPlayerToPlayer(false);
		game->changeSide(1);
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
		CString		Title, FmtString;
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
		game->init();
		game->loadBoard(filePath);
		checkVictory(game->getGameState());
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
	if (game->getGameState() != GAME_STATE_WAIT)
	{
		game->setAIlevel(1);
	}
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
	Invalidate();
	//MessageBox(debug, _T("调试信息"), MB_OK);
}





void CChildView::OnDoban()
{
	game->setBan(true);
}


void CChildView::OnUpdateDoban(CCmdUI *pCmdUI)
{
	if (game->isBan())
		pCmdUI->SetCheck(true);
	else
		pCmdUI->SetCheck(false);
}


void CChildView::OnUnban()
{
	game->setBan(false);
}


void CChildView::OnUpdateUnban(CCmdUI *pCmdUI)
{
	if (!game->isBan())
		pCmdUI->SetCheck(true);
	else
		pCmdUI->SetCheck(false);
}


void CChildView::OnMultithreadOpen()
{
	game->setMultithread(true);
}


void CChildView::OnUpdateMultithreadOpen(CCmdUI *pCmdUI)
{
	if (game->isMultithread())
		pCmdUI->SetCheck(true);
	else
		pCmdUI->SetCheck(false);
}


void CChildView::OnMultithreadClose()
{
	game->setMultithread(false);
}


void CChildView::OnUpdateMultithreadClose(CCmdUI *pCmdUI)
{
	if (!game->isMultithread())
		pCmdUI->SetCheck(true);
	else
		pCmdUI->SetCheck(false);
}


void CChildView::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if (1 == nIDEvent)
	{
		if (game->getGameState() == GAME_STATE_WAIT)
			myProgress.StepIt();
		else
		{
			endProgress();
			InvalidateRect(CRect(0 + BLANK, 0 + BLANK, BROARD_X + BLANK, BROARD_Y + BLANK) , FALSE);
			checkVictory(game->getGameState());
		}
			
	}
	CWnd::OnTimer(nIDEvent);
}


void CChildView::init()
{
	myProgress.Create( WS_CHILD | PBS_SMOOTH, 
		CRect(BROARD_X / 2-50, BLANK + BROARD_Y + 10, BROARD_X / 2 + BLANK+50, BLANK + BROARD_Y + 25),this, 1);
	myStatic.Create(_T("AI计算中："), WS_CHILD | SS_CENTER, 
		CRect(BROARD_X / 2 - 150, BLANK + BROARD_Y+7, BROARD_X / 2 -50, BLANK + BROARD_Y + 30), this);
	
	font.CreatePointFont(110, _T("微软雅黑"), NULL);
	myStatic.SetFont(&font);
}

void CChildView::startProgress()
{
	myProgress.ShowWindow(SW_SHOWNA);
	myStatic.ShowWindow(SW_SHOWNA);
	myProgress.SetPos(0);
	SetTimer(1, 200, NULL);
}

void CChildView::endProgress()
{
	KillTimer(1);
	myProgress.ShowWindow(SW_HIDE);
	myStatic.ShowWindow(SW_HIDE);
}


void CChildView::OnSetAIStep()
{
	if (game->getGameState() != GAME_STATE_WAIT)
	{
		DlgSetCaculateStep dlg;
		dlg.uStep = game->getCaculateStep();
		if (dlg.DoModal() == IDOK)
		{
			game->setCaculateStep(dlg.uStep);
		}
	}
}
