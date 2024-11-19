// RemoteControl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteControl.h"
#include "ServerSocket.h"
#include <direct.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;
void Dump(BYTE* pData, size_t nSize) {
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

int MakeDriverInfo() {//创建磁盘分区信息, 1->A, 2->B, 3->C,..., 26->Z
    std::string result;
    for (int i = 1; i <= 26; i++)
        if (_chdrive(i) == 0) {//改变磁盘驱动
            if (result.size() > 0) result += ',';
            result += 'A' + i - 1;//拿到可用盘符
        }

    
    CPacket pack(1, (BYTE*)result.c_str(), result.size());
    Dump((BYTE*)pack.Data(), pack.Size());
    CServerSocket::getInstance()->Send(pack);
    //CServerSocket::getInstance()->Send(CPacket(1, (BYTE*)result.c_str(), result.size()));
    return 0;
}
#include<stdio.h>
#include<io.h>
#include<list>


int MakeDirectoryInfo() {
    std::string strPath;
    //std::list<FILEINFO> lstFileInfos;
    if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
        OutputDebugString(_T("当前的命令不是获取文件列表，命令解析错误！！"));
        return -1;
    }
    if (_chdir(strPath.c_str()) != 0) {//切换目录失败
        FILEINFO finfo;
        finfo.HasNext = FALSE;
        CPacket pack(2, (BYTE*)& finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        OutputDebugString(_T("没有权限，访问目录！！"));
        return -2;
    }
    _finddata_t fdata;
    //如果x64系统下while (_findnext(hfind,&fdata)== 0)这行代码报异常，可以将 hfind 定义为 intptrt来 确保了句柄可以被完整地存储。
    intptr_t hfind = 0; //用longlong存，否则会爆int
    if ((hfind = _findfirst("*", &fdata)) == -1) {//查找*文件失败，没有文件
        OutputDebugString(_T("没有找到任何文件！！"));
        FILEINFO finfo;
        finfo.HasNext = FALSE;
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        return -3;
    }
    do {
        FILEINFO finfo;
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;//判断是否为文件夹
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
        //lstFileInfos.push_back(finfo);//放进列表内
        TRACE("%s\r\n", finfo.szFileName);
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
	} while (!_findnext(hfind, &fdata));//x64系统的hfind用longlong，即intptr_t存，否则会爆int
    //发送信息到控制端
    FILEINFO finfo;
    finfo.HasNext = FALSE;
    CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int RunFile() {
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);//执行文件
    CPacket pack(3, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int DownloadFile() {
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    long long data = 0;
    FILE* pFile = NULL;
    //如果用fopen报错C4996，要么改用fopen_s，要么添加_CRT_SECURE_NO_WARNINGS,要么#pragma warning(disable:4996)
    errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");//文本文件按二进制方式读
    if (err != 0) {
        CPacket pack(4, (BYTE*)&data, 8);//打开失败，传个NULL结束
        TRACE("打开文件失败！！\r\n");
        CServerSocket::getInstance()->Send(pack);
        return -1;
    }
    if (pFile != NULL) {
        fseek(pFile, 0, SEEK_END);//拿一个文件的长度
        data = _ftelli64(pFile);
        CPacket head(4, (BYTE*)&data, 8);//发送文件长度
		CServerSocket::getInstance()->Send(head);//发送文件
        fseek(pFile, 0, SEEK_SET);
        char buffer[1024] = "";
        size_t rlen = 0;
        do {
            rlen = fread(buffer, 1, 1024, pFile);//一次读1字节，读1024次
            CPacket pack(4, (BYTE*)buffer, rlen);
			CServerSocket::getInstance()->Send(pack);
        } while (rlen >= 1024);
        fclose(pFile);//别忘了关文件
    }
    CPacket pack(4, NULL, 0);//收到NULL代表结尾
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int MouseEvent() {
    MOUSEEV mouse;
    if (CServerSocket::getInstance()->GetMouseEvent(mouse)) {
        DWORD nflags = 0;
        switch (mouse.nButton){
            case 0://左键
                nflags = 1;
                break;
            case 1://右键
                nflags = 2;
                break;
            case 2://中键
                nflags = 4;
                break;
            case 4://没有按键
                nflags = 8;

        default:
            break;
        }
        if (nflags != 8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
        switch (mouse.nAction){
            case 0://单击
                nflags |= 0x10;
                break;
            case 1://双击
                nflags |= 0x20;
                break;
            case 2://按下
                nflags |= 0x40;
                break;
            case 3://放开
                nflags |= 0x80;
                break;
        default:
            break;
        }
        switch (nflags){
			//mouse_event()用于模拟鼠标操作
            case 0x21://左键双击，先执行双击的两个mouse_event,再执行单击的两个mouse_event
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x11://左键单击
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x41://左键按下
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x81://左键放开
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x22://右键双击
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x12://右键单击
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x42://右键按下
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x82://右键放开
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x24://中键双击
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x14://中键单击
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x44://中键按下
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x84://中键放开
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x08://单纯鼠标移动
                mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
                break;
        default:
            break;
        }
        CPacket pack(4, NULL, 0);//操作好了就传一个NULL，表明消息执行完成
        CServerSocket::getInstance()->Send(pack);
    }
    else {
        OutputDebugString(_T("获取鼠标操作参数失败！！"));
        return -1;
    }
    return 0;
}

#include<atlimage.h>//传图片
int SendScreen() {
    CImage screen;//GDI 全局设备接口
    HDC hScreen = ::GetDC(NULL);//获取设备参数
	int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);//获取像素位数
	int nWidth = GetDeviceCaps(hScreen, HORZRES);//获取显示设备的水平分辨率(1920)
	int nHeight = GetDeviceCaps(hScreen, VERTRES);//获取显示设备的垂直分辨率(1080)
	screen.Create(nWidth, nHeight, nBitPerPixel);//创建一个指定大小的位图
	BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);//拷贝屏幕截图到位图,将图片进行传输
	ReleaseDC(NULL, hScreen);//释放设备上下文
	//PNG图片生成时间长，耗CPU多，但图片大小比较小，节省带宽
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);//分配内存
    if (hMem == NULL) return -1;
	IStream* pStream = NULL;//流
	HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);//创建流
	
	if (ret == S_OK) {//如果创建流成功
        screen.Save(pStream, Gdiplus::ImageFormatPNG);//保存PNG图片到流里面
        //screen.Save(_T("screen.png"), Gdiplus::ImageFormatPNG);//直接保存PNG图片
		LARGE_INTEGER bg = { 0 };//大整数
		pStream->Seek(bg, STREAM_SEEK_SET, NULL);// 从流的开头开始
		PBYTE pData = (PBYTE)GlobalLock(hMem);//锁定内存,这样就可以读内存
		SIZE_T nSize = GlobalSize(hMem);//获取内存大小
		CPacket pack(6, pData, nSize);//告诉控制端图片开始传输
		CServerSocket::getInstance()->Send(pack);
        GlobalUnlock(hMem);//解锁内存,表示访问完成
		
	}
	pStream->Release();//释放流
    screen.ReleaseDC();
    GlobalFree(hMem);//释放内存
    return 0;
}

#include "LockDialog.h"
CLockDialog dlg;//全局变量
unsigned threadId = 0;//线程ID

unsigned __stdcall threadLockDlg(void* arg) {//线程函数
    dlg.Create(IDD_DIALOG_INFO, NULL);//创建窗口
    dlg.ShowWindow(SW_SHOW);//显示窗口
    //dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);//窗口置顶
    //获取设备屏幕分辨率并将该窗口置于右下角
    int nWidth = GetSystemMetrics(SM_CXSCREEN);//获取屏幕宽度
    int nHeight = GetSystemMetrics(SM_CYSCREEN);//获取屏幕高度
    dlg.SetWindowPos(&dlg.wndTopMost, nWidth - 300, nHeight - 10, 300, 10, SWP_NOSIZE | SWP_SHOWWINDOW);


    //限制鼠标活动范围和功能
    ShowCursor(false);//隐藏鼠标
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);//隐藏任务栏
    
    CRect rect;//矩形
    dlg.GetWindowRect(rect);//获取窗口矩形
    rect.right = rect.left + 1;
    rect.bottom = rect.top + 1;//限制鼠标活动范围
    ClipCursor(rect);//限制鼠标只能在窗口内活动

    //建立消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);//翻译消息
        DispatchMessage(&msg);//分发消息
        if (msg.message == WM_KEYDOWN) {
            TRACE("msg%08X lparam:%08X wparam:%08x\r\n", msg.message, msg.wParam, msg.lParam);//wParam就是按键的ASCII码，lParam是键盘的扫描码
            if (msg.wParam == 0x1B) {//按下ESC键(0x1B是ESC键的ASCII码）
        
                break;
            }


        }
    }
    dlg.DestroyWindow();//销毁窗口
    ShowCursor(true);
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);//重新显示任务栏

	_endthreadex(0);//别忘了结束线程
	return 0;
}

int LockMachine() {
	if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {//如果窗口不存在
		//不用_beginthreadex创建线程，因为指定不了线程ID，无法向线程发送消息
        _beginthreadex(NULL, 0, threadLockDlg, NULL, 0, &threadId);//创建线程
    }
    CPacket pack(7, NULL, 0);//告诉控制端窗口已经存在
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int UnlockMachine() {
	//dlg.SendMessage(WM_KEYDOWN, 0x1B, 0x01E0001);//发送esc按键消息, 0x01E0001是键盘的扫描码, 扫描码是指键盘上的每个键对应的一个值
	PostThreadMessage(threadId, WM_KEYDOWN, 0x1B, 0x01E0001);//向线程发送esc按键消息,threadId换成GetCurrentThreadId()也可以
    //消息泵需要>30ms，因此为了线程同步，sleep要尽量大于30ms
    CPacket pack(8, NULL, 0);//告诉控制端窗口已经销毁
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
	//mbstowcs(sPath, strPath.c_str(), strPath.size());//多字节转宽字节字符集,但容易乱码
    MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(), 
		sPath, sizeof sPath / sizeof(TCHAR));//多字节转宽字节字符集, sizeof sPath / sizeof(TCHAR)是字符数
	DeleteFileA(strPath.c_str());//删除文件
    CPacket pack(9, NULL, 0);
    bool ret = CServerSocket::getInstance()->Send(pack);
    TRACE("Send ret :%d\r\n", ret);
    return 0;
}

int ExecuteCmd(int nCmd) {
    int ret = 0;
    switch (nCmd) {
    case 1://查看磁盘分区信息
        ret = MakeDriverInfo();
        break;
    case 2://查看指定目录下的文件
        ret = MakeDirectoryInfo();
    case 3://打开文件
        ret = RunFile();
        break;
    case 4://下载文件
        ret = DownloadFile();
        break;
    case 5://鼠标操作
        ret = MouseEvent();
        break;
    case 6://发送屏幕内容->发送屏幕的截图
        ret = SendScreen();
        break;
    case 7: //锁机
        ret = LockMachine();
        break;
    case 8://解锁
        ret = UnlockMachine();
        break;
    case 9://删除文件 
        ret = DeleteLocalFile();
    case 1981:
        ret = TestConnect();
		break;
    }
	return ret;
}

int main()
{
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
            CServerSocket* pserver = CServerSocket::getInstance();
            int count = 0;
            if (pserver->InitSocket() == false) {
                MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态！"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
                exit(0);
			}//初始化只需调用一次即可，accept需调用多次
			while (CServerSocket::getInstance() != NULL) {
				if (pserver->AcceptClient()) {
                    TRACE("AcceptClient成功: %d， 服务器开始调用dealcommand\r\n");
					int ret = pserver->DealCommand();//接收客户端的命令
                    TRACE("DealCommand: %d\r\n", ret);
					if (ret > 0) {//处理成功
						TRACE("服务器接收到的命令: %d\r\n", pserver->GetPacket().sCmd);
                        ret = ExecuteCmd(ret);//执行命令
						if (ret > 0) {
                            TRACE("命令执行失败: %d ret = %d\r\n", pserver->GetPacket().sCmd, ret);
						}
						pserver->CloseClient();//关闭客户端
                    }
                    
                }
                else {
                    if (count >= 3) {
                        MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("客户端连接失败！"), MB_OK | MB_ICONERROR);
					    exit(0);
                    }
					MessageBox(NULL, _T("无法正常接入用户，自动重试！"), _T("客户端连接失败！"), MB_OK | MB_ICONERROR);
                    count++;
                }
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
