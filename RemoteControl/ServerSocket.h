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
	static CServerSocket* getInstance() {//����ģʽ��ÿ��ֻ����һ��ʵ��
		if (m_instance == NULL) {//��̬����û��thisָ�룬���Բ��ܷ��ʷǾ�̬��Ա����
			m_instance = new CServerSocket();
		}
		return m_instance;
	}
private:
	CServerSocket& operator=(const CServerSocket& ss) {//��ֵ���캯��

	}
	CServerSocket(const CServerSocket&) {//���ƹ��캯��

	}
	CServerSocket() {
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���,�����������ã�"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
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
	static void releaseInstance() {//�ͷ�ʵ��
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


