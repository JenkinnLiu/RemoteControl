#pragma once
#include "framework.h"
#include "pch.h"
#include <list>
#include "Packet.h"
#pragma pack(pop)

typedef int(*SOCKET_CALLBACK)(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket);
//这个函数指针类型通常用于回调机制中，允许用户定义一个符合该签名的函数，
// 并在特定事件或条件下调用该函数。例如，在网络编程中，可以使用回调函数处理接收到的数据包
// 或连接状态的变化。

class CServerSocket
{
public:
    //static CServerSocket& GetInstance() {
    //	static CServerSocket instance;
    //	return instance;
    //}
    static CServerSocket* getInstance() {//单例模式，每次只能有一个实例
        if (m_instance == NULL) {//静态函数没有this指针，所以不能访问非静态成员变量
            m_instance = new CServerSocket();
        }
        return m_instance;
    }
    int Run(SOCKET_CALLBACK callback, void* arg, short port = 9257) {//Socket初始化回调函数RunCommand
        bool ret = InitSocket(port);
        if (ret == false) return -1;
        std::list<CPacket> lstPackets;//包列表
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
                m_callback(m_arg, ret, lstPackets, m_packet);//执行回调函数
                while (lstPackets.size() > 0) {
                    Send(lstPackets.front());//发送包
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
        serv_adr.sin_family = AF_INET;//IPV4地址族
        serv_adr.sin_addr.s_addr = INADDR_ANY;//在所有IP上监听
        serv_adr.sin_port = htons(9527);
        if(bind(m_sock, (sockaddr*)&serv_adr, sizeof serv_adr) == -1)
            return false;
        if (listen(m_sock, 1) == -1) 
            return false;
		Run(m_callback, m_arg);
        return true;
    }

    bool AcceptClient() {
		TRACE("accept:服务器已经与客户端连接，等待客户端发送数据\r\n");
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
        //服务端接收是单个命令，但是服务端会发送很大的包，所以接收短命令服务端只需要开一个new char就行
        //客户端的话要用vector接收大的数据包
        if (buffer == NULL) {
			TRACE("内存不足，分配失败！\r\n");
            return -2;
        }
        memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;//未处理数据的长度
        while (true) {
            size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
            if (len <= 0) {
                delete[] buffer;
                return -1;
            }
			TRACE("服务器接收到测试数据，server recv len:%d\r\n", len);
            //TODO:处理命令
			index += len;//未处理数据的长度多了len
            len = index;
			TRACE("buffer的len变成了，:%d\r\n", len);
			m_packet = CPacket((BYTE*)buffer, len);
			TRACE("服务器接收到测试数据，server recv len:%d, cmd:%d\r\n", len, m_packet.sCmd);
			if (len > 0) {//解析成功，len是解析成功的包长
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;//未处理数据的长度，这里处理了len，所以减去len
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
		//Sleep(1);//这里延迟1ms是因为如果发送太快，缓冲区可能会满，导致发送失败，所以要做延迟处理！！
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

    SOCKET_CALLBACK m_callback;//回调函数
    void* m_arg;

    CServerSocket& operator=(const CServerSocket& ss) {//赋值构造函数

    }
    CServerSocket(const CServerSocket& ss) {//复制构造函数
        m_sock == ss.m_sock;
        m_client == ss.m_client;

    }
    CServerSocket() {
        m_client = INVALID_SOCKET;//-1
        if (InitSockEnv() == FALSE) {
            MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置！"), _T("初始化错误"), MB_OK | MB_ICONERROR);
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
    static void releaseInstance() {//释放实例
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


