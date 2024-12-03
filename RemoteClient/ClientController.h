#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include <unordered_map>
#include "resource.h"
#include "Tool.h"

//#define WM_SEND_PACK (WM_USER + 1) //���Ͱ�����
//#define WM_SEND_DATA (WM_USER + 2) //��������
#define WM_SHOW_STATUS (WM_USER + 3) //��ʾ״̬
#define WM_SHOW_WATCH (WM_USER + 4) //Զ�̼��
#define WM_SEND_MESSAGE (WM_USER + 0x1000) //�Զ�����Ϣ����

class CClientController
{
public:
	//��ȡȫ��Ψһ����
	static CClientController* getInstance();
	//��ʼ������
	int InitController();//ָ�����õ�������Ϊ���޸�ָ���ֵ
	//����
	int Invoke(CWnd*& pMainWnd);

	//������Ϣ
	LRESULT SendMessage(MSG msg);
	//��������������ĵ�ַ
	void UpdateAddress(int nIP, int nPort) {
		CClientSocket::getInstance()->UpdateAddress(nIP, nPort);	
	}
	int DealCommand() {
		return CClientSocket::getInstance()->DealCommand();
	}
	void CloseSocket() {
		CClientSocket::getInstance()->CloseSocket();
	}

	//1. �鿴���̷���
	//2. �鿴ָ��Ŀ¼�µ��ļ�
	//3. ���ļ�
	//4. �����ļ�
	//5. ������
	// 6. ������Ļ����
	// 7. ����
	// 8. ����
	// 9. ɾ���ļ�
	// 1981. ��������
	//����ֵ�������cmd, ���С��0�Ǵ���
	int SendCommandPacket(int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0, std::list<CPacket>* plstPacks = nullptr) {
		CClientSocket* pClient = CClientSocket::getInstance();
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);//����һ���¼�
		//TODO: ��Ӧ��ֱ�ӷ��ͣ�����Ͷ�뷢�Ͷ���
		std::list<CPacket> lstPacks;
		if (plstPacks == NULL) plstPacks = &lstPacks;
		pClient->SendPacket(CPacket(nCmd, pData, nLength, hEvent), *plstPacks, bAutoClose);
		CloseHandle(hEvent);//�����¼��������ֹ��Դ�ľ�
		if (plstPacks->size() > 0) {
			return plstPacks->front().sCmd;//���������
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
		WaitForSingleObject(m_hThread, 100);//��������Ĺ����ǵȴ�һ���̵߳Ľ�����ֱ���߳̽�����ŷ���
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
		MsgInfo(const MsgInfo& m) {//���ƹ��캯��
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
	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);//��Ϣ������ָ��
	static std::unordered_map<UINT, MSGFUNC> m_mapFunc;//��Ϣ�����map��������Ϣ�ַ�

	CWatchDialog m_watchDlg;
	CRemoteClientDlg m_remoteDlg;
	CStatusDlg m_statusDlg;
	HANDLE m_hThread;
	HANDLE m_hThreadDownload;
	HANDLE m_hThreadWatch;

	bool m_isClosed;//�����Ƿ�ر�

	CString m_strRemote;//�����ļ���Զ��·��
	CString m_strLocal;//�����ļ��ı��ر���·��

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

