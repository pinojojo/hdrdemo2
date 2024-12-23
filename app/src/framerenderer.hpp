#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core> // 使用 OpenGL 3.3 Core 版本
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QImage>
#include <QTimer>

#include "ICamera.hpp"
#include "TripleBuffer.h"
#include "Frame.h"
#include "Common.h"

#include "logwidget.hpp"

class FrameRenderer : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    enum class Mode
    {
        Normal,
        DrawLines, // Mode for drawing lines
    };

    static FrameRenderer *instance(lzx::TripleBuffer<lzx::Frame> *tripleBuffer, QWidget *parent = nullptr)
    {
        static FrameRenderer *instance = new FrameRenderer(tripleBuffer, parent);
        return instance;
    }

    void setFrameBuffer(lzx::TripleBuffer<lzx::Frame> *tripleBuffer);

    std::vector<MaskPolygon> getMaskPolygons() const;

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

    // wheel event
    void wheelEvent(QWheelEvent *event) override;

    // mouse events
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    // context menu event
    void contextMenuEvent(QContextMenuEvent *event) override;

public slots:
    void onFrameChanged(const QImage &frame);
    void onFrameChangedDirectMode(const unsigned char *data, int width, int height, int channels);
    void onFrameChangedFromCamera(lzx::ICamera *camera);
    void onAutoFit();

    void onModeChanged(FrameRenderer::Mode mode);
    void enterDrawMode(bool enter);
    void clearMask();

    void onEnableUpdate(bool enable); // 当使用自动掉光模式时，就不能在此处更新了，要把消费权交给MaskWindow

private:
    struct Impl;
    Impl *impl;

    lzx::TripleBuffer<lzx::Frame> *frameBuffer;

    Mode currentMode = Mode::Normal; // 当前的绘制模式
    QTimer *updateTimer = nullptr;   // 更新定时器
    bool enableUpdate = true;        // 是否允许更新

private:
    // 单例模式
    explicit FrameRenderer(lzx::TripleBuffer<lzx::Frame> *tripleBuffer, QWidget *parent = nullptr);
    virtual ~FrameRenderer();
    // 禁用拷贝构造函数和赋值运算符
    FrameRenderer(const FrameRenderer &) = delete;
    FrameRenderer &operator=(const FrameRenderer &) = delete;

    void updateOpenGLTexture(GLuint textureID, int width, int height, const GLubyte *data, int bytesPerPixel);
};
