// multiwindowmanager.h
#ifndef MULTIWINDOWMANAGER_H
#define MULTIWINDOWMANAGER_H

#include <QWidget>
#include <QGridLayout>
#include <QVector>
#include <QMouseEvent>

class WindowContainer : public QWidget
{
    Q_OBJECT
public:
    explicit WindowContainer(QWidget *content = nullptr, QWidget *parent = nullptr);
    void setContent(QWidget *widget);
    QWidget *content() const { return m_content; }

signals:
    void doubleClicked();

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QWidget *m_content;
    QVBoxLayout *m_layout;
};

class MultiWindowManager : public QWidget
{
    Q_OBJECT
public:
    explicit MultiWindowManager(QWidget *parent = nullptr);

    // 添加一个新的窗口
    void addWindow(QWidget *widget);
    // 移除指定窗口
    void removeWindow(QWidget *widget);
    // 清除所有窗口
    void clearWindows();
    // 获取当前窗口数量
    int windowCount() const;

private slots:
    void handleWindowDoubleClick();
    void updateLayout();

private:
    QGridLayout *m_layout;
    QVector<WindowContainer *> m_windows;
    WindowContainer *m_maximizedWindow;

    void switchToMaximized(WindowContainer *window);
    void switchToGrid();
    void calculateGrid(int &rows, int &cols) const;
};

#endif // MULTIWINDOWMANAGER_H
