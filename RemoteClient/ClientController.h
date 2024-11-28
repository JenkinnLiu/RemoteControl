#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include <unordered_map>
#include "resource.h"

#define WM_SEND_PACK (WM_USER + 1) //发送包数据
#define WM_SEND_DATA (WM_USER + 2) //发送数据
#define WM_SHOW_STATUS (WM_USER + 3) //显示状态
#define WM_SHOW_WATCH (WM_USER + 4) //远程监控
#define WM_SEND_MESSAGE (WM_USER + 0x1000) //自定义消息处理

class CClientController
{
public:
	//获取全局唯一对象
	static CClientController* getInstance();
	//初始化操作
	int InitController();//指针引用的作用是为了修改指针的值
	//启动
	int Invoke(CWnd*& pMainWnd);

	//发送消息
	LRESULT SendMessage(MSG msg);
protected:
	CClientController() :m_statusDlg(&m_remoteDlg), m_watchDlg(&m_remoteDlg) {
		m_hThread = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;

	}
	~CClientController() {
		WaitForSingleObject(m_hThread, 100);//这个函数的功能是等待一个线程的结束，直到线程结束后才返回
	}
	void threadFunc();
	static unsigned __stdcall threadEntry(void* arg);
	
	static void releaseInstance() {
		if (m_instance != NULL) {
			delete m_instance;
			m_instance = NULL;
		}
	}
	LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam);
private:
	typedef struct MsgInfo{
		MSG msg;
		LRESULT result;
		MsgInfo(MSG m) {
			result = 0;
			memcpy(&msg, &m, sizeof MSG);
		}
		MsgInfo(const MsgInfo& m) {//复制构造函数
			result = m.result;
			memcpy(&msg, &m.msg, sizeof MSG);
		}
		MsgInfo& operator=(const MsgInfo& m) {
			if (this != &m) {
				result = m.result;
				memcpy(&msg, &m.msg, sizeof MSG);
			}
			return *this;
		}
	}MSGINFO;
	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);//消息处理函数指针
	static std::unordered_map<UINT, MSGFUNC> m_mapFunc;//消息处理的map，用于消息分发

	CWatchDialog m_watchDlg;
	CRemoteClientDlg m_remoteDlg;
	CStatusDlg m_statusDlg;
	HANDLE m_hThread;
	unsigned m_nThreadID;
	static CClientController* m_instance;
	class CHelper {
	public:
		CHelper(){
			CClientController::getInstance();
		}
		~CHelper() {
			CClientController::releaseInstance;
		}
	};
	static CHelper m_helper;
};

