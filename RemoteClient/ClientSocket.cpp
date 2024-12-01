#include "pch.h"
#include "ClientSocket.h"

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
	while (m_sock != INVALID_SOCKET) {
		if (m_lstSend.size() > 0) {//有数据发送， 只有当发送队列有数据时才发送，否则一直等待
			TRACE("lstSend size: %d\r\n", m_lstSend.size());
			CPacket& head = m_lstSend.front();
			if (Send(head) == false) {
				TRACE("发送失败！\r\n");
				continue;
			}//用排队的方式解决问题
			m_mapAck[head.hEvent] = std::list<CPacket>();//在map中基于这个事件创建一个list队列
			int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);//接收数据
			if (length > 0 || index > 0) {
				index += length;//index代表接收到的数据总长度
				size_t size = (size_t)index;
				CPacket pack((BYTE*)pBuffer, size);//解析接收到的包
				pack.hEvent = head.hEvent;

				if (size > 0) {//解析包成功， TODO:对于文件夹信息获取，文件信息获取可能产生问题
					//通知对应的事件
					SetEvent(head.hEvent);//设置事件为有信号状态
					std::list<CPacket>().push_back(pack); //将解析出的包放入代表当前事件list队列
				}
			}
			else if (length == 0 && index <= 0) {
				CloseSocket();
			}
			m_lstSend.pop_front();//删除包
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
