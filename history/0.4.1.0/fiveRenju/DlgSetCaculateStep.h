#pragma once
#include "afxwin.h"


// DlgSetCaculateStep �Ի���

class DlgSetCaculateStep : public CDialog
{
	DECLARE_DYNAMIC(DlgSetCaculateStep)

public:
	DlgSetCaculateStep(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~DlgSetCaculateStep();

// �Ի�������
	enum { IDD = IDD_SETAISTEP };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	CEdit edit;
	UINT uStep;
};
