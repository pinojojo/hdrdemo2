#ifndef CAMERACONTROLLERBAR_H
#define CAMERACONTROLLERBAR_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QSpinBox>
#include "GrayMappingWidget.h"

// LUT编辑器弹窗
class LutPopupWindow : public QWidget
{
    Q_OBJECT
public:
    explicit LutPopupWindow(QWidget *parent = nullptr);

    void updateHistogram(const std::vector<int> &histogram, int maxValue);

signals:
    void visibilityChanged(bool visible);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void focusOutEvent(QFocusEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QPoint m_dragPosition;
    bool m_isDragging;

    GrayMappingWidget *m_mappingWidget;
};

class CameraControllerBar : public QWidget
{
    Q_OBJECT

public:
    explicit CameraControllerBar(QWidget *parent = nullptr);
    ~CameraControllerBar();

    // 设置帧率显示
    void setFPS(double fps);

public slots:
    void onCameraStatusChanged(QString status, QString value);
    void onHistogramUpdated(const std::vector<int> &histogram, int maxValue);

signals:
    void connectClicked(bool connect);
    void streamClicked(bool stream);
    void captureClicked();
    void recordingClicked(bool record);
    void exposureChanged(int value);
    void gainChanged(int value);
    void requestHistogram(bool enable);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    // UI 组件
    QPushButton *m_connectButton;
    QPushButton *m_streamButton;
    QPushButton *m_captureButton;
    QPushButton *m_recordingButton;
    QPushButton *m_lutButton;
    QSpinBox *m_exposureSpinBox;
    QSpinBox *m_gainSpinBox;

    QLabel *m_fpsLabel;

    LutPopupWindow *m_lutPopupWindow;

    // 布局
    QHBoxLayout *m_layout;

    // 内部方法
    void setupUI();
    void createConnections();

    QPushButton *createButton(const QString &iconPath, const QString &tooltip);

    void onRecordClicked();

    // 状态
    bool m_isConnected = false;
    bool m_isStreaming = false;
    bool m_isRecording = false;
};

#endif // CAMERACONTROLLERBAR_H