// DlgSetCaculateStep.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "fiveRenju.h"
#include "DlgSetCaculateStep.h"
#include "afxdialogex.h"


// DlgSetCaculateStep �Ի���

IMPLEMENT_DYNAMIC(DlgSetCaculateStep, CDialog)

DlgSetCaculateStep::DlgSetCaculateStep(CWnd* pParent /*=NULL*/)
	: CDialog(DlgSetCaculateStep::IDD, pParent)
	, uStep(0)
{

}

DlgSetCaculateStep::~DlgSetCaculateStep()
{
}

void DlgSetCaculateStep::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_STEP, edit);
	DDX_Text(pDX, IDC_EDIT_STEP, uStep);
	DDV_MinMaxUInt(pDX, uStep, 0, 10);
}


BEGIN_MESSAGE_MAP(DlgSetCaculateStep, CDialog)
END_MESSAGE_MAP()


// DlgSetCaculateStep ��Ϣ�������
