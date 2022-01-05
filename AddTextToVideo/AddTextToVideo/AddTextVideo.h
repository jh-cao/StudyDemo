/*
	yuv文件中添加文字
*/

#pragma once

#include <QObject>


class AddTextVideo : public QObject
{
	Q_OBJECT

public:
	AddTextVideo(QObject *parent);
	~AddTextVideo();


	// 获取添加文字后的数据
	void AddTextVideoFrame(uchar* sAddStr, uchar* pYuvData, const int& iRed, const int& iGreen, const int& iBlue
		,const int& iX, const int& iY, const int& iSize, const int& iVideoWidth, const int& iVideoHeight);

private:
	// 初始化(读取点阵字库文件)
	void init();
	// 获取点阵数据
	void getDotMatrix(uchar* ch, uchar* pDotMatrixData);
	// 字体放大
	void enlargeFont(uchar* pDotMatrixData, uchar* pOut, const int& iSize);
	// 根据放大后的字体占用空间以及起始设置的偏移量计算偏移地址并将点阵替换至yuv数据中
	void SuperpositionCharacter(uchar* pYuvData, const uchar* pDotMat, const int& iChartIndex, 
		const int& iX, const int& iY, const int& iSize, const int& iVideoWidth, const int& iVideoHeight);


private:
	uchar* m_pTextPointData{ nullptr };  // 16*16点阵字库数据

	int m_iy{0};
	int m_iu{0};
	int m_iv{0};
};
