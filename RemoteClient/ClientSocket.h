#pragma once
#include "framework.h"
#include "pch.h"
#include <string>
#include <ws2tcpip.h>//inet_pton
#include <vector>
#include <unordered_map>

#pragma pack(push)
#pragma pack(1)

class CPacket
{
public:
    CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
    CPacket(WORD nCmd, const BYTE* pData, size_t nSize, HANDLE hEvent) {//������캯��
        sHead = 0xFEFF;
        nLength = nSize + 4;
        sCmd = nCmd;
        if (nSize > 0) {
            strData.resize(nSize);
            memcpy((void*)strData.c_str(), pData, nSize);
        }
        else {
            strData.clear();
        }
        sSum = 0;
        for (size_t j = 0; j < strData.size(); j++) {
            sSum += BYTE(strData[j]) & 0xFF;
        }
        this->hEvent = hEvent;
    }
    CPacket(const CPacket& pack) {//�������캯��
        sHead = pack.sHead;
        nLength = pack.nLength;
        strData = pack.strData;
        sCmd = pack.sCmd;
        sSum = pack.sSum;
        hEvent = pack.hEvent;
    }
    CPacket& operator=(const CPacket& pack) {//��ֵ���캯��
        if (this == &pack) return *this;//��ֹ�Ը�ֵ
        sHead = pack.sHead;
        nLength = pack.nLength;
        strData = pack.strData;
        sCmd = pack.sCmd;
        sSum = pack.sSum;
        hEvent = pack.hEvent;
        return *this;
    }
	CPacket(const BYTE* pData, size_t& nSize) :sHead(0), nLength(0), sCmd(0), sSum(0), hEvent(INVALID_HANDLE_VALUE)
    {//������캯�����ڽ������ݰ���������캯��
        size_t i = 0;
        for (i = 0; i < nSize; i++) {
            if (*(WORD*)(pData + i) == 0xFEFF) {
                sHead = *(WORD*)(pData + i);//�ҵ���ͷ
                i += 2;
                break;
            }
        }
        if (i + 4 + 2 + 2 > nSize) {//��ͷ+����+����+У��ͣ������ݲ�ȫ���ͷδ�ҵ�
            nSize = 0;
            return;//û���ҵ���ͷ,����ʧ��
        }
        nLength = *(DWORD*)(pData + i);//����
        i += 4;
        if (nLength + i > nSize) {//��������,��δ��������������ʧ��
            nSize = 0;
            return;
        }
        sCmd = *(WORD*)(pData + i);//����
        i += 2;
        if (nLength > 4) {
            //nLength��sCmd��ʼ��У��ͽ���,���Լ�ȥ4
            strData.resize(nLength - 2 - 2);//����-����-У���
            memcpy((void*)strData.c_str(), pData + i, nLength - 4);
            //TRACE("%s\r\n", strData.c_str() + 12);
            i += nLength - 4;
        }
        sSum = *(WORD*)(pData + i);//У���
        i += 2;
        WORD sum = 0;
        for (size_t j = 0; j < strData.size(); j++) {
            sum += BYTE(strData[j]) & 0xFF;
        }
        if (sum == sSum) {
            nSize = i;
            return;
        }
        nSize = 0;
    };
    ~CPacket()
    {

    };
    int Size() {
        return nLength + 6;//��ͷ+����+�����ݣ������ݵĴ�С
    }
    const char* Data(std::string& strOut) const {//����������������,���Ƶ�������strOut��
        strOut.resize(nLength + 6);
        BYTE* pData = (BYTE*)strOut.c_str();
        *(WORD*)pData = sHead;//��ͷ
		pData += 2;
        *(DWORD*)(pData) = nLength;//����
        pData += 4;
        *(WORD*)pData = sCmd;//����
        pData += 2;
        memcpy(pData, strData.c_str(), strData.size());//������
        pData += strData.size();
        *(WORD*)pData = sSum;//У���
        return strOut.c_str();
    }
public:
    WORD sHead;//��ͷ���̶�ΪFE FF
    DWORD nLength;//�������ӿ������ʼ��У��ͽ�����
    std::string strData;//������
    WORD sCmd;//��������
    WORD sSum;//У���
	HANDLE hEvent;//�¼����
};


#pragma pack(pop)

typedef struct MouseEvent {
    MouseEvent() {
        nAction = 0;
        nButton = -1;
        ptXY.x = 0, ptXY.y = 0;
    }
    WORD nAction;//����� �ƶ��� ˫��
    WORD nButton;//����� �Ҽ��� �м�
    POINT ptXY;//����

}MOUSEEV, * PMOUSEEV;

typedef struct file_info {
    file_info() {//�ṹ�幹�캯��
        IsInvalid = 0;
        IsDirectory = -1;
        HasNext = 1;
        memset(szFileName, 0, sizeof szFileName);
    }
    BOOL IsInvalid;//�Ƿ�Ϊ��ЧĿ¼
    BOOL IsDirectory;//�Ƿ�ΪĿ¼�� 0 �� 1 ��
    BOOL HasNext;//�Ƿ��к����� 0 û�У� 1 ��
    char szFileName[256];//�ļ���
}FILEINFO, * PFILEINFO;

std::string GetErrInfo(int WSAErrCode);
void Dump(BYTE* pData, size_t nSize);

class CClientSocket
{
public:
    //static CClientSocket& GetInstance() {
    //	static CClientSocket instance;
    //	return instance;
    //}
    static CClientSocket* getInstance() {//����ģʽ��ÿ��ֻ����һ��ʵ��
        if (m_instance == NULL) {//��̬����û��thisָ�룬���Բ��ܷ��ʷǾ�̬��Ա����
            m_instance = new CClientSocket();
        }
        return m_instance;
    }
    bool InitSocket() {//�ͻ�����Ҫ����IP��ַ
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
#define BUFFER_SIZE 4096000
    int DealCommand() {
        if (m_sock == -1) return -1;
        //char buffer[1024];
        //char* buffer = new char[BUFFER_SIZE];
        //����˽����ǵ���������Ƿ���˻ᷢ�ͺܴ�İ������Խ��ն���������ֻ��Ҫ��һ��new char����
        //�ͻ��˵Ļ�Ҫ��vector���մ�����ݰ�
		char* buffer = m_buffer.data();//��vector��data��������ָ�������ָ�룬���õ����ڴ�й©
        //���̷߳�������ʱ���ܻ���ֳ�ͻ�����ܽ��յ�ͼƬ��Ҳ�п��ܽ��ܵ����
		static size_t index = 0;//δ�������ݵĳ���,��������Ϊ��̬������
        //����ΪDealCommand��һ��ѭ��������ÿ�ε��ö����õ�index����������óɾ�̬������
        // ÿ�ε���DealCommand�����ʼ��index
        while (true) {
            size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);//���շ�����������
            if (((int)len <= 0) && ((int)index <= 0)) {
                return -1;
            }
            //TODO:��������
			Dump((BYTE*)buffer, index);//��ӡ���յ�������
            index += len;//δ�������ݵĳ��ȶ���len
            len = index;
            m_packet = CPacket((BYTE*)buffer, len);
            if (len > 0) {//�����ɹ���len�ǽ����ɹ��İ���
                memmove(buffer, buffer + len, index - len);
				index -= len;//����δ�������ݵĳ��ȣ����ﴦ����len�����Լ�ȥlen
                return m_packet.sCmd;
            }
        }
        return -1;
    }
    
    bool SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed = true); 
    
    bool GetFilePath(std::string& strPath) {
        if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4)) {//��ȡ�ļ��б�����2, ���ļ���3
            strPath = m_packet.strData;
            return true;
        }
        return false;
    }
    bool GetMouseEvent(MOUSEEV& mouse) {
        if (m_packet.sCmd == 5) {
            memcpy(&mouse, m_packet.strData.c_str(), sizeof MOUSEEV);
            return true;
        }
        return false;
    }
    CPacket& GetPacket() {
        return m_packet;
    }
	void CloseSocket() {
		closesocket(m_sock);
        m_sock = INVALID_SOCKET;//-1
	}
    void UpdateAddress(int nIP, int nPort) {
        if (m_nIP != nIP || m_nPort != nPort) {
            m_nIP = nIP;
            m_nPort = nPort;
        }
    }
private:
    bool m_bAutoClose;

    std::list<CPacket> m_lstSend;
    std::unordered_map<HANDLE, std::list<CPacket>> m_mapAck; 
	std::unordered_map<HANDLE, bool> m_mapAutoCLosed;//�¼��Ƿ��Զ��رյı�־
	int m_nIP;//IP��ַ
	int m_nPort;//�˿�

    std::vector<char> m_buffer;
    SOCKET m_sock;
    CPacket m_packet;
    CClientSocket& operator=(const CClientSocket& ss) {//��ֵ���캯��

    }
    CClientSocket(const CClientSocket& ss) {//���ƹ��캯��
        m_bAutoClose = ss.m_bAutoClose;
        m_sock == ss.m_sock;
        m_nIP = ss.m_nIP;
		m_nPort = ss.m_nPort;
    }
    CClientSocket() :m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), m_bAutoClose(true) {
        if (InitSockEnv() == FALSE) {
            MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���,�����������ã�"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
            exit(0);
        }
		m_buffer.resize(BUFFER_SIZE);
        memset(m_buffer.data(), 0, BUFFER_SIZE);
		//m_sock = socket(PF_INET, SOCK_STREAM, 0);//IPV4, TCP������˿��ԣ��ͻ��˲���

    };
    ~CClientSocket() {
        closesocket(m_sock);
        WSACleanup();
    };
    static void threadEntry(void* arg);
    void threadFunc();
    BOOL InitSockEnv() {
        WSADATA data;
        if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
            return FALSE;
        }
        return TRUE;
    }
    static CClientSocket* m_instance;
    static void releaseInstance() {//�ͷ�ʵ��
        if (m_instance != NULL) {
            CClientSocket* p = m_instance;
            m_instance = NULL;
            delete p;
        }
    }
    bool Send(const char* pData, int size) {
        if (m_sock == -1) return false;
        return send(m_sock, pData, size, 0) > 0;
    }
    bool Send(const CPacket& pack);
    class CHelper {
    public:
        CHelper() {
            CClientSocket::getInstance();
        }
        ~CHelper() {
            CClientSocket::releaseInstance();
        }

    };
    static CHelper m_helper;
};



