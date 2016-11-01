
// ChildView.cpp : CChildView ���ʵ��
//

#include "stdafx.h"
#include "fiveRenju.h"
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
	ON_WM_TIMER()
	ON_COMMAND(ID_SETTINGS, &CChildView::OnSettings)
	ON_COMMAND(ID_MULTITHREAD, &CChildView::OnMultithread)
	ON_UPDATE_COMMAND_UI(ID_MULTITHREAD, &CChildView::OnUpdateMultithread)
	ON_COMMAND(ID_BAN, &CChildView::OnBan)
	ON_UPDATE_COMMAND_UI(ID_BAN, &CChildView::OnUpdateBan)
	ON_COMMAND(ID_SHOWSTEP, &CChildView::OnShowStep)
	ON_UPDATE_COMMAND_UI(ID_SHOWSTEP, &CChildView::OnUpdateShowStep)
END_MESSAGE_MAP()



// CChildView ��Ϣ�������

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
				DrawChess(&dcMemory, game->getStepList());
				DrawProgress(&dcMemory);
				// ���ڴ��豸�����ݿ�����ʵ����Ļ��ʾ���豸
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
	//BITMAP bm;
	//CDC ImageDC;
	//CBitmap ForeBMP;
	//CBitmap *pOldImageBMP;
	//for (int i = 0; i < BOARD_ROW_MAX; i++){
	//	for (int j = 0; j < BOARD_COL_MAX; j++){
	//		if (currentBoard->getPiece(i, j).getState() != STATE_EMPTY){		
	//			ImageDC.CreateCompatibleDC(pDC);
	//			if (currentBoard->getPiece(i, j).getState() == STATE_CHESS_BLACK){
	//				ForeBMP.LoadBitmap(IDB_CHESS_BLACK);
	//				ForeBMP.GetBitmap(&bm);
	//				pOldImageBMP = ImageDC.SelectObject(&ForeBMP);
	//				TransparentBlt(pDC->GetSafeHdc(), 2 + BLANK + j * 35, 4 + BLANK + i * 35, 36, 36,
	//					ImageDC.GetSafeHdc(), 0, 0, bm.bmWidth, bm.bmHeight, RGB(255, 255, 255));
	//				ImageDC.SelectObject(pOldImageBMP);
	//			}
	//			else{
	//				ForeBMP.LoadBitmap(IDB_CHESS_WHITE);
	//				ForeBMP.GetBitmap(&bm);
	//				pOldImageBMP = ImageDC.SelectObject(&ForeBMP);
	//				TransparentBlt(pDC->GetSafeHdc(), 2 + BLANK + j * 35, 4 + BLANK + i * 35, 36, 36,
	//					ImageDC.GetSafeHdc(), 0, 0, bm.bmWidth, bm.bmHeight, RGB(50, 100, 100));
	//				ImageDC.SelectObject(pOldImageBMP);
	//			}		
	//			ForeBMP.DeleteObject();
	//			ImageDC.DeleteDC();
	//		}
	//	}
	//}
	////������
	//if (!game->stepListIsEmpty())
	//{
	//	Piece p = currentBoard->getPiece();//��ȡ��ǰ����
	//	ImageDC.CreateCompatibleDC(pDC);
	//	if (p.getState() == STATE_CHESS_BLACK){
	//		ForeBMP.LoadBitmap(IDB_CHESS_BLACK_FOCUS);
	//		ForeBMP.GetBitmap(&bm);
	//		pOldImageBMP = ImageDC.SelectObject(&ForeBMP);
	//		TransparentBlt(pDC->GetSafeHdc(), 2 + BLANK + p.getCol() * 35, 4 + BLANK + p.getRow() * 35, 36, 36,
	//			ImageDC.GetSafeHdc(), 0, 0, bm.bmWidth, bm.bmHeight, RGB(255, 255, 255));
	//		ImageDC.SelectObject(pOldImageBMP);
	//	}
	//	else{
	//		ForeBMP.LoadBitmap(IDB_CHESS_WHITE_FOCUS);
	//		ForeBMP.GetBitmap(&bm);
	//		pOldImageBMP = ImageDC.SelectObject(&ForeBMP);
	//		TransparentBlt(pDC->GetSafeHdc(), 2 + BLANK + p.getCol() * 35, 4 + BLANK + p.getRow() * 35, 36, 36,
	//			ImageDC.GetSafeHdc(), 0, 0, bm.bmWidth, bm.bmHeight, RGB(50, 100, 100));
	//		ImageDC.SelectObject(pOldImageBMP);
	//	}
	//	ForeBMP.DeleteObject();
	//	ImageDC.DeleteDC();
	//}
	BITMAP bm;
	CDC ImageDC;
	CBitmap ForeBMP;
	CBitmap *pOldImageBMP;
	STEP p;
	for (UINT i = 0; i < stepList.size();++i)
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
	//������
	if (!stepList.empty())
	{
		p = stepList.back();//��ȡ��ǰ����
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
	if (game->getGameState() == GAME_STATE_RUN){
		int col = (point.x - 2 - BLANK) / 35;
		int row = (point.y - 4 - BLANK) / 35;
		if (col < 15 && row < 15 && point.x >= 2 + BLANK&&point.y >= 4 + BLANK){
			if (game->getPiece(row, col).getState() == 0){
				CurrentPoint = &game->getPiece(row, col);
				SetClassLong(this->GetSafeHwnd(),
					GCL_HCURSOR,
					(LONG)LoadCursor(NULL, IDC_HAND));
			}
			else if (game->getPiece(row, col).getState() != 0){
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
	if (rcBroard.PtInRect(point) && game->getGameState() == GAME_STATE_RUN)
	{
		int col = (point.x - 2 - BLANK) / 35;
		int row = (point.y - 4 - BLANK) / 35;
		if (game->getPiece(row, col).getState() == 0 && row < 15 && col < 15 && point.x >= 2 + BLANK&&point.y >= 4 + BLANK)
		{
			//���Ӳ���
			game->playerWork(row, col);
			CurrentPoint = 0;
			SetClassLong(this->GetSafeHwnd(), GCL_HCURSOR, (LONG)LoadCursor(NULL, IDC_NO));
			//InvalidateRect(CRect(2 + BLANK + col * 35, 4 + BLANK + row * 35,38 + BLANK + col * 35, 40 + BLANK + row * 35), FALSE);
			
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
			InvalidateRect(rcBroard, false);
			checkVictory(game->getGameState());
		}
	}
	
	CWnd::OnLButtonDown(nFlags, point);
}

UINT AIWorkThreadFunc(LPVOID lpParam)
{
	srand(unsigned int(time(0)));
	AIWorkInfo* pInfo = (AIWorkInfo*)lpParam;
	Game *game = pInfo->game;
	game->AIWork();
	delete pInfo;
	return 0;
}


void CChildView::init()
{
	myProgress.Create(WS_CHILD | PBS_SMOOTH,
		CRect(BROARD_X / 2 - 50, BLANK + BROARD_Y + 10, BROARD_X / 2 + BLANK + 50, BLANK + BROARD_Y + 25), this, 1);

	myProgressStatic.Create(_T("AI˼���У�"), WS_CHILD | SS_CENTER,
		CRect(BROARD_X / 2 - 150, BLANK + BROARD_Y + 7, BROARD_X / 2 - 50, BLANK + BROARD_Y + 30), this);

	infoStatic.Create(_T(""), WS_CHILD | SS_CENTER | WS_VISIBLE, CRect(BLANK, 0, BROARD_X + BLANK, BLANK), this);

	font.CreatePointFont(110, _T("΢���ź�"), NULL);

	myProgressStatic.SetFont(&font);
	infoStatic.SetFont(&font);
	updateInfoStatic();
}

void CChildView::updateInfoStatic()
{
	CString info;
	if (!game->isPlayerToPlayer())
	{
		info += "��ң�";
		if (game->getPlayerSide() == 1)
			info += "����";
		else
			info += "����";

		info += "    ���֣�";
		if (game->isBan())
			info += "��";
		else
			info += "��";

		info += "    AI�ȼ���";
		switch (game->getAIlevel())
		{
		case 1:
			info += "�ͼ�";
			break;
		case 2:
			info += "�м�";
			break;
		case 3:
			info += "�߼�";
			break;
		default:
			info += "δ֪";
			break;
		}

		if (game->getAIlevel() == 3)
		{
			info += "    ���߳��Ż���";
			if (game->isMultithread())
				info += "��";
			else
				info += "��";
			info += "    ���㲽����";
			if (game->isMultithread())
			{
				char n = game->getCaculateStep() + '0';
				info += n;
			}
			else
				info += "4";
		}
	}
	else
		info += "���˶�ս";
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
		path_and_fileName = fdlg.GetPathName();   //path_and_fileName��Ϊ�ļ�����·��
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
		filePath = fdlg.GetPathName();   // filePath��Ϊ���򿪵��ļ���·��  
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
	

	/*MessageBox(debug, _T("������Ϣ"), MB_OK);*/
}

void CChildView::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ
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
