#include "pch.h"
#include "ClientSocket.h"

CClientSocket* CClientSocket::m_instance = NULL;//静态成员变量初始化,必须显式初始化
CClientSocket::CHelper CClientSocket::m_helper;//确保构造函数全局唯一

CClientSocket* pclient = CClientSocket::getInstance();//单例模式，每次只能有一个实例

std::string GetErrInfo(int WSAErrCode){
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
    if (InitSocket() == false) {
        return;
    }
    std::string strBuffer;
    strBuffer.resize(BUFFER_SIZE);
    char* pBuffer = (char*)strBuffer.c_str();
    int index = 0;
    while (m_sock != INVALID_SOCKET) {
        if (m_lstSend.size() > 0) {//有数据发送， 只有当发送队列有数据时才发送，否则一直等待
            CPacket& head = m_lstSend.front();
            if (Send(head) == false) {
                TRACE("发送失败！\r\n");
                continue;
            }//用排队的方式解决问题
			m_mapAck[head.hEvent] = std::list<CPacket>();
            int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
            if (length > 0 || index > 0) {
                index += length;
                size_t size = (size_t)index;
                CPacket pack((BYTE*)pBuffer, size);
				pack.hEvent = head.hEvent;
                
                if (size > 0) {//解析包成功， 对于文件夹信息获取，文件信息获取可能产生问题
                    //通知对应的事件
                    SetEvent(head.hEvent);
                    std::list<CPacket>().push_back(pack);
                }
            }
            else if (length == 0 && index <= 0) {
                CloseSocket();
            }
            m_lstSend.pop_front();//删除包
        }
    }
}
