// ConnectionManagerDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// ConnectionManagerDialog dialog

class ConnectionManagerDialog : public CDialog
{
// Construction
public:
	ConnectionManagerDialog(CWnd* pParent = NULL);   // standard constructor
	void ConnectionManagerDialog::delsymbs(char *str, int begin, int lng);
// Dialog Data
	//{{AFX_DATA(ConnectionManagerDialog)
	enum { IDD = IDD_CONNMAN };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ConnectionManagerDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CFont mainfont;
	INT ConnectionManagerDialog::GetCountLines(LPCSTR str);
	void SetConnectionState(BOOL value);
	BOOL IsConnected;

	// Generated message map functions
	//{{AFX_MSG(ConnectionManagerDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnAddProfileBtn();
	afx_msg void OnChangeProfileBtn();
	afx_msg void OnConnectProfileBtn();
	afx_msg void OnDeleteProfileBtn();
	afx_msg void OnSelchangeProfilelist();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
friend class MainWindow;
};
