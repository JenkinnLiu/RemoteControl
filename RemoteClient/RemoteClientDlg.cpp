
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "ClientSocket.h"
#include "ClientController.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "CWatchDialog.h"


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
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)
	ON_MESSAGE(WM_SEND_PACK_ACK, &CRemoteClientDlg::OnSendPackAck)
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
	//m_server_address = 0x7F000001;//设置默认IP地址,0X7F000001是127.0.0.1的16进制表示，
	//游戏机是192.168.30.67，虚拟机是192.168.164.128
	m_server_address = 0xC0A8A480;//设置默认IP地址,这里设的是虚拟机地址
	m_nPort = "9527";//设置默认端口号

	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));//更新IP地址和端口号
	UpdateData(FALSE);//将变量的值传给控件
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);//隐藏状态对话框

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
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1981);//发送命令数据1981

}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	std::list<CPacket> lstPackets;
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), true, NULL, 0);//发送命令数据1，请求磁盘信息,拿到盘符
	if (ret == -1) {
		AfxMessageBox(_T("命令处理失败"));
		return;
	}
}


void CRemoteClientDlg::LoadFileCurrent() {
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);
	m_LIst.DeleteAllItems();//清空文件列表
	int nCmd = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false, (BYTE*)(LPCSTR)strPath, strPath.GetLength());//查看指定目录下的文件
	TRACE("执行SendCommandPacket2查看指定目录下的文件 ret:%d\r\n", nCmd);
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	while (pInfo->HasNext) {//如果没有下一个文件
		TRACE("[%s] isdir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (pInfo->IsDirectory) {//如果是文件
			m_LIst.InsertItem(0, pInfo->szFileName);//插入文件名，0表示插入到第一个位置
		}
		int cmd = CClientController::getInstance()->DealCommand();//接收服务器数据
		TRACE("客户端接收服务器端文件目录,ack:%d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}

	CClientController::getInstance()->CloseSocket();
}

void CRemoteClientDlg::LoadFileInfo() {
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
	std::list<CPacket> lstPackets;
	int nCmd = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false, (BYTE*)(LPCSTR)strPath, strPath.GetLength(), (WPARAM)hTreeSelected);//查看指定目录下的文件
	if (lstPackets.size() > 0) {
		std::list<CPacket>::iterator it = lstPackets.begin();
		for (; it != lstPackets.end(); it++) {
			PFILEINFO pInfo = (PFILEINFO)(*it).strData.c_str();//获取文件信息
			if (pInfo->HasNext == FALSE) continue;//如果没有下一个文件,则开始读下一个包
			if (pInfo->IsDirectory) {
				if (CString(pInfo->szFileName) == "." || (CString(pInfo->szFileName) == "..")) {
					continue;
				}
				HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);//插入文件pInfo->szFileName, hTreeSelected表示父节点，TVI_LAST表示最后一个节点
				m_Tree.InsertItem("", hTemp, TVI_LAST);//插入一个空节点
			}
			else {//如果是文件
				m_LIst.InsertItem(0, pInfo->szFileName);//插入文件名，0表示插入到第一个位置
			}
		}
	}

	CClientController::getInstance()->CloseSocket();
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

void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree) {
	HTREEITEM hSub = NULL;
	do {
		hSub = m_Tree.GetChildItem(hTree);
		if (hSub != NULL) m_Tree.DeleteItem(hSub);
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
	m_LIst.ScreenToClient(&ptLIst);//将鼠标位置转换为列表控件的客户区坐标
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


void CRemoteClientDlg::OnDownloadFile()
{
	int nListSelected = m_LIst.GetSelectionMark();//获得选择标记
	CString strFile = m_LIst.GetItemText(nListSelected, 0);//获取文件名
	HTREEITEM hSelected = m_Tree.GetSelectedItem();//获取树控件的选择项
	strFile = GetPath(hSelected) + strFile;//获取路径
	int ret = CClientController::getInstance()->DownFile(strFile);//下载文件
	if (ret != 0) {
		MessageBox(_T("下载失败！！"), _T("失败"));
		TRACE("下载失败！！, ret = %d\r\n", ret);
	}
	//添加线程函数

}


void CRemoteClientDlg::OnDeleteFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_LIst.GetSelectionMark();
	CString strFile = m_LIst.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("删除文件命令执行失败！！");
	}
	//删除后自动刷新
	LoadFileCurrent();//TODO:文件夹文件显示有缺漏
}


void CRemoteClientDlg::OnRunFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_LIst.GetSelectionMark();
	CString strFile = m_LIst.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 3, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("打开文件命令执行失败！！");
	}
}


void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	CClientController::getInstance()->StartWatchScreen();
}


void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);
}


void CRemoteClientDlg::OnEnChangeEditPort()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));//更新网络服务器的地址
}


void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));//更新网络服务器的地址
}

LRESULT CRemoteClientDlg::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || lParam == -2) {
		//TODO: 错误处理
	}
	else if (lParam == 1) {
		//对方关闭了套接字
	}
	else {
		if (wParam != NULL) {
			CPacket head = *(CPacket*)wParam;
			delete (CPacket*)wParam;
			switch (head.sCmd) {
			case 1: {//获取驱动信息
				std::string drivers = head.strData;//拿到盘符
				std::string dr;
				m_Tree.DeleteAllItems();//清空树
				drivers += ',';//加一个逗号，方便最后一个盘符插入
				for (size_t i = 0; i < drivers.size(); i++) {
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
				if (dr.size() > 0) {
					dr += ":";//将盘符后面的\0去掉,加分好 
					HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);//插入可用盘符, TVI_ROOT表示根节点，TVI_LAST表示最后一个节点,表示追加到根目录
					m_Tree.InsertItem(NULL, hTemp, TVI_LAST);// 插入一个空节点,为的是可以双击树控件，获取文件信息
				}
				break;
			}
			case 2: {//获取文件信息
				PFILEINFO pInfo = (PFILEINFO)head.strData.c_str();//获取文件信息
				if (pInfo->HasNext == FALSE) break;//如果没有下一个文件,则开始读下一个包
				if (pInfo->IsDirectory) {
					if (CString(pInfo->szFileName) == "." || (CString(pInfo->szFileName) == "..")) {
						break;
					}
					HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, (HTREEITEM)lParam, TVI_LAST);//插入文件pInfo->szFileName, hTreeSelected表示父节点，TVI_LAST表示最后一个节点
					m_Tree.InsertItem("", hTemp, TVI_LAST);//插入一个空节点
				}
				else {//如果是文件
					m_LIst.InsertItem(0, pInfo->szFileName);//插入文件名，0表示插入到第一个位置
				}
			}
				  break;
			case 3: {
				TRACE("run file done!\r\n");
			}
				  break;
			case 4: {
				static LONGLONG length = 0, index = 0;
				if (length == 0) {
					length = *(long long*)head.strData.c_str();
					if (length == 0) {
						AfxMessageBox("文件长度为零或者无法读取文件！！！");
						CClientController::getInstance()->DownloadEnd();//下载结束

					}
				}
				else if (length > 0 && index >= length) {
					fclose((FILE*)lParam);
					length = index = 0;
					CClientController::getInstance()->DownloadEnd();
				}
				else {
					FILE* pFile = (FILE*)lParam;
					fwrite(head.strData.c_str(), 1, head.strData.size(), pFile);
					index += head.strData.size();
				}
			}
				  break;
			case 9:
				TRACE("delete successfully!\r\n");
				break;
			case 1981:
				TRACE("test successfully!\r\n");
				break;
			default:
				TRACE("unknown data received ! %d\r\n", head.sCmd);
				break;
			}
		}
		return 0;
	}
}
