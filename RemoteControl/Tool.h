#pragma once
class CTool
{
public:
    static void Dump(BYTE* pData, size_t nSize) {
        std::string strOut;
        for (size_t i = 0; i < nSize; i++) {
            char buf[8] = "";
            if (i > 0 && (i % 16 == 0)) strOut += "\n";
            snprintf(buf, sizeof buf, "%02X ", pData[i] & 0xFF);
            strOut += buf;
        }
        strOut += "\n";
        OutputDebugStringA(strOut.c_str());
    }

    static bool IsAdmin() {//检测是否为管理员
        HANDLE hToken = NULL;
        if (!OpenProcessToken(GetCurrentProcess, TOKEN_QUERY, &hToken)) {//打开进程令牌,获取当前进程的访问令牌
            ShowError();
            return false;
        }
        TOKEN_ELEVATION eve;//提权令牌
        DWORD len = 0;
        if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == false) {//获取令牌信息
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
    static bool RunAsAdmin() {//提权,以管理员的方式启动
        //本地策略组， 开启Administrator账户， 禁止空密码只能登录本地控制台
        STARTUPINFO si = { 0 };//进程启动信息
        _PROCESS_INFORMATION pi = { 0 };//进程信息
        TCHAR sPath[MAX_PATH] = _T("");
        GetModuleFileName(NULL, sPath, MAX_PATH);//获取当前模块项目的文件名

        //ret = CreateProcessWithTokenW(hToken, LOGON_WITH_PROFILE, NULL, (LPWSTR)(LPCWSTR)strCmd, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);//创建进程
        BOOL ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON32_LOGON_BATCH, NULL, sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);//创建进程,以管理员的方式启动
        if (!ret) {
            //ShowError();
            MessageBox(NULL, _T("创建进程失败"), _T("错误"), 0);
            return false;
        }
        WaitForSingleObject(pi.hProcess, INFINITE);//等待进程结束
        CloseHandle(pi.hProcess);//关闭进程句柄
        CloseHandle(pi.hThread);//关闭线程句柄
		return true;
    }
    static void ShowError() {
        LPWSTR lpMessageBuf = NULL;
        //strerror(errno);//标准C语言库
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&lpMessageBuf, 0, NULL);//windows API
        OutputDebugString(lpMessageBuf);
        LocalFree(lpMessageBuf);
    }

};

