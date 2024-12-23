#include "logwidget.hpp"

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

    view = new QListView(this);
    view->setModel(model);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(view);
    layout->setContentsMargins(0, 1, 0, 0);

    // 添加一条日志，记录 LogWidget 的构造时间
    QDateTime currentTime = QDateTime::currentDateTime();
    QString log = QString("%1").arg(currentTime.toString());
    qDebug() << log;

    Log::info(log);

    // 每隔 100 毫秒，滚动到底部
    connect(timer, &QTimer::timeout, this, &LogWidget::scrollToBottom);
    timer->start(100);
}