#pragma once
#include "framework.h"
#include "pch.h"
#pragma pack(push)
#pragma pack(1)

class CPacket
{
public:
    CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
    CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {//打包构造函数
        sHead = 0xFEFF;
        nLength = nSize + 4;
        sCmd = nCmd;
        if (nSize > 0) {
            strData.resize(nSize);
            memcpy((void*)strData.c_str(), pData, nSize);
        }
        else {
            strData.clear();
        }
        sSum = 0;
        for (size_t j = 0; j < strData.size(); j++) {
            sSum += BYTE(strData[j]) & 0xFF;
        }
    }
    CPacket(const CPacket& pack) {//拷贝构造函数
        sHead = pack.sHead;
        nLength = pack.nLength;
        strData = pack.strData;
        sCmd = pack.sCmd;
        sSum = pack.sSum;
    }
    CPacket& operator=(const CPacket& pack) {//赋值构造函数
        if (this == &pack) return *this;//防止自赋值
        sHead = pack.sHead;
        nLength = pack.nLength;
        strData = pack.strData;
        sCmd = pack.sCmd;
        sSum = pack.sSum;
        return *this;
    }
    CPacket(const BYTE* pData, size_t& nSize) :sHead(0), nLength(0), sCmd(0), sSum(0)
    {//这个构造函数用于解析数据包，解包构造函数
        size_t i = 0;
        for (i; i < nSize; i++) {
            if (*(WORD*)(pData + i) == 0xFEFF) {
                sHead = *(WORD*)(pData + i);//找到包头
                i += 2;
                break;
            }
        }
        if (i + 4 + 2 + 2 > nSize) {//包头+包长+命令+校验和，包数据不全或包头未找到
            nSize = 0;
            TRACE("没有找到包头，解析失败\r\n");
            return;//没有找到包头,解析失败
        }
        nLength = *(DWORD*)(pData + i);//包长
        i += 4;
        if (nLength + i > nSize) {//包长不够,包未接收完整，解析失败
            nSize = 0;
            TRACE("包长不够,包未接收完整，解析失败\r\n");
            return;
        }
        sCmd = *(WORD*)(pData + i);//命令
        i += 2;
        if (nLength > 4) {
            //nLength从sCmd开始到校验和结束,所以减去4
            strData.resize(nLength - 2 - 2);//包长-命令-校验和
            memcpy((void*)strData.c_str(), pData + i, nLength - 4);
            i += nLength - 4;
        }
        sSum = *(WORD*)(pData + i);//校验和
        i += 2;
        WORD sum = 0;
        for (size_t j = 0; j < strData.size(); j++) {
            sum += BYTE(strData[j]) & 0xFF;
        }
        if (sum == sSum) {
            nSize = i;
            TRACE("校验和通过，包解析成功\r\n");
            return;
        }
        nSize = 0;
    };
    ~CPacket()
    {

    };
    int Size() {
        return nLength + 6;//包头+包长+包数据，包数据的大小
    }
    const char* Data() {//返回整个包的数据,复制到缓冲区strOut中
        strOut.resize(nLength + 6);
        BYTE* pData = (BYTE*)strOut.c_str();
        *(WORD*)pData = sHead;//包头
        pData += 2;
        *(DWORD*)(pData) = nLength;//包长
        pData += 4;
        *(WORD*)pData = sCmd;//命令
        pData += 2;
        memcpy(pData, strData.c_str(), strData.size());//包数据
        pData += strData.size();
        *(WORD*)pData = sSum;//校验和
        return strOut.c_str();
    }
public:
    WORD sHead;//包头，固定为FE FF
    DWORD nLength;//包长（从控制命令开始到校验和结束）
    std::string strData;//包数据
    WORD sCmd;//控制命令
    WORD sSum;//校验和
    std::string strOut;//整个包的数据
};
#pragma pack(pop)
typedef struct MouseEvent {
    MouseEvent() {
        nAction = 0;
        nButton = -1;
        ptXY.x = 0, ptXY.y = 0;
    }
    WORD nAction;//点击， 移动， 双击
    WORD nButton;//左键， 右键， 中键
    POINT ptXY;//坐标

}MOUSEEV, * PMOUSEEV;

typedef struct file_info {
    file_info() {//结构体构造函数
        IsInvalid = 0;
        IsDirectory = -1;
        HasNext = 1;
        memset(szFileName, 0, sizeof szFileName);
    }
    BOOL IsInvalid;//是否为无效目录
    BOOL IsDirectory;//是否为目录： 0 否， 1 是
    BOOL HasNext;//是否还有后续： 0 没有， 1 有
    char szFileName[256];//文件名
}FILEINFO, * PFILEINFO;