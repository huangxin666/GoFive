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
    byte uStep;   
    int maxmemsize;
    int maxTime;
    int mindepth;
    int maxdepth;
    BOOL useTransTable;
    int vct_expend;
    int vcf_expend;
    BOOL useDBSearch;
};
