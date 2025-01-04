#include "CameraControllerBar.h"
#include <QIcon>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPainter>

#include "logwidget.hpp"

CameraControllerBar::CameraControllerBar(QWidget *parent)
    : QWidget(parent), m_isConnected(false), m_isStreaming(false)
{
    setupUI();
    createConnections();
}

CameraControllerBar::~CameraControllerBar()
{
    // 断开所有信号连接
    disconnect(m_connectButton, nullptr, this, nullptr);
    disconnect(m_streamButton, nullptr, this, nullptr);
    disconnect(m_captureButton, nullptr, this, nullptr);
    disconnect(m_recordingButton, nullptr, this, nullptr);
    disconnect(m_exposureSpinBox, nullptr, this, nullptr);
    disconnect(m_gainSpinBox, nullptr, this, nullptr);

    // 确保 LUT 窗口被正确关闭和清理
    if (m_lutPopupWindow)
    {
        disconnect(m_lutPopupWindow, nullptr, this, nullptr);
        if (m_lutPopupWindow->isVisible())
        {
            m_lutPopupWindow->hide();
        }
        // 由于设置了 WA_DeleteOnClose，hide() 会触发删除
        // 但为了安全起见，我们还是显式删除
        m_lutPopupWindow->deleteLater();
        m_lutPopupWindow = nullptr;
    }

    // 清理其他控件
    // Qt的父子关系会自动处理这些控件的删除，但为了安全起见，设置为nullptr
    m_connectButton = nullptr;
    m_streamButton = nullptr;
    m_captureButton = nullptr;
    m_recordingButton = nullptr;
    m_lutButton = nullptr;
    m_exposureSpinBox = nullptr;
    m_gainSpinBox = nullptr;
    m_fpsLabel = nullptr;
    m_layout = nullptr;
}

void CameraControllerBar::setupUI()
{

    m_lutPopupWindow = new LutPopupWindow(this);

    // 创建主布局
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(4, 0, 4, 0);
    m_layout->setSpacing(2);

    // 创建按钮
    m_connectButton = createButton(":/icons8_broken_link.svg", tr("Connect to camera"));
    m_streamButton = createButton(":/icons8_play.svg", tr("Start/Stop stream"));
    m_captureButton = createButton(":/icons8_unsplash.svg", tr("Capture frame"));
    m_recordingButton = createButton(":/icons8_not_record.svg", tr("Start/Stop recording"));
    m_lutButton = createButton(":/icons8_histogram.svg", tr("Open LUT editor"));

    m_exposureSpinBox = new QSpinBox(this);
    m_exposureSpinBox->setPrefix("Exp: ");
    m_exposureSpinBox->setRange(0, 1000000);
    m_exposureSpinBox->setSingleStep(2000);
    m_exposureSpinBox->setSuffix(" μs");
    m_exposureSpinBox->setFixedWidth(120);
    m_exposureSpinBox->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_exposureSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);

    m_gainSpinBox = new QSpinBox(this);
    m_gainSpinBox->setPrefix("Gain: ");
    m_gainSpinBox->setRange(0, 200);
    m_gainSpinBox->setSingleStep(1);
    m_gainSpinBox->setFixedWidth(65);
    m_gainSpinBox->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_gainSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);

    // 创建FPS标签
    m_fpsLabel = new QLabel("0 FPS");
    m_fpsLabel->setMinimumWidth(60);
    m_fpsLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // 添加到布局
    m_layout->addWidget(m_connectButton);
    m_layout->addWidget(m_streamButton);
    m_layout->addWidget(m_captureButton);
    m_layout->addWidget(m_recordingButton);
    m_layout->addWidget(m_lutButton);
    m_layout->addWidget(m_exposureSpinBox);
    m_layout->addWidget(m_gainSpinBox);
    m_layout->addWidget(m_fpsLabel);

    // 初始状态设置
    m_streamButton->setEnabled(false);
    m_captureButton->setEnabled(false);
    m_recordingButton->setEnabled(false);
    m_lutButton->setEnabled(false);
    m_exposureSpinBox->setEnabled(false);
    m_gainSpinBox->setEnabled(false);

    // 设置固定高度
    setFixedHeight(30);
    this->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
}

QPushButton *CameraControllerBar::createButton(const QString &iconPath, const QString &tooltip)
{
    QPushButton *button = new QPushButton(this);
    button->setIcon(QIcon(iconPath));
    button->setToolTip(tooltip);
    button->setFixedSize(20, 20);
    button->setIconSize(QSize(13, 13));
    button->setFocusPolicy(Qt::ClickFocus);
    return button;
}

void CameraControllerBar::onRecordClicked()
{
    m_isRecording = !m_isRecording;

    if (m_isRecording)
    {
        m_recordingButton->setIcon(QIcon(":/icons8_record.svg"));
    }
    else
    {
        m_recordingButton->setIcon(QIcon(":/icons8_not_record.svg"));
    }

    emit recordingClicked(m_isRecording);
}

void CameraControllerBar::createConnections()
{
    connect(m_connectButton, &QPushButton::clicked, this, [this]()
            { emit connectClicked(!m_isConnected); });

    connect(m_streamButton, &QPushButton::clicked, this, [this]()
            { emit streamClicked(!m_isStreaming); });

    connect(m_captureButton, &QPushButton::clicked, this, &CameraControllerBar::captureClicked);

    connect(m_recordingButton, &QPushButton::clicked, this, &CameraControllerBar::onRecordClicked);

    connect(m_exposureSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CameraControllerBar::exposureChanged);

    connect(m_gainSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CameraControllerBar::gainChanged);

    connect(m_lutPopupWindow, &LutPopupWindow::visibilityChanged,
            this, &CameraControllerBar::requestHistogram);

    // LUT按钮点击时显示LUT编辑器
    connect(m_lutButton, &QPushButton::clicked, this, [this]()
            {
                int popupWidth = 360;
                int popupHeight = 200;

                m_lutPopupWindow->resize(popupWidth, popupHeight);
                m_lutPopupWindow->move(mapToGlobal(m_lutButton->pos() - QPoint(0, popupHeight + 40)));
                m_lutPopupWindow->show();
                // m_lutPopupWindow->activateWindow();
            });

    // 对 m_lutPopupWindow的 lutChanged 信号进转发
    connect(m_lutPopupWindow, &LutPopupWindow::lutChanged, this, [this](double min, double max, double gamma)
            { emit lutChanged(min, max, gamma); });
}

void CameraControllerBar::onFPSUpdated(double fps)
{
    m_fpsLabel->setText(QString::number(fps, 'f', 1) + " FPS");
}

void CameraControllerBar::onCameraStatusChanged(QString status, QString value)
{
    if (status == "open")
    {
        if (value == "true")
        {
            m_isConnected = true;
            m_connectButton->setIcon(QIcon(":/icons8_link.svg"));
        }
        else if (value == "false")
        {
            m_isConnected = false;
            m_connectButton->setIcon(QIcon(":/icons8_broken_link.svg"));
        }

        // 根据连接状态启用/禁用其他控件
        m_streamButton->setEnabled(m_isConnected);
        m_captureButton->setEnabled(m_isConnected);
        m_recordingButton->setEnabled(m_isConnected);
        m_lutButton->setEnabled(m_isConnected);
        m_exposureSpinBox->setEnabled(m_isConnected);
        m_gainSpinBox->setEnabled(m_isConnected);
    }

    if (status == "stream")
    {
        if (value == "true")
        {
            m_isStreaming = true;
            m_streamButton->setIcon(QIcon(":/icons8_stop.svg"));
        }
        else if (value == "false")
        {
            m_isStreaming = false;
            m_streamButton->setIcon(QIcon(":/icons8_play.svg"));
        }
    }

    if (status == "exposure")
    {
        m_exposureSpinBox->setValue(value.toInt());
    }

    // gain
    if (status == "gain")
    {
        m_gainSpinBox->setValue(value.toInt());
    }
}

void CameraControllerBar::onHistogramUpdated(const std::vector<int> &histogram, int maxValue)
{
    m_lutPopupWindow->updateHistogram(histogram, maxValue);
}

void CameraControllerBar::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    // painter.setRenderHint(QPainter::Antialiasing);

    // 设置画笔
    QPen pen(QColor(50, 55, 55));
    pen.setWidth(1);
    painter.setPen(pen);

    // 绘制边框
    painter.drawRect(rect().adjusted(0, 1, 0, 1));
}

LutPopupWindow::LutPopupWindow(QWidget *parent)
    : QWidget(parent), m_isDragging(false)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::ToolTip);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_ShowWithoutActivating); // 显示时不激活窗口

    setFocusPolicy(Qt::StrongFocus);

    QVBoxLayout *layout = new QVBoxLayout(this);

    // 创建一个按钮，用于固定窗口一直在最前面
    m_pinButton = new QPushButton(this);
    m_pinButton->setIcon(QIcon(":/icons8_pin.svg")); // 确保你有这个图标资源
    m_pinButton->setFixedSize(16, 16);
    m_pinButton->setIconSize(QSize(12, 12));
    m_pinButton->setCheckable(true);
    layout->addWidget(m_pinButton);

    // LUT 显示控件
    m_mappingWidget = new GrayMappingWidget(this);
    layout->addWidget(m_mappingWidget);
    setLayout(layout);

    m_dragPosition = QPoint();

    qApp->installEventFilter(this);

    // 对 m_mappingWidget的 lutChanged 信号进转发
    connect(m_mappingWidget, &GrayMappingWidget::lutChanged, this, [this](double min, double max, double gamma)
            { emit lutChanged(min, max, gamma); });
}

void LutPopupWindow::updateHistogram(const std::vector<int> &histogram, int maxValue)
{
    m_mappingWidget->setHistogram(histogram, maxValue);
}

void LutPopupWindow::mousePressEvent(QMouseEvent *event)
{
}

void LutPopupWindow::mouseMoveEvent(QMouseEvent *event)
{
}

void LutPopupWindow::mouseReleaseEvent(QMouseEvent *event)
{
}

void LutPopupWindow::focusOutEvent(QFocusEvent *event)
{
    if (!m_pinButton->isChecked())
        hide();
    QWidget::focusOutEvent(event);
}

void LutPopupWindow::hideEvent(QHideEvent *event)
{
    qApp->removeEventFilter(this);

    emit visibilityChanged(false); // 隐藏时发出信号

    QWidget::hideEvent(event);
}

void LutPopupWindow::showEvent(QShowEvent *event)
{
    qApp->installEventFilter(this);

    emit visibilityChanged(true); // 显示时发出信号

    QWidget::showEvent(event);
}

// 事件过滤器，用于点击窗口外部时隐藏窗口
bool LutPopupWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (!this->geometry().contains(mouseEvent->globalPosition().toPoint()))
        {
            if (!m_pinButton->isChecked())
            {
                hide();
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void LutPopupWindow::paintEvent(QPaintEvent *event)
{
    // 绘制一个边框
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(QColor(255, 255, 255, 100));
    pen.setWidth(.2);
    painter.setPen(pen);
    painter.drawRect(rect().adjusted(1, 1, -1, -1));
    QWidget::paintEvent(event);
}
