#include "pch.h"
#include "ServerSocket.h"


CServerSocket* CServerSocket::m_instance = NULL;//��̬��Ա������ʼ��,������ʽ��ʼ��
CServerSocket::CHelper CServerSocket::m_helper;//ȷ�����캯��ȫ��Ψһ

CServerSocket*  pserver = CServerSocket::getInstance();//����ģʽ��ÿ��ֻ����һ��ʵ��

