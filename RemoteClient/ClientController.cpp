#include "pch.h"
#include "ClientController.h"
#include "ClientSocket.h"

std::unordered_map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;
CClientController* CClientController::m_instance = NULL;
CClientController::CHelper CClientController::m_helper;
CClientController* CClientController::getInstance()
{
	if (m_instance == NULL) {
		m_instance = new CClientController();
		//下面的代码是将消息和消息处理函数关联起来
		struct { UINT nMsg; MSGFUNC func; }MsgFuncs[] = {
			//{ WM_SEND_PACK, &CClientController::OnSendPack },
			//{ WM_SEND_DATA, &CClientController::OnSendData },
			{ WM_SHOW_STATUS, &CClientController::OnShowStatus },
			{ WM_SHOW_WATCH, &CClientController::OnShowWatcher },
			{(UINT)-1, NULL }
		};
		for (int i = 0; MsgFuncs[i].func != NULL; i++) {//向map中添加消息和消息处理函数
			m_mapFunc[MsgFuncs[i].nMsg] = MsgFuncs[i].func;
		}
	}
	return m_instance;
}

int CClientController::InitController()
{
	m_hThread = (HANDLE)_beginthreadex(NULL,
		0,
		&CClientController::threadEntry,
		this,
		0,
		&m_nThreadID);//CreateThread
	m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);
	return 0;
}

int CClientController::Invoke(CWnd*& pMainWnd)
{
	pMainWnd = &m_remoteDlg;
	return m_remoteDlg.DoModal();
}

LRESULT CClientController::SendMessage(MSG msg)
{
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);//创建一个事件
	if (hEvent == NULL) return -2;
	MSGINFO info(msg);
	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE, (WPARAM)&info, (LPARAM)hEvent);
	WaitForSingleObject(hEvent, INFINITE);//等待事件
	CloseHandle(hEvent);  
	return info.result;

}

int CClientController::DownFile(CString strPath)
{
	CFileDialog dlg(FALSE, NULL,
		strPath, OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
		NULL, &m_remoteDlg);//打开文件对话框,OFN_OVERWRITEPROMPT表示如果文件已经存在，询问是否覆盖，OFN_HIDEREADONLY表示隐藏只读复选框
	if (dlg.DoModal() == IDOK) {
		m_strRemote = strPath;
		m_strLocal = dlg.GetPathName();//本地文件路径
		CString strLocal = dlg.GetPathName();//获取文件路径
		FILE* pFile = fopen(m_strLocal, "wb+");//打开文件,如果文件不存在则创建
		if (pFile == NULL) {
			AfxMessageBox(_T("本地没有权限保存该文件，或者文件无法创建！！"));
			m_statusDlg.ShowWindow(SW_HIDE);
			m_remoteDlg.EndWaitCursor();
			return -1;
		}
		SendCommandPacket(m_remoteDlg, 4, true, (BYTE*)(LPCSTR)strPath, strPath.GetLength(), (WPARAM)pFile);//发送下载文件命令
		//m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this);
		//if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT) {//没有超时，线程错误
		//	return -1;
		//}
		m_remoteDlg.BeginWaitCursor();
		m_statusDlg.m_info.SetWindowText(_T("命令正在执行中，请稍后..."));
		m_statusDlg.ShowWindow(SW_SHOW);//显示状态对话框
		m_statusDlg.CenterWindow(&m_remoteDlg);
		m_statusDlg.SetActiveWindow();//设置为前台活动窗口
	}
	return 0;
}

void CClientController::StartWatchScreen()
{
	m_isClosed = false;
	//m_watchDlg.SetParent(&m_remoteDlg);//设置父窗口
	m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreen, 0, this);//创建监视线程
	m_watchDlg.DoModal();
	m_isClosed = true;
	WaitForSingleObject(m_hThreadWatch, 500);//等待线程结束
}

void CClientController::DownloadEnd()
{
	m_statusDlg.ShowWindow(SW_HIDE);//隐藏状态对话框
	m_remoteDlg.EndWaitCursor();//结束等待光标
	m_remoteDlg.MessageBox(_T("下载完成！！"), _T("完成"));
}

void CClientController::threadWatchScreen()
{
	Sleep(50);
	while (!m_isClosed) {
		if (m_watchDlg.isFull() == false) {
			std::list<CPacket> lstPacks;
			int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), 6, true, NULL, 0);
			//TODO：添加消息响应函数WM_SEND_PACK_ACK
			//TODO:控制发送频率
			if (ret == 6) { //获取屏幕数据
				if (CTool::Byte2Image(m_watchDlg.GetImage(), lstPacks.front().strData) == 0) { //将数据转换为图片，获取图片成功
					m_watchDlg.SetImageStatus(true);//设置图片缓存有数据
					TRACE("成功设置图片\r\n");
				}
				else {
					TRACE("获取图片失败！！%d\r\n", ret);
				}
			}
		}
		Sleep(1);
	}

}

void CClientController::threadWatchScreen(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadWatchScreen();
	_endthread();
}

void CClientController::threadDownloadFile()
{
	FILE* pFile = fopen(m_strLocal, "wb+");//打开文件,如果文件不存在则创建
	if (pFile == NULL) {
		AfxMessageBox(_T("本地没有权限保存该文件，或者文件无法创建！！"));
		m_statusDlg.ShowWindow(SW_HIDE);
		m_remoteDlg.EndWaitCursor();
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();
	do {
		int ret = SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
		long long nLength = *(long long*)pClient->GetPacket().strData.c_str();//获取文件长度
		if (nLength == 0) {
			AfxMessageBox("文件长度为0或无法读取文件！！");
			fclose(pFile);
			pClient->CloseSocket();
			return;
		}
		long long nCount = 0;
		while (nCount < nLength) {
			int ret = pClient->DealCommand();
			if (ret < 0) {
				AfxMessageBox("传输文件失败！！");
				TRACE("传输文件失败：ret = %d\r\n", ret);
				break;
			}
			//写文件,1表示每次写一个字节,写入的数据为pClient->GetPacket().strData.c_str()
			fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
			nCount += pClient->GetPacket().strData.size();
		}
	} while (false);
	fclose(pFile);
	pClient->CloseSocket();
	m_statusDlg.ShowWindow(SW_HIDE);//隐藏状态对话框
	m_remoteDlg.EndWaitCursor();//结束等待光标
	m_remoteDlg.MessageBox(_T("下载完成！！"), _T("完成"));

}

void CClientController::threadDownloadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadDownloadFile();
	_endthread();
}

void CClientController::threadFunc()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {//MFC线程消息循环
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_SEND_MESSAGE) {
			MSGINFO* pmsg = (MSGINFO*)msg.wParam;
			HANDLE hEvent = (HANDLE)msg.lParam;
			std::unordered_map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(pmsg->msg.message);// 查找消息处理函数
			if (it != m_mapFunc.end()) {
				pmsg->result = (this->*(it->second))(pmsg->msg.message, pmsg->msg.wParam, pmsg->msg.lParam);

			}
			else {
				pmsg->result = -1;
			}
		}
	}
	std::unordered_map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
	if (it != m_mapFunc.end()) {
		(this->*(it->second))(msg.message, msg.wParam, msg.lParam);
	}
}

unsigned __stdcall CClientController::threadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;//获取当前对象
	thiz->threadFunc();
	_endthreadex(0);
	return 0;
}

//LRESULT CClientController::OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
//{
//	CClientSocket* pClient = CClientSocket::getInstance();
//	CPacket* pPacket = (CPacket*)wParam;
//	return pClient->Send(*pPacket);
//}

//LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
//{
//	CClientSocket* pClient = CClientSocket::getInstance();
//	char* pBuffer = (char*)wParam;
//	return pClient->Send(pBuffer, (int)lParam);
//}

LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}
