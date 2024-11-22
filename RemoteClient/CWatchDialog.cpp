// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG_WATCH, pParent)
{

}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point)
{//800*450
	CRect clientRect;
	ScreenToClient(&point);//将全局屏幕坐标转换为客户区坐标,转成800*450的坐标
	//本地坐标转换成远程端的坐标
	m_picture.GetWindowRect(clientRect);//获取控件的矩形区域
	//int width0 = clientRect.Width();
	//int height0 = clientRect.Height();	//获取当前控件的宽高
	int remoteWidth = 1920, remoteHeight = 1080;	//远程端的屏幕宽高
	//int x = point.x * width / width0;//转换成远程端的坐标
	//int y = point.y * height / height0;
	//return CPoint(x ,y)
	return CPoint(point.x * remoteWidth / clientRect.Width(), point.y * remoteHeight / clientRect.Height());
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetTimer(0, 45, NULL);//设置定时器，每50ms刷新一次
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0) {
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		if (pParent->isFull()) {
			CRect rect;
			m_picture.GetWindowRect(rect);//获取控件的矩形区域
			//将图片显示到控件上，SRCCOPY表示直接拷贝，0， 0表示从左上角显示
			//pParent->GetImage().BitBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, SRCCOPY); 
			pParent->GetImage().StretchBlt(
				m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);//缩放图片
			m_picture.InvalidateRect(NULL);//刷新控件
			pParent->GetImage().Destroy();//销毁原来的图片
			pParent->SetImageStatus();//默认为false， 清空缓存
		}
	}
	CDialog::OnTimer(nIDEvent);
}


void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	//坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MouseEvent event;
	event.ptXY = remote;
	event.nButton = 0;//左键
	event.nAction = 2;//双击
	//发送
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));//封装鼠标操作
	pClient->Send(pack);
	CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	
	CDialog::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	////坐标转换
	//CPoint remote = UserPoint2RemoteScreenPoint(point);
	////封装
	//MouseEvent event;
	//event.ptXY = remote;
	//event.nButton = 0;//左键
	//event.nAction = 4;//鼠标释放
	////发送
	//CClientSocket* pClient = CClientSocket::getInstance();
	//CPacket pack(5, (BYTE*)&event, sizeof(event));//封装鼠标操作
	//pClient->Send(pack);
	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	//坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MouseEvent event;
	event.ptXY = remote;
	event.nButton = 2;//右键
	event.nAction = 2;//双击
	//发送
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));//封装鼠标操作
	pClient->Send(pack);
	CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	//坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MouseEvent event;
	event.ptXY = remote;
	event.nButton = 2;//右键
	event.nAction = 3;//按下，TODO:服务端也要做对应修改
	//发送
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));//封装鼠标操作
	pClient->Send(pack);
	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	//坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MouseEvent event;
	event.ptXY = remote;
	event.nButton = 2;//右键
	event.nAction = 4;//释放
	//发送
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));//封装鼠标操作
	pClient->Send(pack);
	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	//坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MouseEvent event;
	event.ptXY = remote;
	event.nButton = 0;//左键
	event.nAction = 1;//移动
	//发送
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));//封装鼠标操作
	pClient->Send(pack);
	CDialog::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch()
{
	CPoint pt;
	GetCursorPos(&pt);//获取鼠标位置
	//坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(pt);
	//封装
	MouseEvent event;
	event.ptXY = remote;
	event.nButton = 0;//左键
	event.nAction = 3;//按下
	//发送
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));//封装鼠标操作
	pClient->Send(pack);
}
