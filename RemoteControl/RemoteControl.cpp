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
