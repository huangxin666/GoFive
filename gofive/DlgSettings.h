#pragma once
#include "afxwin.h"


// DlgSettings �Ի���

class DlgSettings : public CDialog
{
    DECLARE_DYNAMIC(DlgSettings)

public:
    DlgSettings(CWnd* pParent = NULL);   // ��׼���캯��
    virtual ~DlgSettings();

    // �Ի�������
    enum { IDD = IDD_SETTINGS };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

    DECLARE_MESSAGE_MAP()
public:
    CEdit edit;
    byte uStep;
    CEdit algorithm;
    byte algType;
};
