#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include <unordered_map>
#include "resource.h"
#include "Tool.h"

//#define WM_SEND_PACK (WM_USER + 1) //发送包数据
//#define WM_SEND_DATA (WM_USER + 2) //发送数据
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
	//更新网络服务器的地址
	void UpdateAddress(int nIP, int nPort) {
		CClientSocket::getInstance()->UpdateAddress(nIP, nPort);	
	}
	int DealCommand() {
		return CClientSocket::getInstance()->DealCommand();
	}
	void CloseSocket() {
		CClientSocket::getInstance()->CloseSocket();
	}

	//1. 查看磁盘分区
	//2. 查看指定目录下的文件
	//3. 打开文件
	//4. 下载文件
	//5. 鼠标操作
	// 6. 发送屏幕内容
	// 7. 锁机
	// 8. 解锁
	// 9. 删除文件
	// 1981. 测试连接
	//返回值是命令号cmd, 如果小于0是错误
	int SendCommandPacket(int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0, std::list<CPacket>* plstPacks = nullptr) {
		CClientSocket* pClient = CClientSocket::getInstance();
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);//创建一个事件
		//TODO: 不应该直接发送，而是投入发送队列
		std::list<CPacket> lstPacks;
		if (plstPacks == NULL) plstPacks = &lstPacks;
		pClient->SendPacket(CPacket(nCmd, pData, nLength, hEvent), *plstPacks, bAutoClose);
		CloseHandle(hEvent);//回收事件句柄，防止资源耗尽
		if (plstPacks->size() > 0) {
			return plstPacks->front().sCmd;//返回命令号
		}
		return -1;
	}

	int GetImage(CImage& image) {
		CClientSocket* pClient = CClientSocket::getInstance();
		return CTool::Byte2Image(image, pClient->GetPacket().strData);
		
	}
	int DownFile(CString strPath);
	void StartWatchScreen();

protected:
	void threadWatchScreen();
	static void threadWatchScreen(void* arg);
	void threadDownloadFile();
	static void threadDownloadEntry(void* arg);
	CClientController() :m_statusDlg(&m_remoteDlg), m_watchDlg(&m_remoteDlg) {
		m_hThreadWatch = INVALID_HANDLE_VALUE;
		m_hThreadDownload = INVALID_HANDLE_VALUE;
		m_hThread = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;
		m_isClosed = true;

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
	//LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	//LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
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
	HANDLE m_hThreadDownload;
	HANDLE m_hThreadWatch;

	bool m_isClosed;//监视是否关闭

	CString m_strRemote;//下载文件的远程路径
	CString m_strLocal;//下载文件的本地保存路径

	unsigned m_nThreadID;
	static CClientController* m_instance;
	class CHelper {
	public:
		CHelper(){
			
		}
		~CHelper() {
			CClientController::releaseInstance;
		}
	};
	static CHelper m_helper;
};

