
// ChildView.h : CChildView 类的接口
//


#pragma once
#include "FiveBoard.h"

// CChildView 窗口


typedef struct 
{
	int step;          //步数
	FIVEWND * current;	//当前step	
} STEP;	// 五子棋结构体

typedef struct 
{
	int score;          //步数
	UINT x;
	UINT y;				//当前step	
} AISTEP;	// 五子棋结构体

class CChildView : public CWnd
{
// 构造
public:
	CChildView();

// 特性
public:

// 操作
public:

// 重写
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// 实现
public:
	virtual ~CChildView();

	// 生成的消息映射函数
protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
public:
	int playerSide; //玩家棋子的颜色（1黑先手）
	CList<STEP,STEP&> stepList;
	POSITION currentStep;
	FiveBoard * currentBoard;
	FIVEWND * CurrentPoint;
	FIVEWND * oldCurrentPoint;
	void DrawBack(CDC *pDC);
	void DrawChessBoard(CDC *pDC);
	void DrawChess(CDC* pDC);
	void DrawMouseFocus(CDC * pDC);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);

	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	void stepBack(void);
	afx_msg void OnStepback();
	int getStepScores(FIVEWND thisStep,FiveBoard* currentBoard);
	bool isFiveContinue(int derection[]);		// 00000
	bool isFourContinue(int derection[]);		// 0000 
	bool isFourContinueSide(int derection[]);	// 0000X||X0000
	bool isFourContinueVar1(int derection[]);	// 000 0||0 000
	bool isFourContinueVar2(int derection[]);	// 00 00
	bool isThreeContinue(int derection[]);		// 000
	bool isThreeContinueSide(int derection[]);	// 000X||X000
	bool isThreeContinueVar(int derection[]);	// 00 0||0 00
	bool isThreeContinueVarSide(int derection[]);// 0 00X || X0 00 || 00 0X || X00 0
	bool isTwoContinue(int derection[]);			// 00
	bool isTwoContinueVar(int derection[]);		// 0 0
	void InitGame(void);
	afx_msg void OnStart();
	void AI(void);
	bool isOneToOne(int direction[]);
	AISTEP getBestStep(int stepCount,FiveBoard currentBoard,int state);
	UINT uGameState;
	bool bVictory(void);
	RECT searchRect;
};

