#pragma once
#include "pch.h"
#include <atomic>
#include <vector>
#include <mutex>
#include <Windows.h>

class ThreadFuncBase {};//���࣬���ڼ̳�
typedef int (ThreadFuncBase::* FUNCTYPE)();//�̺߳���ָ��
class ThreadWorker {//�̹߳�����
public:
	ThreadWorker() :thiz(NULL), func(NULL) {};//Ĭ�Ϲ��캯��

	ThreadWorker(void* obj, FUNCTYPE f) :thiz((ThreadFuncBase*)obj), func(f) {}//���캯��

	ThreadWorker(const ThreadWorker& worker) {//�������캯��
		thiz = worker.thiz;
		func = worker.func;
	}
	ThreadWorker& operator=(const ThreadWorker& worker) {//��ֵ���������
		if (this != &worker) {
			thiz = worker.thiz;
			func = worker.func;
		}
		return *this;
	}

	int operator()() {//�����������������
		if (IsValid()) {
			return (thiz->*func)();//ֻ�����̺߳���ָ����̺߳���������Ч������²��ܵ����̺߳���
		}
		return -1;
	}
	bool IsValid() const {//�ж��̺߳���ָ����̺߳��������Ƿ���Ч
		return (thiz != NULL) && (func != NULL);
	}

private:
	ThreadFuncBase* thiz;//ָ���̺߳�����ָ��
	FUNCTYPE func;//�̺߳���
};


class CThread//�߳���
{
public:
	CThread() {
		m_hThread = NULL;
		m_bStatus = false;
	}

	~CThread() {
		Stop();
	}

	//true ��ʾ�ɹ� false��ʾʧ��
	bool Start() {//�����߳�
		m_bStatus = true;
		m_hThread = (HANDLE)_beginthread(&CThread::ThreadEntry, 0, this);
		if (!IsValid()) {
			m_bStatus = false;
		}
		return m_bStatus;
	}

	bool IsValid() {//����true��ʾ��Ч ����false��ʾ�߳��쳣�����Ѿ���ֹ
		if (m_hThread == NULL || (m_hThread == INVALID_HANDLE_VALUE))return false;
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;//�ȴ��߳̽������������ֵΪWAIT_TIMEOUT��ʾ�̻߳�������
	}

	bool Stop() {
		if (m_bStatus == false)return true;
		m_bStatus = false;
		bool ret = WaitForSingleObject(m_hThread, INFINITE) == WAIT_OBJECT_0;//�ȴ��߳̽���
		UpdateWorker();//���̹߳��������ÿ�
		return ret;
	}

	void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker()) {//�����̹߳�������
		if (m_worker.load() != NULL && m_worker.load() != &worker) {//����̹߳�������Ϊ�գ����Ҳ������µ��̹߳�������
			::ThreadWorker* pWorker = m_worker.load();//load��ʾ��ȡ�̹߳�������
			m_worker.store(NULL);//store��ʾ�����̹߳�������Ϊ��
			delete pWorker;//��鲢ɾ���ɵ��̹߳�������
		}
		if (!worker.IsValid()) {//����µ��̹߳��������Ƿ���Ч
			m_worker.store(NULL);
			return;
		}
		m_worker.store(new ::ThreadWorker(worker));//�洢�µ��̹߳�������,store��ԭ�Ӳ�������֤�̰߳�ȫ
	}

	//true��ʾ���� false��ʾ�Ѿ������˹���
	bool IsIdle() {//�ж��߳��Ƿ����	
		if (m_worker == NULL)return true;
		return !m_worker.load()->IsValid();
	}
private:
	void ThreadWorker() {//�̹߳�������
		while (m_bStatus) {//�߳�״̬Ϊtrue��ʾ�߳���������
			if (m_worker == NULL) {//����̹߳�������Ϊ��
				Sleep(1);//����1����
				continue;
			}
			::ThreadWorker worker = *m_worker.load();//��ȡ�̹߳�������
			if (worker.IsValid()) {//����̹߳���������Ч
				int ret = worker();//�����̹߳���������̺߳���
				if (ret != 0) {//�������ֵ��Ϊ0�������������ɾ�����־
					CString str;
					str.Format(_T("thread found warning code %d\r\n"), ret);
					OutputDebugString(str);
				}
				if (ret < 0) {//�������ֵС��0����ʾ�̹߳���������Ч�����̹߳��������ÿ�
					m_worker.store(NULL);
				}
			}
			else {
				Sleep(1);
			}
		}
	}
	static void ThreadEntry(void* arg) {
		CThread* thiz = (CThread*)arg;
		if (thiz) {
			thiz->ThreadWorker();
		}
		_endthread();
	}
private:
	HANDLE m_hThread;
	bool m_bStatus;//false ��ʾ�߳̽�Ҫ�ر�  true ��ʾ�߳���������
	std::atomic<::ThreadWorker*> m_worker;//�̹߳�������atomic��ԭ�Ӳ�������֤�̰߳�ȫ
};

class CThreadPool//�̳߳���
{
public:
	CThreadPool(size_t size) {//���캯��
		m_threads.resize(size);//��ʼ���̳߳ش�С
		for (size_t i = 0; i < size; i++)
			m_threads[i] = new CThread();
	}
	CThreadPool() {}
	~CThreadPool() {//��������
		Stop();
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			CThread* pThread = m_threads[i];
			m_threads[i] = NULL;
			delete pThread;
		}

		m_threads.clear();
	}
	bool Invoke() {//�����̳߳أ�����true��ʾ�ɹ�������false��ʾʧ��
		bool ret = true;
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i]->Start() == false) {//һ��һ�������߳�
				ret = false;
				break;
			}
		}
		if (ret == false) {
			for (size_t i = 0; i < m_threads.size(); i++) {//�������ʧ�ܣ�ֹͣ�����߳�
				m_threads[i]->Stop();
			}
		}
		return ret;
	}
	void Stop() {
		for (size_t i = 0; i < m_threads.size(); i++) {
			m_threads[i]->Stop();//ֹͣ�����߳�
		}
	}

	//����-1 ��ʾ����ʧ�ܣ������̶߳���æ ���ڵ���0����ʾ��n���̷߳��������������
	int DispatchWorker(const ThreadWorker& worker) {//�����̹߳�������
		int index = -1;
		m_lock.lock();//����
		//�Ż�˼·�����Խ������̷߳���һ�������У��������Լ����̵߳ı�����������ѯ�ķ�����̫��
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i] != NULL && m_threads[i]->IsIdle()) {//����߳��ǿ��е�
				m_threads[i]->UpdateWorker(worker);//�����̹߳������󣬷����߳�
				index = i;
				break;
			}
		}
		m_lock.unlock();
		return index;
	}

	bool CheckThreadValid(size_t index) {//����߳��Ƿ���Ч
		if (index < m_threads.size()) {
			return m_threads[index]->IsValid();
		}
		return false;
	}
private:
	std::mutex m_lock;
	std::vector<CThread*> m_threads;
};