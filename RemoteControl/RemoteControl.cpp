// RemoteControl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteControl.h"
#include "ServerSocket.h"

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
            CServerSocket::getInstance();
            // TODO: 在此处为应用程序的行为编写代码。
            //WSADATA data;
            //WSAStartup(MAKEWORD(1, 1), &data);//TODO: 返回值处理
            SOCKET serv_sock = socket(PF_INET, SOCK_STREAM, 0);//IPV4, TCP
            //TODO:校验
            sockaddr_in serv_adr;
            memset(&serv_adr, 0, sizeof serv_adr);
            serv_adr.sin_family = AF_INET;//IPV4地址族
            serv_adr.sin_addr.s_addr = INADDR_ANY;//在所有IP上监听
            serv_adr.sin_port = htons(9527);
			bind(serv_sock, (sockaddr*)&serv_adr, sizeof serv_adr);//TODO: 校验
            listen(serv_sock, 1);
			char buffer[1024];
			int cli_sz = sizeof(sockaddr_in);
			//accept(serv_sock, (sockaddr*)&client_adr, &cli_sz);
			/*recv(serv_sock, buffer, sizeof buffer, 0);
			send(serv_sock, buffer, sizeof buffer, 0);*/
			closesocket(serv_sock);
			//WSACleanup();//别忘了清理
            //全局的静态变量
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
