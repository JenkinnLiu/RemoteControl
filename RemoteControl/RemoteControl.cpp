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
    //CServerSocket::getInstance()->Send(pack);
    //CServerSocket::getInstance()->Send(CPacket(1, (BYTE*)result.c_str(), result.size()));
    return 0;
}
#include<stdio.h>
#include<io.h>
#include<list>
typedef struct file_info{
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
}FILEINFO, *PFILEINFO;

int MakeDirectoryInfo() {
    std::string strPath;
    //std::list<FILEINFO> lstFileInfos;
    if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
        OutputDebugString(_T("当前的命令不是获取文件列表，命令解析错误！！"));
        return -1;
    }
    if (_chdir(strPath.c_str()) != 0) {//切换目录失败
        FILEINFO finfo;
        finfo.IsInvalid = TRUE;//标记为无效
        finfo.IsDirectory = TRUE;
        finfo.HasNext = FALSE;
        memcpy(finfo.szFileName, strPath.c_str(), strPath.size());
        //lstFileInfos.push_back(finfo);
        CPacket pack(2, (BYTE*)& finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        OutputDebugString(_T("没有权限，访问目录！！"));
        return -2;
    }
    _finddata_t fdata;
    int hfind = 0;
    if (_findfirst("*", &fdata) == -1) {//查找*文件失败，没有文件
        OutputDebugString(_T("没有找到任何文件！！"));
        return -3;
    }
    do {
        FILEINFO finfo;
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;//判断是否为文件夹
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
        //lstFileInfos.push_back(finfo);//放进列表内
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
    } while (!_findnext(hfind, &fdata));
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
        CServerSocket::getInstance()->Send(pack);
        return -1;
    }
    if (pFile != NULL) {
        fseek(pFile, 0, SEEK_END);//拿一个文件的长度
        data = _ftelli64(pFile);
        CPacket head(4, (BYTE*)&data, 8);//发送文件长度
        fseek(pFile, 0, SEEK_SET);
        char buffer[1024] = "";
        size_t rlen = 0;
        do {
            rlen = fread(buffer, 1, 1024, pFile);//一次读1字节，读1024次
            CPacket pack(4, (BYTE*)buffer, rlen);
        } while (rlen >= 1024);
        fclose(pFile);//别忘了关文件
    }
    CPacket pack(4, NULL, 0);//收到NULL代表结尾
    CServerSocket::getInstance()->Send(pack);
    return 0;
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
  //          CServerSocket* pserver = CServerSocket::getInstance();
  //          int count = 0;
  //          if (pserver->InitSocket() == false) {
  //              MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态！"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
  //              exit(0);
		//	}//初始化只需调用一次即可，accept需调用多次
		//	while (CServerSocket::getInstance() != NULL) {
		//		if (pserver->AcceptClient()) {
		//			int ret = pserver->DealCommand();
		//			//TODO:处理命令
  //              }
  //              else {
  //                  if (count >= 3) {
  //                      MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("客户端连接失败！"), MB_OK | MB_ICONERROR);
		//			    exit(0);
  //                  }
		//				MessageBox(NULL, _T("无法正常接入用户，自动重试！"), _T("客户端连接失败！"), MB_OK | MB_ICONERROR);
  //                  count++;
  //              }
  //          }
		//		
        // 
            int nCmd = 1;
            switch (nCmd) {
			case 1://查看磁盘分区信息
				MakeDriverInfo();
				break;
            case 2://查看指定目录下的文件
                MakeDirectoryInfo();
            case 3://打开文件
                RunFile();
                break;
            case 4://下载文件
                DownloadFile();
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
