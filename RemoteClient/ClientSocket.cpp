#include "pch.h"
#include "ClientSocket.h"

CClientSocket* CClientSocket::m_instance = NULL;//静态成员变量初始化,必须显式初始化
CClientSocket::CHelper CClientSocket::m_helper;//确保构造函数全局唯一

CClientSocket* pclient = CClientSocket::getInstance();//单例模式，每次只能有一个实例