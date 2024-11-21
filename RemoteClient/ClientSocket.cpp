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