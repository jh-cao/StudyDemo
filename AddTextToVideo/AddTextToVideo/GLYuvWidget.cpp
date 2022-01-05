#include "GLYuvWidget.h"
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QDebug>
#define VERTEXIN 0
#define TEXTUREIN 1

GLYuvWidget::GLYuvWidget(QWidget *parent) :
    QOpenGLWidget(parent)
{
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, [=] {
        update();
    });
    timer->start(40);
}

GLYuvWidget::~GLYuvWidget()
{
    makeCurrent();
    m_objGLBuffer.destroy();
    m_pTextureY->destroy();
    m_pTextureU->destroy();
    m_pTextureV->destroy();
    doneCurrent();
}

void GLYuvWidget::RecvVideoStream(uchar *ptr, uint width, uint height)
{
    int ilength = width * height * 3 / 2;
    {
        std::lock_guard<std::mutex> lg(m_mutex);
        if (nullptr == m_pYuvBuffer)
        {
            m_pYuvBuffer = new uchar[ilength];
            m_iBufferLength = ilength;
        }
        if (m_iBufferLength < ilength)
        {
            delete m_pYuvBuffer;
            m_pYuvBuffer = new uchar[ilength];
            m_iBufferLength = ilength;
        }
        memcpy(m_pYuvBuffer, ptr, ilength);
        m_iVideoW = width;
        m_iVideoH = height;
    }
}

void GLYuvWidget::initializeGL()
{
    qDebug() << "initializeGL";
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    static const GLfloat vertices[]{
        //顶点坐标
        -1.0f,-1.0f,
        -1.0f,+1.0f,
        +1.0f,+1.0f,
        +1.0f,-1.0f,
        //纹理坐标
        0.0f,1.0f,
        0.0f,0.0f,
        1.0f,0.0f,
        1.0f,1.0f,
    };


    m_objGLBuffer.create();
    m_objGLBuffer.bind();
    m_objGLBuffer.allocate(vertices, sizeof(vertices));

    QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    const char *vsrc =
        "attribute vec4 vertexIn; \
		attribute vec2 textureIn; \
		varying vec2 textureOut;  \
		void main(void)           \
		{                         \
        gl_Position = vertexIn; \
        textureOut = textureIn; \
		}";
    vshader->compileSourceCode(vsrc);

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
	const char *fsrc =
		"#ifdef GL_ES\n"
		"#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
		"precision highp float;\n"
		"#else\n"
		"precision mediump float;\n"
		"#endif\n"
		"#endif\n"
		"varying vec2 textureOut;\n"
		"uniform sampler2D tex_y;\n"
		"uniform sampler2D tex_u;\n"
		"uniform sampler2D tex_v;\n"
		"void main(void)\n"
		"{\n"
		"    vec3 yuv; \n"
		"    vec3 rgb; \n"
		"    yuv.x = texture2D(tex_y, textureOut).r; \n"
		"    yuv.y = texture2D(tex_u, textureOut).r - 0.5; \n"
		"    yuv.z = texture2D(tex_v, textureOut).r - 0.5; \n"
		"    rgb = mat3( 1,       1,         1, \n"
		"                0,       -0.39465,  2.03211, \n"
		"                1.13983, -0.58060,  0) * yuv; \n"
		"    gl_FragColor = vec4(rgb, 1); \n"
		"}";
    fshader->compileSourceCode(fsrc);

    m_pShaderProgram = new QOpenGLShaderProgram(this);
    m_pShaderProgram->addShader(vshader);
    m_pShaderProgram->addShader(fshader);
    m_pShaderProgram->bindAttributeLocation("vertexIn", VERTEXIN);
    m_pShaderProgram->bindAttributeLocation("textureIn", TEXTUREIN);
    m_pShaderProgram->link();
    m_pShaderProgram->bind();
    m_pShaderProgram->enableAttributeArray(VERTEXIN);
    m_pShaderProgram->enableAttributeArray(TEXTUREIN);
    m_pShaderProgram->setAttributeBuffer(VERTEXIN, GL_FLOAT, 0, 2, 2 * sizeof(GLfloat));
    m_pShaderProgram->setAttributeBuffer(TEXTUREIN, GL_FLOAT, 8 * sizeof(GLfloat), 2, 2 * sizeof(GLfloat));

    m_textureUniformY = m_pShaderProgram->uniformLocation("tex_y");
    m_textureUniformU = m_pShaderProgram->uniformLocation("tex_u");
    m_textureUniformV = m_pShaderProgram->uniformLocation("tex_v");
    m_pTextureY = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_pTextureU = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_pTextureV = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_pTextureY->create();
    m_pTextureU->create();
    m_pTextureV->create();
    m_idY = m_pTextureY->textureId();
    m_idU = m_pTextureU->textureId();
    m_idV = m_pTextureV->textureId();
    glClearColor(0.0, 0.0, 0.0, 0.0);
}

void GLYuvWidget::paintGL()
{
    std::lock_guard<std::mutex> lg(m_mutex);
    if (nullptr == m_pYuvBuffer)
    {
        return;
    }

    // 计算窗口的宽高比和码流的宽高比，设置显示比例
    long yuv_w_view_h = m_iVideoW * height();
    long yuv_h_view_w = m_iVideoH * width();
    if (yuv_w_view_h > yuv_h_view_w)
    { // 横向铺满，纵向显示不全，需要计算位置以及大小
        long lRealViewH = yuv_h_view_w / m_iVideoW;
        int y = (height() - lRealViewH) / 2;
        glViewport(0, y, width(), lRealViewH);
    }
    else
    {
        long lRealViewW = yuv_w_view_h / m_iVideoH;
        int x = (width() - lRealViewW) / 2;
        glViewport(x, 0, lRealViewW, height());
    }
    //此时坐标中心还是在屏幕水平面中间，只是前后左右的距离已限制。
    glActiveTexture(GL_TEXTURE0);  //激活纹理单元GL_TEXTURE0,系统里面的
    glBindTexture(GL_TEXTURE_2D, m_idY); //绑定y分量纹理对象id到激活的纹理单元
                                       //使用内存中的数据创建真正的y分量纹理数据
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_iVideoW, m_iVideoH, 0, GL_RED, GL_UNSIGNED_BYTE, m_pYuvBuffer);
    // 纹理参数解析
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE1); //激活纹理单元GL_TEXTURE1
    glBindTexture(GL_TEXTURE_2D, m_idU);
    //使用内存中的数据创建真正的u分量纹理数据
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_iVideoW >> 1, m_iVideoH >> 1, 0, GL_RED, GL_UNSIGNED_BYTE, m_pYuvBuffer + m_iVideoW * m_iVideoH);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE2); //激活纹理单元GL_TEXTURE2
    glBindTexture(GL_TEXTURE_2D, m_idV);
    //使用内存中的数据创建真正的v分量纹理数据
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_iVideoW >> 1, m_iVideoH >> 1, 0, GL_RED, GL_UNSIGNED_BYTE, m_pYuvBuffer + m_iVideoW*m_iVideoH * 5 / 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFlush();
    //指定y纹理要使用新值
    glUniform1i(m_textureUniformY, 0);
    //指定u纹理要使用新值
    glUniform1i(m_textureUniformU, 1);
    //指定v纹理要使用新值
    glUniform1i(m_textureUniformV, 2);
    //使用顶点数组方式绘制图形
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}