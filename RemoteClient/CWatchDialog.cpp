// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "CWatchDialog.h"
#include "ClientController.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG_WATCH, pParent)
{
	m_nObjWidth = -1;
	m_nObjHeight = -1;
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
	ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDialog::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDialog::OnBnClickedBtnUnlock)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen)//isScreen为是否为屏幕坐标
{//800*450
	CRect clientRect;
	if (!isScreen) {//如果ppint不是全局屏幕坐标而是客户端屏幕坐标
		ClientToScreen(&point);//客户区域坐标到全局坐标,转换为相对屏幕左上角的坐标
	}
	m_picture.ScreenToClient(&point);//将全局屏幕坐标转换为客户区坐标,转成800*450的坐标
	//ScreenToClient(&point);//将全局屏幕坐标转换为客户区坐标,转成800*450的坐标
	//本地坐标转换成远程端的坐标
	m_picture.GetWindowRect(clientRect);//获取控件的矩形区域
	//int width0 = clientRect.Width();
	//int height0 = clientRect.Height();	//获取当前控件的宽高
	int remoteWidth = m_nObjWidth, remoteHeight = m_nObjHeight;	//远程端的屏幕宽高（如1920,1080）
	//int x = point.x * width / width0;//转换成远程端的坐标
	//int y = point.y * height / height0;
	//return CPoint(x ,y)
	return CPoint(point.x * remoteWidth / clientRect.Width(), point.y * remoteHeight / clientRect.Height());
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	m_isFull = false;//初始化缓存没有数据
	SetTimer(0, 45, NULL);//设置定时器，每50ms刷新一次
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0) {
		CClientController* pParent = CClientController::getInstance();
		if (m_isFull) {
			CRect rect;
			m_picture.GetWindowRect(rect);//获取控件的矩形区域
			//将图片显示到控件上，SRCCOPY表示直接拷贝，0， 0表示从左上角显示
			//pParent->GetImage().BitBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, SRCCOPY); 
			m_nObjWidth = m_image.GetWidth();//获取图片的宽度，设置成分辨率宽
			m_nObjHeight = m_image.GetHeight();//获取图片的高度,设置成分辨率高

			m_image.StretchBlt(
				m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);//缩放图片
			m_picture.InvalidateRect(NULL);//刷新控件
			m_image.Destroy();//销毁原来的图片
			m_isFull = false;//缓存清空
		}
	}
	CDialog::OnTimer(nIDEvent);
}


void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {//如果分辨率没有设置，直接返回
		//坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MouseEvent event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 2;//双击
		//发送
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof event);
		
	}
	CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {//如果分辨率没有设置，直接返回
		TRACE("x = %d, y = %d\r\n", point.x, point.y);
		//坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		TRACE("x = %d, y = %d\r\n", point.x, point.y);
		//封装
		MouseEvent event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 2;//按下
		//发送
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof event);
	}
	CDialog::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {//如果分辨率没有设置，直接返回
		//坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MouseEvent event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 3;//鼠标释放
		//发送
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof event);
	}
	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {//如果分辨率没有设置，直接返回
		//坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MouseEvent event;
		event.ptXY = remote;
		event.nButton = 1;//右键
		event.nAction = 1;//双击
		//发送
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof event);
	}
	CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {//如果分辨率没有设置，直接返回
		//坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MouseEvent event;
		event.ptXY = remote;
		event.nButton = 1;//右键
		event.nAction = 2;//按下，TODO:服务端也要做对应修改
		//发送
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof event);
	}
	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {//如果分辨率没有设置，直接返回
		//坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MouseEvent event;
		event.ptXY = remote;
		event.nButton = 1;//右键
		event.nAction = 3;//释放
		//发送
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof event);
	}
	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {//如果分辨率没有设置，直接返回
		//坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MouseEvent event;
		event.ptXY = remote;
		event.nButton = 8;//没有按键
		event.nAction = 0;//移动
		//发送
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof event);
	}
	CDialog::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch()
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {//如果分辨率没有设置，直接返回
		CPoint pt;
		GetCursorPos(&pt);//获取鼠标位置（GetCursorPos获取的是屏幕坐标，而Mousemove拿到的是客户端坐标，不一样，要特判） 
		//坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(pt, true);//这里将屏幕坐标设为true
		//封装
		MouseEvent event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 0;//单击
		//发送
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof event);
	}
}


void CWatchDialog::OnOK()
{

	//CDialog::OnOK();//让回车键失效
}


void CWatchDialog::OnBnClickedBtnLock()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 7);

}


void CWatchDialog::OnBnClickedBtnUnlock()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 8);
}
