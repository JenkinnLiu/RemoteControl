#pragma once
#include "framework.h"
#include "pch.h"
#pragma pack(push)
#pragma pack(1)

class CPacket
{
public:
    CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
    CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {//������캯��
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
    CPacket(const CPacket& pack) {//�������캯��
        sHead = pack.sHead;
        nLength = pack.nLength;
        strData = pack.strData;
        sCmd = pack.sCmd;
        sSum = pack.sSum;
    }
    CPacket& operator=(const CPacket& pack) {//��ֵ���캯��
        if (this == &pack) return *this;//��ֹ�Ը�ֵ
        sHead = pack.sHead;
        nLength = pack.nLength;
        strData = pack.strData;
        sCmd = pack.sCmd;
        sSum = pack.sSum;
        return *this;
    }
    CPacket(const BYTE* pData, size_t& nSize) :sHead(0), nLength(0), sCmd(0), sSum(0)
    {//������캯�����ڽ������ݰ���������캯��
        size_t i = 0;
        for (i; i < nSize; i++) {
            if (*(WORD*)(pData + i) == 0xFEFF) {
                sHead = *(WORD*)(pData + i);//�ҵ���ͷ
                i += 2;
                break;
            }
        }
        if (i + 4 + 2 + 2 > nSize) {//��ͷ+����+����+У��ͣ������ݲ�ȫ���ͷδ�ҵ�
            nSize = 0;
            TRACE("û���ҵ���ͷ������ʧ��\r\n");
            return;//û���ҵ���ͷ,����ʧ��
        }
        nLength = *(DWORD*)(pData + i);//����
        i += 4;
        if (nLength + i > nSize) {//��������,��δ��������������ʧ��
            nSize = 0;
            TRACE("��������,��δ��������������ʧ��\r\n");
            return;
        }
        sCmd = *(WORD*)(pData + i);//����
        i += 2;
        if (nLength > 4) {
            //nLength��sCmd��ʼ��У��ͽ���,���Լ�ȥ4
            strData.resize(nLength - 2 - 2);//����-����-У���
            memcpy((void*)strData.c_str(), pData + i, nLength - 4);
            i += nLength - 4;
        }
        sSum = *(WORD*)(pData + i);//У���
        i += 2;
        WORD sum = 0;
        for (size_t j = 0; j < strData.size(); j++) {
            sum += BYTE(strData[j]) & 0xFF;
        }
        if (sum == sSum) {
            nSize = i;
            TRACE("У���ͨ�����������ɹ�\r\n");
            return;
        }
        nSize = 0;
    };
    ~CPacket()
    {

    };
    int Size() {
        return nLength + 6;//��ͷ+����+�����ݣ������ݵĴ�С
    }
    const char* Data() {//����������������,���Ƶ�������strOut��
        strOut.resize(nLength + 6);
        BYTE* pData = (BYTE*)strOut.c_str();
        *(WORD*)pData = sHead;//��ͷ
        pData += 2;
        *(DWORD*)(pData) = nLength;//����
        pData += 4;
        *(WORD*)pData = sCmd;//����
        pData += 2;
        memcpy(pData, strData.c_str(), strData.size());//������
        pData += strData.size();
        *(WORD*)pData = sSum;//У���
        return strOut.c_str();
    }
public:
    WORD sHead;//��ͷ���̶�ΪFE FF
    DWORD nLength;//�������ӿ������ʼ��У��ͽ�����
    std::string strData;//������
    WORD sCmd;//��������
    WORD sSum;//У���
    std::string strOut;//������������
};
#pragma pack(pop)
typedef struct MouseEvent {
    MouseEvent() {
        nAction = 0;
        nButton = -1;
        ptXY.x = 0, ptXY.y = 0;
    }
    WORD nAction;//����� �ƶ��� ˫��
    WORD nButton;//����� �Ҽ��� �м�
    POINT ptXY;//����

}MOUSEEV, * PMOUSEEV;

typedef struct file_info {
    file_info() {//�ṹ�幹�캯��
        IsInvalid = 0;
        IsDirectory = -1;
        HasNext = 1;
        memset(szFileName, 0, sizeof szFileName);
    }
    BOOL IsInvalid;//�Ƿ�Ϊ��ЧĿ¼
    BOOL IsDirectory;//�Ƿ�ΪĿ¼�� 0 �� 1 ��
    BOOL HasNext;//�Ƿ��к����� 0 û�У� 1 ��
    char szFileName[256];//�ļ���
}FILEINFO, * PFILEINFO;