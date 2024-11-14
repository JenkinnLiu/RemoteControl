# RemoteControl

# 开发中遇到的问题

1. 在开发锁机线程时线程外发送的解锁的按键消息总是影响不到锁机线程，导致解锁失效

解决方法：用_beginthreadex(NULL, 0, threadLockDlg, NULL, 0 &threadId);创建线程而不是_beginthread()
并且用PostThreadMessage(GetCurrentThreadId(), WM_KEYDOWN, 0x1B, 0x01E0001);发送esc按键消息
