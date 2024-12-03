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

bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed)
{
	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE) {
		//if (InitSocket() == false) return false;
		m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);//创建一个线程
	}
	m_lock.lock();//加锁, 保证线程安全, 保证事件队列的安全
	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(pack.hEvent, lstPacks));//在map中基于这个事件创建一个list队列
	m_mapAutoCLosed[pack.hEvent] = isAutoClosed;//是否自动关闭
	m_lstSend.push_back(pack);
	m_lock.unlock();//解锁,保证事件队列只会有一个线程push_back，防止多线程push_back导致数据混乱
	WaitForSingleObject(pack.hEvent, INFINITE);//等待事件
	std::unordered_map<HANDLE, std::list<CPacket>&>::iterator it;
	it = m_mapAck.find(pack.hEvent);//查找事件
	if (it != m_mapAck.end()) {
		m_lock.lock();
		m_mapAck.erase(it);//删除事件, 事件只能处理一次
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
	//利用事件机制和事件队列，实现事件间的同步，
	// 使上一个事件处理完成之前下一个事件不会处理，也就是说要排队
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int index = 0;
	InitSocket();
	while (m_sock != INVALID_SOCKET) {
		if (m_lstSend.size() > 0) {//有数据发送， 只有当发送队列有数据时才发送，否则一直等待
			TRACE("lstSend size: %d\r\n", m_lstSend.size());
			m_lock.lock();//加锁
			CPacket& head = m_lstSend.front();
			m_lock.unlock();//解锁, 保证只有一个线程获取front
			if (Send(head) == false) {
				TRACE("发送失败！\r\n");
				
				continue;
			}//用排队的方式解决问题
			std::unordered_map<HANDLE, std::list<CPacket>&>::iterator it;
			it = m_mapAck.find(head.hEvent);
			if (it != m_mapAck.end()) {//如果没有找到事件，就创建一个事件
				std::unordered_map<HANDLE, bool>::iterator it0 = m_mapAutoCLosed.find(head.hEvent);
				do {
					int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);//接收数据
					if (length > 0 || index > 0) {
						index += length;//index代表接收到的数据总长度
						size_t size = (size_t)index;
						CPacket pack((BYTE*)pBuffer, size);//解析接收到的包
						pack.hEvent = head.hEvent;

						if (size > 0) {//解析包成功， TODO:对于文件夹信息获取，文件信息获取可能产生问题
							//通知对应的事件
							pack.hEvent = head.hEvent;
							it->second.push_back(pack); //将解析出的包放入代表当前事件list队列
							memmove(pBuffer, pBuffer + size, index - size);//将接收到的数据前移
							index -= size; //接收到的数据长度减去已经处理的数据长度, 为下一次接收数据做准备
							if (it0->second) {//如果自动关闭，就通知事件完成
								SetEvent(head.hEvent);
							}
						}
					}
					else if (length == 0 && index <= 0) {
						CloseSocket();
						SetEvent(head.hEvent);//等到服务器关闭命令后，再通知事件完成
						m_mapAutoCLosed.erase(it0);
						break;
					}
				} while (it0->second == false || index > 0);//如果不自动关闭，就一直接收数据
			}
			m_lock.lock();//加锁,
			m_lstSend.pop_front();//删除包
			m_lock.unlock();//解锁, 保证只有一个线程pop_front
			if (InitSocket() == false) {
				InitSocket();
			}
		}
		Sleep(1);//没有数据发送，就休眠1ms,防止CPU占用过高
	}
	CloseSocket();

}

bool CClientSocket::Send(const CPacket& pack)
{
	TRACE("向服务器send测试数据，m_sock == %d\r\n", m_sock);
	if (m_sock == -1) return false;
	std::string strOut;
	pack.Data(strOut);
	return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;
}
