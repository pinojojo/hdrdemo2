// logwidget.hpp
#ifndef LOGWIDGET_HPP
#define LOGWIDGET_HPP

#include <QAbstractListModel>
#include <QListView>
#include <QVBoxLayout>
#include <QStringList>
#include <QDateTime>
#include <QMutex>
#include <QDebug>
#include <QTimer>
#include <string>

enum LogType
{
    Info,
    Warn,
    Error
};

struct LogEntry
{
    QString text;
    LogType type;
};

class LogListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    static LogListModel &instance()
    {
        static LogListModel instance;
        return instance;
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void addLog(const LogEntry &log);

    static void addInfoLog(const QString &text)
    {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
        instance().addLog({timestamp + " [INFO] : " + text, Info});
    }
    static void addWarnLog(const QString &text)
    {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
        instance().addLog({timestamp + " [WARN] : " + text, Warn});
    }
    static void addErrorLog(const QString &text)
    {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
        instance().addLog({timestamp + " [ERROR] : " + text, Error});
    }

private:
    LogListModel(QObject *parent = nullptr);
    LogListModel(const LogListModel &) = delete;
    LogListModel &operator=(const LogListModel &) = delete;

    QList<LogEntry> logs;
    mutable QMutex mutex;
};

namespace Log
{
    // clang-format off
    inline void info(const QString &text) { LogListModel::addInfoLog(text);}
    inline void warn(const QString &text) { LogListModel::addWarnLog(text);}
    inline void error(const QString &text) { LogListModel::addErrorLog(text); }
    // clang-format on
};

class LogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogWidget(QWidget *parent = nullptr);

private slots:
    void scrollToBottom()
    {
        // 假设你的 view 变量名为 view
        view->scrollToBottom();
    }

private:
    QListView *view;
    LogListModel *model;
    QTimer *timer;
};

#endif // LOGWIDGET_HPP