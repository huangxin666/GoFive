
// fiveRenju.cpp : ����Ӧ�ó��������Ϊ��
//

#include "stdafx.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "GoFive.h"
#include "MainFrm.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CfiveRenjuApp

BEGIN_MESSAGE_MAP(CfiveRenjuApp, CWinApp)
    ON_COMMAND(ID_APP_ABOUT, &CfiveRenjuApp::OnAppAbout)
END_MESSAGE_MAP()


// CfiveRenjuApp ����

CfiveRenjuApp::CfiveRenjuApp()
{
    // ֧����������������
    m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
#ifdef _MANAGED
    // ���Ӧ�ó��������ù�����������ʱ֧��(/clr)�����ģ���:
    //     1) �����д˸������ã�������������������֧�ֲ�������������
    //     2) ��������Ŀ�У������밴������˳���� System.Windows.Forms ������á�
    System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

    // TODO: ������Ӧ�ó��� ID �ַ����滻ΪΨһ�� ID �ַ�����������ַ�����ʽ
    //Ϊ CompanyName.ProductName.SubProduct.VersionInformation
    SetAppID(_T("fiveRenju.AppID.NoVersion"));

    // TODO: �ڴ˴���ӹ�����룬
    // ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}

// Ψһ��һ�� CfiveRenjuApp ����

CfiveRenjuApp theApp;


// CfiveRenjuApp ��ʼ��

BOOL CfiveRenjuApp::InitInstance()
{
    // ���һ�������� Windows XP �ϵ�Ӧ�ó����嵥ָ��Ҫ
    // ʹ�� ComCtl32.dll �汾 6 ����߰汾�����ÿ��ӻ���ʽ��
    //����Ҫ InitCommonControlsEx()�����򣬽��޷��������ڡ�
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    // ��������Ϊ��������Ҫ��Ӧ�ó�����ʹ�õ�
    // �����ؼ��ࡣ
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();


    EnableTaskbarInteraction(FALSE);

    // ʹ�� RichEdit �ؼ���Ҫ  AfxInitRichEdit2()	
    // AfxInitRichEdit2();

    // ��׼��ʼ��
    // ���δʹ����Щ���ܲ�ϣ����С
    // ���տ�ִ���ļ��Ĵ�С����Ӧ�Ƴ�����
    // ����Ҫ���ض���ʼ������
    // �������ڴ洢���õ�ע�����
    // TODO: Ӧ�ʵ��޸ĸ��ַ�����
    // �����޸�Ϊ��˾����֯��
    SetRegistryKey(_T("Ӧ�ó��������ɵı���Ӧ�ó���"));


    // ��Ҫ���������ڣ��˴��뽫�����µĿ�ܴ���
    // ����Ȼ��������ΪӦ�ó���������ڶ���
    CMainFrame* pFrame = new CMainFrame;
    if (!pFrame)
        return FALSE;
    m_pMainWnd = pFrame;
    // ���������ؿ�ܼ�����Դ
    pFrame->LoadFrame(IDR_MAINFRAME,
        WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,
        NULL);





    // Ψһ��һ�������ѳ�ʼ���������ʾ����������и���
    pFrame->ShowWindow(SW_SHOW);
    pFrame->UpdateWindow();
    return TRUE;
}

int CfiveRenjuApp::ExitInstance()
{
    //TODO: �����������ӵĸ�����Դ
    return CWinApp::ExitInstance();
}

// CfiveRenjuApp ��Ϣ�������


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();

    // �Ի�������
    enum { IDD = IDD_ABOUTBOX };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// �������жԻ����Ӧ�ó�������
void CfiveRenjuApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}

// CfiveRenjuApp ��Ϣ�������



