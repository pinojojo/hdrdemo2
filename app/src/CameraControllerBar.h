#ifndef CAMERACONTROLLERBAR_H
#define CAMERACONTROLLERBAR_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QSpinBox>

class CameraControllerBar : public QWidget
{
    Q_OBJECT

public:
    explicit CameraControllerBar(QWidget *parent = nullptr);
    ~CameraControllerBar();

    // 设置帧率显示
    void setFPS(double fps);

signals:
    void connectClicked();
    void streamClicked();
    void captureClicked();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    // UI 组件
    QPushButton *m_connectButton;
    QPushButton *m_streamButton;
    QPushButton *m_captureButton;
    QPushButton *m_recordingButton;
    QSpinBox *m_exposureSpinBox;
    QSpinBox *m_gainSpinBox;

    QLabel *m_fpsLabel;

    // 布局
    QHBoxLayout *m_layout;

    // 内部方法
    void setupUI();
    void createConnections();
    QPushButton *createButton(const QString &iconPath, const QString &tooltip);

    // 状态
    bool m_isConnected;
    bool m_isStreaming;
};

#endif // CAMERACONTROLLERBAR_H