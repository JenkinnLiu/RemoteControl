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

    static bool IsAdmin() {//����Ƿ�Ϊ����Ա
        HANDLE hToken = NULL;
        if (!OpenProcessToken(GetCurrentProcess, TOKEN_QUERY, &hToken)) {//�򿪽�������,��ȡ��ǰ���̵ķ�������
            ShowError();
            return false;
        }
        TOKEN_ELEVATION eve;//��Ȩ����
        DWORD len = 0;
        if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == false) {//��ȡ������Ϣ
            ShowError();
            return false;
        }
        if (len == sizeof eve) {
            CloseHandle(hToken);
            return eve.TokenIsElevated;//��ǰ�����Ƿ������Ȩ
        }
        TRACE("length of tokeninformation is %d\r\n", len);
        return false;
    }
    static bool RunAsAdmin() {//��Ȩ,�Թ���Ա�ķ�ʽ����
        //���ز����飬 ����Administrator�˻��� ��ֹ������ֻ�ܵ�¼���ؿ���̨
        STARTUPINFO si = { 0 };//����������Ϣ
        _PROCESS_INFORMATION pi = { 0 };//������Ϣ
        TCHAR sPath[MAX_PATH] = _T("");
        GetModuleFileName(NULL, sPath, MAX_PATH);//��ȡ��ǰģ����Ŀ���ļ���

        //ret = CreateProcessWithTokenW(hToken, LOGON_WITH_PROFILE, NULL, (LPWSTR)(LPCWSTR)strCmd, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);//��������
        BOOL ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON32_LOGON_BATCH, NULL, sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);//��������,�Թ���Ա�ķ�ʽ����
        if (!ret) {
            //ShowError();
            MessageBox(NULL, _T("��������ʧ��"), _T("����"), 0);
            return false;
        }
        WaitForSingleObject(pi.hProcess, INFINITE);//�ȴ����̽���
        CloseHandle(pi.hProcess);//�رս��̾��
        CloseHandle(pi.hThread);//�ر��߳̾��
		return true;
    }
    static void ShowError() {
        LPWSTR lpMessageBuf = NULL;
        //strerror(errno);//��׼C���Կ�
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&lpMessageBuf, 0, NULL);//windows API
        OutputDebugString(lpMessageBuf);
        LocalFree(lpMessageBuf);
    }

};

