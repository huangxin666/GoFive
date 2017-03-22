#pragma once
#include "afxwin.h"


// DlgSettings 对话框

class DlgSettings : public CDialog
{
    DECLARE_DYNAMIC(DlgSettings)

public:
    DlgSettings(CWnd* pParent = NULL);   // 标准构造函数
    virtual ~DlgSettings();

    // 对话框数据
    enum { IDD = IDD_SETTINGS };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

    DECLARE_MESSAGE_MAP()
public:
    CEdit edit;
    byte uStep;
    CEdit algorithm;
    byte algType;
};
