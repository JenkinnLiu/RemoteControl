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

bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed)
{
	if (m_sock == INVALID_SOCKET) {
		//if (InitSocket() == false) return false;
		_beginthread(&CClientSocket::threadEntry, 0, this);//创建一个线程
	}
	m_mapAck[pack.hEvent] = lstPacks;//在map中基于这个事件创建一个list队列
	m_mapAutoCLosed[pack.hEvent] = isAutoClosed;//是否自动关闭
	m_lstSend.push_back(pack);
	WaitForSingleObject(pack.hEvent, INFINITE);//等待事件
	std::unordered_map<HANDLE, std::list<CPacket>>::iterator it;
	it = m_mapAck.find(pack.hEvent);//查找事件
	if (it != m_mapAck.end()) {
		m_mapAck.erase(it);//删除事件, 事件只能处理一次
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
			CPacket& head = m_lstSend.front();
			if (Send(head) == false) {
				TRACE("发送失败！\r\n");
				
				continue;
			}//用排队的方式解决问题
			std::unordered_map<HANDLE, std::list<CPacket>>::iterator it;
			it = m_mapAck.find(head.hEvent);
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
				}
			} while (it0->second == false);//如果不自动关闭，就一直接收数据
			
			m_lstSend.pop_front();//删除包
			InitSocket();
		}
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
