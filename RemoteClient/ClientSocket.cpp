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

bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed)
{
	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE) {
		//if (InitSocket() == false) return false;
		m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);//����һ���߳�
	}
	m_lock.lock();//����, ��֤�̰߳�ȫ, ��֤�¼����еİ�ȫ
	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(pack.hEvent, lstPacks));//��map�л�������¼�����һ��list����
	m_mapAutoCLosed[pack.hEvent] = isAutoClosed;//�Ƿ��Զ��ر�
	m_lstSend.push_back(pack);
	m_lock.unlock();//����,��֤�¼�����ֻ����һ���߳�push_back����ֹ���߳�push_back�������ݻ���
	WaitForSingleObject(pack.hEvent, INFINITE);//�ȴ��¼�
	std::unordered_map<HANDLE, std::list<CPacket>&>::iterator it;
	it = m_mapAck.find(pack.hEvent);//�����¼�
	if (it != m_mapAck.end()) {
		m_lock.lock();
		m_mapAck.erase(it);//ɾ���¼�, �¼�ֻ�ܴ���һ��
		m_lock.unlock();
		return true;
	}
	return false;

}

void CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc();
}

void CClientSocket::threadFunc()
{
	//�����¼����ƺ��¼����У�ʵ���¼����ͬ����
	// ʹ��һ���¼��������֮ǰ��һ���¼����ᴦ��Ҳ����˵Ҫ�Ŷ�
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int index = 0;
	InitSocket();
	while (m_sock != INVALID_SOCKET) {
		if (m_lstSend.size() > 0) {//�����ݷ��ͣ� ֻ�е����Ͷ���������ʱ�ŷ��ͣ�����һֱ�ȴ�
			TRACE("lstSend size: %d\r\n", m_lstSend.size());
			m_lock.lock();//����
			CPacket& head = m_lstSend.front();
			m_lock.unlock();//����, ��ֻ֤��һ���̻߳�ȡfront
			if (Send(head) == false) {
				TRACE("����ʧ�ܣ�\r\n");
				
				continue;
			}//���Ŷӵķ�ʽ�������
			std::unordered_map<HANDLE, std::list<CPacket>&>::iterator it;
			it = m_mapAck.find(head.hEvent);
			if (it != m_mapAck.end()) {//���û���ҵ��¼����ʹ���һ���¼�
				std::unordered_map<HANDLE, bool>::iterator it0 = m_mapAutoCLosed.find(head.hEvent);
				do {
					int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);//��������
					if (length > 0 || index > 0) {
						index += length;//index������յ��������ܳ���
						size_t size = (size_t)index;
						CPacket pack((BYTE*)pBuffer, size);//�������յ��İ�
						pack.hEvent = head.hEvent;

						if (size > 0) {//�������ɹ��� TODO:�����ļ�����Ϣ��ȡ���ļ���Ϣ��ȡ���ܲ�������
							//֪ͨ��Ӧ���¼�
							pack.hEvent = head.hEvent;
							it->second.push_back(pack); //���������İ��������ǰ�¼�list����
							memmove(pBuffer, pBuffer + size, index - size);//�����յ�������ǰ��
							index -= size; //���յ������ݳ��ȼ�ȥ�Ѿ���������ݳ���, Ϊ��һ�ν���������׼��
							if (it0->second) {//����Զ��رգ���֪ͨ�¼����
								SetEvent(head.hEvent);
							}
						}
					}
					else if (length == 0 && index <= 0) {
						CloseSocket();
						SetEvent(head.hEvent);//�ȵ��������ر��������֪ͨ�¼����
						m_mapAutoCLosed.erase(it0);
						break;
					}
				} while (it0->second == false || index > 0);//������Զ��رգ���һֱ��������
			}
			m_lock.lock();//����,
			m_lstSend.pop_front();//ɾ����
			m_lock.unlock();//����, ��ֻ֤��һ���߳�pop_front
			if (InitSocket() == false) {
				InitSocket();
			}
		}
		Sleep(1);//û�����ݷ��ͣ�������1ms,��ֹCPUռ�ù���
	}
	CloseSocket();

}

bool CClientSocket::Send(const CPacket& pack)
{
	TRACE("�������send�������ݣ�m_sock == %d\r\n", m_sock);
	if (m_sock == -1) return false;
	std::string strOut;
	pack.Data(strOut);
	return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;
}
