#include "pch.h"
#include "ClientSocket.h"

CClientSocket* CClientSocket::m_instance = NULL;//��̬��Ա������ʼ��,������ʽ��ʼ��
CClientSocket::CHelper CClientSocket::m_helper;//ȷ�����캯��ȫ��Ψһ

CClientSocket* pclient = CClientSocket::getInstance();//����ģʽ��ÿ��ֻ����һ��ʵ��