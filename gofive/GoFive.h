
// fiveRenju.h : fiveRenju Ӧ�ó������ͷ�ļ�
//
#pragma once

#ifndef __AFXWIN_H__
#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"       // ������


// CfiveRenjuApp:
// �йش����ʵ�֣������ fiveRenju.cpp
//

class CfiveRenjuApp : public CWinApp
{
public:
    CfiveRenjuApp();


    // ��д
public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();

    // ʵ��

public:
    afx_msg void OnAppAbout();
    DECLARE_MESSAGE_MAP()
};

extern CfiveRenjuApp theApp;
