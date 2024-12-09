#pragma once
#include <MSWSock.h>
#include "CThread.h"
#include "CQueue.h"
#include <unordered_map>
#include "Tool.h"



enum EdoyunOperator {
	ENone,
	EAccept,
	ERecv,
	ESend,
	EError
};

class CServer;
class EdoyunClient;
typedef std::shared_ptr<EdoyunClient> PCLIENT;

class EdoyunOverlapped :public ThreadFuncBase {
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;//���� �μ�EdoyunOperator
	std::vector<char> m_buffer;//������
	ThreadWorker m_worker;//��������
	CServer* m_server;//����������
	EdoyunClient* m_client;//��Ӧ�Ŀͻ���
	WSABUF m_wsabuffer;
	virtual ~EdoyunOverlapped() {
		m_client = NULL;
	}
};
template<EdoyunOperator>class AcceptOverlapped;
typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;//�������ӵ�overlapped�ṹ
template<EdoyunOperator>class RecvOverlapped;
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;
template<EdoyunOperator>class SendOverlapped;
typedef SendOverlapped<ESend> SENDOVERLAPPED;

class EdoyunClient :public ThreadFuncBase {
public:
	EdoyunClient();

	~EdoyunClient();

	void SetOverlapped(EdoyunClient* ptr);
	operator SOCKET() {
		return m_sock;
	}
	operator PVOID() {
		return (PVOID)m_buffer.data();
	}
	operator LPOVERLAPPED();

	operator LPDWORD() {
		return &m_received;
	}
	LPWSABUF RecvWSABuffer();
	LPOVERLAPPED RecvOverlapped();
	LPWSABUF SendWSABuffer();
	LPOVERLAPPED SendOverlapped();
	DWORD& flags() { return m_flags; }
	sockaddr_in* GetLocalAddr() { return &m_laddr; }
	sockaddr_in* GetRemoteAddr() { return &m_raddr; }
	size_t GetBufferSize()const { return m_buffer.size(); }
	int Recv();
	int Send(void* buffer, size_t nSize);
	int SendData(std::vector<char>& data);
private:
	SOCKET m_sock;
	DWORD m_received;
	DWORD m_flags;
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped; //�������ӵ�overlapped�ṹ
	std::shared_ptr<RECVOVERLAPPED> m_recv; //�������ݵ�overlapped�ṹ
	std::shared_ptr<SENDOVERLAPPED> m_send; //�������ݵ�overlapped�ṹ
	std::vector<char> m_buffer;
	size_t m_used;//�Ѿ�ʹ�õĻ�������С
	sockaddr_in m_laddr;
	sockaddr_in m_raddr;
	bool m_isbusy;
	EdoyunSendQueue<std::vector<char>> m_vecSend;//�������ݶ���
};

template<EdoyunOperator>
class AcceptOverlapped :public EdoyunOverlapped
{
public:
	AcceptOverlapped();//���ڽ�������accept��overlapped�ص��ṹ  
	virtual ~AcceptOverlapped() {}
	int AcceptWorker();
};


template<EdoyunOperator>
class RecvOverlapped :public EdoyunOverlapped
{
public:
	RecvOverlapped();
	virtual ~RecvOverlapped() {}
	int RecvWorker() {
		int ret = m_client->Recv();
		return ret;
	}

};

template<EdoyunOperator>
class SendOverlapped :public EdoyunOverlapped
{
public:
	SendOverlapped();
	virtual ~SendOverlapped() {}
	int SendWorker() {
		//TODO:
		/*
		* 1 Send���ܲ����������
		*/
		return -1;
	}
};
typedef SendOverlapped<ESend> SENDOVERLAPPED;

template<EdoyunOperator>
class ErrorOverlapped :public EdoyunOverlapped
{
public:
	ErrorOverlapped() :m_operator(EError), m_worker(this, &ErrorOverlapped::ErrorWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}
	virtual ~ErrorOverlapped() {}
	int ErrorWorker() {
		//TODO:
		return -1;
	}
};
typedef ErrorOverlapped<EError> ERROROVERLAPPED;

class CServer :
	public ThreadFuncBase
{
public:
	CServer(const std::string& ip = "0.0.0.0", short port = 9527) :m_pool(10) {//�̳߳����10���߳�
		m_hIOCP = INVALID_HANDLE_VALUE;
		m_sock = INVALID_SOCKET;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(port);
		m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
	}
	~CServer();
	bool StartService();
	bool NewAccept() {
		//PCLIENT pClient(new EdoyunClient());
		EdoyunClient* pClient = new EdoyunClient();
		pClient->SetOverlapped(pClient);
		m_client.insert(std::pair<SOCKET, EdoyunClient*>(*pClient, pClient));
		if (!AcceptEx(m_sock,
			*pClient,
			*pClient,
			0,
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			*pClient, *pClient))
		{
			if (WSAGetLastError() != ERROR_SUCCESS && (WSAGetLastError() != WSA_IO_PENDING)) {
				TRACE("����ʧ�ܣ�%d %s\r\n", WSAGetLastError(), CTool::GetErrInfo(WSAGetLastError()).c_str());
				closesocket(m_sock);
				m_sock = INVALID_SOCKET;
				m_hIOCP = INVALID_HANDLE_VALUE;
				return false;
			}
		}
		return true;
	}
	void BindNewSocket(SOCKET s, ULONG_PTR nKey);
private:
	void CreateSocket() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
			return;
		}
		m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		int opt = 1;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	}
	int threadIocp();
private:
	CThreadPool m_pool;
	HANDLE m_hIOCP;
	SOCKET m_sock;
	sockaddr_in m_addr;
	std::unordered_map<SOCKET, EdoyunClient*> m_client;
};
