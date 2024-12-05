#include "pch.h"
#include "ClientSocket.h"
#include <unordered_map>

CClientSocket* CClientSocket::m_instance = NULL;//��̬��Ա������ʼ��,������ʽ��ʼ��
CClientSocket::CHelper CClientSocket::m_helper;//ȷ�����캯��ȫ��Ψһ

CClientSocket* pclient = CClientSocket::getInstance();//����ģʽ��ÿ��ֻ����һ��ʵ��

std::string GetErrInfo(int WSAErrCode) {
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,//��ʽ����Ϣ
		NULL,
		WSAErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Ĭ������
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
	);//��ȡ������Ϣ,WSAErrCode�Ǵ�����
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}

void Dump(BYTE* pData, size_t nSize) {
	std::string strOut;
	for (size_t i = 0; i < nSize; i++) {
		char buf[8] = "";
		if (i > 0 && (i % 16 == 0)) strOut += "\n";
		snprintf(buf, sizeof buf, "%02X ", pData[i] & 0xFF);
		strOut += buf;
	}
	strOut += "\n";
	OutputDebugStringA(strOut.c_str());
}

CClientSocket::CClientSocket(const CClientSocket& ss) {
	m_bAutoClose = ss.m_bAutoClose;
	m_sock == ss.m_sock;
	m_nIP = ss.m_nIP;
	m_nPort = ss.m_nPort;
	m_hThread = INVALID_HANDLE_VALUE;
	std::unordered_map<UINT, CClientSocket::MSGFUNC>::const_iterator it = ss.m_mapFunc.begin();
	for (; it != ss.m_mapFunc.end(); it++) {
		m_mapFunc.insert(std::pair<UINT, MSGFUNC>(it->first, it->second));
	}
}

CClientSocket::CClientSocket() :m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), m_bAutoClose(true),
m_hThread(INVALID_HANDLE_VALUE)
{
	if (InitSockEnv() == FALSE) {
		MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���,�����������ã�"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL);//����һ���¼�
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);//����һ���߳�
	if (WaitForSingleObject(m_eventInvoke, 100) == WAIT_TIMEOUT) {//�ȴ��¼�
		TRACE("������Ϣ�����߳�����ʧ�ܣ�\r\n");
	}
	CloseHandle(m_eventInvoke);//�ر��¼�
	m_buffer.resize(BUFFER_SIZE);
	memset(m_buffer.data(), 0, BUFFER_SIZE);
	//m_sock = socket(PF_INET, SOCK_STREAM, 0);//IPV4, TCP������˿��ԣ��ͻ��˲���
	struct {
		UINT message;
		MSGFUNC func;
	}funcs[] = {
		{WM_SEND_PACK, &CClientSocket::SendPack},
		{0, NULL}
	};
	for (int i = 0; funcs[i].message != 0; i++) {
		if (m_mapFunc.insert(std::pair<UINT, MSGFUNC>(funcs[i].message, funcs[i].func)).second == false) {
			TRACE("������Ϣ������ʧ�ܣ� ��Ϣֵ%d. ����ֵ: %08X ���: %d\r\n", funcs[i].message, funcs[i].func, i);
			exit(0);

		}
	}

}

bool CClientSocket::InitSocket()
{//�ͻ�����Ҫ����IP��ַ
	if (m_sock != INVALID_SOCKET) CloseSocket();//����Ѿ����ӣ��ȹر�socket���ؿ�һ��
	m_sock = socket(PF_INET, SOCK_STREAM, 0);//IPV4, TCP,�ͻ�����Ҫ���½���socket
	if (m_sock == -1) return false;
	sockaddr_in serv_adr;
	memset(&serv_adr, 0, sizeof serv_adr);
	serv_adr.sin_family = AF_INET;//IPV4��ַ��
	//���inet_addr��C4996, ��inet_pton, inet_addr�ǹ�ʱ�ĺ��������������ԣ�Ԥ�����������м���WINSOCK_DEPRECATED_NO_WARNINGS
	//serv_adr.sin_addr.s_addr = inet_addr(strIPAdress.c_str());//������IP�ϼ���
	//int ret = inet_pton(AF_INET, htol(nIP), &serv_adr.sin_addr.s_addr);

	//ʹ�� sprintf_s ������������ʽ�� IP ��ַ nIP ת��Ϊ�ַ�����ʽ�����洢�� strIP �С�
	//nIP >> 24��nIP >> 16��nIP >> 8 �� nIP �ֱ���ȡ IP ��ַ���ĸ��ֽڣ���ʹ�� & 0xFF ȷ��ÿ���ֽڵ�ֵ�� 0 �� 255 ֮�䡣
	char strIP[INET_ADDRSTRLEN];
	sprintf_s(strIP, "%d.%d.%d.%d", (m_nIP >> 24) & 0xFF, (m_nIP >> 16) & 0xFF, (m_nIP >> 8) & 0xFF, m_nIP & 0xFF);
	int ret = inet_pton(AF_INET, strIP, &serv_adr.sin_addr.s_addr);
	if (ret == 1) {
		TRACE("inet_pton success!\r\n");
	}
	else {
		TRACE("inet_pton failed, %d %s\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
	}
	serv_adr.sin_port = htons(m_nPort);
	if (serv_adr.sin_addr.s_addr == INADDR_NONE) {
		AfxMessageBox("��Ч��IP��ַ��");
		return false;
	}
	ret = connect(m_sock, (sockaddr*)&serv_adr, sizeof serv_adr);//���ӷ�����
	if (ret == -1) {
		//����Ҫ����ʹ�ö��ֽ��ַ�������ʹ��Unicode, ��Ȼ�ᱨ��
		AfxMessageBox("�޷����ӷ�����,�����������ã�");//AfxMessagBox��MFC����Ϣ��
		TRACE("����ʧ�ܣ� %d %s\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
	}
	return true;
}

bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed, WPARAM wParam) {
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;//������Զ��رգ��������Զ��رձ�־
	std::string strOut;
	pack.Data(strOut);
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam);//����һ��PACKET_DATA�ṹ
	bool ret = PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam), (LPARAM)hWnd);//������Ϣ
	if (ret == false) {
		delete pData;
	}
	return ret;
}

//bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed)
//{
//	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE) {
//		//if (InitSocket() == false) return false;
//		m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);//����һ���߳�
//	}
//	m_lock.lock();//����, ��֤�̰߳�ȫ, ��֤�¼����еİ�ȫ
//	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(pack.hEvent, lstPacks));//��map�л�������¼�����һ��list����
//	m_mapAutoCLosed[pack.hEvent] = isAutoClosed;//�Ƿ��Զ��ر�
//	m_lstSend.push_back(pack);
//	m_lock.unlock();//����,��֤�¼�����ֻ����һ���߳�push_back����ֹ���߳�push_back�������ݻ���
//	WaitForSingleObject(pack.hEvent, INFINITE);//�ȴ��¼�
//	std::unordered_map<HANDLE, std::list<CPacket>&>::iterator it;
//	it = m_mapAck.find(pack.hEvent);//�����¼�
//	if (it != m_mapAck.end()) {
//		m_lock.lock();
//		m_mapAck.erase(it);//ɾ���¼�, �¼�ֻ�ܴ���һ��
//		m_lock.unlock();
//		return true;
//	}
//	return false;
//
//}

unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc2();
	_endthreadex(0);
	return 0;
}

//void CClientSocket::threadFunc()
//{
//	//�����¼����ƺ��¼����У�ʵ���¼����ͬ����
//	// ʹ��һ���¼��������֮ǰ��һ���¼����ᴦ��Ҳ����˵Ҫ�Ŷ�
//	std::string strBuffer;
//	strBuffer.resize(BUFFER_SIZE);
//	char* pBuffer = (char*)strBuffer.c_str();
//	int index = 0;
//	InitSocket();
//	while (m_sock != INVALID_SOCKET) {
//		if (m_lstSend.size() > 0) {//�����ݷ��ͣ� ֻ�е����Ͷ���������ʱ�ŷ��ͣ�����һֱ�ȴ�
//			TRACE("lstSend size: %d\r\n", m_lstSend.size());
//			m_lock.lock();//����
//			CPacket& head = m_lstSend.front();
//			m_lock.unlock();//����, ��ֻ֤��һ���̻߳�ȡfront
//			if (Send(head) == false) {
//				TRACE("����ʧ�ܣ�\r\n");
//				
//				continue;
//			}//���Ŷӵķ�ʽ�������
//			std::unordered_map<HANDLE, std::list<CPacket>&>::iterator it;
//			it = m_mapAck.find(head.hEvent);
//			if (it != m_mapAck.end()) {//���û���ҵ��¼����ʹ���һ���¼�
//				std::unordered_map<HANDLE, bool>::iterator it0 = m_mapAutoCLosed.find(head.hEvent);
//				do {
//					int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);//��������
//					if (length > 0 || index > 0) {
//						index += length;//index������յ��������ܳ���
//						size_t size = (size_t)index;
//						CPacket pack((BYTE*)pBuffer, size);//�������յ��İ�
//						pack.hEvent = head.hEvent;
//
//						if (size > 0) {//�������ɹ��� TODO:�����ļ�����Ϣ��ȡ���ļ���Ϣ��ȡ���ܲ�������
//							//֪ͨ��Ӧ���¼�
//							pack.hEvent = head.hEvent;
//							it->second.push_back(pack); //���������İ��������ǰ�¼�list����
//							memmove(pBuffer, pBuffer + size, index - size);//�����յ�������ǰ��
//							index -= size; //���յ������ݳ��ȼ�ȥ�Ѿ���������ݳ���, Ϊ��һ�ν���������׼��
//							if (it0->second) {//����Զ��رգ���֪ͨ�¼����
//								SetEvent(head.hEvent);
//							}
//						}
//					}
//					else if (length == 0 && index <= 0) {
//						CloseSocket();
//						SetEvent(head.hEvent);//�ȵ��������ر��������֪ͨ�¼����
//						if (it0 != m_mapAutoCLosed.end()) {
//							//m_mapAutoCLosed.erase(it0);
//						}
//						else {
//							TRACE("�쳣�����û���ҵ��¼���\r\n");
//						}
//						
//						break;
//					}
//				} while (it0->second == false || index > 0);//������Զ��رգ���һֱ��������
//			}
//			m_lock.lock();//����,
//			m_lstSend.pop_front();//ɾ����
//			m_mapAutoCLosed.erase(head.hEvent);
//			m_lock.unlock();//����, ��ֻ֤��һ���߳�pop_front
//			if (InitSocket() == false) {
//				InitSocket();
//			}
//		}
//		Sleep(1);//û�����ݷ��ͣ�������1ms,��ֹCPUռ�ù���
//	}
//	CloseSocket();
//
//}

void CClientSocket::threadFunc2()
{
	SetEvent(m_eventInvoke);//�����¼�
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end()) {//�����Ϣ����������
			MSGFUNC func = m_mapFunc[msg.message];	//��ȡ��Ϣ������
			(this->*func)(msg.message, msg.wParam, msg.lParam);//�ú���ָ�������Ϣ������
		}
	}
}

bool CClientSocket::Send(const CPacket& pack)
{
	TRACE("�������send�������ݣ�m_sock == %d\r\n", m_sock);
	if (m_sock == -1) return false;
	std::string strOut;
	pack.Data(strOut);
	return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;
}

void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{//����һ����Ϣ�����ݽṹ(���ݺ����ݳ��ȣ� ģʽ)�� �ص���Ϣ�����ݽṹ��HWND, MESSAGE��
	PACKET_DATA data = *(PACKET_DATA*)wParam;//��wParamת��ΪPACKET_DATA����,��ȡ��������
	delete (PACKET_DATA*)wParam; //�ͷ��ڴ�
	size_t nTemp = data.strData.size();
	CPacket current((BYTE*)data.strData.c_str(), nTemp);//���
	if (InitSocket() == true) {
		
		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0);
		if (ret > 0) {
			size_t index = 0;
			std::string strBuffer;
			strBuffer.resize(BUFFER_SIZE);
			char* pBuffer = (char*)strBuffer.c_str();//��stringת��Ϊchar*
			while (m_sock != INVALID_SOCKET) {
				int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);//��������
				if (length > 0) {
					index += (size_t)length;
					size_t nLen = index;
					CPacket pack((BYTE*)pBuffer, nLen);//���
					if (nLen > 0) {//����ɹ�
						::SendMessage((HWND)lParam, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);//֪ͨ��Ϣ������
						if (data.nMode & CSM_AUTOCLOSE) {//������Զ��ر�
							CloseSocket();//������Ϣ֮��͹ر�socket
							return;
						}
						index -= nLen;
						memmove(pBuffer, pBuffer + nLen, index);
					}
				}
				else {//TODO:�Է��ر����׽��֣����������豸�쳣
					CloseSocket();
					::SendMessage((HWND)lParam, WM_SEND_PACK_ACK, (WPARAM)new CPacket(current.sCmd, NULL, 0), 1);//֪ͨ��Ϣ������
				}
			}
		}
		else {
			CloseSocket();//������ֹ����
			::SendMessage((HWND)lParam, WM_SEND_PACK_ACK, NULL, -1);
		}
	}
	else {
		//TODO:������
		::SendMessage((HWND)lParam, WM_SEND_PACK_ACK, NULL, -2);
	}
}
