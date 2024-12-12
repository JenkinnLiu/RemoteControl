#include "pch.h"
#include "CServer.h"
#include "Tool.h"
#pragma warning(disable:4407)

std::string GetErrInfo(int WSAErrCode) {
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,//格式化信息
		NULL,
		WSAErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // 默认语言
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
	);//获取错误信息,WSAErrCode是错误码
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}

template<EdoyunOperator op>
AcceptOverlapped<op>::AcceptOverlapped() {//用于接收连接的overlapped结构
	m_worker = ThreadWorker(this, (FUNCTYPE)&AcceptOverlapped<op>::AcceptWorker);//线程工作对象
	m_operator = EAccept;//操作类型
	memset(&m_overlapped, 0, sizeof(m_overlapped));//初始化overlapped对象
	m_buffer.resize(1024);
	m_server = NULL;
}

template<EdoyunOperator op>
int AcceptOverlapped<op>::AcceptWorker() {//接收连接的工作函数
	TRACE("AcceptWorker this %08X\r\n", this);
	INT lLength = 0, rLength = 0;//本地地址和远程地址的长度
	if (m_client->GetBufferSize() > 0) {//如果缓冲区大小大于0
		LPSOCKADDR pLocalAddr, pRemoteAddr;
		GetAcceptExSockaddrs(*m_client, 0,
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)&pLocalAddr, &lLength,//本地地址
			(sockaddr**)&pRemoteAddr, &rLength//远程地址
		)；//获取本地地址和远程地址，GetAcceptExSockaddrs用于获取AcceptEx函数的地址
			memcpy(m_client->GetLocalAddr(), pLocalAddr, sizeof(sockaddr_in));//拷贝本地和远程地址
		memcpy(m_client->GetRemoteAddr(), pRemoteAddr, sizeof(sockaddr_in));
		m_server->BindNewSocket(*m_client, (ULONG_PTR)m_client);// 绑定新的socket，将新的socket绑定到IOCP上
		//这里的WSARecv是异步接收，是要申请接收数据，然后申请之后再接收的
		int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABuffer(), 1, *m_client, &m_client->flags(), m_client->RecvOverlapped(), NULL);//开始接收数据
		if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING)) {
			//TODO:报错
			TRACE("WSARecv failed %d\r\n", ret);
		}
		if (!m_server->NewAccept())//如果接收连接失败
		{
			return -2;
		}
	}
	return -1;//必须返回-1，否则循环不会终止
}

template<EdoyunOperator op>
inline SendOverlapped<op>::SendOverlapped() {//发送数据的overlapped结构
	m_operator = op;
	m_worker = ThreadWorker(this, (FUNCTYPE)&SendOverlapped<op>::SendWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}

template<EdoyunOperator op>
inline RecvOverlapped<op>::RecvOverlapped() {//接收数据的overlapped结构
	m_operator = op;
	m_worker = ThreadWorker(this, (FUNCTYPE)&RecvOverlapped<op>::RecvWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}


EdoyunClient::EdoyunClient()//构造函数初始化了客户端对象的各种成员，包括 overlapped 结构、socket 和缓冲区。
	:m_isbusy(false), m_flags(0),
	m_overlapped(new ACCEPTOVERLAPPED()),
	m_recv(new RECVOVERLAPPED()),
	m_send(new SENDOVERLAPPED()),
	m_vecSend(this, (SENDCALLBACK)&EdoyunClient::SendData)  
{
	TRACE("m_overlapped %08X\r\n", &m_overlapped);
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024);
	memset(&m_laddr, 0, sizeof(m_laddr)); //初始化本地地址
	memset(&m_raddr, 0, sizeof(m_raddr)); //初始化远程地址
}
EdoyunClient::~EdoyunClient()
{
	m_buffer.clear();
	closesocket(m_sock);
}
void EdoyunClient::SetOverlapped(EdoyunClient* ptr) { //设置overlapped结构
	m_overlapped->m_client = ptr;
	m_recv->m_client = ptr;
	m_send->m_client = ptr;
}
EdoyunClient::operator LPOVERLAPPED() { //重载类型转换
	return &m_overlapped->m_overlapped;
}

LPWSABUF EdoyunClient::RecvWSABuffer() //接收数据的WSABUF
{
	return &m_recv->m_wsabuffer;
}

LPOVERLAPPED EdoyunClient::RecvOverlapped() //接收数据的overlapped结构
{
	return &m_recv->m_overlapped;
}

LPWSABUF EdoyunClient::SendWSABuffer() //发送数据的WSABUF
{
	return &m_send->m_wsabuffer;
}

LPOVERLAPPED EdoyunClient::SendOverlapped() //发送数据的overlapped结构
{
	return &m_send->m_overlapped;
}

int EdoyunClient::Recv() //接收数据
{
	int ret = recv(m_sock, m_buffer.data() + m_used, m_buffer.size() - m_used, 0);
	if (ret <= 0)return -1;
	m_used += (size_t)ret;
	//TODO:解析数据
	CTool::Dump((BYTE*)m_buffer.data(), ret);
	return 0;
}

int EdoyunClient::Send(void* buffer, size_t nSize) //发送数据
{
	std::vector<char> data(nSize);
	memcpy(data.data(), buffer, nSize);
	if (m_vecSend.PushBack(data)) {
		return 0;
	}
	return -1;
}

int EdoyunClient::SendData(std::vector<char>& data) 
{
	if (m_vecSend.Size() > 0) {
		int ret = WSASend(m_sock, SendWSABuffer(), 1, &m_received, m_flags, &m_send->m_overlapped, NULL);
		if (ret != 0 && WSAGetLastError() != WSA_IO_PENDING) {//发送失败，WSA_IO_PENDING表示异步操作正在进行中
			CTool::ShowError();
			return ret;
		}
	}
	return 0;
}

CServer::~CServer()
{
	std::unordered_map<SOCKET, EdoyunClient*>::iterator it = m_client.begin();
	for (; it != m_client.end(); it++) {
		delete it->second; //释放客户端EdoyunClient对象
		it->second = NULL;
	}
	m_client.clear();
}
//1. 启动服务
bool CServer::StartService()//启动服务
{	//1.启动CServer的构造函数
	//2. 创建socket
	CreateSocket();
	//3. bind和listen
	if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return false;
	}
	if (listen(m_sock, 3) == -1) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return false;
	}
	//4. 创建IOCP
	m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);//创建IOCP
	if (m_hIOCP == NULL) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		m_hIOCP = INVALID_HANDLE_VALUE;
		return false;
	}
	CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0); //5. 将监听套接字绑定到IOCP上
	m_pool.Invoke(); //6. 启动线程池
	m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&CServer::threadIocp)); //7. 线程池分配线程工作对象
	if (!NewAccept())return false; //8. 调用accpet接收连接
	//m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&EdoyunServer::threadIocp));
	//m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&EdoyunServer::threadIocp));
	return true;
}

void CServer::BindNewSocket(SOCKET s, ULONG_PTR nKey) 
{
	CreateIoCompletionPort((HANDLE)s, m_hIOCP, nKey, 0);
}

int CServer::threadIocp() //7.1 IOCP线程实现，靠的是线程池调用对应操作的DispatchWorker函数
{
	DWORD tranferred = 0;
	ULONG_PTR CompletionKey = 0;
	OVERLAPPED* lpOverlapped = NULL;
	//7.2 调用GetQueuedCompletionStatus函数，等待IOCP发送消息,接收到的数据放在lpOverlapped中
	if (GetQueuedCompletionStatus(m_hIOCP, &tranferred, &CompletionKey, &lpOverlapped, INFINITE)) { 
		if (CompletionKey != 0) {
			EdoyunOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, EdoyunOverlapped, m_overlapped); //7.3 用CONTAINING_RECORD宏获取overlapped结构的父类对象
			pOverlapped->m_server = this; //7.4 设置overlapped结构的server指针
			TRACE("Operator is %d\r\n", pOverlapped->m_operator);
			switch (pOverlapped->m_operator) { //7.5 根据操作类型调用不同的工作函数
			case EAccept:
			{
				ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped;//接收连接,这里的pOverlapped是AcceptOverlapped对象
				TRACE("pOver %08X\r\n", pOver);
				m_pool.DispatchWorker(pOver->m_worker);//7.5.1分配工作对象
			}
			break;
			case ERecv:
			{
				RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
			}
			break;
			case ESend:
			{
				SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
			}
			break;
			case EError:
			{
				ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
			}
			break;
			}
		}
		else {
			return -1;
		}
	}
	return 0;
}
