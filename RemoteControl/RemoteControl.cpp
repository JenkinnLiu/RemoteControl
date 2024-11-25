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
			CCommand cmd;
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
                        ret = cmd.ExcuteCommand(ret);//执行命令
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
