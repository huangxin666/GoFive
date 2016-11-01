
// ChildView.h : CChildView 类的接口
//


#pragma once
#include "stdafx.h"
#include "Piece.h"
#include "Game.h"
#include "defines.h"
// CChildView 窗口




class CChildView : public CWnd
{
// 构造
public:
	CChildView();
	virtual ~CChildView();

	// 生成的消息映射函数
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
private:
	Game *game;
	Piece *CurrentPoint;
	Piece *oldCurrentPoint;
public:
	void DrawBack(CDC *pDC);
	void DrawChessBoard(CDC *pDC);
	void DrawChess(CDC* pDC, ChessBoard * currentBoard);
	void DrawMouseFocus(CDC * pDC);
	void checkVictory(int state);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnStepback();
	afx_msg void OnStart();
	afx_msg void OnFirsthand();
	afx_msg void OnUpdateFirsthand(CCmdUI *pCmdUI);
	afx_msg void OnSecondhand();
	afx_msg void OnUpdateSecondhand(CCmdUI *pCmdUI);
	afx_msg void OnAIPrimary();
	afx_msg void OnUpdateAIPrimary(CCmdUI *pCmdUI);
	afx_msg void OnAISecondry();
	afx_msg void OnUpdateAISecondry(CCmdUI *pCmdUI);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnAIAdvanced();
	afx_msg void OnUpdateAIAdvanced(CCmdUI *pCmdUI);
	afx_msg void OnSave();
	afx_msg void OnLoad();
	afx_msg void OnHelpPrimary();
	afx_msg void OnHelpSecondry();
	afx_msg void OnHelpAdvanced();
	afx_msg void OnUpdateHelpPrimary(CCmdUI *pCmdUI);
	afx_msg void OnUpdateHelpSecondry(CCmdUI *pCmdUI);
	afx_msg void OnUpdateHelpAdvanced(CCmdUI *pCmdUI);
	afx_msg void OnAIhelp();
	afx_msg void OnDebug();
};

