#pragma once

#include <QWidget>
#include <QString>
#include "ICamera.hpp"
#include "framerenderer.hpp"
#include "CameraControllerBar.h"
#include "TripleBuffer.h"
#include "Frame.h"

class QVBoxLayout;

class CameraViewPanel : public QWidget
{
    Q_OBJECT

public:
    explicit CameraViewPanel(QString desc, QWidget *parent = nullptr);
    ~CameraViewPanel();

    void setCamera(lzx::ICamera *camera);

private slots:
    void onConnectClicked();
    void onStreamClicked();
    void onCaptureClicked();
    void onExposureChanged(int value);
    void onGainChanged(int value);

private:
    void setupUI();
    void createConnections();

private:
    QString m_desc;
    QVBoxLayout *m_layout;
    FrameRenderer *m_frameRenderer;
    CameraControllerBar *m_controlBar;
    lzx::ICamera *m_camera;
    lzx::TripleBuffer<lzx::Frame> *m_frameBuffer;
    bool m_isStreaming;
};