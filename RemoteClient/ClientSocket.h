#pragma once
#include "framework.h"
#include "pch.h"
#include <string>
#include <ws2tcpip.h>//inet_pton
#include <vector>
#include <unordered_map>
#include <mutex>
#define WM_SEND_PACK (WM_USER + 1) //发送包数据
#define WM_SEND_PACK_ACK (WM_USER + 2)//发送包数据应答
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
	CPacket(const BYTE* pData, size_t& nSize) :sHead(0), nLength(0), sCmd(0), sSum(0), hEvent(INVALID_HANDLE_VALUE)
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
            //TRACE("%s\r\n", strData.c_str() + 12);
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
    const char* Data(std::string& strOut) const {//返回整个包的数据,复制到缓冲区strOut中
        strOut.resize(nLength + 6);
        BYTE* pData = (BYTE*)strOut.c_str();
        *(WORD*)pData = sHead;//包头
		pData += 2;
        *(DWORD*)(pData) = nLength;//包长
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
	HANDLE hEvent;//事件句柄
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

typedef struct file_info {
    file_info() {//结构体构造函数
        IsInvalid = 0;
        IsDirectory = -1;
        HasNext = 1;
        memset(szFileName, 0, sizeof szFileName);
    }
    BOOL IsInvalid;//是否为无效目录
    BOOL IsDirectory;//是否为目录： 0 否， 1 是
    BOOL HasNext;//是否还有后续： 0 没有， 1 有
    char szFileName[256];//文件名
}FILEINFO, * PFILEINFO;

enum {
	CSM_AUTOCLOSE = 1,//CSM=ClientSocketMode,自动关闭模式
};

typedef struct PacketData {
    std::string strData;
    UINT nMode;
    WPARAM wParam;
    PacketData(const char* pData, size_t nLen, UINT mode, WPARAM nParam = 0) {
        strData.resize(nLen);
        memcpy((char*)strData.c_str(), pData, nLen);
		nMode = mode;
		wParam = nParam;
    }
	PacketData(const PacketData& pack) {//拷贝构造函数
		strData = pack.strData;
		nMode = pack.nMode;
		wParam = pack.wParam;
	}
    PacketData& operator=(const PacketData& data) {
        if (this != &data) {
            strData = data.strData;
            nMode = data.nMode;
			wParam = data.wParam;
        }
        return *this;
    }
}PACKET_DATA;

std::string GetErrInfo(int WSAErrCode);
void Dump(BYTE* pData, size_t nSize);

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
	bool InitSocket();//初始化套接字
#define BUFFER_SIZE 4096000 //4M
    int DealCommand() {
        if (m_sock == -1) return -1;
        //char buffer[1024];
        //char* buffer = new char[BUFFER_SIZE];
        //服务端接收是单个命令，但是服务端会发送很大的包，所以接收短命令服务端只需要开一个new char就行
        //客户端的话要用vector接收大的数据包
		char* buffer = m_buffer.data();//用vector的data函数返回指向数组的指针，不用担心内存泄漏
        //多线程发送命令时可能会出现冲突，可能接收到图片，也有可能接受到鼠标
		static size_t index = 0;//未处理数据的长度,这里设置为静态变量，
        //是因为DealCommand是一个循环函数，每次调用都会用到index，如果不设置成静态变量，
        // 每次调用DealCommand都会初始化index
        while (true) {
            size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);//接收服务器处理结果
            if (((int)len <= 0) && ((int)index <= 0)) {
                return -1;
            }
            //TODO:处理命令
			Dump((BYTE*)buffer, index);//打印接收到的数据
            index += len;//未处理数据的长度多了len
            len = index;
            m_packet = CPacket((BYTE*)buffer, len);
            if (len > 0) {//解析成功，len是解析成功的包长
                memmove(buffer, buffer + len, index - len);
				index -= len;//更新未处理数据的长度，这里处理了len，所以减去len
                return m_packet.sCmd;
            }
        }
        return -1;
    }
    
    bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed = true, WPARAM wParam = 0);
    //bool SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed = true); 
    
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
    CPacket& GetPacket() {
        return m_packet;
    }
	void CloseSocket() {
		closesocket(m_sock);
        m_sock = INVALID_SOCKET;//-1
	}
    void UpdateAddress(int nIP, int nPort) {
        if (m_nIP != nIP || m_nPort != nPort) {
            m_nIP = nIP;
            m_nPort = nPort;
        }
    }
private:
    HANDLE m_eventInvoke;
    UINT m_nThreadID;
	typedef void(CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	std::unordered_map<UINT, MSGFUNC> m_mapFunc;
    HANDLE m_hThread;
    bool m_bAutoClose;
    std::mutex m_lock;//互斥量

    std::list<CPacket> m_lstSend;
    std::unordered_map<HANDLE, std::list<CPacket>&> m_mapAck;
	std::unordered_map<HANDLE, bool> m_mapAutoCLosed;//事件是否自动关闭的标志
	int m_nIP;//IP地址
	int m_nPort;//端口

    std::vector<char> m_buffer;
    SOCKET m_sock;
    CPacket m_packet;
    CClientSocket& operator=(const CClientSocket& ss) {//赋值构造函数

    }
    CClientSocket(const CClientSocket& ss);//复制构造函数
        
    CClientSocket();
    ~CClientSocket() {
        closesocket(m_sock);
        WSACleanup();
    };
    static unsigned __stdcall threadEntry(void* arg);
    //void threadFunc();
    void threadFunc2();
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
    bool Send(const char* pData, int size) {
        if (m_sock == -1) return false;
        return send(m_sock, pData, size, 0) > 0;
    }
    bool Send(const CPacket& pack);
	void SendPack(UINT nMsg, WPARAM wParam/* 缓冲区的值*/, LPARAM lParam/*缓冲区的长度*/);
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



