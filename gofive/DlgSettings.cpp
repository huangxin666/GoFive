// DlgSettings.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "GoFive.h"
#include "DlgSettings.h"
#include "afxdialogex.h"


// DlgSettings �Ի���

IMPLEMENT_DYNAMIC(DlgSettings, CDialog)

DlgSettings::DlgSettings(CWnd* pParent /*=NULL*/)
    : CDialog(DlgSettings::IDD, pParent)
    , uStep(0)
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
    DDV_MinMaxUInt(pDX, uStep, 1, 6);
}


BEGIN_MESSAGE_MAP(DlgSettings, CDialog)
END_MESSAGE_MAP()


// DlgSettings ��Ϣ�������
