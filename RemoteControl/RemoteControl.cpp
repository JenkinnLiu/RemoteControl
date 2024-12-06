// RemoteControl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteControl.h"
#include "ServerSocket.h"
#include "Tool.h"
#include "Command.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 唯一的应用程序对象

CWinApp theApp;
using namespace std;

void WriteRigisterTable() {
    CString strSubKey = _T("\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
    CString strPath = CString(_T("C:\\Windows\\SysWOW64\\RemoteControl.exe"));
    char sPath[MAX_PATH] = "";
    char sSys[MAX_PATH] = "";
    std::string strExe = "\\RemoteControl.exe ";
    GetCurrentDirectoryA(MAX_PATH, sPath);
    GetSystemDirectoryA(sSys, sizeof sSys);
    std::string strCmd = "mklink " + std::string(sSys) + std::string(sPath) + "\\RemoteControl.exe";
    system(strCmd.c_str());//执行命令
    HKEY hKey = NULL;
    int ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);//打开注册表
    if (ret != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        MessageBox(NULL, _T("打开注册表失败！是否权限不足？"), _T("开机启动错误"), MB_OK | MB_ICONERROR);
        ::exit(0);
    }

    int ret = RegSetValueEx(hKey, _T("RemoteControl"), 0, REG_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));//写入注册表
    if (ret != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        MessageBox(NULL, _T("写入注册表失败！是否权限不足？"), _T("开机启动错误"), MB_OK | MB_ICONERROR);
        ::exit(0);
    }
    RegCloseKey(hKey);//关闭注册表
}

//开机启动的第二种方法，复制到Windows启动目录
void WriteStartupDir(CString strPath) {
    //CString strPath = _T("C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\StartUp\\RemoteControl.exe");
    CString strCmd = GetCommandLine();
	strCmd.Replace(_T("\""), _T(""));//把双引号去掉
    BOOL ret = CopyFile(strCmd, strPath, FALSE);
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
void ChooseAutoInvoke() {//自动启动
	TCHAR wcsSystem[MAX_PATH] = _T("");
    CString strPath = CString(_T("C:\\Windows\\SysWOW64\\RemoteControl.exe"));
    if(PathFileExists(strPath)){
        return;
    }
    
	CString strInfo = _T("该程序只能用于合法的用途！");
	strInfo += _T("继续运行该程序，将是的这台机器处于被监控状态！\n");
	strInfo += _T("如果你不希望这样，请按“否”按钮，退出程序, 系统不会留下任何东西\n");
	strInfo += _T("如果你希望继续，该程序将被复制到您的机器上，并随系统启动而自动运行！\n");
	strInfo += _T("请问是否继续？");
	int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
	if (ret == IDCANCEL) {
		::exit(0);
    }
    else if (ret == IDOK) {
		//WriteRigisterTable(strPath);//第一种方法
		WriteStartupDir(strPath);//第二种方法
    }
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

void ShowError() {
    LPVOID lpMessageBuf = NULL;
    //strerror(errno);//标准C语言库
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
        (LPWSTR)&lpMessageBuf, 0, NULL);//windows API
    OutputDebugString(lpMessageBuf);
    LocalFree(lpMessageBuf);
}

bool IsAdmin() {//检测是否为管理员
    HANDLE hToken = NULL;
	if (!OpenProcessToken(GetCurrentProcess, TOKEN_QUERY, &hToken)) {//打开进程令牌,获取当前进程的访问令牌
        ShowError();
        return false;
    }
	TOKEN_ELEVATION eve;//提权令牌
    DWORD len = 0;
    if(GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == false){//获取令牌信息
        ShowError();
        return false;
    }
    if (len == sizeof eve) {
		CloseHandle(hToken);
		return eve.TokenIsElevated;//当前令牌是否可以提权
    }
	TRACE("length of tokeninformation is %d\r\n", len);
    return false;
}

int main()
{
    if (IsAdmin()) {
        OutputDebugString(L"current is run as administartor!\r\n");
    }
    else {
        OutputDebugString(L"current is not run as normal user!\r\n");
    }
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误。。
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
			CCommand cmd;
            //转换成void*类型是最好加一个强制类型转换reinterpret_cast<SOCKET_CALLBACK>
            int ret = CServerSocket::getInstance()->Run(reinterpret_cast<SOCKET_CALLBACK>(&CCommand::RunCommand), &cmd);//run函数中调用了InitSocket
            switch (ret) {
            case -1:
                MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态！"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
				exit(0);
				break;
            case -2:
                MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("客户端连接失败！"), MB_OK | MB_ICONERROR);
                exit(0);
                break;
            }
			
          
		}
        // TODO: 在此处为应用程序的行为编写代码。
        //WSADATA data;
        //WSAStartup(MAKEWORD(1, 1), &data);//TODO: 返回值处理

            
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
