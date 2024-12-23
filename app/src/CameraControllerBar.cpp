#include "CameraControllerBar.h"
#include <QIcon>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPainter>

CameraControllerBar::CameraControllerBar(QWidget *parent)
    : QWidget(parent), m_isConnected(false), m_isStreaming(false)
{
    setupUI();
    createConnections();
}

CameraControllerBar::~CameraControllerBar()
{
}

void CameraControllerBar::setupUI()
{
    // 创建主布局
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(4, 0, 4, 0);
    m_layout->setSpacing(2);

    // 创建按钮
    m_connectButton = createButton(":/icons8_broken_link.svg", tr("Connect to camera"));
    m_streamButton = createButton(":/icons8_play.svg", tr("Start/Stop stream"));
    m_captureButton = createButton(":/icons8_unsplash.svg", tr("Capture frame"));
    m_recordingButton = createButton(":/icons8_record.svg", tr("Start/Stop recording"));
    m_recordingButton->setCheckable(true);
    m_recordingButton->setChecked(true);
    m_exposureSpinBox = new QSpinBox(this);
    m_exposureSpinBox->setPrefix("曝光: ");
    m_exposureSpinBox->setRange(0, 1000000);
    m_exposureSpinBox->setSingleStep(1);
    m_exposureSpinBox->setSuffix(" μs");
    m_exposureSpinBox->setFixedWidth(120);
    // m_exposureSpinBox->setFocusPolicy(Qt::NoFocus);
    m_exposureSpinBox->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_gainSpinBox = new QSpinBox(this);
    m_gainSpinBox->setPrefix("增益: ");
    m_gainSpinBox->setRange(0, 100);
    m_gainSpinBox->setSingleStep(1);
    m_gainSpinBox->setFixedWidth(80);
    // m_gainSpinBox->setFocusPolicy(Qt::NoFocus);
    m_gainSpinBox->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // 创建FPS标签
    m_fpsLabel = new QLabel("0 FPS");
    m_fpsLabel->setMinimumWidth(60);
    m_fpsLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // 添加到布局
    m_layout->addWidget(m_connectButton);
    m_layout->addWidget(m_streamButton);
    m_layout->addWidget(m_captureButton);
    m_layout->addWidget(m_recordingButton);
    m_layout->addWidget(m_exposureSpinBox);
    m_layout->addWidget(m_gainSpinBox);
    m_layout->addWidget(m_fpsLabel);

    // 初始状态设置
    m_streamButton->setEnabled(false);
    m_captureButton->setEnabled(false);

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
    button->setIconSize(QSize(15, 15));
    button->setFocusPolicy(Qt::NoFocus);
    return button;
}

void CameraControllerBar::createConnections()
{
    connect(m_connectButton, &QPushButton::clicked, this, [this]()
            {
        m_isConnected = !m_isConnected;
        m_connectButton->setIcon(QIcon(m_isConnected ? ":/icons8_brokern_link.svg" : ":/icons8_link.svg"));
        m_streamButton->setEnabled(m_isConnected);
        emit connectClicked(); });

    connect(m_streamButton, &QPushButton::clicked, this, [this]()
            {
        m_isStreaming = !m_isStreaming;
        m_streamButton->setIcon(QIcon(m_isStreaming ? ":/icons/stop.svg" : ":/icons/stream.svg"));
        m_captureButton->setEnabled(m_isStreaming);
        emit streamClicked(); });

    connect(m_captureButton, &QPushButton::clicked, this, &CameraControllerBar::captureClicked);
}

void CameraControllerBar::setFPS(double fps)
{
    m_fpsLabel->setText(QString::number(fps, 'f', 1) + " FPS");
}

void CameraControllerBar::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 设置画笔
    QPen pen(QColor(255, 255, 255, 22));
    pen.setWidth(.5);
    painter.setPen(pen);

    // 绘制边框
    painter.drawRect(rect().adjusted(0, 1, 0, -1));
}