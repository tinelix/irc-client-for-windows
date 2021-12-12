// MainWindow.cpp : implementation file
//

#include "stdafx.h"
#include "TinelixIRC.h"
#include "MainWindow.h"
#include "ConnectionManagerDialog.h"
#include "SettingsDialog.h"
#include "AboutDialog.h"

#include <stdio.h>
#include <iostream.h>
#include <winsock.h>
#include <process.h>
#include <direct.h>
#include <locale.h>
#include <afxtempl.h>
#include <conio.h>

#pragma comment(lib, "wsock32.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

SOCKET sock;
HINSTANCE parserLib;

typedef void (WINAPI *cfunc) ();
typedef void (WINAPI *ParseMessage)(char*, char*[], char*, BOOL);


/////////////////////////////////////////////////////////////////////////////
// MainWindow dialog


MainWindow::MainWindow(CWnd* pParent /*=NULL*/)
	: CDialog(MainWindow::IDD, pParent)
{
	//{{AFX_DATA_INIT(MainWindow)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	irc_chat_page=new IRCChatPage;
}


void MainWindow::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(MainWindow)
	DDX_Control(pDX, IDC_IRC_CHAT_TABS, m_irc_tabs);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(MainWindow, CDialog)
	//{{AFX_MSG_MAP(MainWindow)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_COMMAND(ID_FILE_CONNECT, OnFileConnect)
	ON_COMMAND(ID_VIEW_SETTINGS, OnViewSettings)
	ON_COMMAND(ID_HELP_ABOUT, OnHelpAbout)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// MainWindow message handlers

void MainWindow::delsymbs(char *str, int begin, int lng)
{
	for(begin; begin < strlen(str); begin++) {
		*(str + begin) = 0;	
	};
};

BOOL MainWindow::OnInitDialog() 
{
	CDialog::OnInitDialog();

	char exe_path[MAX_PATH] = {0};
	char dll_path[MAX_PATH] = {0};
	char exe_name[MAX_PATH] = "TLX_IRC.EXE"; // EXE filename
	TCHAR language_string[MAX_PATH] = {0};
	
	GetModuleFileName(NULL, exe_path, MAX_PATH);
	GetModuleFileName(NULL, dll_path, MAX_PATH);

	MainWindow::delsymbs(exe_path, strlen(exe_path) - strlen(exe_name) - 1, strlen(exe_path) - strlen(exe_name) - 1); // deleting EXE filename
	MainWindow::delsymbs(dll_path, strlen(dll_path) - strlen(exe_name) - 1, strlen(dll_path) - strlen(exe_name) - 1); // deleting EXE filename

	strcat(exe_path, "\\settings.ini");	// add settings filename

	strcat(dll_path, "\\parser.dll");

	GetPrivateProfileString("Main", "Language", "", language_string, MAX_PATH, exe_path);

	CString lng_selitemtext_2(language_string);
	
	TC_ITEM tci;
	tci.mask = TCIF_TEXT;
	tci.iImage = -1;
	if(lng_selitemtext_2 == "Russian") {
		tci.pszText = "�����";
	} else {
		tci.pszText = "Thread";
	};
	m_irc_tabs.InsertItem(0, &tci);
	CRect rcClient, rcWindow;

	m_irc_tabs.GetClientRect(&rcClient);
	m_irc_tabs.AdjustRect(FALSE, &rcClient);
	m_irc_tabs.GetWindowRect(&rcWindow);
	ScreenToClient(rcWindow);

	rcClient.OffsetRect(rcWindow.left, rcWindow.top);

	irc_chat_page->Create(IRCChatPage::IDD, &m_irc_tabs);
	hwnd_chat = irc_chat_page->m_hWnd;
	irc_chat_page->MoveWindow(&rcClient, TRUE);
	int page_frame_w = rcWindow.Width() - 4;
	int page_frame_h = rcWindow.Height() - 23;
	irc_chat_page->SetWindowPos(NULL, 2, 21, page_frame_w, page_frame_h, SWP_NOZORDER);
	irc_chat_page->GetDlgItem(IDC_SOCKMSGS)->MoveWindow(2, 2, page_frame_w - 4, page_frame_h - 28);
	irc_chat_page->GetDlgItem(IDC_MSGTEXT)->MoveWindow(2, page_frame_h - 24, page_frame_w - 72, 22);
	irc_chat_page->GetDlgItem(IDC_SENDMSG)->MoveWindow(page_frame_w - 68, page_frame_h - 24, 66, 22);

	TRACE("Tinelix IRC Client ver. 0.1.3 (2021-12-02)\r\nCopyright � 2021 Dmitry Tretyakov (aka. Tinelix)\r\n"
	"https:/github.com/tinelix/irc-client-for-win32s\r\n\r\n");

	char font_string[48] = {0};
	int font_size = 9;
	HDC hDC = ::GetDC(NULL);
	GetPrivateProfileString("Main", "MsgFont", "Fixedsys", font_string, 48, exe_path);
	font_size = GetPrivateProfileInt("Main", "MsgFontSize", 8, exe_path);
	int font_height = 0;
	font_height = -MulDiv(font_size, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	font.CreateFont(font_height, 0, 0, 0, FW_REGULAR, FALSE, FALSE, 0, DEFAULT_CHARSET, 0, 0,
	0, 0, font_string);
	mainfont.CreateFont(8, 0, 0, 0, FW_REGULAR, FALSE, FALSE, 0, DEFAULT_CHARSET, 0, 0,
	0, 0, "MS Sans Serif");
	irc_chat_page->GetDlgItem(IDC_SOCKMSGS)->SetFont(&font);
	irc_chat_page->GetDlgItem(IDC_MSGTEXT)->SetFont(&mainfont);
	irc_chat_page->GetDlgItem(IDC_SENDMSG)->SetFont(&mainfont);
	IsConnected = FALSE;
	SetWindowText("Tinelix IRC Client");

	

	if(!(GetVersion() & 0x80000000 && (GetVersion() & 0xFF) ==3)) {
		parserLib = (HINSTANCE)malloc(sizeof(HINSTANCE));
		parserLib = LoadLibrary(dll_path);
		if(parserLib) {
			TRACE("PARSER.DLL loaded.\r\n");
		};
	};

	CMenu* mainmenu;

	WSADATA WSAData;
	char szTemp[80];

	int status = (WSAStartup(MAKEWORD(1,1), &WSAData));

	//try {
		if (lng_selitemtext_2 == "Russian") {
			irc_chat_page->GetDlgItem(IDC_SENDMSG)->SetWindowText("���������");
			if (status == 0) {
				TRACE("Winsock initialed.\r\n");
    		}
    		else {
				MessageBox("WinSock �� ����� ���� ���������������.", "������", MB_OK|MB_ICONSTOP);
    		};
			setlocale(LC_ALL, "Russian");
			mainmenu = (CMenu*)malloc(sizeof(CMenu));
			mainmenu->m_hMenu = NULL;
			mainmenu->CreateMenu();
			CMenu* file_submenu = (CMenu*)malloc(sizeof(CMenu));
			file_submenu->m_hMenu = NULL;
			file_submenu->CreatePopupMenu();
			file_submenu->AppendMenu(MF_STRING, ID_FILE_CONNECT, "������������...");
			file_submenu->AppendMenu(MF_STRING, ID_FILE_QUIT, "�����");
			CMenu* view_submenu = (CMenu*)malloc(sizeof(CMenu));
			view_submenu->m_hMenu = NULL;
			view_submenu->CreatePopupMenu();
			view_submenu->AppendMenu(MF_STRING, ID_VIEW_SETTINGS, "���������");
			CMenu* help_submenu = (CMenu*)malloc(sizeof(CMenu));
			help_submenu->m_hMenu = NULL;
			help_submenu->CreatePopupMenu();
			help_submenu->AppendMenu(MF_STRING, ID_HELP_ABOUT, "� ���������...");
			mainmenu->AppendMenu(MF_STRING | MF_POPUP, (UINT)file_submenu->m_hMenu, "����");
            mainmenu->AppendMenu(MF_STRING | MF_POPUP, (UINT)view_submenu->m_hMenu, "���");
			mainmenu->AppendMenu(MF_STRING | MF_POPUP, (UINT)help_submenu->m_hMenu, "�������");
			SetMenu(mainmenu);
			free(mainmenu);
			free(file_submenu);
			free(view_submenu);
			free(help_submenu);
			GetDlgItem(IDC_STATUSBAR_TEXT)->SetWindowText("������");
		} if (lng_selitemtext_2 == "English") {
			irc_chat_page->GetDlgItem(IDC_SENDMSG)->SetWindowText("Send");
			if (status == 0) {
				TRACE("WinSock initialed.\r\n");
    		}
    		else {
				MessageBox("WinSock cannot be initialed.", "Error", MB_OK|MB_ICONSTOP);
    		};
			if (GetVersion() & 0x80000000 && (GetVersion() & 0xFF) ==3) {
				MessageBox("String parsing through the \"PARSER.DLL\" library works only in Windows NT or 9x. Running "
                 "this library in Win32s may cause the program to crash.", "Error", MB_OK);
				FreeLibrary(parserLib);
			};
			setlocale(LC_ALL, "English");
			mainmenu = (CMenu*)malloc(sizeof(CMenu));
			mainmenu->m_hMenu = NULL;
			mainmenu->LoadMenu(IDR_MAINMENU);
			SetMenu(mainmenu);
			free(mainmenu);
			GetDlgItem(IDC_STATUSBAR_TEXT)->SetWindowText("Ready");
		} if (lng_selitemtext_2 == "") {
			irc_chat_page->GetDlgItem(IDC_SENDMSG)->SetWindowText("Send");
			if (status == 0) {
				TRACE("Winsock initialed.\r\n");
    		}
    		else {
				MessageBox("WinSock cannot be initialed.", "Error", MB_OK|MB_ICONSTOP);
    		};
			if (GetVersion() & 0x80000000 && (GetVersion() & 0xFF) ==3) {
				MessageBox("String parsing through the \"PARSER.DLL\" library works only in Windows NT or 9x. Running "
                 "this library in Win32s may cause the program to crash.", "Error", MB_OK);
				FreeLibrary(parserLib);
			};
			setlocale(LC_ALL, "English");
			mainmenu = (CMenu*)malloc(sizeof(CMenu));
			mainmenu->m_hMenu = NULL;
			mainmenu->LoadMenu(IDR_MAINMENU);
			SetMenu(mainmenu);
			free(mainmenu);
			WritePrivateProfileString("Main", "Language", "English", exe_path);
			WritePrivateProfileString("Main", "MsgHistory", "Enabled", exe_path);
			GetDlgItem(IDC_STATUSBAR_TEXT)->SetWindowText("Ready");
		};

	irc_chat_page->ShowWindow(SW_SHOW);
	
	CenterWindow();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void MainWindow::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	CRect rcClient, rcWindow;

	if(m_irc_tabs.m_hWnd == NULL || irc_chat_page->m_hWnd == NULL) {
		return;
	};
	GetClientRect(&rcClient);
	m_irc_tabs.SetWindowPos(NULL, 4, 4, cx - 8, cy - 30, SWP_NOZORDER);
	m_irc_tabs.AdjustRect(FALSE, &rcClient);

	m_irc_tabs.GetClientRect(&rcClient);
	m_irc_tabs.AdjustRect(FALSE, &rcClient);
	m_irc_tabs.GetWindowRect(&rcWindow);
	ScreenToClient(rcWindow);

	rcClient.OffsetRect(rcWindow.left, rcWindow.top);
	irc_chat_page->MoveWindow(&rcClient, TRUE);
	int page_frame_w = rcWindow.Width() - 4;
	int page_frame_h = rcWindow.Height() - 23;
	irc_chat_page->SetWindowPos(NULL, 2, 21, page_frame_w, page_frame_h, SWP_NOZORDER);
	irc_chat_page->GetDlgItem(IDC_SOCKMSGS)->MoveWindow(2, 2, page_frame_w - 4, page_frame_h - 28);
	irc_chat_page->GetDlgItem(IDC_MSGTEXT)->MoveWindow(2, page_frame_h - 24, page_frame_w - 72, 22);
	irc_chat_page->GetDlgItem(IDC_SENDMSG)->MoveWindow(page_frame_w - 68, page_frame_h - 24, 66, 22);
	AfxGetMainWnd()->GetDlgItem(IDC_STATUSBAR_TEXT)->MoveWindow(4, cy - 20, cx - 148, 22);
	AfxGetMainWnd()->GetDlgItem(IDC_CONNECTION_PROGRESS)->MoveWindow(cx - 144, cy - 22, 72, 18);
	AfxGetMainWnd()->GetDlgItem(IDC_CONNETCTION_UPTIME)->MoveWindow(cx - 72, cy - 20, 68, 22);

}

void MainWindow::OnDestroy() 
{
	CDialog::OnDestroy();
	
}


void MainWindow::OnFileConnect() 
{
	ConnectionManagerDialog connman;
	connman.SetConnectionState(IsConnected);
	connman.DoModal();
}

void MainWindow::ConnectionFunc(HWND hwnd, PARAMETERS params) 
{
	char exe_path[MAX_PATH] = {0};
	char h_pch[MAX_PATH] = {0};
	char h_pch2[MAX_PATH] = {0};
	char h_fch[MAX_PATH] = {0};
	char exe_name[MAX_PATH] = "TLX_IRC.EXE"; // EXE filename
	GetModuleFileName(NULL, exe_path, MAX_PATH);
	MainWindow* mainwin = (MainWindow*)AfxGetMainWnd();
	mainwin->delsymbs(exe_path, strlen(exe_path) - strlen(exe_name), strlen(exe_path) - strlen(exe_name)); // deleting EXE filename
	strcat(exe_path, "settings.ini");
	char msg_history_string[80];
	char language_string[80];
	GetPrivateProfileString("Main", "MsgHistory", "Enabled", msg_history_string, 80, exe_path);
	GetPrivateProfileString("Main", "Language", "", language_string, 80, exe_path);
	CString msg_history_string2(msg_history_string);
	CString language_string2(language_string);
	char history_path[MAX_PATH] = {0};
	char server[256];
	sprintf(server, "%s", params.server);
	char realname[80];
	sprintf(realname, "%s", params.realname);
	char connecting_msgb[100];
	int port = params.port;
	GetModuleFileName(NULL, history_path, MAX_PATH);
	mainwin->delsymbs(history_path, strlen(history_path) - strlen(exe_name) - 1, strlen(history_path) - strlen(exe_name) - 1); // deleting EXE filename
	strcat(history_path, "\\history");	// add history filename

	if (server == NULL || server == "" || server == " ")
	{
	  	if(language_string2 == "Russian") {
			MessageBox("�� ������ ����� �������.", "������", MB_OK|MB_ICONSTOP);
			char statusbar_text[512];
			sprintf(statusbar_text, "������ �����������: �� ������ ����� �������");
			AfxGetApp()->GetMainWnd()->GetDlgItem(IDC_STATUSBAR_TEXT)->SetWindowText(statusbar_text);
		} else {
			MessageBox("Not specified server address.", "Error", MB_OK|MB_ICONSTOP);
			char statusbar_text[512];
			sprintf(statusbar_text, "Connection error: Not specified server address");
			AfxGetApp()->GetMainWnd()->GetDlgItem(IDC_STATUSBAR_TEXT)->SetWindowText(statusbar_text);
		};
		exit;
	};
	if (port == NULL || port == 0)
	{
		if(language_string2 == "Russian") {
			MessageBox("�� ������ ���� �������.", "������", MB_OK|MB_ICONSTOP);
			char statusbar_text[512];
			sprintf(statusbar_text, "������ �����������: �� ������ ���� �������");
			AfxGetApp()->GetMainWnd()->GetDlgItem(IDC_STATUSBAR_TEXT)->SetWindowText(statusbar_text);
		} else {
			MessageBox("Not specified server port.", "Error", MB_OK|MB_ICONSTOP);
			char statusbar_text[512];
			sprintf(statusbar_text, "Connection error: Not specified server port");
			AfxGetApp()->GetMainWnd()->GetDlgItem(IDC_STATUSBAR_TEXT)->SetWindowText(statusbar_text);
		};
		exit;
	};

	char* connecting_msg;
	connecting_msg = "Trying connecting to %s:%d...\n";
	TRACE(connecting_msg, &server, port);
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN client_param;
	hostent* host;
	if (sock == SOCKET_ERROR) {
		if(language_string2 == "Russian") {
			char* error_msg;
			sprintf(error_msg, "�� ������� ��������� WinSock � ����� ������ %d", WSAGetLastError());
			MessageBox(error_msg, "������", MB_OK|MB_ICONSTOP);
			char statusbar_text[512];
			sprintf(statusbar_text, "������ �����������: �� ������� ��������� WinSock � ����� ������ %d", WSAGetLastError());
			AfxGetApp()->GetMainWnd()->GetDlgItem(IDC_STATUSBAR_TEXT)->SetWindowText(statusbar_text);
		} else {
			char* error_msg;
			sprintf(error_msg, "Could not start WinSock with error code %d", WSAGetLastError());
			MessageBox(error_msg, "Error", MB_OK|MB_ICONSTOP);
			char statusbar_text[512];
			sprintf(statusbar_text, "Connection error: Could not start WinSock with error code %d", WSAGetLastError());
			AfxGetApp()->GetMainWnd()->GetDlgItem(IDC_STATUSBAR_TEXT)->SetWindowText(statusbar_text);
		};
		exit;
	};

	client_param.sin_family = AF_INET;
	connecting_msg = "Getting IP address...\n";
	TRACE0(connecting_msg);
	char statusbar_text[512];
	if(language_string2 == "Russian") {
		sprintf(statusbar_text, "��������� IP-������ ��� %s...", server);
	} else {
		sprintf(statusbar_text, "Getting IP address for %s...", server);
	};
	AfxGetApp()->GetMainWnd()->GetDlgItem(IDC_STATUSBAR_TEXT)->SetWindowText(statusbar_text);
	host = gethostbyname(server);
	if (host) {
		client_param.sin_addr.s_addr = inet_addr((char*)inet_ntoa(**(in_addr**)host->h_addr_list));
	}
	else
	{
		if(language_string2 == "Russian") {
			MessageBox("�� ������� �������� IP-����� ��� ����� ����� �����.", "������", MB_OK|MB_ICONSTOP);
			char statusbar_text[512];
			sprintf(statusbar_text, "������ �����������: �� ������� �������� IP-����� ��� ����� ����� �����");
			AfxGetApp()->GetMainWnd()->GetDlgItem(IDC_STATUSBAR_TEXT)->SetWindowText(statusbar_text);
		} else {
			MessageBox("Failed to get the IP address for this hostname.", "Error", MB_OK|MB_ICONSTOP);
			char statusbar_text[512];
			sprintf(statusbar_text, "Connection error: Failed to get the IP address for this hostname");
			AfxGetApp()->GetMainWnd()->GetDlgItem(IDC_STATUSBAR_TEXT)->SetWindowText(statusbar_text);
		};
		exit;
	};
	client_param.sin_port = htons(port);
	connecting_msg = "Connecting...\n";
	if(language_string2 == "Russian") {
		sprintf(statusbar_text, "����������� � %s:%d...", server, port);
	} else {
		sprintf(statusbar_text, "Connecting to %s:%d...", server, port);
	};
	AfxGetApp()->GetMainWnd()->GetDlgItem(IDC_STATUSBAR_TEXT)->SetWindowText(statusbar_text);
	TRACE(connecting_msg);
	int status;
	status = connect(sock, (SOCKADDR*)&client_param, sizeof(client_param));
	if (status == SOCKET_ERROR || status == INVALID_SOCKET) {
		if(language_string2 == "Russian") {
			char* error_msg;
			sprintf(error_msg, "�� ������� ��������� WinSock � ����� ������ %d", WSAGetLastError());
			MessageBox(error_msg, "������", MB_OK|MB_ICONSTOP);
			char statusbar_text[512];
			sprintf(statusbar_text, "������ �����������: �� ������� ��������� WinSock � ����� ������ %d", WSAGetLastError());
			AfxGetApp()->GetMainWnd()->GetDlgItem(IDC_STATUSBAR_TEXT)->SetWindowText(statusbar_text);
		} else {
			char* error_msg;
			sprintf(error_msg, "Could not start WinSock with error code %d", WSAGetLastError());
			MessageBox(error_msg, "Error", MB_OK|MB_ICONSTOP);
			char statusbar_text[512];
			sprintf(statusbar_text, "Connection error: Could not start WinSock with error code %d", WSAGetLastError());
			AfxGetApp()->GetMainWnd()->GetDlgItem(IDC_STATUSBAR_TEXT)->SetWindowText(statusbar_text);
		};
		exit;
	};
	try {
		mainwin->IsConnected = TRUE;
	} catch(...) {

	};
	char sock_buffer[32768];
	char listing[32768];
	char listing_file[32768];
	char previous_listing[32768];
	char previous_listing_file[32768];

	char window_title[384];
	sprintf(window_title, "Tinelix IRC Client | %s", params.server);
	if(msg_history_string2 == "Enabled") {
		GetModuleFileName(NULL, h_pch, MAX_PATH);
		mainwin->delsymbs(h_pch, strlen(h_pch) - strlen(exe_name) - 1, strlen(h_pch) - strlen(exe_name) - 1); // deleting EXE filename
		DWORD winver = GetVersion();
		//if ((GetVersion() & 0x80000000l) == 0) {
		strcat(h_pch, "\\HISTORY");
		//sprintf(mainwin->history_path, "\0C:\\HISTORY");
		GetModuleFileName(NULL, h_pch2, MAX_PATH);
		mainwin->delsymbs(h_pch2, strlen(h_pch2) - strlen(exe_name), strlen(h_pch2) - strlen(exe_name) - 1); // deleting EXE filename
		strcat(h_pch2, "HISTORY");
		char current_time_ch[9];
		time_t current_time = time(NULL);
		strftime(current_time_ch, 9, "%y_%m_%d", localtime(&current_time));
		strcat(h_pch2, "\\");
		strcat(h_pch2, current_time_ch);	// add history filename
		sprintf(mainwin->history_path, "%s", h_pch);
		sprintf(mainwin->history_path2, "%s", h_pch2);
		char info_msg[120];
		int create_dir_status = CreateDirectory(h_pch, NULL);

		if(create_dir_status != 0 || GetLastError() == ERROR_ALREADY_EXISTS) {
			int create_dir_status_2 = CreateDirectory(h_pch2, NULL);
			if(create_dir_status_2 != 0 || GetLastError() == ERROR_ALREADY_EXISTS) {
				strcat(h_fch, h_pch2);
				strftime(current_time_ch, 9, "%H_%M_%S", localtime(&current_time));
				strcat(h_fch, "\\");
				strcat(h_fch, current_time_ch);
				strcat(h_fch, ".txt");
				sprintf(mainwin->history_file, "%s", h_fch);
			} if(GetLastError() == ERROR_PATH_NOT_FOUND || GetLastError() == ERROR_ACCESS_DENIED
				 || GetLastError() == ERROR_WRITE_PROTECT) {
				if(language_string2 == "Russian") {
                    char err_msg[8192];
                    char err_description[320];
                    if(GetLastError() == ERROR_PATH_NOT_FOUND) {
                        sprintf(err_description, "���� �� ������");
                    } if (GetLastError() == ERROR_ACCESS_DENIED) {
                        sprintf(err_description, "������ ��������");
                    } if (GetLastError() == ERROR_WRITE_PROTECT) {
                        sprintf(err_description, "������ �������� �� ������");
                    };
                    sprintf(err_msg,"������ �������� �����: %s\r\n��� ������: %d (%s)", mainwin->history_path2, GetLastError(),
                            err_description);
                    MessageBox(err_msg, "������", MB_OK|MB_ICONSTOP);
                } else {
                    char err_msg[4096];
                    char err_description[320];
                    if(GetLastError() == ERROR_PATH_NOT_FOUND) {
                        sprintf(err_description, "Path not found");
                    } if (GetLastError() == ERROR_ACCESS_DENIED) {
                        sprintf(err_description, "Access denied");
                    } if (GetLastError() == ERROR_WRITE_PROTECT) {
                        sprintf(err_description, "Data is write-protected");
                    };
                    sprintf(err_msg,"Error creating folder: %s\r\nError code: %d (%s)", mainwin->history_path2, GetLastError(),
                            err_description);
                    MessageBox(err_msg, "Error", MB_OK|MB_ICONSTOP);
                };
			};

		} if(GetLastError() == ERROR_PATH_NOT_FOUND || GetLastError() == ERROR_ACCESS_DENIED
			 || GetLastError() == ERROR_WRITE_PROTECT) {
			if(language_string2 == "Russian") {
                    char err_msg[8192];
                    char err_description[320];
                    if(GetLastError() == ERROR_PATH_NOT_FOUND) {
                        sprintf(err_description, "���� �� ������");
                    } if (GetLastError() == ERROR_ACCESS_DENIED) {
                        sprintf(err_description, "������ ��������");
                    } if (GetLastError() == ERROR_WRITE_PROTECT) {
                        sprintf(err_description, "������ �������� �� ������");
                    };
                    sprintf(err_msg,"������ �������� �����: %s\r\n��� ������: %d (%s)", mainwin->history_path, GetLastError(),
                            err_description);
                    MessageBox(err_msg, "������", MB_OK|MB_ICONSTOP);
            } else {
                    char err_msg[4096];
                    char err_description[320];
                    if(GetLastError() == ERROR_PATH_NOT_FOUND) {
                        sprintf(err_description, "Path not found");
                    } if (GetLastError() == ERROR_ACCESS_DENIED) {
                        sprintf(err_description, "Access denied");
                    } if (GetLastError() == ERROR_WRITE_PROTECT) {
                        sprintf(err_description, "Data is write-protected");
                    };
                    sprintf(err_msg,"Error creating folder: %s\r\nError code: %d (%s)", mainwin->history_path, GetLastError(),
                            err_description);
                    MessageBox(err_msg, "Error", MB_OK|MB_ICONSTOP);
                };
		};
	};

	AfxGetApp()->GetMainWnd()->SetWindowText(window_title);

	int WSAAsync = WSAAsyncSelect(sock, AfxGetApp()->GetMainWnd()->GetSafeHwnd(), WM_SOCKET_MESSAGE, FD_READ | FD_CLOSE);
	if (WSAAsync > 0) {
		if(language_string2 == "Russian") {
			char* error_msg;
			sprintf(error_msg, "�� ������� ��������� WinSock � ����� ������ %d", WSAGetLastError());
			MessageBox(error_msg, "������", MB_OK|MB_ICONSTOP);
			char statusbar_text[512];
			sprintf(statusbar_text, "������ �����������: �� ������� ��������� WinSock � ����� ������ %d", WSAGetLastError());
			AfxGetApp()->GetMainWnd()->GetDlgItem(IDC_STATUSBAR_TEXT)->SetWindowText(statusbar_text);
		} else {
			char* error_msg;
			sprintf(error_msg, "Could not start WinSock with error code %d", WSAGetLastError());
			MessageBox(error_msg, "Error", MB_OK|MB_ICONSTOP);
			char statusbar_text[512];
			sprintf(statusbar_text, "Connection error: Could not start WinSock with error code %d", WSAGetLastError());
			AfxGetApp()->GetMainWnd()->GetDlgItem(IDC_STATUSBAR_TEXT)->SetWindowText(statusbar_text);
		};
	};
	char ident_sending[400];
	int ident_sending_parts;
	ident_sending_parts = sprintf(ident_sending, "USER %s ", params.nickname);
	ident_sending_parts += sprintf(ident_sending + ident_sending_parts, "%s ", params.nickname);
	ident_sending_parts += sprintf(ident_sending + ident_sending_parts, "%s :", params.nickname);
	ident_sending_parts += sprintf(ident_sending + ident_sending_parts, "%s\r\n", params.realname);
	status = send(sock, ident_sending, strlen(ident_sending), 0);
	char nick_sending[400];
	int nick_sending_parts;
	nick_sending_parts = sprintf(nick_sending, "NICK %s\n", params.nickname);
	mainwin->irc_chat_page->GetDlgItem(IDC_MSGTEXT)->EnableWindow(TRUE);
	status = send(sock, nick_sending, strlen(nick_sending), 0);
	if(language_string2 == "Russian") {
		sprintf(statusbar_text, "������");
	} else {
		sprintf(statusbar_text, "Ready");
	};
	AfxGetApp()->GetMainWnd()->GetDlgItem(IDC_STATUSBAR_TEXT)->SetWindowText(statusbar_text);

}

void MainWindow::CreateConnectionThread(PARAMETERS params) 
{
	HWND hwnd;
	hwnd = m_hWnd;
	char change_font[100];
	MainWindow::ConnectionFunc(hwnd, params);

}

LRESULT MainWindow::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	char exe_path[MAX_PATH] = {0};
	char exe_name[MAX_PATH] = "TLX_IRC.EXE"; // EXE filename

	GetModuleFileName(NULL, exe_path, MAX_PATH);  

	MainWindow::delsymbs(exe_path, strlen(exe_path) - strlen(exe_name) - 1, strlen(exe_path) - strlen(exe_name) - 1); // deleting EXE filename

	strcat(exe_path, "\\settings.ini");	// add settings filename
	
	TCHAR language_string[MAX_PATH] = {0};
	TCHAR msg_history_string[MAX_PATH] = {0};
	//try {
	if (message == WM_SOCKET_MESSAGE) {
				GetPrivateProfileString("Main", "Language", "English", language_string, MAX_PATH, exe_path);
				GetPrivateProfileString("Main", "MsgHistory", "Enabled", msg_history_string, MAX_PATH, exe_path);
				CString lng_selitemtext(language_string);
				CString msg_history_string2(msg_history_string);
				int status;
				char sock_buffer[32768] = {0};
				status = recv((SOCKET)wParam, (char*) &sock_buffer, 32767, 0);
				if (status == SOCKET_ERROR) {
					char error_msg[100];
					if(lng_selitemtext == "Russian") {
						sprintf(error_msg, "������ ������: %d", WSAGetLastError());
						MessageBox(error_msg, "������", MB_OK|MB_ICONSTOP);
					} else {
						sprintf(error_msg, "Socket error: %d", WSAGetLastError());
						MessageBox(error_msg, "Error", MB_OK|MB_ICONSTOP);
					};
					try {
						AfxGetApp()->GetMainWnd()->SetWindowText("Tinelix IRC Client");
					} catch(...) {

					};
					exit;
				};
				if (status == 0) {
					if(lng_selitemtext == "Russian") {
						MessageBox("���������� ��������.", "����������", MB_OK|MB_ICONINFORMATION);
					} else {
						MessageBox("Connection lost.", "Information", MB_OK|MB_ICONINFORMATION);
					};
					IsConnected = FALSE;
					closesocket(sock);
					AfxGetApp()->GetMainWnd()->SetWindowText("Tinelix IRC Client");
					exit;
				};
				char listing[32768] = {0};
				char previous_listing[32768] = {0};
				int sp;
				irc_chat_page->GetDlgItem(IDC_SOCKMSGS)->GetWindowText(previous_listing, 32767);
				if(previous_listing == NULL) {
				};
				sp = sprintf(listing, "%s", previous_listing);
				CString sock_buffer_str;
				char pong_msg[400];
				int string_index = 0;
				int array_index_2 = 0;
				char nwline[32768];
				char* parsed_msg;
				char* parsed_msg_list;
				parsed_msg_list = (char*)malloc(sizeof(char) * 32768);
				char* nw_token;
				CArray<CString, CString> new_line_splitter;
				while(AfxExtractSubString(sock_buffer_str, sock_buffer, string_index, '\n'))
				{
					new_line_splitter.Add(sock_buffer_str);
					++string_index;
				};
				CString pong;
				nw_token = strtok(sock_buffer, "\r\n");										   
				while(nw_token != NULL) {
					new_line_splitter[array_index_2++] = nw_token;
					nw_token = strtok(NULL, "\r\n");
				};

				int parsed_msg_index = 0;

                for (int i = 0; i < new_line_splitter.GetSize(); i++) {
					pong = "";
					//TRACE("LINE: %s\r\n", new_line_splitter[i]);
					if (new_line_splitter[i].Left(4) == "PING") {
						int pong_index;
						sprintf(pong_msg, "PONG %s\r\n", new_line_splitter[i].Right(strlen(new_line_splitter[i]) - 5));
						time_t until_ping, after_ping;
						until_ping = clock();
						status = send((SOCKET)wParam, pong_msg, strlen(pong_msg), 0);
						after_ping = clock();
						double after_ping_ms = 1000.0 * after_ping;
						double until_ping_ms = 1000.0 * until_ping;
						char ping_time[20];
						double ping_time_ms = (double)(after_ping_ms - until_ping_ms) / CLOCKS_PER_SEC;
						TRACE("[%lf]\r\n", ping_time_ms);
						sprintf(ping_time, "%.2f ms", ping_time_ms);
						if(ping_time_ms > 0.000) {
							GetDlgItem(IDC_CONNETCTION_UPTIME)->SetWindowText(ping_time);
						} else {
							if(lng_selitemtext == "Russian") {
								GetDlgItem(IDC_CONNETCTION_UPTIME)->SetWindowText("(��� ������)");
							} else {
								GetDlgItem(IDC_CONNETCTION_UPTIME)->SetWindowText("(No data)");
							}
						}
						pong = pong_msg;
						i = new_line_splitter.GetSize();
					} else {
						if(parserLib == NULL) {
							if(i >= new_line_splitter.GetSize() - 1) {
								parsed_msg_index += sprintf(parsed_msg_list + parsed_msg_index, "%s", new_line_splitter[i]);
							} else {
								parsed_msg_index += sprintf(parsed_msg_list + parsed_msg_index, "%s\r\n", new_line_splitter[i]);
							};
						} else {	
							char* unparsed_msg;
							char** parsing_array;
							unparsed_msg = (char*)calloc(sizeof(char), 32768 + 1);
							parsed_msg = (char*)calloc(sizeof(char), 32768 + 1);
							parsing_array = (char**)calloc(sizeof(char*), 32768 + 1);
							for (int array_index = 0; array_index < sizeof(parsing_array); array_index++) {
								parsing_array[array_index] = (char*)calloc(sizeof(char), 32768 + 1);
							};
							sprintf(unparsed_msg, "%s", new_line_splitter[i]);
							ParseMessage ParseMsg;
							ParseMsg = (ParseMessage)GetProcAddress((HMODULE)parserLib, "ParseMessage");
							try {
								ParseMsg(unparsed_msg, parsing_array, parsed_msg, FALSE);
								CString p_msg(parsed_msg);
								//TRACE("OUTPUT: %s\r\n", parsed_msg);
								parsed_msg_index += sprintf(parsed_msg_list + parsed_msg_index, "%s", p_msg);
							} catch(...) {

							};
						};
					};
				};
				if(pong == "") {
					TRACE("TEXT LENGTH: %d symbols\r\n", strlen(previous_listing) + strlen(parsed_msg_list));
					if(strlen(previous_listing) + strlen(parsed_msg_list) < 32768) {
						sp += sprintf(listing + sp, "%s",  parsed_msg_list);
					} else {
						sp = sprintf(listing, "%s",  parsed_msg_list);
					};
					if(msg_history_string2 == "Enabled") {
						if(history_file_stdio.Open(history_file, CFile::modeReadWrite)) {
							history_file_stdio.Write(listing, 32768);
							history_file_stdio.Close();
						} else {
							if (history_file_stdio.Open(history_file, CFile::modeReadWrite | CFile::modeCreate)) {
								history_file_stdio.Write(listing, 32768);
								history_file_stdio.Close();
							} else {
							};
						};
					};
					irc_chat_page->GetDlgItem(IDC_SOCKMSGS)->SetWindowText(listing);
					CEdit* msg_box = (CEdit*)irc_chat_page->GetDlgItem(IDC_SOCKMSGS);
					msg_box->SetSel(0, -1);
					msg_box->SetSel(-1);
				};
	};
	if(message == WM_SENDING_SOCKET_MESSAGE) {
		char *text = (char*)wParam;
		int status = MainWindow::SendSocketMessage(text);
	};
	
	return CDialog::WindowProc(message, wParam, lParam);
}

BOOL MainWindow::DestroyWindow() 
{
	closesocket(sock);
	WSACleanup();
	FreeLibrary(parserLib);
	delete irc_chat_page;
	return CDialog::DestroyWindow();
}


UINT MainWindow::SendSocketMessage(char *str)
{
	return send(sock, str, strlen(str), 0);
};

void MainWindow::OnViewSettings() 
{
	SettingsDialog setgdlg;
	setgdlg.DoModal();
	
}

void MainWindow::OnHelpAbout() 
{
	AboutDialog aboutdlg;
	aboutdlg.DoModal();
	
}
