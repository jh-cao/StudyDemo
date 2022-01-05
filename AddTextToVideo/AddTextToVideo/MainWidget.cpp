#include "MainWidget.h"
#include <iostream>
#include <QDebug>
#include <QString>
#include <QTextCodec>
#include <QByteArray>
#include "GLYuvWidget.h"
#include "AddTextVideo.h"

#define WIDTH 640
#define HEIGHT 360


MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
	m_pGLYuvWidget = new GLYuvWidget(this);
	m_pAddTextVideo = new AddTextVideo(this);
	ui.gridLayout->addWidget(m_pGLYuvWidget);
	ReadYuvData();
	initConnection();
}


// 读取yuv文件并对YUV数据分别存储
void MainWidget::ReadYuvData()
{
	std::string strPath(_pgmptr);
	int iRIndex = strPath.rfind("\\");
	std::string strFilePath = "";
	// 读取的YUV数据的比例 为 4:2:2
	strFilePath = strPath.substr(0, iRIndex) + "\\640_360YUV_4_2_0.yuv";
	FILE* fp = fopen(strFilePath.c_str(), "rb");
	if (NULL != fp)
	{
		fseek(fp, 0, SEEK_END);
		int64_t i64FileSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		m_pYuvBuffer = new unsigned char[i64FileSize];
		m_pYuvAddTextBuffer = new unsigned char[i64FileSize];
		std::cout << "iYUVFilrSzie = " << i64FileSize << std::endl;
		fread(m_pYuvBuffer, i64FileSize, 1, fp);
		fclose(fp);
	}
	else
	{
		std::cout << "test--打开文件失败" << std::endl;
	}
}

void MainWidget::initConnection()
{
	connect(ui.btnOK, &QPushButton::clicked, this, &MainWidget::onOK);
}

// 触发文字叠加以及渲染
void MainWidget::onOK()
{
	qDebug() << "X: " << ui.spinBoxX->value() << ", Y: " << ui.spinBoxY->value() << ", R: " << ui.spinBoxR->value() 
		<< ", G: " << ui.spinBoxG->value() << ", B: " << ui.spinBoxB->value() 
		<< u8", 字体大小: " << ui.spinBox->value() << u8", 叠加文字: "
		<< QString::fromLocal8Bit(ui.lineEdit->text().toLocal8Bit());
	memcpy(m_pYuvAddTextBuffer, m_pYuvBuffer, WIDTH * HEIGHT * 3 / 2);
	QTextCodec* gb2312Codec = QTextCodec::codecForName("gb2312");
	QByteArray ByteGb2312 = gb2312Codec->fromUnicode(ui.lineEdit->text());

	// 高位区号，低位位号
	qDebug() << u8"编码: " << ByteGb2312.toHex();
	
	m_pAddTextVideo->AddTextVideoFrame((uchar*)ByteGb2312.data(), m_pYuvAddTextBuffer, ui.spinBoxR->value(), ui.spinBoxG->value()
		, ui.spinBoxB->value(), ui.spinBoxX->value(), ui.spinBoxY->value(), ui.spinBox->value(), WIDTH, HEIGHT);

	m_pGLYuvWidget->RecvVideoStream(m_pYuvAddTextBuffer, WIDTH, HEIGHT);
}