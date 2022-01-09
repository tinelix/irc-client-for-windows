// ConnectionManagerDialog.cpp : implementation file
//

#include "stdafx.h"
#include "TinelixIRC.h"
#include "ConnectionManagerDialog.h"
#include "MainWindow.h"
#include "EditDialog.h"
#include <locale.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ConnectionManagerDialog dialog

struct PARAMETERS 
{
	char server[256];
	char nickname[80];
	char reserve_nickname[80];
	char realname[80];
	int port;
	HWND hwnd;
	char quit_msg[512];
	BOOL hide_ip;
};

InfoMessageDialog* info_msg_dlg;


ConnectionManagerDialog::ConnectionManagerDialog(CWnd* pParent /*=NULL*/)
	: CDialog(ConnectionManagerDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(ConnectionManagerDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void ConnectionManagerDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ConnectionManagerDialog)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ConnectionManagerDialog, CDialog)
	//{{AFX_MSG_MAP(ConnectionManagerDialog)
	ON_BN_CLICKED(IDC_ADD_PROFILE_BTN, OnAddProfileBtn)
	ON_BN_CLICKED(IDC_CHANGE_PROFILE_BTN, OnChangeProfileBtn)
	ON_BN_CLICKED(IDC_CONNECT_PROFILE_BTN, OnConnectProfileBtn)
	ON_BN_CLICKED(IDC_DELETE_PROFILE_BTn, OnDeleteProfileBtn)
	ON_LBN_SELCHANGE(IDC_PROFILELIST, OnSelchangeProfilelist)
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ConnectionManagerDialog message handlers


void ConnectionManagerDialog::delsymbs(char *str, int begin, int lng)
{
	for(begin; begin < strlen(str); begin++) {
		*(str + begin) = 0;	
	};
};

INT ConnectionManagerDialog::GetCountLines(LPCSTR str)
{
	return 0;
};


void ConnectionManagerDialog::SetConnectionState(BOOL value) 
{
	IsConnected = value;
}

BOOL ConnectionManagerDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();

	info_msg_dlg = new InfoMessageDialog;
	info_msg_dlg->Create(InfoMessageDialog::IDD, this);
	
	char exe_path[MAX_PATH] = {0};
	char exe_name[MAX_PATH] = "TLX_IRC.EXE"; // EXE filename

	GetModuleFileName(NULL, exe_path, MAX_PATH);  
	if (GetFileAttributes(exe_path) == 1) {
	}; 
	*(strrchr(exe_path, '\\')+1)='\0';

	strcat(exe_path, "\\settings.ini");	// add settings filename

	char language_string[MAX_PATH] = {0};

	mainfont.CreateFont(8, 0, 0, 0, FW_REGULAR, FALSE, FALSE, 0, DEFAULT_CHARSET, 0, 0, 
	0, 0, "MS Sans Serif");

	try {
	char settings[32768] = {0};
	int settings_int = 1;
	int settings_section_index = 0;
	try {
	GetPrivateProfileString("Main", "Language", "English", language_string, MAX_PATH, exe_path);
	} catch(CFileException* cfe) {
		exit;	
	};
	CComboBox *language_combo = (CComboBox*)GetDlgItem(IDC_LANGCOMBO1);
	CListBox *profiles_cb = (CListBox*)GetDlgItem(IDC_PROFILELIST);
	language_combo->SelectString(NULL, language_string);
	try 
	{	
		CStdioFile settings_file;
		try {
			if(settings_file.Open(exe_path, CFile::modeReadWrite)) {
				settings_file.Read(settings, 32767);
			} else {
				if (settings_file.Open(exe_path, CFile::modeReadWrite | CFile::modeCreate)) {
					settings_file.Read(settings, 32767);
				} else {
					exit;
				};
			}
		} catch(CFileException* cfe) {
			language_combo->SelectString(NULL, "English");
			exit;
		};

		CString settings_str(settings);

		BOOL IsSection;
		CString section;
		
		for(int i = 0; i < strlen(settings_str); i++)
		{
			CString settings_symbol(settings[i]);
			settings_symbol.Right(i);
			if (settings_symbol.Find("]") != -1) {
				IsSection = FALSE;
				if (section.IsEmpty() == FALSE && section != "Main" && section != "Parser"
					&& profiles_cb->FindString(0, section) == CB_ERR) {
					profiles_cb->AddString(section);
				};
				section = "";
			};
			if (IsSection == TRUE) {
				section += settings_symbol;
			};
			if (settings_symbol.Find("[") != -1) {
				IsSection = TRUE;
			};	
		};
	}
	catch(...)
	{
		TRACE("Error creating file");
	};

	CString lng_selitemtext_2(language_string);
	if (lng_selitemtext_2 == "Russian") {
		GetDlgItem(IDC_LANGLABEL)->SetWindowText("���� (Language)");
		GetDlgItem(IDC_PROFILES_GROUP)->SetWindowText("�������");
		GetDlgItem(IDC_ADD_PROFILE_BTN)->SetWindowText("��������");
		GetDlgItem(IDC_CHANGE_PROFILE_BTN)->SetWindowText("��������");
		GetDlgItem(IDC_CONNECT_PROFILE_BTN)->SetWindowText("������������");
		GetDlgItem(IDC_DELETE_PROFILE_BTn)->SetWindowText("�������");
		GetDlgItem(IDCANCEL)->SetWindowText("������");
		SetWindowText("�������������� ��������� �������");
	} else {
		GetDlgItem(IDC_LANGLABEL)->SetWindowText("Language");
		GetDlgItem(IDC_PROFILES_GROUP)->SetWindowText("Profiles");
		GetDlgItem(IDC_ADD_PROFILE_BTN)->SetWindowText("Add");
		GetDlgItem(IDC_CHANGE_PROFILE_BTN)->SetWindowText("Change");
		GetDlgItem(IDC_CONNECT_PROFILE_BTN)->SetWindowText("Connect");
		GetDlgItem(IDC_DELETE_PROFILE_BTn)->SetWindowText("Delete");
		GetDlgItem(IDCANCEL)->SetWindowText("Cancel");
		SetWindowText("Initial client setup");
	};
	//GetPrivateProfileSectionNames(sections, 32767, exe_path);
	//profiles_cb->AddString(sections);
	} 
	catch(CException exception)
	{
	};
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void ConnectionManagerDialog::OnAddProfileBtn() 
{
	EditDialog edittext;
	edittext.DoModal();

	char selitemtext[80];
	char section[800];
	char port[5];
	char port_to_str[20];
	char nicknames[80]; 
	char exe_path[MAX_PATH] = {0};
	char exe_name[MAX_PATH] = "TLX_IRC.EXE"; // EXE filename
	char settings[32768] = {0};
	CListBox *profiles_cb = (CListBox*)GetDlgItem(IDC_PROFILELIST);
	int selindex = profiles_cb->GetCurSel();
	GetModuleFileName(NULL, exe_path, MAX_PATH);
	*(strrchr(exe_path, '\\')+1)='\0';
	strcat(exe_path, "\\settings.ini");	// add settings filename
	char instruction_txt[512] = {0};
	int instruction_msgb;

	char show_infomsg[12];
	char language_string[MAX_PATH] = {0};
	GetPrivateProfileString("Main", "Language", "English", language_string, MAX_PATH, exe_path);
	GetPrivateProfileString("Main", "ShowInfoMessages", "", show_infomsg, 12, exe_path);
	CString lng_selitemtext_2(language_string);

	info_msg_dlg->SetInfoMessageCode(1);

	if(strcmp(show_infomsg, "Enabled") == 0 && lng_selitemtext_2 == "Russian") {
		sprintf(instruction_txt, "��� ������ �����������, ��� ���� ����� ��������� ���������:\r\n\r\n"
			"Nickname - �������� �������,\r\nReserveNickname - �������� �������,\r\n"
			"Password - ������ ��� NickServ,\r\nRealname - ��������� ���,\r\n"
			"Server - ����� �������,\r\n"
			"Port - ���� �������,\r\nQuitMessage - ��������� ��� ������ �� �������,\r\n"
			"HideIP - ����� ���������� IP-������\r\n\r\n��������: Enabled - �������, Disabled - ��������.\r\n\r\n"
			"������� \"OK\" ��� �������� ���������� ���������.");
		info_msg_dlg->GetDlgItem(IDC_INFOMSG_TEXT)->SetWindowText(instruction_txt);
		info_msg_dlg->SetWindowText("����������");
		info_msg_dlg->CenterWindow();
		info_msg_dlg->ShowWindow(SW_SHOW);
	} else {
		SendMessage(WM_CLOSE_INFOMSG, NULL, NULL);
	};
		
	try 
	{	
		int profiles_count = profiles_cb->GetCount();
		for (int index = 0; index < profiles_count; index++) {
		 	profiles_cb->DeleteString(index);
		};
		CStdioFile settings_file;
		try {
			if(settings_file.Open(exe_path, CFile::modeReadWrite)) {
				settings_file.Read(settings, 32767);
			} else {
				if (settings_file.Open(exe_path, CFile::modeReadWrite | CFile::modeCreate)) {
					settings_file.Read(settings, 32767);
				} else {
					exit;
				};
			}
		} catch(CFileException* cfe) {
			exit;
		};

		CString settings_str(settings);

		BOOL IsSection;
		CString section;
		
		for(int i = 0; i < strlen(settings_str); i++)
		{
			CString settings_symbol(settings[i]);
			settings_symbol.Right(i);
			if (settings_symbol.Find("]") != -1) {
				IsSection = FALSE;
				if (section.IsEmpty() == FALSE && section != "Main" && section != "Parser" 
					&& profiles_cb->FindString(0, section) == CB_ERR) {
					profiles_cb->AddString(section);
				};
				section = "";
			};
			if (IsSection == TRUE) {
				section += settings_symbol;
			};
			if (settings_symbol.Find("[") != -1) {
				IsSection = TRUE;
			};	
		};
	}
	catch(...)
	{
		TRACE("Error creating file");
	};
	
}

void ConnectionManagerDialog::OnChangeProfileBtn() 
{
	char exe_path[MAX_PATH] = {0};
	char exe_name[MAX_PATH] = "TLX_IRC.EXE"; // EXE filename

	GetModuleFileName(NULL, exe_path, MAX_PATH);  

	*(strrchr(exe_path, '\\')+1)='\0';

	strcat(exe_path, "\\settings.ini");

	char language_string[MAX_PATH] = {0};
	GetPrivateProfileString("Main", "Language", "English", language_string, MAX_PATH, exe_path);
	CString lng_selitemtext_2(language_string);
	if((UINT)ShellExecute(NULL, "open", "notepad.exe", exe_path, NULL, SW_SHOWNORMAL) <= 32) {
		try {
			if(lng_selitemtext_2 == "Russian") {
				MessageBox("������� �� ������. �������������� ������ ���������� ��� ��������� ������� ����������������� �����.", "������", MB_OK | MB_ICONSTOP);
			} else {
				MessageBox("Notepad not found. Use another editor to modify this configuration file.", "Error", MB_OK | MB_ICONSTOP);
			};
		} catch(...) {
		
		};
	};
}

void ConnectionManagerDialog::OnConnectProfileBtn() 
{
	MainWindow* mainwin = (MainWindow*)GetParent();
	IRCClient* application = (IRCClient*)AfxGetApp();
	char selitemtext[80];
	char section[800];
	char port[5];
	char port_to_str[20];
	char nickname[80]; 
	char exe_path[MAX_PATH] = {0};
	char exe_name[MAX_PATH] = "TLX_IRC.EXE"; // EXE filename
	GetModuleFileName(NULL, exe_path, MAX_PATH);
	CListBox* profile_cb = (CListBox*)GetDlgItem(IDC_PROFILELIST);
	int selindex = profile_cb->GetCurSel(); 
	*(strrchr(exe_path, '\\')+1)='\0';
	strcat(exe_path, "\\settings.ini");	// add settings filename
	char hide_ip_value[12];
	PARAMETERS params;
	profile_cb->GetText(selindex, selitemtext);
	GetPrivateProfileString(selitemtext, "Server", "", params.server, 256, exe_path);
	GetPrivateProfileString(selitemtext, "Port", "", port, 5, exe_path);
	GetPrivateProfileString(selitemtext, "Nickname", "", params.nickname, 80, exe_path);
	GetPrivateProfileString(selitemtext, "ReserveNickname", "", params.reserve_nickname, 80, exe_path);
	GetPrivateProfileString(selitemtext, "Realname", "", params.realname, 80, exe_path);
	GetPrivateProfileString(selitemtext, "QuitMessage", "", params.quit_msg, 512, exe_path);
	GetPrivateProfileString(selitemtext, "HideIP", "", hide_ip_value, 512, exe_path);
	if(strlen(params.realname) == 0) {
		sprintf(params.realname, "Member");
	};
	if(strlen(params.nickname) == 0) {
		sprintf(params.realname, "Member");
	};
	if(strlen(params.quit_msg) == 0) {
		sprintf(params.quit_msg, "Tinelix IRC Client ver. %s (%s)", application->version, application->build_date);
		WritePrivateProfileString(selitemtext, "QuitMessage", params.quit_msg, exe_path);
	};
	if(strlen(hide_ip_value) == 0) {
		WritePrivateProfileString(selitemtext, "HideIP", "Disabled", exe_path);
		params.hide_ip = FALSE;
	} else if(strcmp(hide_ip_value, "Enabled") == 0) {
		params.hide_ip = TRUE;
	} else {
		params.hide_ip = FALSE;
	};
	params.port = atoi(port);
	GetDlgItem(IDC_CONNECT_PROFILE_BTN)->EnableWindow(FALSE);
		 
	mainwin->CreateConnectionThread(params);
	OnCancel();
	
}

void ConnectionManagerDialog::OnDeleteProfileBtn() 
{
	// TODO: Add your control notification handler code here
	
}

void ConnectionManagerDialog::OnSelchangeProfilelist() 
{
	if (IsConnected == FALSE) {
		GetDlgItem(IDC_CONNECT_PROFILE_BTN)->EnableWindow(TRUE);
	};
	GetDlgItem(IDC_CHANGE_PROFILE_BTN)->EnableWindow(TRUE);	
	
}

void ConnectionManagerDialog::OnOK() 
{
	char selitemtext[80];
	CComboBox* cb = (CComboBox*)GetDlgItem(IDC_LANGCOMBO1);
	int selindex = cb->GetCurSel();
	cb->GetLBText(selindex, selitemtext);
	CString lng_selitemtext_2(selitemtext);
	char language_string[80] = {0};
	char exe_path[MAX_PATH] = {0};
	char settings_path[MAX_PATH] = {0};
	char exe_name[MAX_PATH] = "TLX_IRC.EXE"; // EXE filename
	GetModuleFileName(NULL, exe_path, MAX_PATH);
	GetModuleFileName(NULL, settings_path, MAX_PATH);
	*(strrchr(exe_path, '\\')+1)='\0';
	strcat(settings_path, "\\settings.ini");	// add settings filename
	if(GetFileAttributes(settings_path) == 1 && GetFileAttributes(exe_path) == 1) {
		MessageBox("The disk may be write protected.", "Warning", MB_OK|MB_ICONEXCLAMATION);
	};
	GetPrivateProfileString("Main", "Language", "English", language_string, MAX_PATH, settings_path);
	CString lng_selitemtext_3(language_string);
	CMenu *eng_mainmenu;
	CMenu *rus_mainmenu;
	CMenu *file_submenu;
	CMenu *help_submenu;
	CMenu *view_submenu;
	eng_mainmenu = new CMenu;
	rus_mainmenu = new CMenu;
	file_submenu = new CMenu;
	help_submenu = new CMenu;
	view_submenu = new CMenu;
	
	MainWindow* mainwin = (MainWindow*)GetParent();

	if(lng_selitemtext_2 == "Russian") {
		if (lng_selitemtext_3 != "Russian") {
			WritePrivateProfileString("Main", "Language", "Russian", settings_path);

			file_submenu->m_hMenu = NULL;
			file_submenu->CreatePopupMenu();
			file_submenu->AppendMenu(MF_STRING, ID_FILE_CONNECT, "������������...");
			file_submenu->AppendMenu(MF_SEPARATOR);
			file_submenu->AppendMenu(MF_STRING, ID_FILE_STATISTICS, "����������");
			file_submenu->AppendMenu(MF_SEPARATOR);
			file_submenu->AppendMenu(MF_STRING, ID_FILE_QUIT, "�����");
			view_submenu->m_hMenu = NULL;
			view_submenu->CreatePopupMenu();
			view_submenu->AppendMenu(MF_STRING, ID_VIEW_SETTINGS, "���������");
			help_submenu->m_hMenu = NULL;
			help_submenu->CreatePopupMenu();
			help_submenu->AppendMenu(MF_STRING, ID_HELP_ABOUT, "� ���������...");
			rus_mainmenu->m_hMenu = NULL;
			rus_mainmenu->CreateMenu();
			rus_mainmenu->AppendMenu(MF_STRING | MF_POPUP, (UINT)file_submenu->m_hMenu, "����");
            rus_mainmenu->AppendMenu(MF_STRING | MF_POPUP, (UINT)view_submenu->m_hMenu, "���");
			rus_mainmenu->AppendMenu(MF_STRING | MF_POPUP, (UINT)help_submenu->m_hMenu, "�������");
			GetParent()->SetMenu(rus_mainmenu);
			mainwin->irc_chat_page->GetDlgItem(IDC_SENDMSG)->SetWindowText("���������");
		} else {
			WritePrivateProfileString("Main", "Language", "Russian", settings_path);
		};
	} else {
		WritePrivateProfileString("Main", "Language", "English", settings_path);
		setlocale(LC_ALL, "English");
		eng_mainmenu->m_hMenu = NULL;
		eng_mainmenu->LoadMenu(IDR_MAINMENU);
		GetParent()->SetMenu(eng_mainmenu);
		mainwin->irc_chat_page->GetDlgItem(IDC_SENDMSG)->SetWindowText("Send");
	};

	delete(eng_mainmenu);
	delete(rus_mainmenu);
	delete(file_submenu);
	delete(view_submenu);
	delete(help_submenu);
	
	CDialog::OnOK();
}

void ConnectionManagerDialog::OnDestroy() 
{		
	
	CDialog::OnDestroy();
	
}

void ConnectionManagerDialog::OnClose() 
{		
	CDialog::OnClose();
}

LRESULT ConnectionManagerDialog::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	char selitemtext[80];
	char section[800];
	char port[5];
	char port_to_str[20];
	char nicknames[80]; 
	char exe_path[MAX_PATH] = {0};
	char exe_name[MAX_PATH] = "TLX_IRC.EXE"; // EXE filename
	char settings[32768] = {0};
	GetModuleFileName(NULL, exe_path, MAX_PATH);
	*(strrchr(exe_path, '\\')+1)='\0';
	strcat(exe_path, "\\settings.ini");	// add settings filename
	char instruction_txt[512] = {0};
	int instruction_msgb;

	char show_infomsg[12];
	char language_string[MAX_PATH] = {0};
	GetPrivateProfileString("Main", "Language", "English", language_string, MAX_PATH, exe_path);
	GetPrivateProfileString("Main", "ShowInfoMessages", "", show_infomsg, 12, exe_path);
	CString lng_selitemtext_2(language_string);
	
	if(message == WM_CLOSE_INFOMSG) {
		int code = (int)wParam;
		if(code == 1) {
			if(lng_selitemtext_2 == "Russian" && (UINT)ShellExecute(NULL, "open", "notepad.exe", exe_path, NULL, SW_SHOWNORMAL) <= 32) {
				try {
					MessageBox("������� �� ������. �������������� ������ ���������� ��� ��������� ������� ����������������� �����.", "������", MB_OK | MB_ICONSTOP);
				} catch(...) {
				
				};
			} else if(lng_selitemtext_2 != "Russian" && (UINT)ShellExecute(NULL, "open", "notepad.exe", exe_path, NULL, SW_SHOWNORMAL) <= 32) {
				try {
					MessageBox("Notepad not found. Use another editor to modify this configuration file.", "Error", MB_OK | MB_ICONSTOP);
				} catch(...) {
				
				};
			};
		};
	};
	
	return CDialog::WindowProc(message, wParam, lParam);
}
