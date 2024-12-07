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
    if(PathFileExists(strPath)){
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
HANDLE hIOCP = INVALID_HANDLE_VALUE;//IO Completion Port

#define IOCP_LIST_ADD 0
#define IOCP_LIST_PUSH 1
#define IOCP_LIST_POP 2

enum {
    IocpListEmpty, 
	IocpListPush,
	IocpListPop
};

typedef struct IocpParam {
    int nOperator;//操作
    std::string strData;//数据
    _beginthread_proc_type cbFunc;//回调函数
    HANDLE hEvent;//pop操作需要的事件
    IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL) {
        nOperator = op;
        strData = sData;
        cbFunc = cb;
    }
    IocpParam() {
        nOperator = -1;
    }
}IOCP_PARAM;//POST Parameter, IOCP中用于传递参数的结构体

void threadmain(HANDLE hIOCP) {
    std::list<std::string> lstString;
    DWORD dwTransferred = 0;//传输的字节数
    ULONG_PTR CompletionKey = 0;//传输的数据
    OVERLAPPED* pOverlapped = NULL;//重叠结构
    int count = 0, count0 = 0;
    while (GetQueuedCompletionStatus(hIOCP, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE)) {//唤醒IOCP，开始实现请求

        if (dwTransferred == 0 && CompletionKey == NULL) {//线程结束退出
            printf("thread is ready to exit!\r\n");
            break;
        }
        IOCP_PARAM* pParam = (IOCP_PARAM*)CompletionKey;//获取传递的参数,CompletionKey是一个指针
        if (pParam->nOperator == IocpListPush) {//如果是push操作
            lstString.push_back(pParam->strData);//添加到list
            count++;
        }
        else if (pParam->nOperator == IocpListPop) {//如果是pop操作
            std::string* pStr = NULL;
            if (lstString.size() > 0) {
                std::string* pstr = new std::string(lstString.front());//取出list的第一个元素
                lstString.pop_front();//删除第一个元素
            }
            if (pParam->cbFunc) {
                pParam->cbFunc(pStr);
            }
            count0++;
        }
        else if (pParam->nOperator == IocpListEmpty) {
            lstString.clear();
        }
        delete pParam;//释放内存

    }
    printf("list size:%d, push:%d, pop:%d\r\n", lstString.size(), count, count0);
}

void threadQueueEntry(HANDLE hIOCP) {
    
	threadmain(hIOCP);//这里要加一个线程函数，阻止内存泄漏，析构变量
	_endthread();//代码到此为止，会导致本地变量无法调用析构吗，从而使得内存发生泄漏
}

void func(void* arg) {//回调函数
    std::string* pstr = (std::string*)arg;
    if (pstr != NULL) {
        printf("pop frpm list:%s\r\n", arg);
        delete pstr;
    }
    else {
		printf("list is empty:NULL\r\n");
    }
}

int main()
{
    if (!Init()) return 1;//初始化失败
    printf("press any key to exit!\r\n");
    CQueue<std::string> lstStrings;
    ULONGLONG tick0 = GetTickCount64(), tick = GetTickCount64();
 //   HANDLE hIOCP = INVALID_HANDLE_VALUE;    
	//hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);//创建IOCP,1表示只有一个线程处理, 允许多线程处理是与epoll的区别1
 //   if (hIOCP == NULL || hIOCP == INVALID_HANDLE_VALUE) {
	//	printf("create IOCP failed! %d\r\n", GetLastError());
	//	return 1;
 //   }
 //   HANDLE hThread = (HANDLE)_beginthread(threadQueueEntry, 0, hIOCP);//创建一个线程，用于处理IOCP
	//
 //   ULONGLONG tick = GetTickCount64();
	//ULONGLONG tick0 = GetTickCount64();
 //   int count = 0, count0 = 0;

	while (_kbhit() != 0) {//如果按下任意键,完成端口把请求和实现分离了，请求是由PostQueuedCompletionStatus函数发出的，实现是由GetQueuedCompletionStatus函数完成的
		if (GetTickCount64() - tick > 1300) {//每隔1.3s读一次状态
            //PostQueuedCompletionStatus(hIOCP, sizeof IOCP_PARAM, (ULONG_PTR)new IOCP_PARAM(IocpListPop, "hellp world", func), NULL);//传递端口的状态的句柄给hIOCP
            lstStrings.PushBack("hello world");
            tick0 = GetTickCount64();
        }
		if (GetTickCount64() - tick0 > 2000) {//每隔2s写一次状态
            std::string str;
			//PostQueuedCompletionStatus(hIOCP, sizeof IOCP_PARAM, (ULONG_PTR)new IOCP_PARAM(IocpListPush, "hellp world"), NULL);//传递端口的状态的句柄给hIOCP
            lstStrings.PopFront(str);
            tick0 = GetTickCount64();//重新计时
			printf("pop from queue: %s\r\n", str.c_str());
		}
        Sleep(1);
    }
  //  if (hIOCP != NULL) {//IOCP创建成功
		//PostQueuedCompletionStatus(hIOCP, 0, NULL, NULL);//唤醒IOCP,传递端口的状态的句柄给hIOCP
		//WaitForSingleObject(hIOCP, INFINITE);//等待IOCP的线程结束
  //  }
    CloseHandle(hIOCP);
	printf("exit done! %d", lstStrings.Size());
    ::exit(0);
    

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
