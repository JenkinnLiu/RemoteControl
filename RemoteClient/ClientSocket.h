#pragma once
#include "framework.h"
#include "pch.h"
#include <string>
#include <ws2tcpip.h>//inet_pton

#pragma pack(push)
#pragma pack(1)

class CPacket
{
public:
    CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
    CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {//������캯��
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
    }
    CPacket(const CPacket& pack) {//�������캯��
        sHead = pack.sHead;
        nLength = pack.nLength;
        strData = pack.strData;
        sCmd = pack.sCmd;
        sSum = pack.sSum;
    }
    CPacket& operator=(const CPacket& pack) {//��ֵ���캯��
        if (this == &pack) return *this;//��ֹ�Ը�ֵ
        sHead = pack.sHead;
        nLength = pack.nLength;
        strData = pack.strData;
        sCmd = pack.sCmd;
        sSum = pack.sSum;
        return *this;
    }
    CPacket(const BYTE* pData, size_t& nSize) :sHead(0), nLength(0), sCmd(0), sSum(0)
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
    const char* Data() {//����������������,���Ƶ�������strOut��
        strOut.resize(nLength + 6);
        BYTE* pData = (BYTE*)strOut.c_str();
        *(WORD*)pData = sHead;//��ͷ
        *(DWORD*)(pData + 2) = nLength;//����
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
    std::string strOut;//������������
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

std::string GetErrorInfo(int WSAErrCode) {
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
    bool InitSocket(const std::string& strIPAdress) {//�ͻ�����Ҫ����IP��ַ

        if (m_sock == -1) return false;
        sockaddr_in serv_adr;
        memset(&serv_adr, 0, sizeof serv_adr);
        serv_adr.sin_family = AF_INET;//IPV4��ַ��
		//���inet_addr��C4996, ��inet_pton, inet_addr�ǹ�ʱ�ĺ��������������ԣ�Ԥ�����������м���WINSOCK_DEPRECATED_NO_WARNINGS
        //serv_adr.sin_addr.s_addr = inet_addr(strIPAdress.c_str());//������IP�ϼ���
		serv_adr.sin_addr.s_addr = inet_pton(AF_INET, strIPAdress.c_str(), &serv_adr.sin_addr.s_addr);

        serv_adr.sin_port = htons(9527);
        if (serv_adr.sin_addr.s_addr == INADDR_NONE) {
			AfxMessageBox("��Ч��IP��ַ��");
            return false;
        }
		int ret = connect(m_sock, (sockaddr*)&serv_adr, sizeof serv_adr);//���ӷ�����
        if (ret == -1) {
			//����Ҫ����ʹ�ö��ֽ��ַ�������ʹ��Unicode, ��Ȼ�ᱨ����
			AfxMessageBox("�޷����ӷ�����,�����������ã�");//AfxMessagBox��MFC����Ϣ��
			TRACE("����ʧ�ܣ� %d %s\r\n", WSAGetLastError(), GetErrorInfo(WSAGetLastError()).c_str());
        }
        return true;
    }
#define BUFFER_SIZE 4096
    int DealCommand() {
        if (m_sock == -1) return -1;
        //char buffer[1024];
        char* buffer = new char[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        size_t index = 0;//δ�������ݵĳ���
        while (true) {
            size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
            if (len <= 0) return -1;
            //TODO:��������
            index += len;//δ�������ݵĳ��ȶ���len
            len = index;
            m_packet = CPacket((BYTE*)buffer, len);
            if (len > 0) {//�����ɹ���len�ǽ����ɹ��İ���
                memmove(buffer, buffer + len, BUFFER_SIZE - len);
                index -= len;//δ�������ݵĳ��ȣ����ﴦ����len�����Լ�ȥlen
                return m_packet.sCmd;
            }
        }
        return -1;
    }
    bool Send(const char* pData, int size) {
        if (m_sock == -1) return false;
        return send(m_sock, pData, size, 0) > 0;
    }
    bool Send(CPacket& pack) {
        if (m_sock == -1) return false;
        return send(m_sock, pack.Data(), pack.Size(), 0) > 0;
    }
    bool GetFilePath(std::string& strPath) {
        if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4)) {//��ȡ�ļ��б������2, ���ļ���3
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
private:
    SOCKET m_sock;
    CPacket m_packet;
    CClientSocket& operator=(const CClientSocket& ss) {//��ֵ���캯��

    }
    CClientSocket(const CClientSocket& ss) {//���ƹ��캯��
        m_sock == ss.m_sock;

    }
    CClientSocket() {
        if (InitSockEnv() == FALSE) {
            MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���,�����������ã�"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
            exit(0);
        }
        m_sock = socket(PF_INET, SOCK_STREAM, 0);//IPV4, TCP
    };
    ~CClientSocket() {
        closesocket(m_sock);
        WSACleanup();
    };
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


