#pragma once
template <typename T>
class CQueue
{//线程安全的队列（利用IOCP实现）
public:
	CQueue() ;
	~CQueue();
	bool PushBack(const T& data);
	bool PopFront(T& data);
	size_t Size();
	void Clear();
	typedef struct IocpParam {
		int nOperator;//操作
		T strData;//数据
		_beginthread_proc_type cbFunc;//回调函数
		HANDLE hEvent;//pop操作需要的事件
		IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL) {
			nOperator = op;
			strData = sData;
		}
		IocpParam() {
			nOperator = -1;
		}
	}PPARAM;//POST Parameter, IOCP中用于传递参数的结构体
	enum {
		EQPush,
		EQPop,
		EQSize,
		EQEmpty,
	};
private:
	static void treadEntry(void* arg);
	void threadMain();

	std::list<T> m_lstData;//数据队列
	HANDLE m_hCompletionPort;//IOCP句柄
	HANDLE m_hThread;//线程句柄
};

