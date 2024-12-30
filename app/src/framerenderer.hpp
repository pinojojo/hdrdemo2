#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core> // 使用 OpenGL 3.3 Core 版本
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QImage>
#include <QTimer>
#include "ICamera.hpp"

#include "ICamera.hpp"
#include "TripleBuffer.h"
#include "Frame.h"
#include "Common.h"

#include <QMediaRecorder>
#include <QVideoSink>
#include <QVideoFrame>
#include <QVideoFrameInput>

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

    enum class HistogramSamplingMode
    {
        Fine = 1,   // 逐像素采样
        Medium = 4, // 每4个像素采样一次
        Coarse = 16 // 每16个像素采样一次
    };

    explicit FrameRenderer(QWidget *parent = nullptr);
    virtual ~FrameRenderer();
    void setAssociateCamera(lzx::ICamera *camera) { associateCamera = camera; }
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

signals:

    void histogramCalculated(const std::vector<int> &histogram, int maxValue);
    void fpsUpdated(double fps);

public slots:
    void onFrameChanged(const QImage &frame);
    void onFrameChangedDirectMode(const unsigned char *data, int width, int height, int channels, int bitDepth = 8);
    void onFrameChangedFromCamera(lzx::ICamera *camera);
    void onAutoFit();
    void onModeChanged(FrameRenderer::Mode mode);
    void enterDrawMode(bool enter);
    void clearMask();
    void onEnableUpdate(bool enable); // 当使用自动调光模式时，就不能在此处更新了，要把消费权交给MaskWindow
    void onCaptureFrame(QString fileName)
    {
        m_requestCapture = true;
        m_captureFileName = fileName;
    }

    void startRecording(QString filename);
    void stopRecording();

    void setHistogramEnabled(bool enabled)
    {
        m_histogramEnabled = enabled;
        Log::info(QString("Histogram enabled: %1").arg(enabled ? "true" : "false"));
    }

    void setHistogramBins(int bins) { m_histogramBins = bins; }
    void setHistogramSamplingMode(HistogramSamplingMode mode) { m_histogramSamplingMode = mode; }

private:
    FrameRenderer(const FrameRenderer &) = delete;
    FrameRenderer &operator=(const FrameRenderer &) = delete;

    bool m_isFirstUpdate = true;

    // 拍照
    bool m_requestCapture = false;
    QString m_captureFileName;

    // 录像
    std::unique_ptr<QMediaCaptureSession> m_captureSession;
    std::unique_ptr<QVideoFrameInput> m_frameInput;
    std::unique_ptr<QMediaRecorder> m_recorder;
    std::unique_ptr<QVideoSink> m_videoSink;
    bool m_isRecording = false;
    QString m_recordingFile;
    qint64 m_recordingStartTime = 0;
    qint64 m_lastFrameTime = 0;
    const qint64 FRAME_INTERVAL = 20; // 50fps = 20ms per framebool

    // 直方图
    bool m_histogramEnabled = false;
    int m_histogramBins = 100; // 默认100个bin
    HistogramSamplingMode m_histogramSamplingMode = HistogramSamplingMode::Medium;

    // FPS 计算相关成员
    QElapsedTimer m_fpsTimer;
    int m_frameCount = 0;
    qint64 m_lastFpsUpdate = 0;
    static constexpr int FPS_UPDATE_INTERVAL = 1000; // 每秒更新一次FPS

    struct Impl;
    Impl *impl;

    Mode currentMode = Mode::Normal; // 当前的绘制模式
    bool enableUpdate = true;        // 是否允许持续更新

    lzx::ICamera *associateCamera = nullptr; // 关联的相机

    std::vector<unsigned char> frameData; // 临时存储图像数据，用于绘制

    void updateOpenGLTexture(GLuint textureID, int width, int height, const GLubyte *data, int channels, int bitDepth);

    void calculateHistogram(const unsigned char *data, int width, int height,
                            int channels, int bitDepth);
};
