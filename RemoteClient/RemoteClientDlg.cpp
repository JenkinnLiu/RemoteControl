
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "ClientSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_nPort(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_LIst);
}

int CRemoteClientDlg::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)//发送命令数据cmd
{	
	UpdateData();//取控件的值（IP地址和端口）
	/*atoi((LPCTSTR)m_nPort);*///LPCTSTR是一个指向字符的指针，atoi是将字符串转换为整数
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret = pClient->InitSocket(m_server_address, atoi((LPCTSTR)m_nPort));//TODO 返回值的处理
	if (ret == false) {
		AfxMessageBox("网络初始化失败！");
		return -1;
	}
	CPacket pack(nCmd, pData, nLength);
	ret = pClient->Send(pack);//向服务器发送测试数据
	TRACE("SendCommandPacket函数：Send ret :%d， 1代表向服务器发送成功\r\n", ret);
	int cmd = pClient->DealCommand();//接收测试服务器数据
	TRACE("SendCommandPacket函数：ack:%d\r\n", cmd);
	if(bAutoClose)//如果设置自动关闭套接字
		pClient->CloseSocket();
	return cmd;
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMRClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();//将控件的值传给变量
	m_server_address = 0x7F000001;//设置默认IP地址,0X7F000001是127.0.0.1的16进制表示
	m_nPort = "9527";//设置默认端口号
	UpdateData(FALSE);//将变量的值传给控件

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}




void CRemoteClientDlg::OnBnClickedBtnTest()//发送测试数据
{
	SendCommandPacket(1981, NULL, 0);//发送命令数据1981

}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	int ret = SendCommandPacket(1);//发送命令数据1，请求磁盘信息,拿到盘符
	if (ret == -1) {
		AfxMessageBox(_T("命令处理失败"));
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();
	std::string drivers = pClient->GetPacket().strData;//拿到盘符
	std::string dr;
	m_Tree.DeleteAllItems();//清空树
	drivers += ',';//加一个逗号，方便最后一个盘符插入
	for (size_t i = 0; i < drivers.size(); i ++) {
		if (drivers[i] == ',') {
			dr += ":";//将盘符后面的\0去掉,加分好
			HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);//插入可用盘符, TVI_ROOT表示根节点，TVI_LAST表示最后一个节点,表示追加到根目录
			m_Tree.InsertItem(NULL, hTemp, TVI_LAST);// 插入一个空节点,为的是可以双击树控件，获取文件信息
			//"C:" "D:" "E:"
			dr.clear();
			continue;
		}
		dr += drivers[i];
	}
}

void CRemoteClientDlg::LoadFileInfo(){
	CPoint ptMouse;
	GetCursorPos(&ptMouse);//获取鼠标位置
	m_Tree.ScreenToClient(&ptMouse);//将鼠标位置转换为树控件的客户区坐标
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);//获取鼠标所在的树控件的节点
	if (hTreeSelected == NULL)
		return;
	if (m_Tree.GetChildItem(hTreeSelected) == NULL)
		return;//如果没有子节点,即双击的是文件，直接返回
	DeleteTreeChildrenItem(hTreeSelected);//删除树控件的子节点,避免多次双击重复添加
	m_LIst.DeleteAllItems();//清空文件列表
	CString strPath = GetPath(hTreeSelected);
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCSTR)strPath, strPath.GetLength());//查看指定目录下的文件
	TRACE("执行SendCommandPacket2查看指定目录下的文件 ret:%d\r\n", nCmd);
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();
	while (pInfo->HasNext) {//如果没有下一个文件
		TRACE("[%s] isdir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (pInfo->IsDirectory) {
			if (CString(pInfo->szFileName) == "." || (CString(pInfo->szFileName) == "..")) {
				//如果是当前目录或者上级目录，不添加到树控件,避免无限递归死循环
				int cmd = pClient->DealCommand();//接收服务器数据
				TRACE("客户端接收服务器端文件目录,ack:%d\r\n", cmd);
				if (cmd < 0) break;
				pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
				continue;
			}
			HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);//插入文件pInfo->szFileName, hTreeSelected表示父节点，TVI_LAST表示最后一个节点
			m_Tree.InsertItem("", hTemp, TVI_LAST);//插入一个空节点
		}
		else {//如果是文件
			m_LIst.InsertItem(0, pInfo->szFileName);//插入文件名，0表示插入到第一个位置
		}
		int cmd = pClient->DealCommand();//接收服务器数据
		TRACE("客户端接收服务器端文件目录,ack:%d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}

	pClient->CloseSocket();
}

CString CRemoteClientDlg::GetPath(HTREEITEM hTree) {//获取路径
	CString strRet, strTmp;
	do {
		strTmp = m_Tree.GetItemText(hTree);//获取树控件的节点文本
		strRet = strTmp + "\\" + strRet;//将节点文本拼接起来
		hTree = m_Tree.GetParentItem(hTree);//获取父节点,一直拿到父节点为空
	} while (hTree != NULL);
	return strRet;
}

void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree){
	HTREEITEM hSub = NULL;
	do {
		hSub = m_Tree.GetChildItem(hTree);
		if(hSub != NULL) m_Tree.DeleteItem(hSub);
	} while (hSub != NULL);
}


void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)//双击树控件，获取文件信息
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMRClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	CPoint ptMouse, ptLIst;
	GetCursorPos(&ptMouse);//获取鼠标位置
	m_LIst.ScreenToClient(&ptMouse);//将鼠标位置转换为列表控件的客户区坐标
	int ListSelected = m_LIst.HitTest(ptLIst);//获取鼠标所在的列表控件的节点
	if (ListSelected < 0)
		return;
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);//加载菜单资源
	CMenu* pPupup = menu.GetSubMenu(0);//获取弹出菜单
	if (pPupup != NULL) {
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);//弹出菜单，左对齐，用右键弹出， ptMouse.x, ptMouse.y是鼠标位置
	}
}


void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	CPoint ptMouse, ptLIst;
	GetCursorPos(&ptMouse);//获取鼠标位置
	ptLIst = ptMouse;//初始化列表控件的鼠标位置
	m_LIst.ScreenToClient(&ptMouse);//将鼠标位置转换为列表控件的客户区坐标
	int ListSelected = m_LIst.HitTest(ptLIst);//获取鼠标所在的列表控件的节点
	if (ListSelected < 0)
		return;
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);//加载菜单资源
	CMenu* pPupup = menu.GetSubMenu(0);//获取弹出菜单
	if (pPupup != NULL) {
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);//弹出菜单，左对齐，用右键弹出， ptMouse.x, ptMouse.y是鼠标位置
	}
}
