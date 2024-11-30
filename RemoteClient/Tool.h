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
		BYTE* pData = (BYTE*)strBuffer.c_str();//�õ�����������Ļ����
		//TODO:����CImage
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);//�����ڴ�
		if (hMem == NULL) {
			TRACE("�ڴ治���ˣ�");
			Sleep(1);
			return -1;
		}
		IStream* pStream = NULL;//����������
		HRESULT hRet = CreateStreamOnHGlobal(NULL, TRUE, &pStream);//����������
		if (hRet == S_OK) {
			ULONG length = 0;
			LARGE_INTEGER bg = { 0 };
			pStream->Write(pData, strBuffer.size(), &length);//��pDataд��������
			if ((HBITMAP)image != NULL) image.Destroy();//����ԭ����ͼƬ,HBITMAP��λͼ���
			image.Load(pStream);//����ͼ
		}
		return hRet;
    }
};

