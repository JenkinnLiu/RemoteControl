#include "pch.h"
#include "ClientSocket.h"

CClientSocket* CClientSocket::m_instance = NULL;//��̬��Ա������ʼ��,������ʽ��ʼ��
CClientSocket::CHelper CClientSocket::m_helper;//ȷ�����캯��ȫ��Ψһ

CClientSocket* pclient = CClientSocket::getInstance();//����ģʽ��ÿ��ֻ����һ��ʵ��

std::string GetErrInfo(int WSAErrCode){
    std::string ret;
    LPVOID lpMsgBuf = NULL;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,//��ʽ����Ϣ
        NULL,
        WSAErrCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Ĭ������
        (LPTSTR)&lpMsgBuf,
        0,
        NULL
    );//��ȡ������Ϣ,WSAErrCode�Ǵ�����
    ret = (char*)lpMsgBuf;
    LocalFree(lpMsgBuf);
    return ret;
}