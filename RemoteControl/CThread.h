#pragma once
#include "pch.h"
#include <atomic>
#include <vector>
#include <mutex>
#include <Windows.h>

class ThreadFuncBase {};//空类，用于继承
typedef int (ThreadFuncBase::* FUNCTYPE)();//线程函数指针
class ThreadWorker {//线程工作类
public:
	ThreadWorker() :thiz(NULL), func(NULL) {};//默认构造函数

	ThreadWorker(void* obj, FUNCTYPE f) :thiz((ThreadFuncBase*)obj), func(f) {}//构造函数

	ThreadWorker(const ThreadWorker& worker) {//拷贝构造函数
		thiz = worker.thiz;
		func = worker.func;
	}
	ThreadWorker& operator=(const ThreadWorker& worker) {//赋值运算符重载
		if (this != &worker) {
			thiz = worker.thiz;
			func = worker.func;
		}
		return *this;
	}

	int operator()() {//函数调用运算符重载
		if (IsValid()) {
			return (thiz->*func)();//只有在线程函数指针和线程函数对象都有效的情况下才能调用线程函数
		}
		return -1;
	}
	bool IsValid() const {//判断线程函数指针和线程函数对象是否有效
		return (thiz != NULL) && (func != NULL);
	}

private:
	ThreadFuncBase* thiz;//指向线程函数的指针
	FUNCTYPE func;//线程函数
};


class CThread//线程类
{
public:
	CThread() {
		m_hThread = NULL;
		m_bStatus = false;
	}

	~CThread() {
		Stop();
	}

	//true 表示成功 false表示失败
	bool Start() {//启动线程
		m_bStatus = true;
		m_hThread = (HANDLE)_beginthread(&CThread::ThreadEntry, 0, this);
		if (!IsValid()) {
			m_bStatus = false;
		}
		return m_bStatus;
	}

	bool IsValid() {//返回true表示有效 返回false表示线程异常或者已经终止
		if (m_hThread == NULL || (m_hThread == INVALID_HANDLE_VALUE))return false;
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;//等待线程结束，如果返回值为WAIT_TIMEOUT表示线程还在运行
	}

	bool Stop() {
		if (m_bStatus == false)return true;
		m_bStatus = false;
		bool ret = WaitForSingleObject(m_hThread, INFINITE) == WAIT_OBJECT_0;//等待线程结束
		UpdateWorker();//将线程工作对象置空
		return ret;
	}

	void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker()) {//更新线程工作对象
		if (m_worker.load() != NULL && m_worker.load() != &worker) {//如果线程工作对象不为空，并且不等于新的线程工作对象
			::ThreadWorker* pWorker = m_worker.load();//load表示获取线程工作对象
			m_worker.store(NULL);//store表示设置线程工作对象为空
			delete pWorker;//检查并删除旧的线程工作对象
		}
		if (!worker.IsValid()) {//检查新的线程工作对象是否有效
			m_worker.store(NULL);
			return;
		}
		m_worker.store(new ::ThreadWorker(worker));//存储新的线程工作对象,store是原子操作，保证线程安全
	}

	//true表示空闲 false表示已经分配了工作
	bool IsIdle() {//判断线程是否空闲	
		if (m_worker == NULL)return true;
		return !m_worker.load()->IsValid();
	}
private:
	void ThreadWorker() {//线程工作函数
		while (m_bStatus) {//线程状态为true表示线程正在运行
			if (m_worker == NULL) {//如果线程工作对象为空
				Sleep(1);//休眠1毫秒
				continue;
			}
			::ThreadWorker worker = *m_worker.load();//获取线程工作对象
			if (worker.IsValid()) {//如果线程工作对象有效
				int ret = worker();//调用线程工作对象的线程函数
				if (ret != 0) {//如果返回值不为0，则不正常，生成警告日志
					CString str;
					str.Format(_T("thread found warning code %d\r\n"), ret);
					OutputDebugString(str);
				}
				if (ret < 0) {//如果返回值小于0，表示线程工作对象无效，将线程工作对象置空
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
	bool m_bStatus;//false 表示线程将要关闭  true 表示线程正在运行
	std::atomic<::ThreadWorker*> m_worker;//线程工作对象，atomic是原子操作，保证线程安全
};

class CThreadPool//线程池类
{
public:
	CThreadPool(size_t size) {//构造函数
		m_threads.resize(size);//初始化线程池大小
		for (size_t i = 0; i < size; i++)
			m_threads[i] = new CThread();
	}
	CThreadPool() {}
	~CThreadPool() {//析构函数
		Stop();
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			CThread* pThread = m_threads[i];
			m_threads[i] = NULL;
			delete pThread;
		}

		m_threads.clear();
	}
	bool Invoke() {//启动线程池，返回true表示成功，返回false表示失败
		bool ret = true;
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i]->Start() == false) {//一个一个启动线程
				ret = false;
				break;
			}
		}
		if (ret == false) {
			for (size_t i = 0; i < m_threads.size(); i++) {//如果启动失败，停止所有线程
				m_threads[i]->Stop();
			}
		}
		return ret;
	}
	void Stop() {
		for (size_t i = 0; i < m_threads.size(); i++) {
			m_threads[i]->Stop();//停止所有线程
		}
	}

	//返回-1 表示分配失败，所有线程都在忙 大于等于0，表示第n个线程分配来做这个事情
	int DispatchWorker(const ThreadWorker& worker) {//分配线程工作对象
		int index = -1;
		m_lock.lock();//加锁
		//优化思路：可以将空闲线程放在一个数组中，这样可以减少线程的遍历次数，轮询的方法不太好
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i] != NULL && m_threads[i]->IsIdle()) {//如果线程是空闲的
				m_threads[i]->UpdateWorker(worker);//更新线程工作对象，分配线程
				index = i;
				break;
			}
		}
		m_lock.unlock();
		return index;
	}

	bool CheckThreadValid(size_t index) {//检查线程是否有效
		if (index < m_threads.size()) {
			return m_threads[index]->IsValid();
		}
		return false;
	}
private:
	std::mutex m_lock;
	std::vector<CThread*> m_threads;
};