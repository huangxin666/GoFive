
// ChildView.cpp : CChildView ���ʵ��
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
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()



// CChildView ��Ϣ�������

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
	// TODO: �ڴ˴������Ϣ����������

	// ��ҪΪ������Ϣ������ CWnd::OnPaint()
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
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
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
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	CClientDC dc(this);
	CRect rcBroard(0+BLANK,0+BLANK,BROARD_X+BLANK,BROARD_Y+BLANK);
	if(rcBroard.PtInRect(point)&&uGameState==GAME_STATE_RUN){
		int col=(point.x-2-BLANK)/35;
		int row=(point.y-4-BLANK)/35;
		if (currentBoard->m_pFive[row][col].uState == 0 && row<15 && col<15 && point.x >= 2 + BLANK&&point.y >= 4 + BLANK){
			//���Ӳ���
			currentBoard->doNextStep(row, col,playerSide);
			currentBoard->m_pFive[row][col].isFocus = false;
			//temp.current->isFocus=false;
			CurrentPoint=0;
			InvalidateRect(rcBroard,FALSE);
			SetClassLong(this->GetSafeHwnd(), GCL_HCURSOR ,(LONG)LoadCursor(NULL , IDC_NO));
			int sss = currentBoard->stepListGetCount();
			if (currentBoard->stepListGetCount() == 225)
				uGameState=GAME_STATE_DRAW;
			Invalidate(FALSE);
			if(isVictory()||uGameState==GAME_STATE_DRAW){ 
				Invalidate(FALSE);
			}else{
				AIWork();
			}
			
			if(uGameState==GAME_STATE_BLACKWIN)
				MessageBox(_T("��������ʤ����"), _T("ʤ���ѷ�"), MB_OK);
			else if(uGameState==GAME_STATE_WHITEWIN)
				MessageBox(_T("��������ʤ����"), _T("ʤ���ѷ�"), MB_OK);
			else if(uGameState==GAME_STATE_DRAW)
				MessageBox( _T("����;�ʤ����"), _T("���ܣ���սһ�֣�"), MB_OK);
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
	if(uGameState!=GAME_STATE_RUN)
		uGameState=GAME_STATE_RUN;
	Invalidate(FALSE);
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
			if (!currentBoard.m_pFive[i][j].isHot)
				continue;
			if (currentBoard.m_pFive[i][j].uState == 0){
				tempBoard = currentBoard;
				tempBoard.doNextStep(i, j, state);
				StepScore = tempBoard.getStepScores(*tempBoard.getPiece());
				//score = currentBoard.getGlobalScore(state);
				for (int a = 0; a<BOARD_ROW_MAX; ++a){
					for (int b = 0; b<BOARD_COL_MAX; ++b){
						if (!tempBoard.m_pFive[a][b].isHot)
							continue;
						if (tempBoard.m_pFive[a][b].uState == 0){
							tempBoard.m_pFive[a][b].uState = -state;
							score = tempBoard.getStepScores(a, b, -state);
							//score = currentBoard.getGlobalScore(state);
							if (score>HighestScoreTemp){
								HighestScoreTemp = score;
							}
							tempBoard.m_pFive[a][b].uState = 0;
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
	int score;
	for (int i = 0; i<BOARD_ROW_MAX; ++i){
		for (int j = 0; j<BOARD_COL_MAX; ++j){
			if (!currentBoard.m_pFive[i][j].isHot)
				continue;
			if (currentBoard.m_pFive[i][j].uState == 0){
				HighestScoreTemp = 0;
				totalScore = 0;
				tempBoard = currentBoard;
				tempBoard.doNextStep(i, j, state);
				StepScore = tempBoard.getStepScores(*tempBoard.getPiece());
				//score = currentBoard.getGlobalScore(state);
				for (int a = 0; a<BOARD_ROW_MAX; ++a){
					for (int b = 0; b<BOARD_COL_MAX; ++b){
						if (!tempBoard.m_pFive[a][b].isHot)
							continue;
						if (tempBoard.m_pFive[a][b].uState == 0){
							tempBoard.m_pFive[a][b].uState = -state;
							score = tempBoard.getStepScores(a, b, -state);
							//score = currentBoard.getGlobalScore(state);
							totalScore += score;
							if (score>HighestScoreTemp){
								HighestScoreTemp = score;
							}
							tempBoard.m_pFive[a][b].uState = 0;
						}

					}
				}
				if (StepScore >= 10000 ){
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
		AIstep=getBestStepAI2(3,*currentBoard,-playerSide);
	//���Ӳ���
	currentBoard->doNextStep(AIstep.x, AIstep.y, -playerSide);
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
		//���Ӳ���
		currentBoard->doNextStep(7, 7, -playerSide);
	}
	uGameState=GAME_STATE_RUN;	
}


void CChildView::OnStart()
{
	InitGame();	
	Invalidate(FALSE);
}

//����Ƿ�ʤ��
bool CChildView::isVictory(void)
{
	int col = currentBoard->getPiece()->uRow;
	int row = currentBoard->getPiece()->uCol;
	int state = currentBoard->getPiece()->uState;
	int direction[4][9];//�ĸ��������棨0��ʾ�գ�-1��ʾ�ϣ�1��ʾ����

	for(int i=0;i<9;++i){//����
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
	for(int i=0;i<9;++i){//����
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
	for(int i=0;i<9;++i){//������
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
	for(int i=0;i<9;++i){//������
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
	if(AIlevel==2)
		pCmdUI->SetCheck( true );
	else
		pCmdUI->SetCheck( false );
	/*pCmdUI->Enable(false);*/
}


void CChildView::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ
	OnStepback();
	CWnd::OnRButtonDown(nFlags, point);
}
