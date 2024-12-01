#include "pch.h"
#include "ClientSocket.h"

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
	while (m_sock != INVALID_SOCKET) {
		if (m_lstSend.size() > 0) {//�����ݷ��ͣ� ֻ�е����Ͷ���������ʱ�ŷ��ͣ�����һֱ�ȴ�
			TRACE("lstSend size: %d\r\n", m_lstSend.size());
			CPacket& head = m_lstSend.front();
			if (Send(head) == false) {
				TRACE("����ʧ�ܣ�\r\n");
				continue;
			}//���Ŷӵķ�ʽ�������
			m_mapAck[head.hEvent] = std::list<CPacket>();//��map�л�������¼�����һ��list����
			int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);//��������
			if (length > 0 || index > 0) {
				index += length;//index������յ��������ܳ���
				size_t size = (size_t)index;
				CPacket pack((BYTE*)pBuffer, size);//�������յ��İ�
				pack.hEvent = head.hEvent;

				if (size > 0) {//�������ɹ��� TODO:�����ļ�����Ϣ��ȡ���ļ���Ϣ��ȡ���ܲ�������
					//֪ͨ��Ӧ���¼�
					SetEvent(head.hEvent);//�����¼�Ϊ���ź�״̬
					std::list<CPacket>().push_back(pack); //���������İ��������ǰ�¼�list����
				}
			}
			else if (length == 0 && index <= 0) {
				CloseSocket();
			}
			m_lstSend.pop_front();//ɾ����
		}
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
