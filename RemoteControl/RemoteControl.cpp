// RemoteControl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteControl.h"
#include "ServerSocket.h"
#include "Tool.h"
#include "Command.h"
#include<conio.h>
#include "CQueue.h"
#include<MSWSock.h>
#include "CServer.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 唯一的应用程序对象
#define INVOKE_PATH _T("C:\\Users\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteControl.exe")
#define INVOKE_PATH2 _T("C:\\Windows\\SysWOW64\\RemoteControl.exe")

CWinApp theApp;
using namespace std;

void WriteRigisterTable() {
    CString strSubKey = _T("\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
    CString strPath = CString(_T("C:\\Windows\\SysWOW64\\RemoteControl.exe"));
    TCHAR sPath[MAX_PATH] = _T("");
    GetModuleFileName(NULL, sPath, MAX_PATH);
    BOOL ret = CopyFile(sPath, strPath, FALSE);
    //fopen CFile system(copy) CopyFile OpenFile
    if (ret == FALSE) {
        MessageBox(NULL, _T("复制文件失败！是否权限不足？"), _T("错误"), MB_OK | MB_ICONERROR);
        ::exit(0);
    }
    HKEY hKey = NULL;
    ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);//打开注册表
    if (ret != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        MessageBox(NULL, _T("打开注册表失败！是否权限不足？"), _T("开机启动错误"), MB_OK | MB_ICONERROR);
        ::exit(0);
    }

ret = RegSetValueEx(hKey, _T("RemoteControl"), 0, REG_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));//写入注册表
if (ret != ERROR_SUCCESS) {
    RegCloseKey(hKey);
    MessageBox(NULL, _T("写入注册表失败！是否权限不足？"), _T("开机启动错误"), MB_OK | MB_ICONERROR);
    ::exit(0);
}
RegCloseKey(hKey);//关闭注册表
}

//开机启动的第二种方法，复制到Windows启动目录
void WriteStartupDir(CString strPath) {

    TCHAR sPath[MAX_PATH] = _T("");
    GetModuleFileName(NULL, sPath, MAX_PATH);
    //CString strPath = _T("C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\StartUp\\RemoteControl.exe");
    BOOL ret = CopyFile(sPath, strPath, FALSE);
    //fopen CFile system(copy) CopyFile OpenFile
    if (ret == FALSE) {
        MessageBox(NULL, _T("复制文件失败！是否权限不足？"), _T("错误"), MB_OK | MB_ICONERROR);
        ::exit(0);
    }
}

//开机权限是跟随启动用户的，如果两者权限不一致，可能会出现问题
//开机启动环境变量有影响，如果依赖dll(动态库），则可能启动失败
// //解决方法：
//1.复制这些dll到system32下或者sysWOW64下，或者将程序放到系统目录下
//system32下面多是64位的dll，sysWOW64下面多是32位的dll
//2.使用静态库，而非动态库
bool ChooseAutoInvoke(const CString& strPath) {//自动启动
    TCHAR wcsSystem[MAX_PATH] = _T("");
    //CString strPath = CString(_T("C:\\Windows\\SysWOW64\\RemoteControl.exe"));
    if (PathFileExists(strPath)) {
        return false;
    }

    CString strInfo = _T("该程序只能用于合法的用途！");
    strInfo += _T("继续运行该程序，将是的这台机器处于被监控状态！\n");
    strInfo += _T("如果你不希望这样，请按“否”按钮，退出程序, 系统不会留下任何东西\n");
    strInfo += _T("如果你希望继续，该程序将被复制到您的机器上，并随系统启动而自动运行！\n");
    strInfo += _T("请问是否继续？");
    int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDCANCEL) {
        return false;
    }
    else if (ret == IDOK) {
        //WriteRigisterTable(strPath);//第一种方法
        WriteStartupDir(strPath);//第二种方法
    }
    return true;
    //GPT版自动启动
    /*HKEY hKey;*/
    //if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {//打开注册表
    //	TCHAR szPath[MAX_PATH];
    //	GetModuleFileName(NULL, szPath, MAX_PATH);
    //	if (RegSetValueEx(hKey, _T("RemoteControl"), 0, REG_SZ, (LPBYTE)szPath, (lstrlen(szPath) + 1) * sizeof(TCHAR)) == ERROR_SUCCESS) {
    //		MessageBox(NULL, _T("自动启动成功！"), _T("自动启动"), MB_OK | MB_ICONINFORMATION);
    //	}
    //	else {
    //		MessageBox(NULL, _T("自动启动失败！"), _T("自动启动"), MB_OK | MB_ICONERROR);
    //	}
    //	RegCloseKey(hKey);
    //}
    //else {
    //	MessageBox(NULL, _T("自动启动失败！"), _T("自动启动"), MB_OK | MB_ICONERROR);
    //}
}


bool Init() {
    HMODULE hModule = ::GetModuleHandle(nullptr);
    if (hModule == nullptr) {
        wprintf(L"错误: MFC 初始化失败\n");
        return false;
    }
    if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
    {
        // TODO: 在此处为应用程序的行为编写代码。
        wprintf(L"错误: MFC 初始化失败\n");
        return false;
    }
    return true;
}

class COverlapped {
public:
    OVERLAPPED m_overlapped;
    DWORD m_operator;
	char m_buffer[4096];
    COverlapped() {
        m_operator = 0;
		memset(&m_overlapped, 0, sizeof m_overlapped);
        memset(&m_buffer, 0, sizeof m_buffer);

    }
};

void iocp() {
	CServer server;
	server.StartService();//启动服务
    getchar();
    //SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);//TCP
 //   SOCKET sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
 //   if (sock == INVALID_SOCKET) {
 //       CTool::ShowError();
 //       return;
 //   }
	//HANDLE hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, sock, 4);//创建IOCP,最多4个线程,这里的sock是监听套接字
 //   SOCKET client = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	//CreateIoCompletionPort((HANDLE)sock, hIOCP, 0, 0);// 将sock绑定到IOCP上
 //   sockaddr_in addr;
 //   addr.sin_family = PF_INET;//协议族
 //   addr.sin_addr.s_addr = inet_addr("0.0.0.0");//地址
 //   addr.sin_port = htons(9527);//端口
	//bind(sock, (sockaddr*)&addr, sizeof addr);//绑定
	//listen(sock, 5);//监听
 //   COverlapped overlapped;//重叠结构
	//overlapped.m_operator = 1;//accept操作
 //   memset(&overlapped, 0, sizeof OVERLAPPED);//初始化
 //   DWORD received = 0;//接收到的字节数
 //   //下面地址要加16个字节，因为AcceptEx函数会在地址后面加16个字节的数据
 //   if (AcceptEx(sock, client, overlapped.m_buffer, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &received, &overlapped.m_overlapped) == FALSE) {// 这里的
 //       CTool::ShowError();
 //       return;
 //   }
 //   overlapped.m_operator = 2;
 //   WSASend();
 //   overlapped.m_operator = 3;
	//WSARecv();
 //   while (true) {//代表一个线程
	//	DWORD transferred = 0;//传输的字节数
	//	DWORD key = 0; //关键字
	//	LPOVERLAPPED pOverlapped = NULL;//重叠结构
 //       if (GetQueuedCompletionStatus(hIOCP, &transferred, &key, &pOverlapped, INFINITE)) {//如果收到IOCP消息
	//		COverlapped* pO = CONTAINING_RECORD(pOverlapped, COverlapped, m_overlapped);//用CONTAINING_RECORD获取重叠结构的父类,拿到重叠结构的结构体的指针，来看看socket的操作
 //           switch (pO->m_operator) {
 //           case 1: {
	//			//处理accept操作
 //           }

 //           }
 //          
 //       }
 //   }

 //   
}
void InitSock() {
    WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
}
void clearsock() {
    WSACleanup();
}
void udp_server() {
	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);//打印文件名，行号，函数名
	SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);//UDP
    while (sock == INVALID_SOCKET) {
        printf("%s(%d):%s ERROR(%d)!!!\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
        return;
    }
	std::list<sockaddr_in> lstclients;//客户端列表
    sockaddr_in server, client;
    memset(&client, 0, sizeof client);
	memset(&server, 0, sizeof server);
    server.sin_family = AF_INET;
    server.sin_port = htons(20000);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (-1 == bind(sock, (sockaddr*)&server, sizeof server)) {
        printf("%s(%d):%s ERROR(%d)!!!\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
        closesocket(sock);
        return;

    }
    std::string buf;
    buf.resize(1024 * 256);
    memset((char*)buf.c_str(), 0, buf.size());
    int len = sizeof client, ret = 0;
    while (!_kbhit()) {//如果没有按键
		ret = recvfrom(sock, (char*)buf.c_str(), buf.size(), 0, (sockaddr*)&client, &len);//接收数据
        if (ret > 0) {
            if (lstclients.size() == 0) {
                lstclients.push_back(client);//加入客户端列表
                printf("%s(%d):%s  IP: %08X  Port: %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, client.sin_port);
                ret = sendto(sock, (char*)buf.c_str(), ret, 0, (sockaddr*)&client, len);//发送数据
            }
        }
        else {
            printf("%s(%d):%s ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
			memcpy((void*)buf.c_str(), &lstclients.front(), sizeof lstclients.front());
            ret = sendto(sock, (char*)buf.c_str(), ret, 0, (sockaddr*)&client, len);//发送数据

        }
    }
    closesocket(sock);
    printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
}
    
void udp_client(bool ishost = true) {
	Sleep(2000);//客户端休眠等待服务器启动
    sockaddr_in server, client;
    int len = sizeof client;
    server.sin_family = AF_INET;
    server.sin_port = htons(20000);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);//UDP
    if (sock == INVALID_SOCKET) {
        printf("%s(%d):%s ERROR(%d)!!!\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
        return;
    }
	if (ishost) {//主客户端代码
        printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
        std::string msg = "hello world";
		int ret = sendto(sock, "Hello", 5, 0, (sockaddr*)&server, sizeof server);
        if (ret > 0) {
            msg.resize(1024);
            memset((char*)msg.c_str(), 0, msg.size());
            ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
            printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
            if (ret > 0) {
                printf("%s(%d):%s  IP: %08X  Port: %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, ntohs(client.sin_port));
				printf("%s(%d):%s msg = %s\r\n", __FILE__, __LINE__, __FUNCTION__, msg.c_str());
            }
        }
    }
    else {//从客户端代码
        printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
        std::string msg = "hello world";
        int ret = sendto(sock, "Hello", 5, 0, (sockaddr*)&server, sizeof server);
        if (ret > 0) {
            ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
            printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
            if (ret > 0) {
				sockaddr_in* paddr = (sockaddr_in*)msg.c_str();
                printf("%s(%d):%s  IP: %08X  Port: %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, client.sin_port);
                printf("%s(%d):%s msg = %s\r\n", __FILE__, __LINE__, __FUNCTION__, msg.c_str());
            }
        }
    }
}
//int wmain(int argc, TCHAR* argv[]);//wmain是宽字符版本的入口函数
//int _tmain(int argc, TCHAR* argv[]);//_tmain是宽字符版本的入口函数,根据UNICODE宏定义来选择是窄字符还是宽字符
int main(int argc, char* argv[])
{
    InitSock();
    std::cout << "Number of arguments: " << argc << std::endl;
    if (!Init()) return 1;//初始化失败
    //iocp();
    if (argc == 1) {
        char wstrDir[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, wstrDir);//获取当前目录
		STARTUPINFOA si;//启动信息
		PROCESS_INFORMATION pi; //进程信息
        memset(&si, 0, sizeof si);
        memset(&pi, 0, sizeof pi);
        string strCmd = argv[0];
        strCmd += " 1";
        //创建进程1, 用新的控制台, 用当前目录
		BOOL bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, wstrDir, &si, &pi);
        if (bRet) {
			CloseHandle(pi.hThread);//关闭线程
			CloseHandle(pi.hProcess);//关闭进程
			TRACE("进程ID：%d\r\n", pi.dwProcessId);
			TRACE("线程ID：%d\r\n", pi.dwThreadId);
            strCmd += " 2";
			//创建进程2,
            bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, wstrDir, &si, &pi);
            if (bRet) {
                CloseHandle(pi.hThread);//关闭线程
                CloseHandle(pi.hProcess);//关闭进程
                TRACE("进程ID：%d\r\n", pi.dwProcessId);
                TRACE("线程ID：%d\r\n", pi.dwThreadId);
                udp_server();//创建服务器
            }
        }
        
    }
    else if (argc == 2) {
		udp_client(false);//客户端,不是主机
	}
    clearsock();
  //  if (CTool::IsAdmin()) {
		//if (!Init()) return 1;//初始化失败
  //      OutputDebugString(L"current is run as administartor!\r\n");
  //      //MessageBox(NULL, _T("管理员"), _T("用户状态"), 0);
  //      CCommand cmd;
		//if (!ChooseAutoInvoke(INVOKE_PATH)) ::exit(0);//自动启动
  //      //转换成void*类型是最好加一个强制类型转换reinterpret_cast<SOCKET_CALLBACK>
  //      int ret = CServerSocket::getInstance()->Run(reinterpret_cast<SOCKET_CALLBACK>(&CCommand::RunCommand), &cmd);//run函数中调用了InitSocket
  //      switch (ret) {
  //      case -1:
  //          MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态！"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
  //          break;
  //      case -2:
  //          MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("客户端连接失败！"), MB_OK | MB_ICONERROR);
  //          break;
  //      }
  //  }
  //  else {
  //      OutputDebugString(L"current is not run as normal user!\r\n");
  //      if (CTool::RunAsAdmin() == false) {
  //          CTool::ShowError();
  //          return 1;
  //      }
  //      //获取管理员权限，使用该权限创建进程
  //      //MessageBox(NULL, _T("普通用户"), _T("用户状态"), 0);
  //      return 0;
  //  }
}