#pragma once

#include <QtWidgets/QWidget>
#include "ui_MainWidget.h"

class GLYuvWidget;
class AddTextVideo;
class MainWidget : public QWidget
{
    Q_OBJECT

public:
    MainWidget(QWidget *parent = Q_NULLPTR);

	// 读取yuv文件
	void ReadYuvData();

private slots:
	// 触发文字叠加以及渲染
	void onOK();

private:
	void initConnection();

private:
    Ui::MainWidgetClass ui;

	unsigned char* m_pYuvBuffer{ nullptr };
	unsigned char* m_pYuvAddTextBuffer{ nullptr };

	GLYuvWidget* m_pGLYuvWidget{ nullptr };
	AddTextVideo* m_pAddTextVideo{ nullptr };
};				   
