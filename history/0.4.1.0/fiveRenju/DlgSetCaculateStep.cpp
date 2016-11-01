// DlgSetCaculateStep.cpp : 实现文件
//

#include "stdafx.h"
#include "fiveRenju.h"
#include "DlgSetCaculateStep.h"
#include "afxdialogex.h"


// DlgSetCaculateStep 对话框

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


// DlgSetCaculateStep 消息处理程序
