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

	// ��ȡyuv�ļ�
	void ReadYuvData();

private slots:
	// �������ֵ����Լ���Ⱦ
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
