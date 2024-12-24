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
        Normal,    // 常规模式，显示图像即可
        DrawLines, // 绘制多边形模式，在显示图像的基础上，绘制多边形、线条之类的
    };

    explicit FrameRenderer(lzx::TripleBuffer<lzx::Frame> *tripleBuffer, QWidget *parent = nullptr);
    explicit FrameRenderer(QWidget *parent = nullptr);

    virtual ~FrameRenderer();

    void setFrameBuffer(lzx::TripleBuffer<lzx::Frame> *tripleBuffer);

    std::vector<MaskPolygon> getMaskPolygons() const;

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

    void wheelEvent(QWheelEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

public slots:
    void onFrameChanged(const QImage &frame);
    void onFrameChangedDirectMode(const unsigned char *data, int width, int height, int channels);
    void onFrameChangedFromCamera(lzx::ICamera *camera);
    void onAutoFit();
    void onModeChanged(FrameRenderer::Mode mode);
    void enterDrawMode(bool enter);
    void clearMask();
    void onEnableUpdate(bool enable); // 当使用自动调光模式时，就不能在此处更新了，要把消费权交给MaskWindow

private:
    FrameRenderer(const FrameRenderer &) = delete;
    FrameRenderer &operator=(const FrameRenderer &) = delete;

    struct Impl;
    Impl *impl;
    lzx::TripleBuffer<lzx::Frame> *frameBuffer; // 图像输入源，使用三缓，这样相机可以在另一个线程中不断地生产图像，这样能更好地区分真实帧率和显示帧率
    Mode currentMode = Mode::Normal;            // 当前的绘制模式
    bool enableUpdate = true;                   // 是否允许持续更新

    void updateOpenGLTexture(GLuint textureID, int width, int height, const GLubyte *data, int bytesPerPixel);
};
