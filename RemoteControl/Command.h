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
#include "Packet.h"

class CCommand {
public:
	CCommand();
	~CCommand() {};
	int ExcuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket);
	static void RunCommand(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket) {
		CCommand* thiz = (CCommand*)arg;
		if (status > 0) {
			int ret = thiz->ExcuteCommand(status, lstPacket, inPacket);
			if (ret != 0) {
				TRACE("命令执行失败: %d ret = %d\r\n", status, ret);
			}
		}
		else {
			MessageBox(NULL, _T("无法正常接入用户，自动重试！"), _T("客户端连接失败！"), MB_OK | MB_ICONERROR);
		}
	}
protected:
	typedef int(CCommand::* CMDFUNC)(std::list<CPacket>&, CPacket& inPacket);//成员函数指针
	std::unordered_map<int, CMDFUNC> m_mapFunction;//从命令号到功能的映射map

	CLockDialog dlg;//全局变量
	unsigned threadId;//线程ID

	static unsigned __stdcall threadLockDlg(void* arg) {//线程函数
		CCommand* thiz = (CCommand*)arg;
		thiz->threaLockDlgMain();//调用成员函数
		_endthreadex(0);//别忘了结束线程
		return 0;
	}
	void threaLockDlgMain() {
		dlg.Create(IDD_DIALOG_INFO, NULL);//创建窗口
		dlg.ShowWindow(SW_SHOW);//显示窗口


		//dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);//窗口置顶
		//获取设备屏幕分辨率并将该窗口置于右下角
		int nWidth = GetSystemMetrics(SM_CXSCREEN);//获取屏幕宽度
		int nHeight = GetSystemMetrics(SM_CYSCREEN);//获取屏幕高度
		dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);


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
		ClipCursor(NULL);//限制鼠标只能在窗口内活动
		ShowCursor(true);
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);//重新显示任务栏

	}

	int MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) {//创建磁盘分区信息, 1->A, 2->B, 3->C,..., 26->Z
		std::string result;
		for (int i = 1; i <= 26; i++)
			if (_chdrive(i) == 0) {//改变磁盘驱动
				if (result.size() > 0) result += ',';
				result += 'A' + i - 1;//拿到可用盘符
			}

		lstPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size()));//将发送的包放到包列表里
		return 0;
	}
	int MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		std::string strPath = inPacket.strData;
		//std::list<FILEINFO> lstFileInfos;
		if (_chdir(strPath.c_str()) != 0) {//切换目录失败
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
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
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			return -3;
		}
		int count = 0;
		do {
			FILEINFO finfo;
			finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;//判断是否为文件夹
			memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
			//lstFileInfos.push_back(finfo);//放进列表内
			TRACE("%s\r\n", finfo.szFileName);
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			count++;
		} while (!_findnext(hfind, &fdata));//x64系统的hfind用longlong，即intptr_t存，否则会爆int
		TRACE("服务器发送文件数量：server:count = %d\r\n", count);
		//发送信息到控制端
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
		return 0;
	}

	int RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		std::string strPath = inPacket.strData;
		ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);//执行文件
		lstPacket.push_back(CPacket(3, NULL, 0));
		return 0;
	}

	int DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		std::string strPath = inPacket.strData;
		long long data = 0;
		FILE* pFile = NULL;
		//如果用fopen报错C4996，要么改用fopen_s，要么添加_CRT_SECURE_NO_WARNINGS,要么#pragma warning(disable:4996)
		errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");//文本文件按二进制方式读
		if (err != 0) {
			TRACE("打开文件失败！！\r\n");
			lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
			return -1;
		}
		if (pFile != NULL) {
			fseek(pFile, 0, SEEK_END);//拿一个文件的长度
			data = _ftelli64(pFile);
			lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
			fseek(pFile, 0, SEEK_SET);
			char buffer[1024] = "";
			size_t rlen = 0;
			do {
				rlen = fread(buffer, 1, 1024, pFile);//一次读1字节，读1024次
				lstPacket.push_back(CPacket(4, (BYTE*)buffer, rlen));
			} while (rlen >= 1024);
			fclose(pFile);//别忘了关文件
		}
		else {
			lstPacket.push_back(CPacket(4, NULL, 0));
		}
		return 0;
	}

	int MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		MOUSEEV mouse;
		memcpy(&mouse, inPacket.strData.c_str(), sizeof MOUSEEV);
		DWORD nflags = 0;
		switch (mouse.nButton) {
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
		switch (mouse.nAction) {
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
		TRACE("鼠标操作：%08X x:%d y:%d\r\n", nflags, mouse.ptXY.x, mouse.ptXY.y);
		switch (nflags) {
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
		lstPacket.push_back(CPacket(5, NULL, 0));
		return 0;
	}

	int SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket) {
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
			lstPacket.push_back(CPacket(6, pData, nSize));
			GlobalUnlock(hMem);//解锁内存,表示访问完成

		}
		pStream->Release();//释放流
		screen.ReleaseDC();
		GlobalFree(hMem);//释放内存
		return 0;
	}
	int LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {//如果窗口不存在
			//不用_beginthreadex创建线程，因为指定不了线程ID，无法向线程发送消息
			_beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadId);//创建线程
		}
		lstPacket.push_back(CPacket(7, NULL, 0));
		return 0;
	}

	int UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		//dlg.SendMessage(WM_KEYDOWN, 0x1B, 0x01E0001);//发送esc按键消息, 0x01E0001是键盘的扫描码, 扫描码是指键盘上的每个键对应的一个值
		PostThreadMessage(threadId, WM_KEYDOWN, 0x1B, 0x01E0001);//向线程发送esc按键消息,threadId换成GetCurrentThreadId()也可以
		//消息泵需要>30ms，因此为了线程同步，sleep要尽量大于30ms
		lstPacket.push_back(CPacket(8, NULL, 0));
		return 0;
	}

	int TestConnect(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		lstPacket.push_back(CPacket(1981, NULL, 0));
		return 0;
	}

	int DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		std::string strPath = inPacket.strData;
		TCHAR sPath[MAX_PATH] = _T("");
		//mbstowcs(sPath, strPath.c_str(), strPath.size());//多字节转宽字节字符集,但容易乱码
		MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(),
			sPath, sizeof sPath / sizeof(TCHAR));//多字节转宽字节字符集, sizeof sPath / sizeof(TCHAR)是字符数
		DeleteFileA(strPath.c_str());//删除文件
		lstPacket.push_back(CPacket(9, NULL, 0));
		return 0;
	}
};

