// MaskOpenGLWidget.hpp
#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_3_Core> // 使用 OpenGL 3.3 Core 版本
#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMatrix4x4>

#include <QMainWindow>
#include <QCloseEvent>

#include <QDebug>

#include "Common.h"
#include "polygonrenderer.hpp"
#include "ImageRenderer.hpp"

class MaskOpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    enum UpdateMode
    {
        Single,    // 单次模式：不循环，单次响应数据变化
        Continuous // 连续模式：最快的循环速度
    };

    MaskOpenGLWidget(QWidget *parent = nullptr)
        : QOpenGLWidget(parent),
          polygonRenderer(new PolygonRenderer()),
          imageRenderer(new ImageRenderer())
    {
    }

    ~MaskOpenGLWidget()
    {
        makeCurrent();
        doneCurrent();
    }

public slots:
    // 处理Mask数据变化
    void onMaskDataChanged(const MaskData &data)
    {
        this->data = data;
        mode = data.continuousMode ? UpdateMode::Continuous : UpdateMode::Single;
        update();
    }

    // 处理翻转信息变化
    void onFlipped(bool xFlipped, bool yFlipped)
    {
        this->xFlipped = xFlipped;
        this->yFlipped = yFlipped;
        update();
    }

    // 处理旋转信息变化
    void onRotated(float angle)
    {
        rotateAngle = angle;
        update();
    }

    // 处理偏移
    void onTranslated(int x, int y)
    {
        xTranslate = x;
        yTranslate = y;
        update();
    }

    void onLumOffsetChanged(int offset)
    {
        lumOffset = offset;
        update();
    }

    // 处理全局反转
    void onGlobalInverse(bool inverse)
    {
        globalInverse = inverse;
        update();
    }

    // 处理只显示红色通道
    void onOnlyRedChannel(bool onlyRed)
    {
        qDebug() << "mask onOnlyRedChannel" << onlyRed;
        this->onlyRed = onlyRed;
        update();
    }

    // 切换连续模式
    void onContinuousMode(bool continuous)
    {
        qDebug() << "mask onContinuousMode" << continuous;
        mode = continuous ? UpdateMode::Continuous : UpdateMode::Single;
        update();
    }

    // 处理TransferFuntion值变化
    void onTransferFunctionChanged(const TransferFunction &tf)
    {
        transferFunction = tf;
        update();
    }

    // 处理DMD工作模式变化
    void onDMDWorkModeChanged(DMDWorkMode mode)
    {
        workMode = mode;

        update();
    }

protected:
    void initializeGL() override
    {
        initializeOpenGLFunctions();

        {

            // create a framebuffer object for intermediate rendering
            QOpenGLFramebufferObjectFormat format;
            format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
            format.setTextureTarget(GL_TEXTURE_2D);
            format.setInternalTextureFormat(GL_RGBA8);
            fboInter = new QOpenGLFramebufferObject(1024, 768, format);
        }

        {
            // crteate the qaud VAO
            vaoQuad = new QOpenGLVertexArrayObject(this);
            vaoQuad->create();
            vaoQuad->bind();

            // create the quad VBO
            float quadVertices[] = {
                // positions        // texture Coords
                1.0f, 1.0f, 0.0f, 1.0f, 1.0f,   // top right
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,  // bottom right
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom left
                -1.0f, 1.0f, 0.0f, 0.0f, 1.0f   // top left
            };
            QOpenGLBuffer vboQuad(QOpenGLBuffer::VertexBuffer);
            vboQuad.create();
            vboQuad.bind();
            vboQuad.allocate(quadVertices, sizeof(quadVertices));

            // set the vertex attribute pointers
            int stride = 5 * sizeof(float);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(float)));

            vboQuad.release();
            vaoQuad->release();
        }

        {
            // Vertex shader
            const char *vertexShaderSource = R"(
                #version 330 core
                layout (location = 0) in vec3 aPos;
                layout (location = 1) in vec2 aTexCoord;

                out vec2 TexCoord;

                void main()
                {
                    gl_Position = vec4(aPos, 1.0);
                    TexCoord = aTexCoord;
                }
            )";

            // Fragment shader
            const char *fragmentShaderSource = R"(
                #version 330 core
                out vec4 FragColor;
                in vec2 TexCoord;

                uniform sampler2D texture0;

                void main()
                {
                    ivec2 pixel = ivec2(gl_FragCoord.xy);
                    pixel.y = 2719 - pixel.y; // flip the y axis
                    int picIndex = pixel.y / 32; // 32 pixels per line
                    picIndex+=1;
                    int pixelIndexCompressed = 3072 * (pixel.y % 32) + pixel.x;
                    int pixelIndex = pixelIndexCompressed * 8;
                    int pixelRow = pixelIndex / 1024;
                    pixelRow = 767 - pixelRow; // flip the row
                    int pixelCol = pixelIndex % 1024;
                   
                    int redByte = 0;
                    int greenByte = 0;
                    int blueByte = 0;
                    for (int i = 0; i < 8; i++)
                    {
                        int pixelColNew = pixelCol + 7 - i;
                        vec2 texCoordReal = vec2(float(pixelColNew) / 1024.0, float(pixelRow) / 768.0) + vec2(0.5 / 1024.0, 0.5 / 768.0);
                        float texelValue = texture(texture0, texCoordReal).r;
                        //float texelValue = 0.75;

                        float texelScalar = texelValue * 255.0 + 0.1;

                        // red
                        if (texelScalar > picIndex)
                        {
                            redByte |= 1 << (i);
                        }

                        // green
                        if (texelScalar > (picIndex + 85))
                        {
                            greenByte |= 1 << (i);
                        }

                        // blue
                        if (texelScalar > (picIndex + 2 * 85))
                        {
                            blueByte |= 1 << (i);
                        }
                    }

                    //vec3 color = vec3(float(pixelRow) / 768.0, float(pixelCol) / 1024.0, float(0) / 255.0);
                    vec3 color = vec3(redByte / 255.0,  greenByte / 255.0,  blueByte / 255.0);
                  
                    FragColor =  vec4(color, 1.0);
                }
            )";

            // Initialize the shader program
            shaderProgramEncoding = new QOpenGLShaderProgram(this);
            shaderProgramEncoding->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
            shaderProgramEncoding->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
            shaderProgramEncoding->link();

            // Check for errors
            if (!shaderProgramEncoding->isLinked())
            {
                qDebug() << "Shader program failed to link:" << shaderProgramEncoding->log();
            }
        }

        polygonRenderer->initialize(this);
        imageRenderer->initialize(this);
    }

    void resizeGL(int w, int h) override
    {
        glViewport(0, 0, w, h);
    }

    void paintGL() override
    {
        qDebug() << "mask paintGL";

        if (workMode == DMDWorkMode::Normal)
        {
            renderCommonPart();
        }
        else
        {

            fboInter->bind();
            renderCommonPart();
            fboInter->release();

            // 渲染到屏幕 3072x2720
            glViewport(0, 0, 3072, 2720);
            glClearColor(0.0f, 0.f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            shaderProgramEncoding->bind();

            GLuint texID = fboInter->texture();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texID);

            vaoQuad->bind();
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            vaoQuad->release();

            shaderProgramEncoding->release();
            glBindTexture(GL_TEXTURE_2D, 0);

            // 检查OpenGL错误
            GLenum error = glGetError();
            if (error != GL_NO_ERROR)
            {
                qDebug() << "OpenGL error XXX:" << error;
            }
        }

        // 连续模式下，需要不断更新
        if (mode == UpdateMode::Continuous)
        {
            update();
        }
    }

private:
    PolygonRenderer *polygonRenderer = nullptr;
    ImageRenderer *imageRenderer = nullptr;
    UpdateMode mode = UpdateMode::Single;
    MaskData data;
    bool xFlipped = false;
    bool yFlipped = false;
    float rotateAngle = 0;
    int xTranslate = 0;
    int yTranslate = 0;
    bool globalInverse = false;
    bool onlyRed = false;
    int lumOffset = 0;
    TransferFunction transferFunction;
    DMDWorkMode workMode = DMDWorkMode::Normal;
    QOpenGLFramebufferObject *fboInter = nullptr;
    QOpenGLShaderProgram *shaderProgramEncoding = nullptr; // 编码模式的着色器程序
    QOpenGLVertexArrayObject *vaoQuad = nullptr;           // 用于渲染到屏幕的四边形的VAO

private:
    // 渲染公共部分 也就是不包含压缩变化的部分
    void renderCommonPart()
    {

        // 直接渲染到屏幕 1024x768
        glViewport(0, 0, 1024, 768);
        // 是否只渲染红色通道
        if (this->onlyRed)
        {
            qDebug() << "mask only red";
            glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
        }
        else
        {
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        }

        if (globalInverse)
        {
            glClearColor(1.0f - data.globalBackgroundIntensity, 1.0f - data.globalBackgroundIntensity, 1.0f - data.globalBackgroundIntensity, 1.0f);
        }
        else
        {
            glClearColor(data.globalBackgroundIntensity, data.globalBackgroundIntensity, data.globalBackgroundIntensity, 1.0f);
        }

        glClear(GL_COLOR_BUFFER_BIT);

        if (mode == UpdateMode::Continuous)
        {
            // Draw the image
            imageRenderer->draw(globalInverse,
                                transferFunction,
                                rotateAngle * 3.1415926 / 180,
                                QVector2D(xTranslate / 1024.f, yTranslate / 768.f),
                                xFlipped, yFlipped, lumOffset);
        }
        else if (mode == UpdateMode::Single)
        {
            for (const auto &polygon : data.polygons)
            {
                // 转换坐标系，原始坐标系为纹理坐标系，需要转换为NDC坐标系（ 把0-1.0的坐标转换为-1.0-1.0的坐标）
                std::vector<QVector2D> verticesNDC;
                for (const auto &v : polygon.vertices)
                {
                    auto pNDC = QVector2D(2.0f * v.x() - 1.0f, 2.0f * v.y() - 1.0f);

                    pNDC.setY(pNDC.y() / (1024 / 768.f));

                    // 翻转
                    if (xFlipped)
                    {
                        pNDC.setX(-pNDC.x());
                    }
                    if (yFlipped)
                    {
                        pNDC.setY(-pNDC.y());
                    }
                    // 旋转
                    auto angleRad = rotateAngle * 3.1415926 / 180;
                    pNDC = QVector2D(pNDC.x() * cos(angleRad) - pNDC.y() * sin(angleRad), pNDC.x() * sin(angleRad) + pNDC.y() * cos(angleRad));

                    // 平移 （需要将像素数转为NDC坐标）
                    pNDC.setX(pNDC.x() + xTranslate * 2.0f / width() / devicePixelRatioF());
                    pNDC.setY(pNDC.y() + yTranslate * 2.0f / height() / devicePixelRatioF());

                    // 乘以Ortho矩阵 x: -1.0 - 1.0, y: -1.0/aspect - 1.0/aspect
                    pNDC.setY(pNDC.y() / (768 / 1024.f));

                    verticesNDC.push_back(pNDC);
                }
                float polygonIntensity = globalInverse ? 1.0f - polygon.intensity : polygon.intensity;
                polygonRenderer->draw(verticesNDC, PolygonRenderer::Mode::Filler, polygonIntensity);
            }
        }
    }
};

class MaskWindow : public QMainWindow
{
    Q_OBJECT

public:
    static MaskWindow *instance()
    {
        static MaskWindow *instance = new MaskWindow();

        return instance;
    }

public slots:
    // 处理窗体信息变化
    void onPropertyChanged(const MaskWindowProperty &property)
    {
        property.visible ? show() : hide();
        qreal scaleFactor = devicePixelRatioF();
        resize(property.width / scaleFactor, property.height / scaleFactor);  // Ensure window has 1024x768 for different screen dpi.
        move(property.offsetX / scaleFactor, property.offsetY / scaleFactor); // 设置窗口位置
    }

    // 桥接 其中widget的更新信号
    void onMaskDataChanged(const MaskData &data)
    {
        maskWidget->onMaskDataChanged(data);
    }

    void onFlipped(bool xFlipped, bool yFlipped)
    {
        maskWidget->onFlipped(xFlipped, yFlipped);
    }

    void onRotated(float angle)
    {
        maskWidget->onRotated(angle);
    }

    void onTranslated(int x, int y)
    {
        maskWidget->onTranslated(x, y);
    }

    void onLumOffsetChanged(int offset)
    {
        maskWidget->onLumOffsetChanged(offset);
    }

    void onGlobalInverse(bool inverse)
    {
        maskWidget->onGlobalInverse(inverse);
    }

    void onOnlyRedChannel(bool onlyRed)
    {
        maskWidget->onOnlyRedChannel(onlyRed);
    }

    void onContinuousMode(bool continuous)
    {
        maskWidget->onContinuousMode(continuous);
    }

    void onTransferFunctionChanged(const TransferFunction &tf)
    {
        maskWidget->onTransferFunctionChanged(tf);
    }

    void onDMDWorkModeChanged(DMDWorkMode mode)
    {

        float scaleFactor = devicePixelRatioF();
        // resize the window
        if (mode == DMDWorkMode::Normal)
        {
            resize(1024 / scaleFactor, 768 / scaleFactor);
        }
        else
        {
            resize(3072 / scaleFactor, 2720 / scaleFactor);
        }

        maskWidget->onDMDWorkModeChanged(mode);
    }

private:
    MaskWindow()
    {
        setWindowFlags(Qt::FramelessWindowHint);
        resize(1024, 768);
        hide();

        // 创建 MaskOpenGLWidget 实例并设置为中心窗口
        maskWidget = new MaskOpenGLWidget(this);

        // 设置 OpenGL 上下文格式
        QSurfaceFormat format;
        format.setVersion(3, 3);                              // 设置 OpenGL 版本为 3.3
        format.setProfile(QSurfaceFormat::CoreProfile);       // 设置为 Core Profile
        format.setDepthBufferSize(24);                        // 设置深度缓冲区大小
        format.setStencilBufferSize(8);                       // 设置模板缓冲区大小
        format.setSamples(4);                                 // 设置多重采样（抗锯齿）
        format.setSwapBehavior(QSurfaceFormat::DoubleBuffer); // 设置为双缓冲
        format.setSwapInterval(1);                            // 设置交换间隔
        maskWidget->setFormat(format);                        // 设置 OpenGL 上下文格式

        setCentralWidget(maskWidget);
    }

    // 禁用拷贝构造函数和赋值操作符
    MaskWindow(const MaskWindow &) = delete;
    MaskWindow &operator=(const MaskWindow &) = delete;

private:
    MaskOpenGLWidget *maskWidget = nullptr;
};