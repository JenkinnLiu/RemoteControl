#include "pch.h"
#include "ClientSocket.h"
#include <unordered_map>

CClientSocket* CClientSocket::m_instance = NULL;//静态成员变量初始化,必须显式初始化
CClientSocket::CHelper CClientSocket::m_helper;//确保构造函数全局唯一

CClientSocket* pclient = CClientSocket::getInstance();//单例模式，每次只能有一个实例

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
		MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置！"), _T("初始化错误"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL);//创建一个事件
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);//创建一个线程
	if (WaitForSingleObject(m_eventInvoke, 100) == WAIT_TIMEOUT) {//等待事件
		TRACE("网络消息处理线程启动失败！\r\n");
	}
	CloseHandle(m_eventInvoke);//关闭事件
	m_buffer.resize(BUFFER_SIZE);
	memset(m_buffer.data(), 0, BUFFER_SIZE);
	//m_sock = socket(PF_INET, SOCK_STREAM, 0);//IPV4, TCP，服务端可以，客户端不行
	struct {
		UINT message;
		MSGFUNC func;
	}funcs[] = {
		{WM_SEND_PACK, &CClientSocket::SendPack},
		{0, NULL}
	};
	for (int i = 0; funcs[i].message != 0; i++) {
		if (m_mapFunc.insert(std::pair<UINT, MSGFUNC>(funcs[i].message, funcs[i].func)).second == false) {
			TRACE("插入消息处理函数失败！ 消息值%d. 函数值: %08X 序号: %d\r\n", funcs[i].message, funcs[i].func, i);
			exit(0);

		}
	}

}

bool CClientSocket::InitSocket()
{//客户端需要输入IP地址
	if (m_sock != INVALID_SOCKET) CloseSocket();//如果已经连接，先关闭socket再重开一个
	m_sock = socket(PF_INET, SOCK_STREAM, 0);//IPV4, TCP,客户端需要重新建立socket
	if (m_sock == -1) return false;
	sockaddr_in serv_adr;
	memset(&serv_adr, 0, sizeof serv_adr);
	serv_adr.sin_family = AF_INET;//IPV4地址族
	//如果inet_addr报C4996, 用inet_pton, inet_addr是过时的函数，或者在属性，预处理器定义中加入WINSOCK_DEPRECATED_NO_WARNINGS
	//serv_adr.sin_addr.s_addr = inet_addr(strIPAdress.c_str());//在所有IP上监听
	//int ret = inet_pton(AF_INET, htol(nIP), &serv_adr.sin_addr.s_addr);

	//使用 sprintf_s 函数将整数形式的 IP 地址 nIP 转换为字符串形式，并存储在 strIP 中。
	//nIP >> 24、nIP >> 16、nIP >> 8 和 nIP 分别提取 IP 地址的四个字节，并使用 & 0xFF 确保每个字节的值在 0 到 255 之间。
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
		AfxMessageBox("无效的IP地址！");
		return false;
	}
	ret = connect(m_sock, (sockaddr*)&serv_adr, sizeof serv_adr);//连接服务器
	if (ret == -1) {
		//这里要设置使用多字节字符集，不使用Unicode, 不然会报错，
		AfxMessageBox("无法连接服务器,请检查网络设置！");//AfxMessagBox是MFC的消息框
		TRACE("连接失败， %d %s\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
	}
	return true;
}

bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed, WPARAM wParam) {
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;//如果是自动关闭，就设置自动关闭标志
	std::string strOut;
	pack.Data(strOut);
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam);//创建一个PACKET_DATA结构
	bool ret = PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam), (LPARAM)hWnd);//发送消息
	if (ret == false) {
		delete pData;
	}
	return ret;
}

//bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed)
//{
//	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE) {
//		//if (InitSocket() == false) return false;
//		m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);//创建一个线程
//	}
//	m_lock.lock();//加锁, 保证线程安全, 保证事件队列的安全
//	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(pack.hEvent, lstPacks));//在map中基于这个事件创建一个list队列
//	m_mapAutoCLosed[pack.hEvent] = isAutoClosed;//是否自动关闭
//	m_lstSend.push_back(pack);
//	m_lock.unlock();//解锁,保证事件队列只会有一个线程push_back，防止多线程push_back导致数据混乱
//	WaitForSingleObject(pack.hEvent, INFINITE);//等待事件
//	std::unordered_map<HANDLE, std::list<CPacket>&>::iterator it;
//	it = m_mapAck.find(pack.hEvent);//查找事件
//	if (it != m_mapAck.end()) {
//		m_lock.lock();
//		m_mapAck.erase(it);//删除事件, 事件只能处理一次
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
//	//利用事件机制和事件队列，实现事件间的同步，
//	// 使上一个事件处理完成之前下一个事件不会处理，也就是说要排队
//	std::string strBuffer;
//	strBuffer.resize(BUFFER_SIZE);
//	char* pBuffer = (char*)strBuffer.c_str();
//	int index = 0;
//	InitSocket();
//	while (m_sock != INVALID_SOCKET) {
//		if (m_lstSend.size() > 0) {//有数据发送， 只有当发送队列有数据时才发送，否则一直等待
//			TRACE("lstSend size: %d\r\n", m_lstSend.size());
//			m_lock.lock();//加锁
//			CPacket& head = m_lstSend.front();
//			m_lock.unlock();//解锁, 保证只有一个线程获取front
//			if (Send(head) == false) {
//				TRACE("发送失败！\r\n");
//				
//				continue;
//			}//用排队的方式解决问题
//			std::unordered_map<HANDLE, std::list<CPacket>&>::iterator it;
//			it = m_mapAck.find(head.hEvent);
//			if (it != m_mapAck.end()) {//如果没有找到事件，就创建一个事件
//				std::unordered_map<HANDLE, bool>::iterator it0 = m_mapAutoCLosed.find(head.hEvent);
//				do {
//					int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);//接收数据
//					if (length > 0 || index > 0) {
//						index += length;//index代表接收到的数据总长度
//						size_t size = (size_t)index;
//						CPacket pack((BYTE*)pBuffer, size);//解析接收到的包
//						pack.hEvent = head.hEvent;
//
//						if (size > 0) {//解析包成功， TODO:对于文件夹信息获取，文件信息获取可能产生问题
//							//通知对应的事件
//							pack.hEvent = head.hEvent;
//							it->second.push_back(pack); //将解析出的包放入代表当前事件list队列
//							memmove(pBuffer, pBuffer + size, index - size);//将接收到的数据前移
//							index -= size; //接收到的数据长度减去已经处理的数据长度, 为下一次接收数据做准备
//							if (it0->second) {//如果自动关闭，就通知事件完成
//								SetEvent(head.hEvent);
//							}
//						}
//					}
//					else if (length == 0 && index <= 0) {
//						CloseSocket();
//						SetEvent(head.hEvent);//等到服务器关闭命令后，再通知事件完成
//						if (it0 != m_mapAutoCLosed.end()) {
//							//m_mapAutoCLosed.erase(it0);
//						}
//						else {
//							TRACE("异常情况！没有找到事件！\r\n");
//						}
//						
//						break;
//					}
//				} while (it0->second == false || index > 0);//如果不自动关闭，就一直接收数据
//			}
//			m_lock.lock();//加锁,
//			m_lstSend.pop_front();//删除包
//			m_mapAutoCLosed.erase(head.hEvent);
//			m_lock.unlock();//解锁, 保证只有一个线程pop_front
//			if (InitSocket() == false) {
//				InitSocket();
//			}
//		}
//		Sleep(1);//没有数据发送，就休眠1ms,防止CPU占用过高
//	}
//	CloseSocket();
//
//}

void CClientSocket::threadFunc2()
{
	SetEvent(m_eventInvoke);//设置事件
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end()) {//如果消息处理函数存在
			MSGFUNC func = m_mapFunc[msg.message];	//获取消息处理函数
			(this->*func)(msg.message, msg.wParam, msg.lParam);//用函数指针调用消息处理函数
		}
	}
}

bool CClientSocket::Send(const CPacket& pack)
{
	TRACE("向服务器send测试数据，m_sock == %d\r\n", m_sock);
	if (m_sock == -1) return false;
	std::string strOut;
	pack.Data(strOut);
	return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;
}

void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{//定义一个消息的数据结构(数据和数据长度， 模式)， 回调消息的数据结构（HWND, MESSAGE）
	PACKET_DATA data = *(PACKET_DATA*)wParam;//将wParam转换为PACKET_DATA类型,获取发送数据
	delete (PACKET_DATA*)wParam; //释放内存
	size_t nTemp = data.strData.size();
	CPacket current((BYTE*)data.strData.c_str(), nTemp);//解包
	if (InitSocket() == true) {
		
		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0);
		if (ret > 0) {
			size_t index = 0;
			std::string strBuffer;
			strBuffer.resize(BUFFER_SIZE);
			char* pBuffer = (char*)strBuffer.c_str();//将string转换为char*
			while (m_sock != INVALID_SOCKET) {
				int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);//接收数据
				if (length > 0) {
					index += (size_t)length;
					size_t nLen = index;
					CPacket pack((BYTE*)pBuffer, nLen);//解包
					if (nLen > 0) {//解包成功
						::SendMessage((HWND)lParam, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);//通知消息处理函数
						if (data.nMode & CSM_AUTOCLOSE) {//如果是自动关闭
							CloseSocket();//发送消息之后就关闭socket
							return;
						}
						index -= nLen;
						memmove(pBuffer, pBuffer + nLen, index);
					}
				}
				else {//TODO:对方关闭了套接字，或者网络设备异常
					CloseSocket();
					::SendMessage((HWND)lParam, WM_SEND_PACK_ACK, (WPARAM)new CPacket(current.sCmd, NULL, 0), 1);//通知消息处理函数
				}
			}
		}
		else {
			CloseSocket();//网络终止处理
			::SendMessage((HWND)lParam, WM_SEND_PACK_ACK, NULL, -1);
		}
	}
	else {
		//TODO:错误处理
		::SendMessage((HWND)lParam, WM_SEND_PACK_ACK, NULL, -2);
	}
}
