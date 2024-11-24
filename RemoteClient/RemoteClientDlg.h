
// RemoteClientDlg.h: 头文件
//

#pragma once
#include "ClientSocket.h"
#include "StatusDlg.h"

#define	WM_SEND_PACKET (WM_USER + 1) //发送数据包的消息，①

// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
// 构造
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

public:
	bool isFull() const {//const不允许修改成员变量
		return m_isFull;
	}
	CImage& GetImage() {
		return m_image;
	}
	void SetImageStatus(bool isFull = false) {
		m_isFull = isFull;
	}
private:
	CImage m_image;//图片缓存
	bool m_isFull;//缓存是否有数据，true,有数据，false,无数据
	bool m_isClosed;//监视是否关闭

	static void threadEntryForWatchData(void* arg);//监控数据线程入口函数，静态函数不能使用this指针
	void threadWatchData();//成员函数可以使用this指针，会非常方便
	static void threadEntryForDownFile(void* arg);
	void threadDownFile();
	void LoadFileCurrent();//加载当前目录下的文件
	void LoadFileInfo();
	CString GetPath(HTREEITEM hTree);//获取路径,让这个函数可以使用m_Tree
	void DeleteTreeChildrenItem(HTREEITEM hTree);//删除树的子节点
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
	int SendCommandPacket(int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0);
	
	//实现

// 实现
protected:
	HICON m_hIcon;
	CStatusDlg m_dlgStatus;
	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnTest();
	DWORD m_server_address;
	CString m_nPort;
	afx_msg void OnBnClickedBtnFileinfo();
	CTreeCtrl m_Tree;
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	// 显示文件
	CListCtrl m_LIst;
	afx_msg void OnNMRClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDownloadFile();
	afx_msg void OnDeleteFile();
	afx_msg void OnRunFile();
	afx_msg LRESULT OnSendPacket(WPARAM wParam, LPARAM lParam);//定义自定义消息响应函数
	afx_msg void OnBnClickedBtnStartWatch();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
