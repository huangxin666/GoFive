
// ChildView.cpp : CChildView 类的实现
//

#include "stdafx.h"
#include "fiveRenju.h"
#include "ChildView.h"
#include "defines.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CChildView

CChildView::CChildView()
{	
	InitGame();
	playerSide = 1;
	AIlevel = 1;
	HelpLevel = 1;
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
	ON_COMMAND(ID_AI_PRIMARY, &CChildView::OnAiPrimary)
	ON_UPDATE_COMMAND_UI(ID_AI_PRIMARY, &CChildView::OnUpdateAiPrimary)
	ON_COMMAND(ID_AI_SECONDRY, &CChildView::OnAiSecondry)
	ON_UPDATE_COMMAND_UI(ID_AI_SECONDRY, &CChildView::OnUpdateAiSecondry)
	ON_WM_RBUTTONDOWN()
	ON_COMMAND(ID_AI_ADVANCED, &CChildView::OnAiAdvanced)
	ON_UPDATE_COMMAND_UI(ID_AI_ADVANCED, &CChildView::OnUpdateAiAdvanced)
	ON_COMMAND(ID_SAVE, &CChildView::OnSave)
	ON_COMMAND(ID_LOAD, &CChildView::OnLoad)
	ON_COMMAND(ID_HELP_PRIMARY, &CChildView::OnHelpPrimary)
	ON_COMMAND(ID_HELP_SECONDRY, &CChildView::OnHelpSecondry)
	ON_COMMAND(ID_HELP_ADVANCED, &CChildView::OnHelpAdvanced)
	ON_UPDATE_COMMAND_UI(ID_HELP_PRIMARY, &CChildView::OnUpdateHelpPrimary)
	ON_UPDATE_COMMAND_UI(ID_HELP_SECONDRY, &CChildView::OnUpdateHelpSecondry)
	ON_UPDATE_COMMAND_UI(ID_HELP_ADVANCED, &CChildView::OnUpdateHelpAdvanced)
	ON_COMMAND(ID_AIHELP, &CChildView::OnAihelp)
END_MESSAGE_MAP()



// CChildView 消息处理程序

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), NULL);

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
				DrawChess(&dcMemory);
					// 将内存设备的内容拷贝到实际屏幕显示的设备
				dc.BitBlt(m_rcClient.left, m_rcClient.top, m_rcClient.right, m_rcClient.bottom, &dcMemory, 0, 0, SRCCOPY);
				bitmap.DeleteObject();
			}
		}
	}
	// TODO: 在此处添加消息处理程序代码

	// 不要为绘制消息而调用 CWnd::OnPaint()
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
	pDC->StretchBlt(0,0,rect.Width(),rect.Height(),&dcMemory,0,0,rect.Width(),rect.Height(),SRCCOPY);	
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
	pDC->StretchBlt(BLANK,BLANK,BROARD_X,BROARD_Y,&dcMemory,0,0,BROARD_X,BROARD_Y,SRCCOPY);	
}

void CChildView::DrawChess(CDC* pDC)
{
	for (int i = 0; i < BOARD_ROW_MAX;i++){
		for (int j = 0; j < BOARD_COL_MAX;j++){	
			if (currentBoard->getPiece(i, j).getState() != STATE_EMPTY){
				BITMAP bm;
				CDC ImageDC;
				ImageDC.CreateCompatibleDC(pDC);
				CBitmap ForeBMP;
				if (currentBoard->getPiece(i, j).getState() == STATE_CHESS_BLACK){
					if (currentBoard->getPiece(i, j).isFlag()){
						ForeBMP.LoadBitmap(IDB_CHESS_BLACK_FOCUS);
					}else{
						ForeBMP.LoadBitmap(IDB_CHESS_BLACK);
					}
					ForeBMP.GetBitmap(&bm);
					CBitmap *pOldImageBMP=ImageDC.SelectObject(&ForeBMP);
					TransparentBlt(pDC->GetSafeHdc(),2+BLANK+j*35,4+BLANK+i*35,36,36,
					ImageDC.GetSafeHdc(),0,0,bm.bmWidth,bm.bmHeight,RGB(255,255,255));
					ImageDC.SelectObject(pOldImageBMP);
				}else{
					if (currentBoard->getPiece(i, j).isFlag()){
						ForeBMP.LoadBitmap(IDB_CHESS_WHITE_FOCUS);
					}else{
						ForeBMP.LoadBitmap(IDB_CHESS_WHITE);
					}
					ForeBMP.GetBitmap(&bm);
					CBitmap *pOldImageBMP=ImageDC.SelectObject(&ForeBMP);
					TransparentBlt(pDC->GetSafeHdc(),2+BLANK+j*35,4+BLANK+i*35,36,36,
					ImageDC.GetSafeHdc(),0,0,bm.bmWidth,bm.bmHeight,RGB(50,100,100));
					ImageDC.SelectObject(pOldImageBMP);
				}	
				ForeBMP.DeleteObject();
				ImageDC.DeleteDC();
			}
		}
	}
	//POSITION pos=stepList.GetHeadPosition();
	//	for (int i = 0; i < stepList.GetCount(); i++){	
	//		BITMAP bm;
	//		CDC ImageDC;
	//		ImageDC.CreateCompatibleDC(pDC);
	//		CBitmap ForeBMP;
	//		if(stepList.GetAt(pos).current->uState){
	//			if(stepList.GetAt(pos).current->uState==STATE_CHESS_BLACK){
	//				if(stepList.GetAt(pos).current->isFlag){
	//					ForeBMP.LoadBitmap(IDB_CHESS_BLACK_FOCUS);
	//				}else{
	//					ForeBMP.LoadBitmap(IDB_CHESS_BLACK);
	//				}
	//				ForeBMP.GetBitmap(&bm);
	//				CBitmap *pOldImageBMP=ImageDC.SelectObject(&ForeBMP);
	//				TransparentBlt(pDC->GetSafeHdc(),2+BLANK+stepList.GetAt(pos).current->uCol*35,4+BLANK+stepList.GetAt(pos).current->uRow*35,36,36,
	//				ImageDC.GetSafeHdc(),0,0,bm.bmWidth,bm.bmHeight,RGB(255,255,255));
	//				ImageDC.SelectObject(pOldImageBMP);
	//			}else{
	//				if(stepList.GetAt(pos).current->isFlag){
	//					ForeBMP.LoadBitmap(IDB_CHESS_WHITE_FOCUS);
	//				}else{
	//					ForeBMP.LoadBitmap(IDB_CHESS_WHITE);
	//				}
	//				ForeBMP.GetBitmap(&bm);
	//				CBitmap *pOldImageBMP=ImageDC.SelectObject(&ForeBMP);
	//				TransparentBlt(pDC->GetSafeHdc(),2+BLANK+stepList.GetAt(pos).current->uCol*35,4+BLANK+stepList.GetAt(pos).current->uRow*35,36,36,
	//				ImageDC.GetSafeHdc(),0,0,bm.bmWidth,bm.bmHeight,RGB(50,100,100));
	//				ImageDC.SelectObject(pOldImageBMP);
	//			}			
	//		}
	//		ForeBMP.DeleteObject();
	//		ImageDC.DeleteDC();
	//		stepList.GetNext(pos);
	//	}
	
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
		CBitmap *pOldImageBMP=ImageDC.SelectObject(&ForeBMP);
		TransparentBlt(pDC->GetSafeHdc(),2+BLANK+CurrentPoint->getCol()*35,4+BLANK+CurrentPoint->getRow()*35,36,36,
			ImageDC.GetSafeHdc(),0,0,bm.bmWidth,bm.bmHeight,RGB(255,255,255));
		ImageDC.SelectObject(pOldImageBMP);
		ForeBMP.DeleteObject();
		ImageDC.DeleteDC(); 
	}
    /*CDC dcMemory;	                        
	dcMemory.CreateCompatibleDC(pDC);               
	CBitmap   bmpBackground;                
	bmpBackground.LoadBitmap(IDB_TEST);             
	dcMemory.SelectObject(&bmpBackground);               
	pDC->StretchBlt(2+CurrentPoint->uCol*35,4+CurrentPoint->uRow*35,36,36
		,&dcMemory,0,0,36,36,SRCCOPY);				*/
}

//AISTEP CChildView::getBestStepAI1(int stepCount,ChessBoard currentBoard,int state)
//{
//	ChessBoard tempBoard;
//	AISTEP stepCurrent;
//	AISTEP randomStep[225];
//	randomStep[0].score=0;
//	randomStep[0].x=0;
//	randomStep[0].y=0;
//	int randomCount=0;
//	stepCurrent.score=0;
//	int HighestScore=-500000;
//	int StepScore=0;		
//	int score=0;
//	for (int i = 0; i<BOARD_ROW_MAX; ++i){
//		for (int j = 0; j<BOARD_COL_MAX; ++j){
//			if (!currentBoard.m_pFive[i][j].isHot)
//				continue;
//			if(currentBoard.m_pFive[i][j].uState==0){
//				tempBoard = currentBoard;
//				tempBoard.doNextStep(i, j, state);
//				score = tempBoard.getStepScores(*tempBoard.getPiece());
//				//score = currentBoard.getGlobalScore(state);
//				if(stepCount>1)
//					StepScore = score - getBestStepAI1(stepCount - 1, tempBoard, -state).score;
//				else
//					StepScore=score;
//				if(StepScore>HighestScore){
//					HighestScore=StepScore;
//					stepCurrent.score=HighestScore;
//					stepCurrent.x = i;
//					stepCurrent.y = j;
//					randomCount=0;
//					randomStep[randomCount]=stepCurrent;
//				}else if(StepScore==HighestScore){
//					stepCurrent.score=HighestScore;
//					stepCurrent.x = i;
//					stepCurrent.y = j;
//					randomCount++;
//					randomStep[randomCount]=stepCurrent;
//				}
//			}		
//		}
//	}
//	srand(unsigned int(time(0)));
//	int random=rand()%(randomCount+1);
//	return randomStep[random];
//}

AISTEP CChildView::getBestStepAI1(int stepCount, ChessBoard currentBoard, int state)
{
	ChessBoard tempBoard;
	AISTEP stepCurrent;
	AISTEP randomStep[225];
	randomStep[0].score = 0;
	randomStep[0].x = 0;
	randomStep[0].y = 0;
	int randomCount = 0;
	stepCurrent.score = 0;
	int HighestScore = -500000;
	int HighestScoreTemp = -500000;
	int StepScore = 0;
	int score = 0;
	for (int i = 0; i<BOARD_ROW_MAX; ++i){
		for (int j = 0; j<BOARD_COL_MAX; ++j){
			HighestScoreTemp = -500000;
			if (!currentBoard.getPiece(i, j).isHot())
				continue;
			if (currentBoard.getPiece(i, j).getState() == 0){
				tempBoard = currentBoard;
				tempBoard.doNextStep(i, j, state);
				StepScore = tempBoard.getStepScores(tempBoard.getPiece());
				//score = currentBoard.getGlobalScore(state);
				for (int a = 0; a<BOARD_ROW_MAX; ++a){
					for (int b = 0; b<BOARD_COL_MAX; ++b){
						if (!tempBoard.getPiece(a, b).isHot())
							continue;
						if (tempBoard.getPiece(a, b).getState() == 0){
							tempBoard.getPiece(a, b).setState(-state);
							score = tempBoard.getStepScores(a, b, -state);
							//score = currentBoard.getGlobalScore(state);
							if (score>HighestScoreTemp){
								HighestScoreTemp = score;
							}
							tempBoard.getPiece(a, b).setState(0);
						}
						
					}
				}
				StepScore = StepScore - HighestScoreTemp;
				if (StepScore>HighestScore){
					HighestScore = StepScore;
					stepCurrent.score = HighestScore;
					stepCurrent.x = i;
					stepCurrent.y = j;
					randomCount = 0;
					randomStep[randomCount] = stepCurrent;
				}
				else if (StepScore == HighestScore){
					stepCurrent.score = HighestScore;
					stepCurrent.x = i;
					stepCurrent.y = j;
					randomCount++;
					randomStep[randomCount] = stepCurrent;
				}
			}
		}
	}
	srand(unsigned int(time(0)));
	int random = rand() % (randomCount + 1);
	return randomStep[random];
}


AISTEP CChildView::getBestStepAI2(int stepCount,ChessBoard currentBoard,int state)
{
	ChessBoard tempBoard;
	AISTEP stepCurrent;
	AISTEP randomStep[225];
	randomStep[0].score = 0;
	randomStep[0].x = 0;
	randomStep[0].y = 0;
	int randomCount = 0;
	stepCurrent.score = 0;
	int HighestScore = -500000;
	int HighestScoreTemp = 0;
	int StepScore = 0;
	int totalScore = 0;
	for (int i = 0; i<BOARD_ROW_MAX; ++i){
		for (int j = 0; j<BOARD_COL_MAX; ++j){
			if (!currentBoard.getPiece(i, j).isHot())
				continue;
			if (currentBoard.getPiece(i, j).getState() == 0){
				HighestScoreTemp = 0;
				totalScore = 0;
				tempBoard = currentBoard;
				tempBoard.doNextStep(i, j, state);
				StepScore = tempBoard.getStepScores(tempBoard.getPiece());
				//score = currentBoard.getGlobalScore(state);
				THREATINFO info = tempBoard.getThreatInfo(-state,1);
				totalScore = info.totalScore;
				HighestScoreTemp = info.HighestScore;
				if (StepScore >= 100000){
					stepCurrent.score = StepScore;
					stepCurrent.x = i;
					stepCurrent.y = j;
					return stepCurrent;
				}
				else if (StepScore >= 10000 ){
					if (HighestScoreTemp < 100000){
						stepCurrent.score = StepScore;
						stepCurrent.x = i;
						stepCurrent.y = j;
						return stepCurrent;
					}
				}
				else if (StepScore >= 8000){
					if (HighestScoreTemp < 10000){
						stepCurrent.score = StepScore;
						stepCurrent.x = i;
						stepCurrent.y = j;
						return stepCurrent;
					}
				}
				StepScore = StepScore - totalScore;
				if (StepScore>HighestScore){
					HighestScore = StepScore;
					stepCurrent.score = HighestScore;
					stepCurrent.x = i;
					stepCurrent.y = j;
					randomCount = 0;
					randomStep[randomCount] = stepCurrent;
				}
				else if (StepScore == HighestScore){
					stepCurrent.score = HighestScore;
					stepCurrent.x = i;
					stepCurrent.y = j;
					randomCount++;
					randomStep[randomCount] = stepCurrent;
				}
			}
		}
	}
	srand(unsigned int(time(0)));
	int random = rand() % (randomCount + 1);
	return randomStep[random];
}



void CChildView::AIWork(void)
{
	AISTEP AIstep;
	if(AIlevel==1)
		AIstep=getBestStepAI1(2,*currentBoard,-playerSide);
	else if(AIlevel==2)
		AIstep=getBestStepAI2(2,*currentBoard,-playerSide);
	else if (AIlevel == 3)
		AIstep = getBestStepAI2(2, *currentBoard, -playerSide);
	//棋子操作
	currentBoard->doNextStep(AIstep.x, AIstep.y, -playerSide);
	if(isVictory()){ 
		
	}
	else if (currentBoard->stepListGetCount() == 225){
		uGameState = GAME_STATE_DRAW;
	}
	Invalidate(FALSE);
	if (uGameState == GAME_STATE_BLACKWIN)
		MessageBox(_T("黑棋五连胜利！"), _T("胜负已分"), MB_OK);
	else if (uGameState == GAME_STATE_WHITEWIN)
		MessageBox(_T("白棋五连胜利！"), _T("胜负已分"), MB_OK);
	else if (uGameState == GAME_STATE_DRAW)
		MessageBox(_T("白棋和局胜利！"), _T("别跑，再战一局！"), MB_OK);
}

void CChildView::AIHelp(void){
	CRect rcBroard(0 + BLANK, 0 + BLANK, BROARD_X + BLANK, BROARD_Y + BLANK);
	if (uGameState == GAME_STATE_RUN){
		//棋子操作
		if (currentBoard->stepListIsEmpty()){
			currentBoard->doNextStep(7, 7, playerSide);
		}
		else{
			AISTEP AIstep;
			if (HelpLevel == 1)
				AIstep = getBestStepAI1(2, *currentBoard, playerSide);
			else if (HelpLevel == 2)
				AIstep = getBestStepAI2(2, *currentBoard, playerSide);
			else if (HelpLevel == 3)
				AIstep = getBestStepAI2(2, *currentBoard, playerSide);
			//棋子操作
			currentBoard->doNextStep(AIstep.x, AIstep.y, playerSide);
		}
		//temp.current->isFocus=false;
		if (isVictory()){

		}
		else if (currentBoard->stepListGetCount() == 225){
			uGameState = GAME_STATE_DRAW;
		}
		InvalidateRect(rcBroard, FALSE);

		if (uGameState == GAME_STATE_RUN){
			AIWork();
		}
		else{
			if (uGameState == GAME_STATE_BLACKWIN)
				MessageBox(_T("黑棋五连胜利！"), _T("胜负已分"), MB_OK);
			else if (uGameState == GAME_STATE_WHITEWIN)
				MessageBox(_T("白棋五连胜利！"), _T("胜负已分"), MB_OK);
			else if (uGameState == GAME_STATE_DRAW)
				MessageBox(_T("白棋和局胜利！"), _T("别跑，再战一局！"), MB_OK);
		}
	}
}

//检测是否胜利
bool CChildView::isVictory(void)
{
	//int col = currentBoard->getPiece().getCol();
	//int row = currentBoard->getPiece().getRow();
	int state = currentBoard->getPiece().getState();
	int score=currentBoard->getStepScores(currentBoard->getPiece());
	if (score>=100000){
		if (state == 1){
			uGameState = GAME_STATE_BLACKWIN;
		}
		else if (state == -1){
			uGameState = GAME_STATE_WHITEWIN;
		}
		return true;
	}
	return false;
}


void CChildView::InitGame(void)
{
	currentBoard = new ChessBoard();
	CurrentPoint=0;
	oldCurrentPoint=0;
	if(playerSide==-1){
		//棋子操作
		currentBoard->doNextStep(7, 7, -playerSide);
	}
	uGameState=GAME_STATE_RUN;	
}

void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
	CClientDC dc(this);
	CRect rcBroard(BLANK, BLANK, BROARD_X + BLANK, BROARD_Y + BLANK);
	if (uGameState == GAME_STATE_RUN){
		int col = (point.x - 2 - BLANK) / 35;
		int row = (point.y - 4 - BLANK) / 35;
		if (col<15 && row<15 && point.x >= 2 + BLANK&&point.y >= 4 + BLANK){
			if (currentBoard->getPiece(row, col).getState() == 0){
				CurrentPoint = &currentBoard->getPiece(row, col);
				SetClassLong(this->GetSafeHwnd(),
					GCL_HCURSOR,
					(LONG)LoadCursor(NULL, IDC_HAND));
			}
			else if (currentBoard->getPiece(row, col).getState() != 0){
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
	if (rcBroard.PtInRect(point) && uGameState == GAME_STATE_RUN){
		int col = (point.x - 2 - BLANK) / 35;
		int row = (point.y - 4 - BLANK) / 35;
		if (currentBoard->getPiece(row, col).getState() == 0 && row<15 && col<15 && point.x >= 2 + BLANK&&point.y >= 4 + BLANK){
			//棋子操作
			currentBoard->doNextStep(row, col, playerSide);
			currentBoard->getPiece(row, col).setFocus(false);
			//temp.current->isFocus=false;
			CurrentPoint = 0;
			SetClassLong(this->GetSafeHwnd(), GCL_HCURSOR, (LONG)LoadCursor(NULL, IDC_NO));
			if (isVictory()){

			}
			else if (currentBoard->stepListGetCount() == 225){
				uGameState = GAME_STATE_DRAW;
			}
			InvalidateRect(rcBroard, FALSE);
			if (uGameState == GAME_STATE_RUN){
				AIWork();
			}
			else{
				if (uGameState == GAME_STATE_BLACKWIN)
					MessageBox(_T("黑棋五连胜利！"), _T("胜负已分"), MB_OK);
				else if (uGameState == GAME_STATE_WHITEWIN)
					MessageBox(_T("白棋五连胜利！"), _T("胜负已分"), MB_OK);
				else if (uGameState == GAME_STATE_DRAW)
					MessageBox(_T("白棋和局胜利！"), _T("别跑，再战一局！"), MB_OK);
			}
		}
	}
	CWnd::OnLButtonDown(nFlags, point);
}


void CChildView::stepBack(void)
{
	currentBoard->stepBack(playerSide);

}


void CChildView::OnStepback()
{
	stepBack();
	if (uGameState != GAME_STATE_RUN)
		uGameState = GAME_STATE_RUN;
	Invalidate(FALSE);
}

void CChildView::OnStart()
{
	InitGame();	
	Invalidate(FALSE);
}

void CChildView::ChangeSide(void)
{
	playerSide=-playerSide;
	OnStart();
}

void CChildView::OnFirsthand()
{
	if (playerSide != 1){
		playerSide=1;
		if (uGameState == GAME_STATE_RUN)
			AIWork();
	}
	Invalidate(FALSE);
}

void CChildView::OnUpdateFirsthand(CCmdUI *pCmdUI)
{
	if(playerSide==1)
		pCmdUI->SetCheck( true );
	else
		pCmdUI->SetCheck( false );
}

void CChildView::OnSecondhand()
{
	if (playerSide != -1){
		playerSide = -1;
		if (uGameState == GAME_STATE_RUN)
			if (currentBoard->stepListIsEmpty())
				OnStart();
			else
				AIWork();
	}
	Invalidate(FALSE);
}

void CChildView::OnUpdateSecondhand(CCmdUI *pCmdUI)
{
	if(playerSide==-1)
		pCmdUI->SetCheck( true );
	else
		pCmdUI->SetCheck( false );
}

void CChildView::OnAiPrimary()
{
	AIlevel=1;
}

void CChildView::OnUpdateAiPrimary(CCmdUI *pCmdUI)
{
	if(AIlevel==1)
		pCmdUI->SetCheck( true );
	else
		pCmdUI->SetCheck( false );
}

void CChildView::OnAiSecondry()
{
	AIlevel=2;
}


void CChildView::OnUpdateAiSecondry(CCmdUI *pCmdUI)
{
	if(AIlevel==2)
		pCmdUI->SetCheck( true );
	else
		pCmdUI->SetCheck( false );
	/*pCmdUI->Enable(false);*/
}


void CChildView::OnRButtonDown(UINT nFlags, CPoint point)
{
	OnStepback();
	CWnd::OnRButtonDown(nFlags, point);
}


void CChildView::OnAiAdvanced()
{
	AIlevel = 3;
}


void CChildView::OnUpdateAiAdvanced(CCmdUI *pCmdUI)
{
	if (AIlevel == 3)
		pCmdUI->SetCheck(true);
	else
		pCmdUI->SetCheck(false);
}


void CChildView::OnSave()
{
	CString		Title, FmtString;
	CString		PathName;
	CString		path_and_fileName;

	UpdateData(TRUE);

	PathName = _T("ChessBoard");

	CString szFilter =_T( "ChessBoard Files(*.cshx)|*.cshx||");

	CFileDialog	fdlg(FALSE, _T("cshx"), PathName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter);

	if (IDOK != fdlg.DoModal()) return;
	path_and_fileName = fdlg.GetPathName();   //path_and_fileName即为文件保存路径
	currentBoard->saveBoard(path_and_fileName);
	UpdateData(FALSE);
}



void CChildView::OnLoad()
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
	//currentBoard = new ChessBoard();
	currentBoard->loadBoard(filePath);

	if (isVictory()){

	}
	else if (currentBoard->stepListGetCount() == 225){
		uGameState = GAME_STATE_DRAW;
	}
	InvalidateRect(rcBroard, FALSE);

	if (uGameState == GAME_STATE_RUN){
		if (currentBoard->getPiece().getState() == playerSide){
			AIWork();
		}
	}
	else{
		if (uGameState == GAME_STATE_BLACKWIN)
			MessageBox(_T("黑棋五连胜利！"), _T("胜负已分"), MB_OK);
		else if (uGameState == GAME_STATE_WHITEWIN)
			MessageBox(_T("白棋五连胜利！"), _T("胜负已分"), MB_OK);
		else if (uGameState == GAME_STATE_DRAW)
			MessageBox(_T("白棋和局胜利！"), _T("别跑，再战一局！"), MB_OK);
	}

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


void CChildView::OnAihelp()
{
	AIHelp();
}
