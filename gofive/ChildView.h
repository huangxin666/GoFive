
// ChildView.h : CChildView 类的接口
//


#pragma once
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
	CursorPosition currentPos;
	CursorPosition oldPos;
public:
	CProgressCtrl myProgress;
	CStatic myProgressStatic;
	CStatic infoStatic;
    CStatic debugStatic;
	CFont font;
    int debugCount;
    void init();
    void DrawBack(CDC *pDC);
    void DrawChessBoard(CDC *pDC);
    void DrawChess(CDC* pDC, const vector<STEP> &stepList);
    void DrawProgress(CDC * pDC);
    void DrawMouseFocus(CDC * pDC);
	void updateInfoStatic();
	void startProgress();
	void endProgress();
	void checkVictory(int state);
    void AIWork();
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
	afx_msg void OnPlayertoplayer();
	afx_msg void OnUpdatePlayertoplayer(CCmdUI *pCmdUI);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnSettings();
	afx_msg void OnMultithread();
	afx_msg void OnUpdateMultithread(CCmdUI *pCmdUI);
	afx_msg void OnBan();
	afx_msg void OnUpdateBan(CCmdUI *pCmdUI);
	afx_msg void OnShowStep();
	afx_msg void OnUpdateShowStep(CCmdUI *pCmdUI);
};

