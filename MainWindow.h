// MainWindow.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// MainWindow dialog

#define WM_SOCKET_MESSAGE 0xAFFF
#define WM_SENDING_SOCKET_MESSAGE 0xAFFE
#define WM_CHANGING_MSGFONT 0xAFFD
#define WM_SOCKET_TIMER 1;

#include "IRCChatPage.h"

class MainWindow : public CDialog
{
// Construction
public:
	MainWindow(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(MainWindow)
	enum { IDD = IDD_MAINWINDOW };
	CTabCtrl	m_irc_tabs;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(MainWindow)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:
	IRCChatPage* irc_chat_page;
	CFont font;
	CFont mainfont;

	// Generated message map functions
	//{{AFX_MSG(MainWindow)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnFileConnect();
	afx_msg void OnViewSettings();
	afx_msg void OnHelpAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	struct PARAMETERS
	{
		char server[256];
		char nickname[80];
		char reserve_nickname[80];
		char realname[80];
		int port;
		HWND hwnd;
	};
	char channel[256];
	char exe_path[MAX_PATH];
	char history_path[MAX_PATH];
	char history_path2[MAX_PATH];
	char history_file[MAX_PATH];
	time_t current_time;
	CStdioFile history_file_stdio;
	BOOL IsConnected;
	HWND hwnd_chat;
	void MainWindow::ConnectionFunc(HWND hwnd, PARAMETERS params);
	void MainWindow::delsymbs(char *str, int begin, int lng);
	void MainWindow::CreateConnectionThread(PARAMETERS params);
	UINT MainWindow::SendSocketMessage(char *str);

friend class IRCChatPage;
};
