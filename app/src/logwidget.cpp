#include "logwidget.hpp"
#include <QPushButton>

int LogListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return logs.size();
}

QVariant LogListModel::data(const QModelIndex &index, int role) const
{
    QMutexLocker locker(&mutex);

    if (!index.isValid())
        return QVariant();

    if (index.row() >= logs.size())
        return QVariant();

    if (role == Qt::DisplayRole)
        return logs[index.row()].text;
    else if (role == Qt::ForegroundRole)
    {
        switch (logs[index.row()].type)
        {
        case Info:
            return QBrush(Qt::white);
        case Warn:
            return QBrush(Qt::yellow);
        case Error:
            return QBrush(Qt::red);
        default:
            return QBrush(Qt::white);
        }
    }

    return QVariant();
}

void LogListModel::addLog(const LogEntry &log)
{
    QMutexLocker locker(&mutex);

    beginInsertRows(QModelIndex(), logs.size(), logs.size());
    logs.append(log);
    endInsertRows();
}

LogListModel::LogListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

LogWidget::LogWidget(QWidget *parent)
    : QWidget(parent),
      model(&LogListModel::instance()),
      timer(new QTimer(this))
{

    // 创建自动滚动控制按钮
    autoScrollButton = new QPushButton(this);
    autoScrollButton->setCheckable(true); // 使按钮可切换
    autoScrollButton->setChecked(true);   // 默认开启
    autoScrollButton->setIcon(QIcon(":/icons8_lock.svg"));
    autoScrollButton->setToolTip("Auto scroll");
    autoScrollButton->setFixedSize(20, 20);
    autoScrollButton->setIconSize(QSize(14, 14));
    autoScrollButton->setFocusPolicy(Qt::NoFocus);

    // 创建水平布局来放置按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(); // 添加弹性空间，使按钮靠右
    buttonLayout->setContentsMargins(2, 2, 2, 0);
    buttonLayout->addWidget(autoScrollButton);

    view = new LogListView(this);
    view->setModel(model);

    QFont font("Consolas", 9);
    font.setFixedPitch(true); // 确保使用等宽字体
    view->setFont(font);
    view->setSpacing(0);

    // 修改主布局为垂直布局
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(buttonLayout); // 添加按钮布局
    layout->addWidget(view);
    layout->setContentsMargins(0, 1, 0, 0);

    // 添加一条日志，记录 LogWidget 的构造时间
    QDateTime currentTime = QDateTime::currentDateTime();
    QString log = QString("%1").arg(currentTime.toString());
    qDebug() << log;

    Log::info(log);

    // 连接滚轮滚动信号到槽
    connect(view, &LogListView::wheelScrolled, this, &LogWidget::onWheelScrolled);

    // 连接按钮信号到槽
    connect(autoScrollButton, &QPushButton::toggled, this, [this](bool checked)
            {
                m_autoScroll = checked;
                if (checked)
                {
                    scrollToBottom(); // 如果重新启用自动滚动，立即滚动到底部
                }

                autoScrollButton->setIcon(checked ? QIcon(":/icons8_lock.svg") : QIcon(":/icons8_unlock.svg")); });

    connect(timer, &QTimer::timeout, this, &LogWidget::scrollToBottom);
    timer->start(300);
}