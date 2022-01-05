#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QTimer>
#include <mutex>
#include <memory>

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)
QT_FORWARD_DECLARE_CLASS(QOpenGLTexture)

class GLYuvWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    GLYuvWidget(QWidget *parent = 0);
    ~GLYuvWidget();

    void RecvVideoStream(uchar *ptr, uint width, uint height); //显示一帧Yuv图像
    void initializeGL() Q_DECL_OVERRIDE;

public:
    void paintGL() Q_DECL_OVERRIDE;


private:
    QOpenGLShaderProgram *m_pShaderProgram{ nullptr };
    QOpenGLBuffer m_objGLBuffer;
    GLuint m_textureUniformY, m_textureUniformU, m_textureUniformV; //opengl中y、u、v分量位置
    QOpenGLTexture *m_pTextureY{ nullptr }, *m_pTextureU{ nullptr }, *m_pTextureV{ nullptr };
    GLuint m_idY, m_idU, m_idV; //自己创建的纹理对象ID，创建错误返回0
    uint m_iVideoW, m_iVideoH;  // 一帧yuv的宽高
    uchar *m_pYuvBuffer{ nullptr };  // 一帧yuv数据的缓存
    int m_iBufferLength{ 0 };

    QTimer* timer;
    std::mutex m_mutex;
};
