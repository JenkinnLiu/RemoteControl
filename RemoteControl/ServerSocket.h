#pragma once
#include "framework.h"
#include "pch.h"
#include <list>
#include "Packet.h"
#pragma pack(pop)

typedef int(*SOCKET_CALLBACK)(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket);
//�������ָ������ͨ�����ڻص������У������û�����һ�����ϸ�ǩ���ĺ�����
// �����ض��¼��������µ��øú��������磬���������У�����ʹ�ûص�����������յ������ݰ�
// ������״̬�ı仯��

class CServerSocket
{
public:
    //static CServerSocket& GetInstance() {
    //	static CServerSocket instance;
    //	return instance;
    //}
    static CServerSocket* getInstance() {//����ģʽ��ÿ��ֻ����һ��ʵ��
        if (m_instance == NULL) {//��̬����û��thisָ�룬���Բ��ܷ��ʷǾ�̬��Ա����
            m_instance = new CServerSocket();
        }
        return m_instance;
    }
    int Run(SOCKET_CALLBACK callback, void* arg, short port = 9257) {//Socket��ʼ���ص�����RunCommand
        bool ret = InitSocket(port);
        if (ret == false) return -1;
        std::list<CPacket> lstPackets;//���б�
        m_callback = callback;
        m_arg = arg;
        int count = 0;
        while (true) {
            if (AcceptClient() == false) {
                if (count >= 3) {
                    return -2;
                }
                count++;
            }
            int ret = DealCommand();
            if (ret > 0) {
                m_callback(m_arg, ret, lstPackets, m_packet);//ִ�лص�����
                while (lstPackets.size() > 0) {
                    Send(lstPackets.front());//���Ͱ�
                    lstPackets.pop_front();
                }

            }
            CloseClient();
        }
        return 0;
    }
protected:
    bool InitSocket(short port = 9527) {
        
        if (m_sock == -1) return false;
        sockaddr_in serv_adr;
        memset(&serv_adr, 0, sizeof serv_adr);
        serv_adr.sin_family = AF_INET;//IPV4��ַ��
        serv_adr.sin_addr.s_addr = INADDR_ANY;//������IP�ϼ���
        serv_adr.sin_port = htons(9527);
        if(bind(m_sock, (sockaddr*)&serv_adr, sizeof serv_adr) == -1)
            return false;
        if (listen(m_sock, 1) == -1) 
            return false;
		Run(m_callback, m_arg);
        return true;
    }

    bool AcceptClient() {
		TRACE("accept:�������Ѿ���ͻ������ӣ��ȴ��ͻ��˷�������\r\n");
        sockaddr_in client_adr;
        int cli_sz = sizeof client_adr;
        m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_sz);
        if (m_client == -1) return false;
        return true;
    }
#define BUFFER_SIZE 4096
    int DealCommand() {
        if (m_client == -1) return -1;
        //char buffer[1024];
        char* buffer = new char[BUFFER_SIZE];
        //����˽����ǵ���������Ƿ���˻ᷢ�ͺܴ�İ������Խ��ն���������ֻ��Ҫ��һ��new char����
        //�ͻ��˵Ļ�Ҫ��vector���մ�����ݰ�
        if (buffer == NULL) {
			TRACE("�ڴ治�㣬����ʧ�ܣ�\r\n");
            return -2;
        }
        memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;//δ�������ݵĳ���
        while (true) {
            size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
            if (len <= 0) {
                delete[] buffer;
                return -1;
            }
			TRACE("���������յ��������ݣ�server recv len:%d\r\n", len);
            //TODO:��������
			index += len;//δ�������ݵĳ��ȶ���len
            len = index;
			TRACE("buffer��len����ˣ�:%d\r\n", len);
			m_packet = CPacket((BYTE*)buffer, len);
			TRACE("���������յ��������ݣ�server recv len:%d, cmd:%d\r\n", len, m_packet.sCmd);
			if (len > 0) {//�����ɹ���len�ǽ����ɹ��İ���
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;//δ�������ݵĳ��ȣ����ﴦ����len�����Լ�ȥlen
				delete[] buffer;
                return m_packet.sCmd;
            }
        }
        delete[] buffer;
        return -1;
    }
    bool Send(const char* pData, int size) {
        if (m_client == -1) return false;
        return send(m_client, pData, size, 0) > 0;
    }
    bool Send(CPacket& pack) {
        if (m_client == -1) return false;
        //Dump((BYTE*)pack.Data(), pack.Size());
		//Sleep(1);//�����ӳ�1ms����Ϊ�������̫�죬���������ܻ��������·���ʧ�ܣ�����Ҫ���ӳٴ�����
        return send(m_client, pack.Data(), pack.Size(), 0) > 0;
    }
	void CloseClient() {
		if (m_client != INVALID_SOCKET) {
			closesocket(m_client);
			m_client = INVALID_SOCKET;
		}
	}
     
private:
    SOCKET m_client;
    SOCKET m_sock;
	CPacket m_packet;

    SOCKET_CALLBACK m_callback;//�ص�����
    void* m_arg;

    CServerSocket& operator=(const CServerSocket& ss) {//��ֵ���캯��

    }
    CServerSocket(const CServerSocket& ss) {//���ƹ��캯��
        m_sock == ss.m_sock;
        m_client == ss.m_client;

    }
    CServerSocket() {
        m_client = INVALID_SOCKET;//-1
        if (InitSockEnv() == FALSE) {
            MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���,�����������ã�"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
            exit(0);
        }
        m_sock = socket(PF_INET, SOCK_STREAM, 0);//IPV4, TCP
    };
    ~CServerSocket() {
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
    static CServerSocket* m_instance;
    static void releaseInstance() {//�ͷ�ʵ��
        if (m_instance != NULL) {
            CServerSocket* p = m_instance;
            m_instance = NULL;
            delete p;
        }
    }
    class CHelper {
    public:
        CHelper() {
            CServerSocket::getInstance();
        }
        ~CHelper() {
            CServerSocket::releaseInstance();
        }

    };
    static CHelper m_helper;
};


