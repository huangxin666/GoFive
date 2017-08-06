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
    , mindepth(0)
    , maxdepth(0)
    , useTransTable(FALSE)
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
    DDV_MinMaxUInt(pDX, uStep, 2, 14);
    DDX_Control(pDX, IDC_EDIT1, algorithm);
    DDX_Text(pDX, IDC_EDIT1, algType);
    DDV_MinMaxUInt(pDX, algType, 1, 2);
    DDX_Control(pDX, IDC_EDIT2, maxSearchTime);
    DDX_Text(pDX, IDC_EDIT2, maxTime);
    DDV_MinMaxUInt(pDX, maxTime, 1, 100000);
    DDX_Text(pDX, IDC_EDIT3, mindepth);
    DDV_MinMaxInt(pDX, mindepth, 1, 10);
    DDX_Text(pDX, IDC_EDIT4, maxdepth);
    DDV_MinMaxInt(pDX, maxdepth, 4, 20);
    DDX_Check(pDX, IDC_CHECK1, useTransTable);
}


BEGIN_MESSAGE_MAP(DlgSettings, CDialog)
END_MESSAGE_MAP()


// DlgSettings 消息处理程序
