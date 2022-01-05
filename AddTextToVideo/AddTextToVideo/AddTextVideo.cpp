#include "AddTextVideo.h"
#include <iostream>
#include <QDebug>

// �����С
#define SIZE 16*16

//�����ֽڵĴ�С
#define DOT_SIZE SIZE/8

// λ����
uchar BitMask[8] = { 1,2,4,8,16,32,64,128 };
// �ֽ�����
uchar ByteMask[2] = { 0x00, 0x01 };

AddTextVideo::AddTextVideo(QObject *parent)
	: QObject(parent)
{
	init();
}

AddTextVideo::~AddTextVideo()
{
}

/* ��ʼ��
	��ȡ�����ֿ��ļ�
*/
void AddTextVideo::init()
{
	std::string strPath(_pgmptr);
	int iRIndex = strPath.rfind("\\");
	std::string strFilePath = "";
	// ��ȡ��YUV���ݵı��� Ϊ 4:2:2
	strFilePath = strPath.substr(0, iRIndex) + "\\jtzw_16x16.zk";
	FILE* fp = fopen(strFilePath.c_str(), "rb");
	if (NULL != fp)
	{
		fseek(fp, 0, SEEK_END);
		int64_t iFileSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		m_pTextPointData = new unsigned char[iFileSize];
		qDebug() << "zk iFileSize = " << iFileSize;
		fread(m_pTextPointData, iFileSize, 1, fp);
		fclose(fp);
	}
	else
	{
		qDebug() << u8"zk���ļ�ʧ��";
	}
}

/*
������ַ���������gb2312����ĸ�ʽ
	sAddStr ��ӵ��ַ���
	pYuvData ���yuv����
	iX  ��ӵĺ�����
	iY  ��ӵ�������
	iSize �ֺŴ�С
	����16*16�ĺ������

*/
void AddTextVideo::AddTextVideoFrame(uchar* sAddStr, uchar* pYuvData, const int& iRed, const int& iGreen, const int& iBlue,
	const int& iX, const int& iY, const int& iSize, const int& iVideoWidth, const int& iVideoHeight)
{
	int iIndex = 0;
	uchar* pDotMat = new uchar[SIZE];
	uchar* pLargeDotMat = new uchar[SIZE * iSize * iSize];
	//��ɫת��ʧ��ʹ������ֵ
	m_iy = 0.257*iRed + 0.504*iGreen + 0.098*iBlue + 16;
	m_iu = -0.148*iRed - 0.291*iGreen + 0.439*iBlue + 128;
	m_iv = 0.439*iRed - 0.368*iGreen - 0.071*iBlue + 128;
	int iXOffset = iX;

	while (sAddStr[iIndex] != '\0')
	{
		memset(pDotMat, 0, SIZE);
		memset(pLargeDotMat, 0, SIZE * iSize * iSize);

		// ���ݺ��ֻ����������GB2312����λ�룬������λ�����������ֿ����ʼλ�ã���ȡ��������
		getDotMatrix(sAddStr + iIndex, pDotMat);
		for (int i = 0; i < 16; ++i)
		{
			for (int j = 0; j < 16; ++j)
			{
				std::cout << pDotMat[j + i * 16] << " ";
			}
			std::cout << std::endl;
		}
		
		// ����Ŵ�,��ʵ������Ҫ�Ż���������Ч�ڴ�����루����Ŵ�
		enlargeFont(pDotMat, pLargeDotMat, iSize);
		for (int i = 0; i < 16; ++i)
		{
			for (int j = 0; j < 16; ++j)
			{
				std::cout << pLargeDotMat[j + i * 16] << " ";
			}
			std::cout << std::endl;
		}

		// ���ݷŴ�������ռ�ÿռ��Լ���ʼ���õ�ƫ��������ƫ�Ƶ�ַ���������滻��yuv������
		iXOffset += iIndex * 16 * iSize;
		SuperpositionCharacter(pYuvData, pLargeDotMat, int(iIndex + 1), iXOffset, iY, iSize, iVideoWidth, iVideoHeight);

		// ��������һ���ַ�ռ��2���ֽ�
		iIndex += 2;
	}

	delete[] pDotMat;
	delete[] pLargeDotMat;
}

// ��ȡ�������ݣ�pDotMatrixData�������ݴ洢��ַ��
void AddTextVideo::getDotMatrix(uchar* ch, uchar* pDotMatrixData)
{
	/*
		��λ���������Ĺ�ϵ(H��ʾʮ������)��
			�����char hz[2]����ʾһ������,��ô�ҿ��Լ����������ֵ���λ��Ϊ:
				���� = hz[0] - 128 - 32 = hz[0] - 160
				λ�� = hz[1] - 128 - 32 = hz[1] - 160
		GB2312 ������94*94��С�� һ��Ϊһ������ ��Ϊλ  ������λ���ӦΨһ���ַ�

		������ʼλ�� = ((����- 1)*94 + (λ�� �C 1)) * ���ֵ����ֽ���
	*/
	qDebug() << ch[0] << ", " << ch[1];
	int iOffset = ((ch[0] - 0xA1) * 94 + ch[1] - 0xA1) * DOT_SIZE;
	qDebug() << "iOffset = " << iOffset;
	// �ӵ����ֿ��ж�ȡ��������(�����ֿ����ǰ���λ���б�־λ�洢�ģ���Ҫ��ÿһ��λתΪһ���ֽڴ��������ں����Ĳ���)
	uchar* pTemp = m_pTextPointData + iOffset;
	uchar* pOut = pDotMatrixData;
	// һ��32���ֽڣ�ÿ���ֽ�8��λ
	for (int i = 0; i < DOT_SIZE; ++i)
	{
		/*��λ��ǰ����λ�ں� �����������ó���*/
		for (int iBitIndex = 7; iBitIndex >= 0; --iBitIndex)
		{
			int iByteMaskIndex = (pTemp[i] & BitMask[iBitIndex]) ? 1 : 0;
			// qDebug() << "iByteMaskIndex = " << iByteMaskIndex;
			*pOut = *pOut | ByteMask[iByteMaskIndex];
			std::cout << *pOut << " ";
			pOut++;
		}
		if (i % 2 != 0)
		{
			std::cout << std::endl;
		}
	}
}

// ����Ŵ�
void AddTextVideo::enlargeFont(uchar* pDotMatrixData, uchar* pOut, const int& iSize)
{
	if (iSize < 1)
	{
		qDebug() << u8"���õ������С��Ч: " << iSize;
	}

	for (int i = 0; i < SIZE; ++i)
	{
		for (int j = 0; j < iSize; ++j)
		{
			pOut[i + j] = pDotMatrixData[i];
		}
	}
}

// ���ݷŴ�������ռ�ÿռ��Լ���ʼ���õ�ƫ��������ƫ�Ƶ�ַ���������滻��yuv������
void AddTextVideo::SuperpositionCharacter(uchar* pYuvData, const uchar* pDotMat, const int& iChartIndex, 
	const int& iX, const int& iY, const int& iSize, const int& iVideoWidth, const int& iVideoHeight)
{
	/*
		y�׵�ַ = pYStart + (iY -1) * VideoW + x + (��ǰ��ʾ���ֵ����� - 1) * 16 * �����С
	
	*/
	bool bNew = true;
	for (int j = 0; j < 16 * iSize; ++j)
	{
		for (int i = 0; i < 16 * iSize; ++i)
		{
			if (pDotMat[i * iSize * 16 + j] == 1)
			{
				static uchar *data = NULL;
				static uint  ybase = 0;
				static uint  ubase = 0;
				static uint  vbase = 0;

				if (bNew)
				{
					bNew = false;
					data = pYuvData;
					ybase = iY*iVideoWidth + iX;
					ubase = iVideoWidth * iVideoHeight + iY*iVideoWidth / 4 + iX / 2;
					vbase = iVideoWidth * iVideoHeight * 5 / 4 + iY*iVideoWidth / 4 + iX / 2;
				}

				// uv ����	
				if (j % 2 == 0)
				{
					*(data + ybase + i + j*iVideoWidth) = m_iy;
					*(data + ubase + i / 2 + j*iVideoWidth / 4) = m_iu;
					*(data + vbase + i / 2 + j*iVideoWidth / 4) = m_iv;
				}
				else
				{
					*(data + ybase + i + j*iVideoWidth) = m_iy;
				}
			}
			else
			{
				
			}
		}
	}

}