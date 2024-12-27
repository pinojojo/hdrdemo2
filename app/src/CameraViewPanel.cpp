#include "CameraViewPanel.h"
#include <QVBoxLayout>

#include "DummyTestCamera.h"

#include "logwidget.hpp"

CameraViewPanel::CameraViewPanel(QString desc, QWidget *parent)
    : QWidget(parent), m_desc(desc), m_layout(nullptr), m_frameRenderer(nullptr), m_controlBar(nullptr), m_camera(nullptr), m_frameBuffer(new lzx::TripleBuffer<lzx::Frame>()), m_isStreaming(false)
{
    setupUI();
    createConnections();

    if (m_desc == "test")
    {
        m_camera = new lzx::DummyTestCamera();
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
    delete m_frameBuffer;
}

void CameraViewPanel::setCamera(lzx::ICamera *camera)
{
    if (m_camera)
    {
        m_camera->stop();
        m_camera->close();
    }

    m_camera = camera;
    m_frameRenderer->setFrameBuffer(m_frameBuffer);
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

    QString fileName = m_camera->label().c_str();

    QDateTime current_date_time = QDateTime::currentDateTime();
    QString current_date = current_date_time.toString("yyyy-MM-dd_hh-mm-ss");
    fileName += "-" + current_date + ".png";

    m_frameRenderer->onCaptureFrame(fileName);
}

void CameraViewPanel::onRecordClicked(bool record)
{
    if (record)
    {
        QString filename = m_camera->label().c_str();
        filename += "-" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + ".avi";
        m_frameRenderer->startRecording(filename);
    }
    else
    {
        m_frameRenderer->stopRecording();
    }
}

void CameraViewPanel::onExposureChanged(int value)
{
    m_camera->set("exp", value);
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
    m_frameRenderer = new FrameRenderer(this);
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
}

void CameraViewPanel::handleCameraState(const std::string &state, const std::string &value)
{
    m_controlBar->setStatus(QString::fromStdString(state), QString::fromStdString(value));
}