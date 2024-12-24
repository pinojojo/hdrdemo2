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

void CameraViewPanel::onConnectClicked()
{
    if (!m_camera)
        return;

    if (m_camera->open())
    {
        // 可以在这里更新UI状态
    }
}

void CameraViewPanel::onStreamClicked()
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
    m_camera->snap();
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
    m_frameRenderer = new FrameRenderer(m_frameBuffer, this);
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
    connect(m_controlBar, &CameraControllerBar::exposureChanged,
            this, &CameraViewPanel::onExposureChanged);
    connect(m_controlBar, &CameraControllerBar::gainChanged,
            this, &CameraViewPanel::onGainChanged);
}