#pragma once
#include "framework.h"
#include "pch.h"

class CServerSocket
{
public:
	//static CServerSocket& GetInstance() {
	//	static CServerSocket instance;
	//	return instance;
	//}
	static CServerSocket* getInstance() {//单例模式，每次只能有一个实例
		if (m_instance == NULL) {//静态函数没有this指针，所以不能访问非静态成员变量
			m_instance = new CServerSocket();
		}
		return m_instance;
	}
private:
	CServerSocket& operator=(const CServerSocket& ss) {//赋值构造函数

	}
	CServerSocket(const CServerSocket&) {//复制构造函数

	}
	CServerSocket() {
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置！"), _T("初始化错误"), MB_OK | MB_ICONERROR);
			exit(0);
		}
	};
	~CServerSocket() {
		WSACleanup();
	};
	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;
		}
		return TRUE;
	}
	static CServerSocket* m_instance;
	static void releaseInstance() {//释放实例
		if (m_instance != NULL) {
			CServerSocket* p = m_instance;
			m_instance = NULL;
			delete p;
		}
	}
	class CHelper {
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			CServerSocket::releaseInstance();
		}

	};
	static CHelper m_helper;
};


