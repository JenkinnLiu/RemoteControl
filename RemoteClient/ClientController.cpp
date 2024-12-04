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
		//����Ĵ����ǽ���Ϣ����Ϣ��������������
		struct { UINT nMsg; MSGFUNC func; }MsgFuncs[] = {
			//{ WM_SEND_PACK, &CClientController::OnSendPack },
			//{ WM_SEND_DATA, &CClientController::OnSendData },
			{ WM_SHOW_STATUS, &CClientController::OnShowStatus },
			{ WM_SHOW_WATCH, &CClientController::OnShowWatcher },
			{(UINT)-1, NULL }
		};
		for (int i = 0; MsgFuncs[i].func != NULL; i++) {//��map�������Ϣ����Ϣ������
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
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);//����һ���¼�
	if (hEvent == NULL) return -2;
	MSGINFO info(msg);
	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE, (WPARAM)&info, (LPARAM)hEvent);
	WaitForSingleObject(hEvent, INFINITE);//�ȴ��¼�
	CloseHandle(hEvent);  
	return info.result;

}

int CClientController::DownFile(CString strPath)
{
	CFileDialog dlg(FALSE, NULL,
		strPath, OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
		NULL, &m_remoteDlg);//���ļ��Ի���,OFN_OVERWRITEPROMPT��ʾ����ļ��Ѿ����ڣ�ѯ���Ƿ񸲸ǣ�OFN_HIDEREADONLY��ʾ����ֻ����ѡ��
	if (dlg.DoModal() == IDOK) {
		m_strRemote = strPath;
		m_strLocal = dlg.GetPathName();//�����ļ�·��
		CString strLocal = dlg.GetPathName();//��ȡ�ļ�·��
		FILE* pFile = fopen(m_strLocal, "wb+");//���ļ�,����ļ��������򴴽�
		if (pFile == NULL) {
			AfxMessageBox(_T("����û��Ȩ�ޱ�����ļ��������ļ��޷���������"));
			m_statusDlg.ShowWindow(SW_HIDE);
			m_remoteDlg.EndWaitCursor();
			return -1;
		}
		SendCommandPacket(m_remoteDlg, 4, true, (BYTE*)(LPCSTR)strPath, strPath.GetLength(), (WPARAM)pFile);//���������ļ�����
		//m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this);
		//if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT) {//û�г�ʱ���̴߳���
		//	return -1;
		//}
		m_remoteDlg.BeginWaitCursor();
		m_statusDlg.m_info.SetWindowText(_T("��������ִ���У����Ժ�..."));
		m_statusDlg.ShowWindow(SW_SHOW);//��ʾ״̬�Ի���
		m_statusDlg.CenterWindow(&m_remoteDlg);
		m_statusDlg.SetActiveWindow();//����Ϊǰ̨�����
	}
	return 0;
}

void CClientController::StartWatchScreen()
{
	m_isClosed = false;
	//m_watchDlg.SetParent(&m_remoteDlg);//���ø�����
	m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreen, 0, this);//���������߳�
	m_watchDlg.DoModal();
	m_isClosed = true;
	WaitForSingleObject(m_hThreadWatch, 500);//�ȴ��߳̽���
}

void CClientController::DownloadEnd()
{
	m_statusDlg.ShowWindow(SW_HIDE);//����״̬�Ի���
	m_remoteDlg.EndWaitCursor();//�����ȴ����
	m_remoteDlg.MessageBox(_T("������ɣ���"), _T("���"));
}

void CClientController::threadWatchScreen()
{
	Sleep(50);
	while (!m_isClosed) {
		if (m_watchDlg.isFull() == false) {
			std::list<CPacket> lstPacks;
			int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), 6, true, NULL, 0);
			//TODO�������Ϣ��Ӧ����WM_SEND_PACK_ACK
			//TODO:���Ʒ���Ƶ��
			if (ret == 6) { //��ȡ��Ļ����
				if (CTool::Byte2Image(m_watchDlg.GetImage(), lstPacks.front().strData) == 0) { //������ת��ΪͼƬ����ȡͼƬ�ɹ�
					m_watchDlg.SetImageStatus(true);//����ͼƬ����������
					TRACE("�ɹ�����ͼƬ\r\n");
				}
				else {
					TRACE("��ȡͼƬʧ�ܣ���%d\r\n", ret);
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
	FILE* pFile = fopen(m_strLocal, "wb+");//���ļ�,����ļ��������򴴽�
	if (pFile == NULL) {
		AfxMessageBox(_T("����û��Ȩ�ޱ�����ļ��������ļ��޷���������"));
		m_statusDlg.ShowWindow(SW_HIDE);
		m_remoteDlg.EndWaitCursor();
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();
	do {
		int ret = SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
		long long nLength = *(long long*)pClient->GetPacket().strData.c_str();//��ȡ�ļ�����
		if (nLength == 0) {
			AfxMessageBox("�ļ�����Ϊ0���޷���ȡ�ļ�����");
			fclose(pFile);
			pClient->CloseSocket();
			return;
		}
		long long nCount = 0;
		while (nCount < nLength) {
			int ret = pClient->DealCommand();
			if (ret < 0) {
				AfxMessageBox("�����ļ�ʧ�ܣ���");
				TRACE("�����ļ�ʧ�ܣ�ret = %d\r\n", ret);
				break;
			}
			//д�ļ�,1��ʾÿ��дһ���ֽ�,д�������ΪpClient->GetPacket().strData.c_str()
			fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
			nCount += pClient->GetPacket().strData.size();
		}
	} while (false);
	fclose(pFile);
	pClient->CloseSocket();
	m_statusDlg.ShowWindow(SW_HIDE);//����״̬�Ի���
	m_remoteDlg.EndWaitCursor();//�����ȴ����
	m_remoteDlg.MessageBox(_T("������ɣ���"), _T("���"));

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
	while (::GetMessage(&msg, NULL, 0, 0)) {//MFC�߳���Ϣѭ��
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_SEND_MESSAGE) {
			MSGINFO* pmsg = (MSGINFO*)msg.wParam;
			HANDLE hEvent = (HANDLE)msg.lParam;
			std::unordered_map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(pmsg->msg.message);// ������Ϣ������
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
	CClientController* thiz = (CClientController*)arg;//��ȡ��ǰ����
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
