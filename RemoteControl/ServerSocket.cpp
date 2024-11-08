#include "pch.h"
#include "ServerSocket.h"


CServerSocket* CServerSocket::m_instance = NULL;//静态成员变量初始化,必须显式初始化
CServerSocket::CHelper CServerSocket::m_helper;//确保构造函数全局唯一

CServerSocket*  pserver = CServerSocket::getInstance();//单例模式，每次只能有一个实例

