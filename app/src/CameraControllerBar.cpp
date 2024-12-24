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

    m_lutPopupWindow = new LutPopupWindow(this);

    // 创建主布局
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(4, 0, 4, 0);
    m_layout->setSpacing(2);

    // 创建按钮
    m_connectButton = createButton(":/icons8_broken_link.svg", tr("Connect to camera"));
    m_streamButton = createButton(":/icons8_play.svg", tr("Start/Stop stream"));
    m_captureButton = createButton(":/icons8_unsplash.svg", tr("Capture frame"));
    m_recordingButton = createButton(":/icons8_record.svg", tr("Start/Stop recording"));
    m_lutButton = createButton(":/icons8_histogram.svg", tr("Open LUT editor"));
    m_recordingButton->setCheckable(true);
    m_recordingButton->setChecked(true);
    m_exposureSpinBox = new QSpinBox(this);
    m_exposureSpinBox->setPrefix("Exp: ");
    m_exposureSpinBox->setRange(0, 1000000);
    m_exposureSpinBox->setSingleStep(1);
    m_exposureSpinBox->setSuffix(" μs");
    m_exposureSpinBox->setFixedWidth(90);
    m_exposureSpinBox->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_exposureSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);

    m_gainSpinBox = new QSpinBox(this);
    m_gainSpinBox->setPrefix("Gain: ");
    m_gainSpinBox->setRange(0, 100);
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
        
        emit connectClicked(); });

    connect(m_streamButton, &QPushButton::clicked, this, [this]()
            {
        m_isStreaming = !m_isStreaming;
        m_streamButton->setIcon(QIcon(m_isStreaming ? ":/icons/stop.svg" : ":/icons/stream.svg"));
        emit streamClicked(); });

    connect(m_captureButton, &QPushButton::clicked, this, &CameraControllerBar::captureClicked);

    connect(m_exposureSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CameraControllerBar::exposureChanged);

    connect(m_gainSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CameraControllerBar::gainChanged);

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

LutPopupWindow::LutPopupWindow(QWidget *parent)
    : QWidget(parent), m_isDragging(false)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::ToolTip);
    // setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_ShowWithoutActivating); // 显示时不激活窗口

    setFocusPolicy(Qt::StrongFocus);

    // setMouseTracking(true);

    m_mappingWidget = new GrayMappingWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_mappingWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setLayout(layout);

    m_dragPosition = QPoint();

    qApp->installEventFilter(this);
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
    hide();
    QWidget::focusOutEvent(event);
}

void LutPopupWindow::hideEvent(QHideEvent *event)
{
    qApp->removeEventFilter(this);
    QWidget::hideEvent(event);
}

void LutPopupWindow::showEvent(QShowEvent *event)
{
    qApp->installEventFilter(this);
    QWidget::showEvent(event);
}

bool LutPopupWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (!this->geometry().contains(mouseEvent->globalPosition().toPoint()))
        {
            hide();
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
