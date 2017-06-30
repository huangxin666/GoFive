// DlgSettings.cpp : 实现文件
//

#include "stdafx.h"
#include "GoFive.h"
#include "DlgSettings.h"
#include "afxdialogex.h"


// DlgSettings 对话框

IMPLEMENT_DYNAMIC(DlgSettings, CDialog)

DlgSettings::DlgSettings(CWnd* pParent /*=NULL*/)
    : CDialog(DlgSettings::IDD, pParent)
    , uStep(0), algType(2)
{

}

DlgSettings::~DlgSettings()
{
}

void DlgSettings::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_STEP, edit);
    DDX_Text(pDX, IDC_EDIT_STEP, uStep);
    DDV_MinMaxUInt(pDX, uStep, 1, 8);
    DDX_Control(pDX, IDC_EDIT1, algorithm);
    DDX_Text(pDX, IDC_EDIT1, algType);
    DDV_MinMaxUInt(pDX, algType, 1, 2);
    DDX_Control(pDX, IDC_EDIT2, maxSearchTime);
    DDX_Text(pDX, IDC_EDIT2, maxTime);
    DDV_MinMaxUInt(pDX, maxTime, 1, 100000);
}


BEGIN_MESSAGE_MAP(DlgSettings, CDialog)
END_MESSAGE_MAP()


// DlgSettings 消息处理程序
