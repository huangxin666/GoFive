
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
			if (currentBoard->getPiece(i, j)->uState != STATE_EMPTY){
				BITMAP bm;
				CDC ImageDC;
				ImageDC.CreateCompatibleDC(pDC);
				CBitmap ForeBMP;
				if (currentBoard->getPiece(i, j)->uState == STATE_CHESS_BLACK){
					if (currentBoard->getPiece(i, j)->isFlag){
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
					if (currentBoard->getPiece(i, j)->isFlag){
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
		TransparentBlt(pDC->GetSafeHdc(),2+BLANK+CurrentPoint->uCol*35,4+BLANK+CurrentPoint->uRow*35,36,36,
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


void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CClientDC dc(this);
	CRect rcBroard(BLANK,BLANK,BROARD_X+BLANK,BROARD_Y+BLANK);
	if(rcBroard.PtInRect(point)&&uGameState==GAME_STATE_RUN){
		int col=(point.x-2-BLANK)/35;
		int row=(point.y-4-BLANK)/35;
		if (currentBoard->m_pFive[row][col].uState == 0 && col<15 && row<15 && point.x >= 2 + BLANK&&point.y >= 4 + BLANK){
			CurrentPoint = currentBoard->getPiece(row, col);
			SetClassLong(this->GetSafeHwnd(),
                             GCL_HCURSOR ,
                             (LONG)LoadCursor(NULL , IDC_HAND));
		} else{
			CurrentPoint=0;
			SetClassLong(this->GetSafeHwnd(),
                             GCL_HCURSOR ,
                             (LONG)LoadCursor(NULL , IDC_NO));
		}
		if(CurrentPoint&&oldCurrentPoint!=CurrentPoint){
			InvalidateRect(CRect(2+BLANK+CurrentPoint->uCol*35,4+BLANK+CurrentPoint->uRow*35,
				38+BLANK+CurrentPoint->uCol*35,40+BLANK+CurrentPoint->uRow*35),FALSE);			
		}
		if(oldCurrentPoint){
			InvalidateRect(CRect(2+BLANK+oldCurrentPoint->uCol*35,4+BLANK+oldCurrentPoint->uRow*35,
				38+BLANK+oldCurrentPoint->uCol*35,40+BLANK+oldCurrentPoint->uRow*35),FALSE);
		}
		oldCurrentPoint=CurrentPoint;
		
	}else{
		SetClassLong(this->GetSafeHwnd(),
                             GCL_HCURSOR ,
                             (LONG)LoadCursor(NULL , IDC_ARROW));
	}
	CWnd::OnMouseMove(nFlags, point);
}


void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CClientDC dc(this);
	CRect rcBroard(0+BLANK,0+BLANK,BROARD_X+BLANK,BROARD_Y+BLANK);
	if(rcBroard.PtInRect(point)&&uGameState==GAME_STATE_RUN){
		int col=(point.x-2-BLANK)/35;
		int row=(point.y-4-BLANK)/35;
		if (currentBoard->m_pFive[row][col].uState == 0 && row<15 && col<15 && point.x >= 2 + BLANK&&point.y >= 4 + BLANK){

			//链表操作
			STEP temp;
			if (currentBoard->stepListIsEmpty()){
				temp.step=1;
				searchRect = CRect(row - 1, col - 1, row + 1, col + 1);
			}else{
				currentBoard->getPiece()->setFlag(false);
				temp.step = currentBoard->stepListGetCount() + 1;
			}
			temp.uRow = row;
			temp.uCol = col;
			currentBoard->currentStep = currentBoard->stepListAddTail(temp);
			//搜索区域操作
			if (temp.step>1)
			{
				if (col<searchRect.left + 2){
					searchRect.left = (col - 2) >= 0 ? (col - 2) : 0;
				}
				else if (col>searchRect.right - 2){
					searchRect.right = (col + 2) <= 14 ? (col + 2) : 14;
				}
				if (row<searchRect.top + 2){
					searchRect.top = (row - 2) >= 0 ? (row - 2) : 0;
				}
				else if (row>searchRect.bottom - 2){
					searchRect.bottom = (row + 2) <= 14 ? (row + 2) : 14;
				} 
			}
			//棋子操作
			currentBoard->m_pFive[row][col].uState = playerSide;
			currentBoard->m_pFive[row][col].isFlag = true;
			currentBoard->m_pFive[row][col].isFocus = false;
			//temp.current->isFocus=false;
			CurrentPoint=0;
			InvalidateRect(rcBroard,FALSE);
			SetClassLong(this->GetSafeHwnd(),
                             GCL_HCURSOR ,
                             (LONG)LoadCursor(NULL , IDC_NO));
			if (currentBoard->stepListGetCount() == 225)
				uGameState=GAME_STATE_DRAW;
			if(isVictory()||uGameState==GAME_STATE_DRAW){ 
				Invalidate(FALSE);
			}
			else {
				AIWork();
			}
			
			if(uGameState==GAME_STATE_BLACKWIN)
				MessageBox(_T("黑棋胜利！"), _T("胜负已分"), MB_OK);
			else if(uGameState==GAME_STATE_WHITEWIN)
				MessageBox(_T("白棋胜利！"), _T("胜负已分"), MB_OK);
			else if(uGameState==GAME_STATE_DRAW)
				MessageBox( _T("和棋！"), _T("别跑，再战一局！"), MB_OK);
		} 
	}
	CWnd::OnLButtonDown(nFlags, point);
}


void CChildView::stepBack(void)
{
	if (currentBoard->stepListGetCount()>1){
		if (currentBoard->getPiece()->uState == playerSide){
			currentBoard->getPiece()->isFocus = false;
			currentBoard->getPiece()->isFlag = false;
			currentBoard->getPiece()->uState = 0;
				currentBoard->stepListGetPrev();
				currentBoard->stepListRemoveTail();
			}else{
			currentBoard->getPiece()->isFocus = false;
			currentBoard->getPiece()->isFlag = false;
			currentBoard->getPiece()->uState = 0;
				currentBoard->stepListGetPrev();
				currentBoard->stepListRemoveTail();
				currentBoard->getPiece()->isFocus = false;
				currentBoard->getPiece()->isFlag = false;
				currentBoard->getPiece()->uState = 0;
				currentBoard->stepListGetPrev();
				currentBoard->stepListRemoveTail();
			}
			if (currentBoard->currentStep)
				currentBoard->getPiece()->isFlag = true;
		} 
}


void CChildView::OnStepback()
{
	stepBack();
	if(uGameState!=GAME_STATE_RUN)
		uGameState=GAME_STATE_RUN;
	Invalidate(FALSE);
}




AISTEP CChildView::getBestStepAI1(int stepCount,ChessBoard currentBoard,int state)
{
	AISTEP stepCurrent;
	AISTEP randomStep[225];
	randomStep[0].score=0;
	randomStep[0].x=0;
	randomStep[0].y=0;
	int randomCount=0;
	stepCurrent.score=0;
	int HighestScore=-500000;
	int StepScore=0;		
	int score=0;
	for(int i=searchRect.left;i<searchRect.right+1;++i){
		for(int j=searchRect.top;j<searchRect.bottom+1;++j){
			if(currentBoard.m_pFive[j][i].uState==0){
				currentBoard.m_pFive[j][i].uState=state;
				score = currentBoard.getStepScores(currentBoard.m_pFive[j][i]);
				if(stepCount>1)
					StepScore=score-getBestStepAI1(stepCount-1,currentBoard,-state).score;
				else
					StepScore=score;
				if(StepScore>HighestScore){
					HighestScore=StepScore;
					stepCurrent.score=HighestScore;
					stepCurrent.x=j;
					stepCurrent.y=i;
					randomCount=1;
					randomStep[randomCount]=stepCurrent;
				}else if(StepScore==HighestScore){
					stepCurrent.score=HighestScore;
					stepCurrent.x=j;
					stepCurrent.y=i;
					randomCount++;
					randomStep[randomCount]=stepCurrent;
				}
				currentBoard.m_pFive[j][i].uState=0;
			}
				
		}
	}
	srand(unsigned int(time(0)));
	int random=rand()%randomCount+1;
	return randomStep[random];
}


AISTEP CChildView::getBestStepAI2(int stepCount,ChessBoard currentBoard,int state)
{
	AISTEP stepCurrent;
	return stepCurrent;
}



void CChildView::AIWork(void)
{
	AISTEP AIstep;
	if(AIlevel==1)
		AIstep=getBestStepAI1(2,*currentBoard,-playerSide);
	else if(AIlevel==2)
		AIstep=getBestStepAI2(3,*currentBoard,-playerSide);
	STEP temp;
	currentBoard->getPiece(currentBoard->stepListGetAt(currentBoard->currentStep))->isFlag = false;
	temp.step = currentBoard->stepListGetCount() + 1;
	temp.uRow = AIstep.x;
	temp.uCol = AIstep.y;
	currentBoard->currentStep = currentBoard->stepListAddTail(temp);
	//棋子操作
	currentBoard->m_pFive[AIstep.x][AIstep.y].uState=-playerSide;
	currentBoard->m_pFive[AIstep.x][AIstep.y].isFlag=true;
	if (currentBoard->stepListGetCount() == 225)
				uGameState=GAME_STATE_DRAW;
	if(isVictory()||uGameState==GAME_STATE_DRAW){ 
				Invalidate(FALSE);
	}
}


//


void CChildView::InitGame(void)
{
	currentBoard = new ChessBoard();
	CurrentPoint=0;
	oldCurrentPoint=0;
	if(playerSide==-1){
		STEP temp;
		temp.step=1;
		temp.uRow = 7;
		temp.uCol = 7;
		currentBoard->currentStep = currentBoard->stepListAddTail(temp);
		//棋子操作
		currentBoard->m_pFive[7][7].uState=-playerSide;
		currentBoard->m_pFive[7][7].isFlag=true;
		searchRect=CRect(6,6,8,8);
	}
	uGameState=GAME_STATE_RUN;	
}


void CChildView::OnStart()
{
	InitGame();	
	Invalidate(FALSE);
}









bool CChildView::isVictory(void)
{
	int col = currentBoard->stepListGetAt(currentBoard->currentStep).uRow;
	int row = currentBoard->stepListGetAt(currentBoard->currentStep).uCol;
	int state = currentBoard->getPiece(currentBoard->stepListGetAt(currentBoard->currentStep))->uState;
	int direction[4][9];//四个方向棋面（0表示空，-1表示断，1表示连）

	for(int i=0;i<9;++i){//横向
		if(col-4+i<0)
			direction[0][i]=-1;
		else if(col-4+i>14)
			direction[0][i]=-1;
		else if(currentBoard->m_pFive[col-4+i][row].uState==-state)
			direction[0][i]=-1;
		else if(currentBoard->m_pFive[col-4+i][row].uState==state)
			direction[0][i]=1;
		else if(currentBoard->m_pFive[col-4+i][row].uState==0)
			direction[0][i]=0;
	}
	for(int i=0;i<9;++i){//纵向
		if(row-4+i<0)
			direction[1][i]=-1;
		else if(row-4+i>14)
			direction[1][i]=-1;
		else if(currentBoard->m_pFive[col][row-4+i].uState==-state)
			direction[1][i]=-1;
		else if(currentBoard->m_pFive[col][row-4+i].uState==state)
			direction[1][i]=1;
		else if(currentBoard->m_pFive[col][row-4+i].uState==0)
			direction[1][i]=0;
	}
	for(int i=0;i<9;++i){//右下向
			if(col-4+i<0||row-4+i<0)
				direction[2][i]=-1;
			else if(col-4+i>14||row-4+i>14)
				direction[2][i]=-1;
			else if(currentBoard->m_pFive[col-4+i][row-4+i].uState==-state)
				direction[2][i]=-1;
			else if(currentBoard->m_pFive[col-4+i][row-4+i].uState==state)
				direction[2][i]=1;
			else if(currentBoard->m_pFive[col-4+i][row-4+i].uState==0)
				direction[2][i]=0;
		}
	for(int i=0;i<9;++i){//左上向
			if(col-4+i<0||row+4-i<0)
				direction[3][i]=-1;
			else if(col-4+i>14||row+4-i>14)
				direction[3][i]=-1;
			else if(currentBoard->m_pFive[col-4+i][row+4-i].uState==-state)
				direction[3][i]=-1;
			else if(currentBoard->m_pFive[col-4+i][row+4-i].uState==state)
				direction[3][i]=1;
			else if(currentBoard->m_pFive[col-4+i][row+4-i].uState==0)
				direction[3][i]=0;
		}
	for(int i=0;i<4;i++){
		if (currentBoard->isFiveContinue(direction[i])){
			if(state==1){
				uGameState=GAME_STATE_BLACKWIN;
			}else if(state==-1){
				uGameState=GAME_STATE_WHITEWIN;
			}
			return true;
		}
	}
	return false;
}


void CChildView::ChangeSide(void)
{
	playerSide=-playerSide;
	OnStart();
}





void CChildView::OnFirsthand()
{
	playerSide=1;
	OnStart();
	
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
	playerSide=-1;
	OnStart();
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
	/*if(AIlevel==2)
		pCmdUI->SetCheck( true );
	else
		pCmdUI->SetCheck( false );*/
	pCmdUI->Enable(false);
}
