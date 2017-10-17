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
    , uStep(0), maxmemsize(0)
    , mindepth(0)
    , maxdepth(0)
    , useTransTable(FALSE)
    , vct_expend(0)
    , vcf_expend(0)
    , useDBSearch(FALSE)
{

}

DlgSettings::~DlgSettings()
{
}

void DlgSettings::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT_STEP, uStep);
    DDV_MinMaxUInt(pDX, uStep, 2, 14);
    DDX_Text(pDX, IDC_MAXMEM, maxmemsize);
    DDV_MinMaxUInt(pDX, maxmemsize, 0, INT_MAX);
    DDX_Text(pDX, IDC_EDIT2, maxTime);
    DDV_MinMaxUInt(pDX, maxTime, 1, 100000);
    DDX_Text(pDX, IDC_EDIT3, mindepth);
    DDV_MinMaxInt(pDX, mindepth, 1, 10);
    DDX_Text(pDX, IDC_EDIT4, maxdepth);
    DDV_MinMaxInt(pDX, maxdepth, 4, 20);
    DDX_Check(pDX, IDC_CHECK_TRANSTABLE, useTransTable);
    DDX_Text(pDX, IDC_VCT_EXPEND, vct_expend);
    DDV_MinMaxInt(pDX, vct_expend, 0, 100);
    DDX_Text(pDX, IDC_VCF_EXPEND, vcf_expend);
    DDV_MinMaxInt(pDX, vcf_expend, 0, 100);
    DDX_Check(pDX, IDC_CHECK_FULLSEARCH, useDBSearch);
}


BEGIN_MESSAGE_MAP(DlgSettings, CDialog)
END_MESSAGE_MAP()


// DlgSettings 消息处理程序
