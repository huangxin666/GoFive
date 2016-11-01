#pragma once
#include "afxwin.h"


// DlgSetCaculateStep 对话框

class DlgSetCaculateStep : public CDialog
{
	DECLARE_DYNAMIC(DlgSetCaculateStep)

public:
	DlgSetCaculateStep(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~DlgSetCaculateStep();

// 对话框数据
	enum { IDD = IDD_SETAISTEP };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CEdit edit;
	UINT uStep;
};
