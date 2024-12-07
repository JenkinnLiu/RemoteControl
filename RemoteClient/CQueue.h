#pragma once
template <typename T>
class CQueue
{//�̰߳�ȫ�Ķ��У�����IOCPʵ�֣�
public:
	CQueue() ;
	~CQueue();
	bool PushBack(const T& data);
	bool PopFront(T& data);
	size_t Size();
	void Clear();
	typedef struct IocpParam {
		int nOperator;//����
		T strData;//����
		_beginthread_proc_type cbFunc;//�ص�����
		HANDLE hEvent;//pop������Ҫ���¼�
		IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL) {
			nOperator = op;
			strData = sData;
		}
		IocpParam() {
			nOperator = -1;
		}
	}PPARAM;//POST Parameter, IOCP�����ڴ��ݲ����Ľṹ��
	enum {
		EQPush,
		EQPop,
		EQSize,
		EQEmpty,
	};
private:
	static void treadEntry(void* arg);
	void threadMain();

	std::list<T> m_lstData;//���ݶ���
	HANDLE m_hCompletionPort;//IOCP���
	HANDLE m_hThread;//�߳̾��
};

