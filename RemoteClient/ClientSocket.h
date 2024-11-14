#pragma once
#include "framework.h"
#include "pch.h"
#include <string>
#include <ws2tcpip.h>//inet_pton

#pragma pack(push)
#pragma pack(1)

class CPacket
{
public:
    CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
    CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {//打包构造函数
        sHead = 0xFEFF;
        nLength = nSize + 4;
        sCmd = nCmd;
        if (nSize > 0) {
            strData.resize(nSize);
            memcpy((void*)strData.c_str(), pData, nSize);
        }
        else {
            strData.clear();
        }
        sSum = 0;
        for (size_t j = 0; j < strData.size(); j++) {
            sSum += BYTE(strData[j]) & 0xFF;
        }
    }
    CPacket(const CPacket& pack) {//拷贝构造函数
        sHead = pack.sHead;
        nLength = pack.nLength;
        strData = pack.strData;
        sCmd = pack.sCmd;
        sSum = pack.sSum;
    }
    CPacket& operator=(const CPacket& pack) {//赋值构造函数
        if (this == &pack) return *this;//防止自赋值
        sHead = pack.sHead;
        nLength = pack.nLength;
        strData = pack.strData;
        sCmd = pack.sCmd;
        sSum = pack.sSum;
        return *this;
    }
    CPacket(const BYTE* pData, size_t& nSize) :sHead(0), nLength(0), sCmd(0), sSum(0)
    {//这个构造函数用于解析数据包，解包构造函数
        size_t i = 0;
        for (i = 0; i < nSize; i++) {
            if (*(WORD*)(pData + i) == 0xFEFF) {
                sHead = *(WORD*)(pData + i);//找到包头
                i += 2;
                break;
            }
        }
        if (i + 4 + 2 + 2 > nSize) {//包头+包长+命令+校验和，包数据不全或包头未找到
            nSize = 0;
            return;//没有找到包头,解析失败
        }
        nLength = *(DWORD*)(pData + i);//包长
        i += 4;
        if (nLength + i > nSize) {//包长不够,包未接收完整，解析失败
            nSize = 0;
            return;
        }
        sCmd = *(WORD*)(pData + i);//命令
        i += 2;
        if (nLength > 4) {
            //nLength从sCmd开始到校验和结束,所以减去4
            strData.resize(nLength - 2 - 2);//包长-命令-校验和
            memcpy((void*)strData.c_str(), pData + i, nLength - 4);
            i += nLength - 4;
        }
        sSum = *(WORD*)(pData + i);//校验和
        i += 2;
        WORD sum = 0;
        for (size_t j = 0; j < strData.size(); j++) {
            sum += BYTE(strData[j]) & 0xFF;
        }
        if (sum == sSum) {
            nSize = i;
            return;
        }
        nSize = 0;
    };
    ~CPacket()
    {

    };
    int Size() {
        return nLength + 6;//包头+包长+包数据，包数据的大小
    }
    const char* Data() {//返回整个包的数据,复制到缓冲区strOut中
        strOut.resize(nLength + 6);
        BYTE* pData = (BYTE*)strOut.c_str();
        *(WORD*)pData = sHead;//包头
        *(DWORD*)(pData + 2) = nLength;//包长
        pData += 4;
        *(WORD*)pData = sCmd;//命令
        pData += 2;
        memcpy(pData, strData.c_str(), strData.size());//包数据
        pData += strData.size();
        *(WORD*)pData = sSum;//校验和
        return strOut.c_str();
    }
public:
    WORD sHead;//包头，固定为FE FF
    DWORD nLength;//包长（从控制命令开始到校验和结束）
    std::string strData;//包数据
    WORD sCmd;//控制命令
    WORD sSum;//校验和
    std::string strOut;//整个包的数据
};


#pragma pack(pop)

typedef struct MouseEvent {
    MouseEvent() {
        nAction = 0;
        nButton = -1;
        ptXY.x = 0, ptXY.y = 0;
    }
    WORD nAction;//点击， 移动， 双击
    WORD nButton;//左键， 右键， 中键
    POINT ptXY;//坐标

}MOUSEEV, * PMOUSEEV;

std::string GetErrorInfo(int WSAErrCode) {
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

class CClientSocket
{
public:
    //static CClientSocket& GetInstance() {
    //	static CClientSocket instance;
    //	return instance;
    //}
    static CClientSocket* getInstance() {//单例模式，每次只能有一个实例
        if (m_instance == NULL) {//静态函数没有this指针，所以不能访问非静态成员变量
            m_instance = new CClientSocket();
        }
        return m_instance;
    }
    bool InitSocket(const std::string& strIPAdress) {//客户端需要输入IP地址

        if (m_sock == -1) return false;
        sockaddr_in serv_adr;
        memset(&serv_adr, 0, sizeof serv_adr);
        serv_adr.sin_family = AF_INET;//IPV4地址族
		//如果inet_addr报C4996, 用inet_pton, inet_addr是过时的函数，或者在属性，预处理器定义中加入WINSOCK_DEPRECATED_NO_WARNINGS
        //serv_adr.sin_addr.s_addr = inet_addr(strIPAdress.c_str());//在所有IP上监听
		serv_adr.sin_addr.s_addr = inet_pton(AF_INET, strIPAdress.c_str(), &serv_adr.sin_addr.s_addr);

        serv_adr.sin_port = htons(9527);
        if (serv_adr.sin_addr.s_addr == INADDR_NONE) {
			AfxMessageBox("无效的IP地址！");
            return false;
        }
		int ret = connect(m_sock, (sockaddr*)&serv_adr, sizeof serv_adr);//连接服务器
        if (ret == -1) {
			//这里要设置使用多字节字符集，不使用Unicode, 不然会报错，
			AfxMessageBox("无法连接服务器,请检查网络设置！");//AfxMessagBox是MFC的消息框
			TRACE("连接失败， %d %s\r\n", WSAGetLastError(), GetErrorInfo(WSAGetLastError()).c_str());
        }
        return true;
    }
#define BUFFER_SIZE 4096
    int DealCommand() {
        if (m_sock == -1) return -1;
        //char buffer[1024];
        char* buffer = new char[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        size_t index = 0;//未处理数据的长度
        while (true) {
            size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
            if (len <= 0) return -1;
            //TODO:处理命令
            index += len;//未处理数据的长度多了len
            len = index;
            m_packet = CPacket((BYTE*)buffer, len);
            if (len > 0) {//解析成功，len是解析成功的包长
                memmove(buffer, buffer + len, BUFFER_SIZE - len);
                index -= len;//未处理数据的长度，这里处理了len，所以减去len
                return m_packet.sCmd;
            }
        }
        return -1;
    }
    bool Send(const char* pData, int size) {
        if (m_sock == -1) return false;
        return send(m_sock, pData, size, 0) > 0;
    }
    bool Send(CPacket& pack) {
        if (m_sock == -1) return false;
        return send(m_sock, pack.Data(), pack.Size(), 0) > 0;
    }
    bool GetFilePath(std::string& strPath) {
        if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4)) {//获取文件列表的命令：2, 打开文件：3
            strPath = m_packet.strData;
            return true;
        }
        return false;
    }
    bool GetMouseEvent(MOUSEEV& mouse) {
        if (m_packet.sCmd == 5) {
            memcpy(&mouse, m_packet.strData.c_str(), sizeof MOUSEEV);
            return true;
        }
        return false;
    }
private:
    SOCKET m_sock;
    CPacket m_packet;
    CClientSocket& operator=(const CClientSocket& ss) {//赋值构造函数

    }
    CClientSocket(const CClientSocket& ss) {//复制构造函数
        m_sock == ss.m_sock;

    }
    CClientSocket() {
        if (InitSockEnv() == FALSE) {
            MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置！"), _T("初始化错误"), MB_OK | MB_ICONERROR);
            exit(0);
        }
        m_sock = socket(PF_INET, SOCK_STREAM, 0);//IPV4, TCP
    };
    ~CClientSocket() {
        closesocket(m_sock);
        WSACleanup();
    };
    BOOL InitSockEnv() {
        WSADATA data;
        if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
            return FALSE;
        }
        return TRUE;
    }
    static CClientSocket* m_instance;
    static void releaseInstance() {//释放实例
        if (m_instance != NULL) {
            CClientSocket* p = m_instance;
            m_instance = NULL;
            delete p;
        }
    }
    class CHelper {
    public:
        CHelper() {
            CClientSocket::getInstance();
        }
        ~CHelper() {
            CClientSocket::releaseInstance();
        }

    };
    static CHelper m_helper;
};



