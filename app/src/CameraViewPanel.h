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
    explicit CameraViewPanel(QString desc, QWidget *parent = nullptr, bool isReference = false);
    ~CameraViewPanel();

    void setCamera(lzx::ICamera *camera);

private slots:
    void onConnectClicked(bool connect);
    void onStreamClicked(bool stream);
    void onCaptureClicked();
    void onRecordClicked(bool record);
    void onExposureChanged(int value);
    void onGainChanged(int value);

private:
    void setupUI();
    void createConnections();
    void handleCameraState(const std::string &state, const std::string &value);

private:
    QString m_desc;
    QVBoxLayout *m_layout;
    FrameRenderer *m_frameRenderer;
    CameraControllerBar *m_controlBar;
    lzx::ICamera *m_camera;
    bool m_isStreaming;
    bool m_isReference;
};