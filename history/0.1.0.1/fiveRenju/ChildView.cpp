
// ChildView.cpp : CChildView ���ʵ��
//

#include "stdafx.h"
#include "fiveRenju.h"
#include "ChildView.h"
#include "macros.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CChildView

CChildView::CChildView()
{
	currentBoard=new FiveBoard;
	InitGame();
	playerSide=1;
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
	POSITION pos=stepList.GetHeadPosition();
		for (int i = 0; i < stepList.GetCount(); i++){	
			BITMAP bm;
			CDC ImageDC;
			ImageDC.CreateCompatibleDC(pDC);
			CBitmap ForeBMP;
			if(stepList.GetAt(pos).current->uState){
				if(stepList.GetAt(pos).current->uState==STATE_CHESS_BLACK){
					if(stepList.GetAt(pos).current->isFlag){
						ForeBMP.LoadBitmap(IDB_CHESS_BLACK_FOCUS);
					}else{
						ForeBMP.LoadBitmap(IDB_CHESS_BLACK);
					}
					ForeBMP.GetBitmap(&bm);
					CBitmap *pOldImageBMP=ImageDC.SelectObject(&ForeBMP);
					TransparentBlt(pDC->GetSafeHdc(),2+BLANK+stepList.GetAt(pos).current->uCol*35,4+BLANK+stepList.GetAt(pos).current->uRow*35,36,36,
					ImageDC.GetSafeHdc(),0,0,bm.bmWidth,bm.bmHeight,RGB(255,255,255));
					ImageDC.SelectObject(pOldImageBMP);
				}else{
					if(stepList.GetAt(pos).current->isFlag){
						ForeBMP.LoadBitmap(IDB_CHESS_WHITE_FOCUS);
					}else{
						ForeBMP.LoadBitmap(IDB_CHESS_WHITE);
					}
					ForeBMP.GetBitmap(&bm);
					CBitmap *pOldImageBMP=ImageDC.SelectObject(&ForeBMP);
					TransparentBlt(pDC->GetSafeHdc(),2+BLANK+stepList.GetAt(pos).current->uCol*35,4+BLANK+stepList.GetAt(pos).current->uRow*35,36,36,
					ImageDC.GetSafeHdc(),0,0,bm.bmWidth,bm.bmHeight,RGB(50,100,100));
					ImageDC.SelectObject(pOldImageBMP);
				}			
			}
			ForeBMP.DeleteObject();
			ImageDC.DeleteDC();
			stepList.GetNext(pos);
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
		int x=(point.x-2-BLANK)/35;
		int y=(point.y-4-BLANK)/35;
		if(currentBoard->m_pFive[x][y].uState==0&&x<15&&y<15&&point.x>=2+BLANK&&point.y>=4+BLANK){
			CurrentPoint=&currentBoard->m_pFive[x][y];
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
		int x=(point.x-2-BLANK)/35;
		int y=(point.y-4-BLANK)/35;
		if(currentBoard->m_pFive[x][y].uState==0&&x<15&&y<15&&point.x>=2+BLANK&&point.y>=4+BLANK){
			//�����������
			if(x<searchRect.left+2){
				searchRect.left=(x-2)>=0?(x-2):0;
			}else if(x>searchRect.right-2){
				searchRect.right=(x+2)<=14?(x+2):14;
			}
			if(y<searchRect.top+2){
				searchRect.top=(y-2)>=0?(y-2):0;
			}else if(y>searchRect.bottom-2){
				searchRect.bottom=(y+2)<=14?(y+2):14;
			}
			//�������
			STEP temp;
			if(stepList.IsEmpty()){
				temp.step=1;
			}else{
			stepList.GetAt(currentStep).current->isFlag=false;
			temp.step=stepList.GetAt(currentStep).step+1;
			}
			temp.current=&currentBoard->m_pFive[x][y];
			currentStep=stepList.AddTail(temp);
			//���Ӳ���
			currentBoard->m_pFive[x][y].uState=playerSide;
			currentBoard->m_pFive[x][y].isFlag=true;
			temp.current->isFocus=false;
			CurrentPoint=0;
			SetClassLong(this->GetSafeHwnd(),
                             GCL_HCURSOR ,
                             (LONG)LoadCursor(NULL , IDC_NO));
			if(stepList.GetCount()==225)
				uGameState=GAME_STATE_DRAW;
			if(bVictory()||uGameState==GAME_STATE_DRAW){ 
				Invalidate(FALSE);
			}
			else {
				AI();
			}
			InvalidateRect(rcBroard,FALSE);
			if(uGameState==GAME_STATE_BLACKWIN)
				MessageBox(_T("����ʤ����"));
			else if(uGameState==GAME_STATE_WHITEWIN)
				MessageBox(_T("����ʤ����"));
			else if(uGameState==GAME_STATE_DRAW)
				MessageBox(_T("���壡"));
		} 
	}
	CWnd::OnLButtonDown(nFlags, point);
}


void CChildView::stepBack(void)
{
		if(stepList.GetCount()>1){
			stepList.GetAt(currentStep).current->isFocus=false;
			stepList.GetAt(currentStep).current->isFlag=false;
			stepList.GetAt(currentStep).current->uState=0;
			stepList.GetPrev(currentStep);
			stepList.RemoveTail();
			stepList.GetAt(currentStep).current->isFocus=false;
			stepList.GetAt(currentStep).current->isFlag=false;
			stepList.GetAt(currentStep).current->uState=0;
			stepList.GetPrev(currentStep);
			stepList.RemoveTail();
			if(currentStep)
				stepList.GetAt(currentStep).current->isFlag=true;
		} 
}


void CChildView::OnStepback()
{
	stepBack();
	if(uGameState!=GAME_STATE_RUN)
		uGameState=GAME_STATE_RUN;
	Invalidate(FALSE);
}


int CChildView::getStepScores(FIVEWND thisStep,FiveBoard* currentBoard)
{
	int stepScore=0;
	UINT row=thisStep.uRow;
	UINT col=thisStep.uCol;
	int state=thisStep.uState;
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
			if(col-4+i<0||row+4-i>14)
				direction[3][i]=-1;
			else if(col-4+i>14||row+4-i<0)
				direction[3][i]=-1;
			else if(currentBoard->m_pFive[col-4+i][row+4-i].uState==-state)
				direction[3][i]=-1;
			else if(currentBoard->m_pFive[col-4+i][row+4-i].uState==state)
				direction[3][i]=1;
			else if(currentBoard->m_pFive[col-4+i][row+4-i].uState==0)
				direction[3][i]=0;
		}


	for(int i=0;i<4;++i){//�ĸ�������,��������AI���߼����ȶ�
		if(isFiveContinue(direction[i])){
			stepScore+=50000;
		}else if(isFourContinue(direction[i])){
			stepScore+=10000;
		}else if (isFourContinueSide(direction[i])){
			stepScore+=1000;
		}else if(isFourContinueVar1(direction[i])){
			stepScore+=1000;
		}else if(isFourContinueVar2(direction[i])){
			stepScore+=950;
		}else if(isThreeContinue(direction[i])){
			stepScore+=500;
		}else if(isThreeContinueVar(direction[i])){
			stepScore+=450;
		}else if(isThreeContinueSide(direction[i])){
			stepScore+=150;
		}else if(isThreeContinueVarSide(direction[i])){
			stepScore+=150;
		}else if(isTwoContinue(direction[i])){
			stepScore+=80;
		}else if(isTwoContinueVar(direction[i])){
			stepScore+=45;
		/*}else if(isOneToOne(direction[i])){
			stepScore+=10;*/
		}
	}
	return stepScore;
}

AISTEP CChildView::getBestStep(int stepCount,FiveBoard currentBoard,int state)
{
	AISTEP stepCurrent;
	AISTEP stepNext;
	stepNext.score=0;
	stepCurrent.score=0;
	if(stepCount>0){
		int HighestScore=-50000;
		int StepScore=0;		
		int score=0;
		
		//for(int i=0;i<15;++i){
		//	for(int j=0;j<15;++j){
		//		if(currentBoard.m_pFive[i][j].uState==0){
		//			currentBoard.m_pFive[i][j].uState=state;
		//			score=getStepScores(currentBoard.m_pFive[i][j],&currentBoard);
		//			stepNext=getBestStep(stepCount-1,currentBoard,-state);
		//			StepScore=score-stepNext.score;
		//			if(StepScore>HighestScore){
		//				HighestScore=StepScore;
		//				stepCurrent.score=HighestScore;
		//				stepCurrent.x=i;
		//				stepCurrent.y=j;
		//			}
		//		}
		//		currentBoard.m_pFive[i][j].uState=0;
		//	}
		//}

		for(int i=searchRect.left;i<searchRect.right+1;++i){
			for(int j=searchRect.top;j<searchRect.bottom+1;++j){
				if(currentBoard.m_pFive[i][j].uState==0){
					currentBoard.m_pFive[i][j].uState=state;
					score=getStepScores(currentBoard.m_pFive[i][j],&currentBoard);
					if(stepCount>1)
						StepScore=score-getBestStep(stepCount-1,currentBoard,-state).score;
					else
						StepScore=score;
					if(StepScore>HighestScore){
						HighestScore=StepScore;
						stepCurrent.score=HighestScore;
						stepCurrent.x=i;
						stepCurrent.y=j;
					}
					currentBoard.m_pFive[i][j].uState=0;
				}
				
			}
		}
	}
	return stepCurrent;
}


void CChildView::AI(void)
{
	int state=-playerSide;
	AISTEP AIstep=getBestStep(2,*currentBoard,state);
	STEP temp;
	stepList.GetAt(currentStep).current->isFlag=false;
	temp.step=stepList.GetAt(currentStep).step+1;
	temp.current=&currentBoard->m_pFive[AIstep.x][AIstep.y];
	currentStep=stepList.AddTail(temp);
	//���Ӳ���
	currentBoard->m_pFive[AIstep.x][AIstep.y].uState=-playerSide;
	currentBoard->m_pFive[AIstep.x][AIstep.y].isFlag=true;
	if(stepList.GetCount()==225)
				uGameState=GAME_STATE_DRAW;
	if(bVictory()||uGameState==GAME_STATE_DRAW){ 
				Invalidate(FALSE);
	}
}


//
//AI�㷨

// 00000
bool CChildView::isFiveContinue(int direction[])
{
	int flag=0;
	for(int i=0;i<9;++i){
		if(direction[i]==1){
			flag+=1;
			if(flag==5)
				return true;
		}else
			flag=0;
	}
	return false;
}

// 0000 
bool CChildView::isFourContinue(int direction[])
{
	int flag=0;
	for(int i=0;i<9;++i){
		if(direction[i]==1){
			flag+=1;
			if(flag==4){
				if(direction[i+1]==0&&direction[i-4]==0)
					return true;
			}
		}else{
			flag=0;
		}
	}
	return false;
}

// 0000X||X0000
bool CChildView::isFourContinueSide(int direction[])
{
	int flag=0;
	for(int i=0;i<9;++i){
		if(direction[i]==1){
			flag+=1;
			if(flag==4){
				if(direction[i+1]==-1&&direction[i-4]==0)
					return true;
				else if(direction[i+1]==0&&direction[i-4]==-1)
					return true;
			}
		}else{
			flag=0;
		}
	}
	return false;
}

//  000 0||0 000
bool CChildView::isFourContinueVar1(int direction[])
{
	int flag=0;
	for(int i=0;i<9;++i){
		if(direction[i]==1){
			flag+=1;
			if(flag==4){
				if(direction[i-1]==0&&direction[i-2]==1&&direction[i-3]==1&&direction[i-4]==1)
					return true;
				else if(direction[i-1]==1&&direction[i-2]==1&&direction[i-3]==0&&direction[i-4]==1)
					return true;
			}
		}else if(direction[i]==0){
			
		}else if(direction[i]==-1){
			flag=0;
		}		
	}
	return false;
}

 // 00 00
bool CChildView::isFourContinueVar2(int direction[])//�п���������
{
	int flag=0;
	for(int i=0;i<9;++i){
		if(direction[i]==1){
			flag+=1;
			if(flag==4){
				if(direction[i-1]==1&&direction[i-2]==0&&direction[i-3]==1&&direction[i-4]==1)
					return true;
			}
		}else if(direction[i]==0){
			
		}else if(direction[i]==-1){
			flag=0;
		}		
	}
	return false;
}

// 000
bool CChildView::isThreeContinue(int direction[]) 
{
	if(direction[3]==1)//0o0
		if(direction[5]==1)
			if(direction[2]==0)
				if(direction[6]==0)
					return true;
	if(direction[3]==0)//o00
		if(direction[5]==1)
			if(direction[6]==1)
				if(direction[7]==0)
					return true;
	if(direction[3]==1)//00o
		if(direction[5]==0)
			if(direction[2]==1)
				if(direction[1]==0)
					return true;
	return false;
}

// 000X||X000
bool CChildView::isThreeContinueSide(int direction[]) 
{
	if(direction[3]==1){//0o0
		if(direction[5]==1){
			if(direction[2]==0){
				if(direction[6]==1){
					if(direction[1]==0)
						return true;
				}
			}else if(direction[2]==1){
				if(direction[6]==0){
					if(direction[7]==0)
						return true;
				}
			}
		}
	}
	if(direction[5]==1){//o00
		if(direction[6]==1){
			if(direction[3]==0){
				if(direction[7]==1){
					if(direction[2]==0)
						return true;
				}
			}else if(direction[3]==1){
				if(direction[7]==0){
					if(direction[8]==0)
						return true;
				}
			}
		}
	}
	if(direction[3]==1){//00o
		if(direction[2]==1){
			if(direction[1]==0){
				if(direction[5]==1){
					if(direction[0]==0)
						return true;
				}
			}else if(direction[1]==1){
				if(direction[5]==0){
					if(direction[6]==0)
						return true;
				}
			}
		}
	}
	return false;
}

// 00 0||0 00
bool CChildView::isThreeContinueVar(int direction[])//�п���������
{
	int flag=0;
	for(int i=0;i<9;++i){
		if(direction[i]==1){
			flag+=1;
			if(flag==3){
				if(direction[i+1]==0&&direction[i-4]==0)//�п������
					return true;
				else
					return false;
			}
		}else if(direction[i]==0){
			if(flag){
				if(direction[i-1]==1&&direction[i+1]==1){//�п������
				}else{
					flag=0;
				}
			}
		}else if(direction[i]==-1){
				flag=0;
		}		
	}
	return false;
}

// 0 00X || X0 00 || 00 0X || X00 0
bool CChildView::isThreeContinueVarSide(int direction[]) 
{
	int flag=0;
	int startFlag=0;
	for(int i=0;i<9;++i){
		if(direction[i]==1){
			flag+=1;
		}else if(direction[i]==0){
			if(flag==1&&!startFlag)
				startFlag=1;//0 00
			else if(flag==2&&!startFlag)
				startFlag=2;//00 0
			else{
				flag=0;
				startFlag=0;
			}
		}else if(direction[i]==-1){
			if(startFlag&&flag==3)
				return true;
			flag=-1;
			startFlag=0;
		}
	}
	flag=0;
	startFlag=0;
	for(int i=8;i>=0;--i){
			if(direction[i]==1){
			flag+=1;
		}else if(direction[i]==0){
			if(flag==1&&!startFlag)
				startFlag=1;//0 00
			else if(flag==2&&!startFlag)
				startFlag=2;//00 0
			else{
				flag=0;
				startFlag=0;
			}
		}else if(direction[i]==-1){
			if(startFlag&&flag==3)
				return true;
			flag=-1;
			startFlag=0;
		}
	}
	return false;
}

// 00
bool CChildView::isTwoContinue(int direction[]) 
{
	if(direction[5]==1){//o0
		if(direction[6]==0){
			if(direction[3]==0){
				if(direction[2]==0||direction[7]==0)
					return true;
			}
		}
	}
	if(direction[3]==1){//0o
		if(direction[2]==0){
			if(direction[5]==0){
				if(direction[1]==0||direction[6]==0)
					return true;
			}
		}
	}
	return false;
}

// 0 0
bool CChildView::isTwoContinueVar(int direction[])
{
	if(direction[5]==0){//o 0
		if(direction[6]==1){
			if(direction[3]==0){
				if(direction[7]==0)
					return true;
			}
		}
	}
	if(direction[3]==0){//0 o
		if(direction[2]==1){
			if(direction[5]==0){
				if(direction[1]==0)
					return true;
			}
		}
	}
	return false;
}

//00X || X00)
//bool CChildView::isDeadTwo(int direction[])
//{
//	if(direction[3]==-1&&direction[5]==1){
//		if(direction[6]==0&&direction[7]==0&&direction[8]==0)
//			return true;
//	}
//	if(direction[2]==-1&&direction[3]==1){
//		if(direction[5]==0&&direction[6]==0&&direction[7]==0)
//			return true;
//	}
//	if(direction[5]==-1&&direction[3]==1){
//		if(direction[2]==0&&direction[1]==0&&direction[0]==0)
//			return true;
//	}
//	if(direction[5]==1&&direction[6]==-1){
//		if(direction[3]==0&&direction[2]==0&&direction[1]==0)
//			return true;
//	}
//	return false;
//}

//end AI�㷨

void CChildView::InitGame(void)
{
	for(int i=0;i<15;++i){
		for(int j=0;j<15;++j){
			currentBoard->m_pFive[i][j].uCol=i;
			currentBoard->m_pFive[i][j].uRow=j;
			currentBoard->m_pFive[i][j].uState=STATE_EMPTY;
			currentBoard->m_pFive[i][j].isFocus=false;
			currentBoard->m_pFive[i][j].isFlag=false;
		}
	}
	CurrentPoint=0;
	oldCurrentPoint=0;
	if(playerSide==-1){
		STEP temp;
		temp.step=1;
		temp.current=&currentBoard->m_pFive[7][7];
		currentStep=stepList.AddTail(temp);
		//���Ӳ���
		currentBoard->m_pFive[7][7].uState=-playerSide;
		currentBoard->m_pFive[7][7].isFlag=true;
	}
	uGameState=GAME_STATE_RUN;
	searchRect.left=6;
	searchRect.top=6;
	searchRect.right=8;
	searchRect.bottom=8;
}


void CChildView::OnStart()
{
	InitGame();	
	Invalidate();
}









bool CChildView::bVictory(void)
{
	int col=stepList.GetAt(currentStep).current->uCol;
	int row=stepList.GetAt(currentStep).current->uRow;
	int state=stepList.GetAt(currentStep).current->uState;
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
		if(isFiveContinue(direction[i])){
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
