#pragma once
#include <unordered_map>
#include "ServerSocket.h"
#include<stdio.h>
#include<io.h>
#include<list>
#include<atlimage.h>
#include<direct.h>
#include "Tool.h"
#include "LockDialog.h"
#include "resource.h"

class CCommand{
public:
	CCommand();
    ~CCommand() {};
	int ExcuteCommand(int nCmd);
protected:
	typedef int(CCommand::* CMDFUNC)();//��Ա����ָ��
	std::unordered_map<int, CMDFUNC> m_mapFunction;//������ŵ����ܵ�ӳ��map

    CLockDialog dlg;//ȫ�ֱ���
    unsigned threadId;//�߳�ID

    static unsigned __stdcall threadLockDlg(void* arg) {//�̺߳���
		CCommand* thiz = (CCommand*)arg;
		thiz->threaLockDlgMain();//���ó�Ա����
        _endthreadex(0);//�����˽����߳�
        return 0;
    }
    void threaLockDlgMain() {
        dlg.Create(IDD_DIALOG_INFO, NULL);//��������
        dlg.ShowWindow(SW_SHOW);//��ʾ����


        //dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);//�����ö�
        //��ȡ�豸��Ļ�ֱ��ʲ����ô����������½�
        int nWidth = GetSystemMetrics(SM_CXSCREEN);//��ȡ��Ļ����
        int nHeight = GetSystemMetrics(SM_CYSCREEN);//��ȡ��Ļ�߶�
        dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);


        //���������Χ�͹���
        ShowCursor(false);//�������
        ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);//����������

        CRect rect;//����
        dlg.GetWindowRect(rect);//��ȡ���ھ���
        rect.right = rect.left + 1;
        rect.bottom = rect.top + 1;//���������Χ
        ClipCursor(rect);//�������ֻ���ڴ����ڻ

        //������Ϣѭ��
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);//������Ϣ
            DispatchMessage(&msg);//�ַ���Ϣ
            if (msg.message == WM_KEYDOWN) {
                TRACE("msg%08X lparam:%08X wparam:%08x\r\n", msg.message, msg.wParam, msg.lParam);//wParam���ǰ�����ASCII�룬lParam�Ǽ��̵�ɨ����
                if (msg.wParam == 0x1B) {//����ESC��(0x1B��ESC����ASCII�룩

                    break;
                }


            }
        }
        dlg.DestroyWindow();//���ٴ���
        ClipCursor(NULL);//�������ֻ���ڴ����ڻ
        ShowCursor(true);
        ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);//������ʾ������

    }

    int MakeDriverInfo() {//�������̷�����Ϣ, 1->A, 2->B, 3->C,..., 26->Z
        std::string result;
        for (int i = 1; i <= 26; i++)
            if (_chdrive(i) == 0) {//�ı��������
                if (result.size() > 0) result += ',';
                result += 'A' + i - 1;//�õ������̷�
            }


        CPacket pack(1, (BYTE*)result.c_str(), result.size());
        CTool::Dump((BYTE*)pack.Data(), pack.Size());
        CServerSocket::getInstance()->Send(pack);
        //CServerSocket::getInstance()->Send(CPacket(1, (BYTE*)result.c_str(), result.size()));
        return 0;
    }
    int MakeDirectoryInfo() {
        std::string strPath;
        //std::list<FILEINFO> lstFileInfos;
        if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
            OutputDebugString(_T("��ǰ������ǻ�ȡ�ļ��б�������������󣡣�"));
            return -1;
        }
        if (_chdir(strPath.c_str()) != 0) {//�л�Ŀ¼ʧ��
            FILEINFO finfo;
            finfo.HasNext = FALSE;
            CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
            CServerSocket::getInstance()->Send(pack);
            OutputDebugString(_T("û��Ȩ�ޣ�����Ŀ¼����"));
            return -2;
        }
        _finddata_t fdata;
        //���x64ϵͳ��while (_findnext(hfind,&fdata)== 0)���д��뱨�쳣�����Խ� hfind ����Ϊ intptrt�� ȷ���˾�����Ա������ش洢��
        intptr_t hfind = 0; //��longlong�棬����ᱬint
        if ((hfind = _findfirst("*", &fdata)) == -1) {//����*�ļ�ʧ�ܣ�û���ļ�
            OutputDebugString(_T("û���ҵ��κ��ļ�����"));
            FILEINFO finfo;
            finfo.HasNext = FALSE;
            CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
            CServerSocket::getInstance()->Send(pack);
            return -3;
        }
        int count = 0;
        do {
            FILEINFO finfo;
            finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;//�ж��Ƿ�Ϊ�ļ���
            memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
            //lstFileInfos.push_back(finfo);//�Ž��б���
            TRACE("%s\r\n", finfo.szFileName);
            CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
            CServerSocket::getInstance()->Send(pack);
            count++;
        } while (!_findnext(hfind, &fdata));//x64ϵͳ��hfind��longlong����intptr_t�棬����ᱬint
        TRACE("�����������ļ�������server:count = %d\r\n", count);
        //������Ϣ�����ƶ�
        FILEINFO finfo;
        finfo.HasNext = FALSE;
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }

    int RunFile() {
        std::string strPath;
        CServerSocket::getInstance()->GetFilePath(strPath);
        ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);//ִ���ļ�
        CPacket pack(3, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }

    int DownloadFile() {
        std::string strPath;
        CServerSocket::getInstance()->GetFilePath(strPath);
        long long data = 0;
        FILE* pFile = NULL;
        //�����fopen����C4996��Ҫô����fopen_s��Ҫô����_CRT_SECURE_NO_WARNINGS,Ҫô#pragma warning(disable:4996)
        errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");//�ı��ļ��������Ʒ�ʽ��
        if (err != 0) {
            CPacket pack(4, (BYTE*)&data, 8);//��ʧ�ܣ�����NULL����
            TRACE("���ļ�ʧ�ܣ���\r\n");
            CServerSocket::getInstance()->Send(pack);
            return -1;
        }
        if (pFile != NULL) {
            fseek(pFile, 0, SEEK_END);//��һ���ļ��ĳ���
            data = _ftelli64(pFile);
            CPacket head(4, (BYTE*)&data, 8);//�����ļ�����
            CServerSocket::getInstance()->Send(head);//�����ļ�
            fseek(pFile, 0, SEEK_SET);
            char buffer[1024] = "";
            size_t rlen = 0;
            do {
                rlen = fread(buffer, 1, 1024, pFile);//һ�ζ�1�ֽڣ���1024��
                CPacket pack(4, (BYTE*)buffer, rlen);
                CServerSocket::getInstance()->Send(pack);
            } while (rlen >= 1024);
            fclose(pFile);//�����˹��ļ�
        }
        CPacket pack(4, NULL, 0);//�յ�NULL������β
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }

    int MouseEvent() {
        MOUSEEV mouse;
        if (CServerSocket::getInstance()->GetMouseEvent(mouse)) {
            DWORD nflags = 0;
            switch (mouse.nButton) {
            case 0://���
                nflags = 1;
                break;
            case 1://�Ҽ�
                nflags = 2;
                break;
            case 2://�м�
                nflags = 4;
                break;
            case 4://û�а���
                nflags = 8;

            default:
                break;
            }
            if (nflags != 8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
            switch (mouse.nAction) {
            case 0://����
                nflags |= 0x10;
                break;
            case 1://˫��
                nflags |= 0x20;
                break;
            case 2://����
                nflags |= 0x40;
                break;
            case 3://�ſ�
                nflags |= 0x80;
                break;
            default:
                break;
            }
            TRACE("��������%08X x:%d y:%d\r\n", nflags, mouse.ptXY.x, mouse.ptXY.y);
            switch (nflags) {
                //mouse_event()����ģ��������
            case 0x21://���˫������ִ��˫��������mouse_event,��ִ�е���������mouse_event
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x11://�������
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x41://�������
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x81://����ſ�
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x22://�Ҽ�˫��
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x12://�Ҽ�����
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x42://�Ҽ�����
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x82://�Ҽ��ſ�
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x24://�м�˫��
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x14://�м�����
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x44://�м�����
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x84://�м��ſ�
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x08://��������ƶ�
                mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
                break;
            default:
                break;
            }
            CPacket pack(4, NULL, 0);//�������˾ʹ�һ��NULL��������Ϣִ�����
            CServerSocket::getInstance()->Send(pack);
        }
        else {
            OutputDebugString(_T("��ȡ����������ʧ�ܣ���"));
            return -1;
        }
        return 0;
    }

    int SendScreen() {
        CImage screen;//GDI ȫ���豸�ӿ�
        HDC hScreen = ::GetDC(NULL);//��ȡ�豸����
        int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);//��ȡ����λ��
        int nWidth = GetDeviceCaps(hScreen, HORZRES);//��ȡ��ʾ�豸��ˮƽ�ֱ���(1920)
        int nHeight = GetDeviceCaps(hScreen, VERTRES);//��ȡ��ʾ�豸�Ĵ�ֱ�ֱ���(1080)
        screen.Create(nWidth, nHeight, nBitPerPixel);//����һ��ָ����С��λͼ
        BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);//������Ļ��ͼ��λͼ,��ͼƬ���д���
        ReleaseDC(NULL, hScreen);//�ͷ��豸������
        //PNGͼƬ����ʱ�䳤����CPU�࣬��ͼƬ��С�Ƚ�С����ʡ����
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);//�����ڴ�
        if (hMem == NULL) return -1;
        IStream* pStream = NULL;//��
        HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);//������

        if (ret == S_OK) {//����������ɹ�
            screen.Save(pStream, Gdiplus::ImageFormatPNG);//����PNGͼƬ��������
            //screen.Save(_T("screen.png"), Gdiplus::ImageFormatPNG);//ֱ�ӱ���PNGͼƬ
            LARGE_INTEGER bg = { 0 };//������
            pStream->Seek(bg, STREAM_SEEK_SET, NULL);// �����Ŀ�ͷ��ʼ
            PBYTE pData = (PBYTE)GlobalLock(hMem);//�����ڴ�,�����Ϳ��Զ��ڴ�
            SIZE_T nSize = GlobalSize(hMem);//��ȡ�ڴ��С
            CPacket pack(6, pData, nSize);//���߿��ƶ�ͼƬ��ʼ����
            CServerSocket::getInstance()->Send(pack);
            GlobalUnlock(hMem);//�����ڴ�,��ʾ�������

        }
        pStream->Release();//�ͷ���
        screen.ReleaseDC();
        GlobalFree(hMem);//�ͷ��ڴ�
        return 0;
    }
	int LockMachine() {
		if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {//������ڲ�����
			//����_beginthreadex�����̣߳���Ϊָ�������߳�ID���޷����̷߳�����Ϣ
			_beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadId);//�����߳�
		}
		CPacket pack(7, NULL, 0);//���߿��ƶ˴����Ѿ�����
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	int UnlockMachine() {
		//dlg.SendMessage(WM_KEYDOWN, 0x1B, 0x01E0001);//����esc������Ϣ, 0x01E0001�Ǽ��̵�ɨ����, ɨ������ָ�����ϵ�ÿ������Ӧ��һ��ֵ
		PostThreadMessage(threadId, WM_KEYDOWN, 0x1B, 0x01E0001);//���̷߳���esc������Ϣ,threadId����GetCurrentThreadId()Ҳ����
		//��Ϣ����Ҫ>30ms�����Ϊ���߳�ͬ����sleepҪ��������30ms
		CPacket pack(8, NULL, 0);//���߿��ƶ˴����Ѿ�����
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	int TestConnect() {
		CPacket pack(1981, NULL, 0);
		bool ret = CServerSocket::getInstance()->Send(pack);
		TRACE("Send ret :%d\r\n", ret);
		return 0;
	}

	int DeleteLocalFile() {
		std::string strPath;
		CServerSocket::getInstance()->GetFilePath(strPath);
		TCHAR sPath[MAX_PATH] = _T("");
		//mbstowcs(sPath, strPath.c_str(), strPath.size());//���ֽ�ת���ֽ��ַ���,����������
		MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(),
			sPath, sizeof sPath / sizeof(TCHAR));//���ֽ�ת���ֽ��ַ���, sizeof sPath / sizeof(TCHAR)���ַ���
		DeleteFileA(strPath.c_str());//ɾ���ļ�
		CPacket pack(9, NULL, 0);
		bool ret = CServerSocket::getInstance()->Send(pack);
		TRACE("Send ret :%d\r\n", ret);
		return 0;
	}
};
