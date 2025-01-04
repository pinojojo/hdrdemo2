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
#include <QScrollBar>
#include <QPushButton>
#include <QString>

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
        instance().addLog({timestamp + " info -> " + text, Info});
    }
    static void addWarnLog(const QString &text)
    {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
        instance().addLog({timestamp + " warn -> " + text, Warn});
    }
    static void addErrorLog(const QString &text)
    {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
        instance().addLog({timestamp + " error-> " + text, Error});
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

class LogListView : public QListView
{
    Q_OBJECT

public:
    explicit LogListView(QWidget *parent = nullptr) : QListView(parent) {}

signals:
    void wheelScrolled();

protected:
    void wheelEvent(QWheelEvent *event) override
    {
        QListView::wheelEvent(event);
        emit wheelScrolled();
    }
};

class LogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogWidget(QWidget *parent = nullptr);

private slots:
    void scrollToBottom()
    {
        if (m_autoScroll)
            view->scrollToBottom();
    }

    void onWheelScrolled()
    {
        // 当用户滚动时，暂时禁用自动滚动
        m_autoScroll = false;
        autoScrollButton->setChecked(false);
    }

private:
    LogListView *view;
    LogListModel *model;
    QTimer *timer;
    bool m_autoScroll = true;
    QPushButton *autoScrollButton;
};

#endif // LOGWIDGET_HPP