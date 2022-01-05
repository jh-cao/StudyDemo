#include "AddTextVideo.h"
#include <iostream>
#include <QDebug>

// 点阵大小
#define SIZE 16*16

//点阵字节的大小
#define DOT_SIZE SIZE/8

// 位掩码
uchar BitMask[8] = { 1,2,4,8,16,32,64,128 };
// 字节掩码
uchar ByteMask[2] = { 0x00, 0x01 };

AddTextVideo::AddTextVideo(QObject *parent)
	: QObject(parent)
{
	init();
}

AddTextVideo::~AddTextVideo()
{
}

/* 初始化
	读取点阵字库文件
*/
void AddTextVideo::init()
{
	std::string strPath(_pgmptr);
	int iRIndex = strPath.rfind("\\");
	std::string strFilePath = "";
	// 读取的YUV数据的比例 为 4:2:2
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
		qDebug() << u8"zk打开文件失败";
	}
}

/*
传入的字符串必须是gb2312编码的格式
	sAddStr 添加的字符串
	pYuvData 添加yuv数据
	iX  添加的横坐标
	iY  添加的纵坐标
	iSize 字号大小
	点阵16*16的横向矩阵

*/
void AddTextVideo::AddTextVideoFrame(uchar* sAddStr, uchar* pYuvData, const int& iRed, const int& iGreen, const int& iBlue,
	const int& iX, const int& iY, const int& iSize, const int& iVideoWidth, const int& iVideoHeight)
{
	int iIndex = 0;
	uchar* pDotMat = new uchar[SIZE];
	uchar* pLargeDotMat = new uchar[SIZE * iSize * iSize];
	//白色转换失真使用上面值
	m_iy = 0.257*iRed + 0.504*iGreen + 0.098*iBlue + 16;
	m_iu = -0.148*iRed - 0.291*iGreen + 0.439*iBlue + 128;
	m_iv = 0.439*iRed - 0.368*iGreen - 0.071*iBlue + 128;
	int iXOffset = iX;

	while (sAddStr[iIndex] != '\0')
	{
		memset(pDotMat, 0, SIZE);
		memset(pLargeDotMat, 0, SIZE * iSize * iSize);

		// 根据汉字机内码推算出GB2312的区位码，再由区位码计算出点阵字库的起始位置，获取点阵数据
		getDotMatrix(sAddStr + iIndex, pDotMat);
		for (int i = 0; i < 16; ++i)
		{
			for (int j = 0; j < 16; ++j)
			{
				std::cout << pDotMat[j + i * 16] << " ";
			}
			std::cout << std::endl;
		}
		
		// 点阵放大,真实开发需要优化，避免无效内存的申请（字体放大）
		enlargeFont(pDotMat, pLargeDotMat, iSize);
		for (int i = 0; i < 16; ++i)
		{
			for (int j = 0; j < 16; ++j)
			{
				std::cout << pLargeDotMat[j + i * 16] << " ";
			}
			std::cout << std::endl;
		}

		// 根据放大后的字体占用空间以及起始设置的偏移量计算偏移地址并将点阵替换至yuv数据中
		iXOffset += iIndex * 16 * iSize;
		SuperpositionCharacter(pYuvData, pLargeDotMat, int(iIndex + 1), iXOffset, iY, iSize, iVideoWidth, iVideoHeight);

		// 简体中文一个字符占用2个字节
		iIndex += 2;
	}

	delete[] pDotMat;
	delete[] pLargeDotMat;
}

// 获取点阵数据（pDotMatrixData点阵数据存储地址）
void AddTextVideo::getDotMatrix(uchar* ch, uchar* pDotMatrixData)
{
	/*
		区位码与机内码的关系(H表示十六进制)：
			如果用char hz[2]来表示一个汉字,那么我可以计算出这个汉字的区位码为:
				区码 = hz[0] - 128 - 32 = hz[0] - 160
				位码 = hz[1] - 128 - 32 = hz[1] - 160
		GB2312 编码是94*94大小， 一行为一个区， 列为位  所以区位码对应唯一的字符

		点阵起始位置 = ((区码- 1)*94 + (位码 – 1)) * 汉字点阵字节数
	*/
	qDebug() << ch[0] << ", " << ch[1];
	int iOffset = ((ch[0] - 0xA1) * 94 + ch[1] - 0xA1) * DOT_SIZE;
	qDebug() << "iOffset = " << iOffset;
	// 从点阵字库中读取点阵数据(点阵字库中是按照位进行标志位存储的，需要将每一个位转为一个字节传出，便于后续的操作)
	uchar* pTemp = m_pTextPointData + iOffset;
	uchar* pOut = pDotMatrixData;
	// 一共32个字节，每个字节8个位
	for (int i = 0; i < DOT_SIZE; ++i)
	{
		/*高位在前，低位在后 （混淆过倒置出错）*/
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

// 字体放大
void AddTextVideo::enlargeFont(uchar* pDotMatrixData, uchar* pOut, const int& iSize)
{
	if (iSize < 1)
	{
		qDebug() << u8"设置的字体大小无效: " << iSize;
	}

	for (int i = 0; i < SIZE; ++i)
	{
		for (int j = 0; j < iSize; ++j)
		{
			pOut[i + j] = pDotMatrixData[i];
		}
	}
}

// 根据放大后的字体占用空间以及起始设置的偏移量计算偏移地址并将点阵替换至yuv数据中
void AddTextVideo::SuperpositionCharacter(uchar* pYuvData, const uchar* pDotMat, const int& iChartIndex, 
	const int& iX, const int& iY, const int& iSize, const int& iVideoWidth, const int& iVideoHeight)
{
	/*
		y首地址 = pYStart + (iY -1) * VideoW + x + (当前显示文字的索引 - 1) * 16 * 字体大小
	
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

				// uv 数据	
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