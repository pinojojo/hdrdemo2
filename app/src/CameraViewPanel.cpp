#include "CameraViewPanel.h"
#include <QVBoxLayout>

#include "DummyTestCamera.h"
#include "PlayerOne.hpp"
#include "USBCamera.hpp"

#include "Settings.hpp"

#include "logwidget.hpp"

CameraViewPanel::CameraViewPanel(QString desc, QWidget *parent, bool isReference)
    : QWidget(parent),
      m_desc(desc),
      m_layout(nullptr),
      m_frameRenderer(nullptr),
      m_controlBar(nullptr),
      m_camera(nullptr),
      m_isStreaming(false),
      m_isReference(isReference)
{
    setupUI();
    createConnections();

    if (m_desc == "test8")
    {
        m_camera = new lzx::DummyTestCamera(8);
    }
    else if (m_desc == "test16")
    {
        m_camera = new lzx::DummyTestCamera(16);
    }
    else if (m_desc == "PlayerOne")
    {
        m_camera = new PlayerOne();
    }
    else if (m_desc == "MVS")
    {
        m_camera = new USBCamera("Hikvision");
    }

    // 设置状态回调
    if (m_camera)
    {
        m_camera->setStateChangedCallback([this](const std::string &state, const std::string &value)
                                          {
            // 使用Qt::QueuedConnection确保在主线程中更新UI
            QMetaObject::invokeMethod(this, [this, state, value]() {
                handleCameraState(state, value);
            }, Qt::QueuedConnection); });
    }

    m_frameRenderer->setAssociateCamera(m_camera);
}

CameraViewPanel::~CameraViewPanel()
{
    if (m_camera)
    {
        m_camera->stop();
        m_camera->close();
    }
}

void CameraViewPanel::setCamera(lzx::ICamera *camera)
{
    if (m_camera)
    {
        m_camera->stop();
        m_camera->close();
    }

    m_camera = camera;
}

void CameraViewPanel::onConnectClicked(bool connect)
{
    if (!m_camera)
        return;

    if (connect)
    {
        m_camera->open();
    }
    else
    {
        m_camera->close();
    }
}

void CameraViewPanel::onStreamClicked(bool stream)
{
    if (!m_camera)
        return;

    if (m_isStreaming)
    {
        m_camera->stop();
        m_frameRenderer->onEnableUpdate(false);
    }
    else
    {
        m_camera->start();
        m_frameRenderer->onEnableUpdate(true);
    }

    m_isStreaming = !m_isStreaming;
}

void CameraViewPanel::onCaptureClicked()
{
    if (!m_camera)
        return;

    QString filename = m_camera->label().c_str();

    QDateTime current_date_time = QDateTime::currentDateTime();
    QString current_date = current_date_time.toString("yyyy-MM-dd_hh-mm-ss");
    filename += "-" + current_date + ".png";

    filename = Settings::getInstance().getDefaultSavePath() + "/" + filename;

    m_frameRenderer->onCaptureFrame(filename);
}

void CameraViewPanel::onRecordClicked(bool record)
{
    if (record)
    {
        QString filename = m_camera->label().c_str();
        filename += "-" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + ".avi";

        filename = Settings::getInstance().getDefaultSavePath() + "/" + filename;

        m_frameRenderer->startRecording(filename);
    }
    else
    {
        m_frameRenderer->stopRecording();
    }
}

void CameraViewPanel::onExposureChanged(int value)
{
    m_camera->set("exposure", value);
}

void CameraViewPanel::onGainChanged(int value)
{
    m_camera->set("gain", value);
}

void CameraViewPanel::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    // 创建渲染器
    m_frameRenderer = new FrameRenderer(this, m_isReference);
    m_layout->addWidget(m_frameRenderer, 1); // 1表示拉伸比例

    // 创建控制栏
    m_controlBar = new CameraControllerBar(this);
    m_layout->addWidget(m_controlBar);

    setLayout(m_layout);
}

void CameraViewPanel::createConnections()
{
    connect(m_controlBar, &CameraControllerBar::connectClicked,
            this, &CameraViewPanel::onConnectClicked);
    connect(m_controlBar, &CameraControllerBar::streamClicked,
            this, &CameraViewPanel::onStreamClicked);
    connect(m_controlBar, &CameraControllerBar::captureClicked,
            this, &CameraViewPanel::onCaptureClicked);
    connect(m_controlBar, &CameraControllerBar::recordingClicked,
            this, &CameraViewPanel::onRecordClicked);
    connect(m_controlBar, &CameraControllerBar::exposureChanged,
            this, &CameraViewPanel::onExposureChanged);
    connect(m_controlBar, &CameraControllerBar::gainChanged,
            this, &CameraViewPanel::onGainChanged);

    // 连接直方图请求信号
    connect(m_controlBar, &CameraControllerBar::requestHistogram,
            m_frameRenderer, &FrameRenderer::setHistogramEnabled);

    // 将 FrameRenderer 的直方图数据信号连接到 CameraControllerBar
    connect(m_frameRenderer, &FrameRenderer::histogramCalculated,
            m_controlBar, &CameraControllerBar::onHistogramUpdated);

    // 将 FrameRenderer 的FPS信号连接到 CameraControllerBar
    connect(m_frameRenderer, &FrameRenderer::fpsUpdated,
            m_controlBar, &CameraControllerBar::onFPSUpdated);
}

void CameraViewPanel::handleCameraState(const std::string &state, const std::string &value)
{
    m_controlBar->onCameraStatusChanged(QString::fromStdString(state), QString::fromStdString(value));
}