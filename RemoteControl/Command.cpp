#include "pch.h"
#include "Command.h"

CCommand::CCommand() : threadId(0)
{
	struct {
		int nCmd;
		CMDFUNC func;
	}data[] = {
		{1, &CCommand::MakeDriverInfo},
		{2, &CCommand::MakeDirectoryInfo},
		{3, &CCommand::RunFile},
		{4, &CCommand::DownloadFile},
		{5, &CCommand::MouseEvent},
		{6, &CCommand::SendScreen},
		{7, &CCommand::LockMachine},
		{8, &CCommand::UnlockMachine},
		{9, &CCommand::DeleteLocalFile},
		{1981, &CCommand::TestConnect},
		{-1, NULL},
	};//初始化数据,用map的好处是方便插入与修改
	for (int i = 0; data[i].nCmd != -1; i++)
	{
		m_mapFunction[data[i].nCmd] = data[i].func;//初始化map
	}
}

int CCommand::ExcuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	std::unordered_map<int, CMDFUNC>::iterator it = m_mapFunction.find(nCmd);//查找命令号
	if (it == m_mapFunction.end()) {
		return -1;//没找到
	}
	return (this->*it->second)(lstPacket, inPacket);
}
