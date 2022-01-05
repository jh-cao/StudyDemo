/*
	yuv�ļ����������
*/

#pragma once

#include <QObject>


class AddTextVideo : public QObject
{
	Q_OBJECT

public:
	AddTextVideo(QObject *parent);
	~AddTextVideo();


	// ��ȡ������ֺ������
	void AddTextVideoFrame(uchar* sAddStr, uchar* pYuvData, const int& iRed, const int& iGreen, const int& iBlue
		,const int& iX, const int& iY, const int& iSize, const int& iVideoWidth, const int& iVideoHeight);

private:
	// ��ʼ��(��ȡ�����ֿ��ļ�)
	void init();
	// ��ȡ��������
	void getDotMatrix(uchar* ch, uchar* pDotMatrixData);
	// ����Ŵ�
	void enlargeFont(uchar* pDotMatrixData, uchar* pOut, const int& iSize);
	// ���ݷŴ�������ռ�ÿռ��Լ���ʼ���õ�ƫ��������ƫ�Ƶ�ַ���������滻��yuv������
	void SuperpositionCharacter(uchar* pYuvData, const uchar* pDotMat, const int& iChartIndex, 
		const int& iX, const int& iY, const int& iSize, const int& iVideoWidth, const int& iVideoHeight);


private:
	uchar* m_pTextPointData{ nullptr };  // 16*16�����ֿ�����

	int m_iy{0};
	int m_iu{0};
	int m_iv{0};
};
