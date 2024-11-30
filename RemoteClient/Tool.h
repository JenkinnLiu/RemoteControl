#pragma once
#include <Windows.h>
#include <string>
#include <atlimage.h>
class CTool
{
public:
    static void Dump(BYTE* pData, size_t nSize) {
        std::string strOut;
        for (size_t i = 0; i < nSize; i++) {
            char buf[8] = "";
            if (i > 0 && (i % 16 == 0)) strOut += "\n";
            snprintf(buf, sizeof buf, "%02X ", pData[i] & 0xFF);
            strOut += buf;
        }
        strOut += "\n";
        OutputDebugStringA(strOut.c_str());
    }
    static int Byte2Image(CImage& image, const std::string& strBuffer) {
		BYTE* pData = (BYTE*)strBuffer.c_str();//得到服务器的屏幕内容
		//TODO:存入CImage
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);//分配内存
		if (hMem == NULL) {
			TRACE("内存不足了！");
			Sleep(1);
			return -1;
		}
		IStream* pStream = NULL;//创建流对象
		HRESULT hRet = CreateStreamOnHGlobal(NULL, TRUE, &pStream);//创建流对象
		if (hRet == S_OK) {
			ULONG length = 0;
			LARGE_INTEGER bg = { 0 };
			pStream->Write(pData, strBuffer.size(), &length);//将pData写入流对象
			if ((HBITMAP)image != NULL) image.Destroy();//销毁原来的图片,HBITMAP是位图句柄
			image.Load(pStream);//加载图
		}
		return hRet;
    }
};

