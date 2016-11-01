
// ChildView.h : CChildView ��Ľӿ�
//


#pragma once
#include "ChessBoard.h"
#include "Piece.h"
// CChildView ����




class CChildView : public CWnd
{
// ����
public:
	CChildView();

// ����
public:

// ����
public:

// ��д
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// ʵ��
public:
	virtual ~CChildView();

	// ���ɵ���Ϣӳ�亯��
protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
public:
	int playerSide; //������ӵ���ɫ��1�����֣�
	//CList<STEP,STEP&> stepList;
	//POSITION currentStep;
	ChessBoard * currentBoard;
	Piece * CurrentPoint;
	Piece * oldCurrentPoint;
	void DrawBack(CDC *pDC);
	void DrawChessBoard(CDC *pDC);
	void DrawChess(CDC* pDC);
	void DrawMouseFocus(CDC * pDC);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);

	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	void stepBack(void);
	afx_msg void OnStepback();

	void InitGame(void);
	afx_msg void OnStart();
	void AIWork(void);
	
	AISTEP getBestStepAI1(int stepCount,ChessBoard currentBoard,int state);
	AISTEP getBestStepAI2(int stepCount,ChessBoard currentBoard,int state);
	UINT uGameState;
	bool isVictory(void);
	RECT searchRect;
	void ChangeSide(void);
	int AIlevel;	
	afx_msg void OnFirsthand();
	afx_msg void OnUpdateFirsthand(CCmdUI *pCmdUI);
	afx_msg void OnSecondhand();
	afx_msg void OnUpdateSecondhand(CCmdUI *pCmdUI);
	afx_msg void OnAiPrimary();
	afx_msg void OnUpdateAiPrimary(CCmdUI *pCmdUI);
	afx_msg void OnAiSecondry();
	afx_msg void OnUpdateAiSecondry(CCmdUI *pCmdUI);
};

