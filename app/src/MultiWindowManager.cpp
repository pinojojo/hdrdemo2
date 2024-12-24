// multiwindowmanager.cpp
#include "MultiWindowManager.h"
#include <cmath>

#include <QPainter>

WindowContainer::WindowContainer(QWidget *content, QWidget *parent)
    : QWidget(parent), m_content(nullptr), m_layout(new QVBoxLayout(this))
{
    m_layout->setContentsMargins(1, 1, 1, 1);

    if (content)
    {
        setContent(content);
    }
}

void WindowContainer::setContent(QWidget *widget)
{
    if (m_content)
    {
        m_layout->removeWidget(m_content);
        m_content->setParent(nullptr);
    }

    m_content = widget;
    if (m_content)
    {
        m_layout->addWidget(m_content);
    }
}

void WindowContainer::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 设置画笔
    QPen pen(QColor(255, 255, 255, 50));
    pen.setWidth(.5);
    painter.setPen(pen);

    // 绘制边框
    // painter.drawRect(rect().adjusted(1, 1, -1, -1));
    painter.drawRect(rect().adjusted(0, 0, 0, 0));
}

void WindowContainer::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        emit doubleClicked();
    }
    QWidget::mouseDoubleClickEvent(event);
}

MultiWindowManager::MultiWindowManager(QWidget *parent)
    : QWidget(parent), m_layout(new QGridLayout(this)), m_maximizedWindow(nullptr)
{
    m_layout->setSpacing(0);
    m_layout->setContentsMargins(0, 0, 0, 0);
}

void MultiWindowManager::addWindow(QWidget *widget)
{
    WindowContainer *container = new WindowContainer(widget, this);
    connect(container, &WindowContainer::doubleClicked,
            this, &MultiWindowManager::handleWindowDoubleClick);

    m_windows.append(container);
    updateLayout();
}

void MultiWindowManager::removeWindow(QWidget *widget)
{
    for (int i = 0; i < m_windows.size(); ++i)
    {
        if (m_windows[i]->content() == widget)
        {
            if (m_maximizedWindow == m_windows[i])
            {
                m_maximizedWindow = nullptr;
            }
            delete m_windows.takeAt(i);
            updateLayout();
            break;
        }
    }
}

void MultiWindowManager::clearWindows()
{
    qDeleteAll(m_windows);
    m_windows.clear();
    m_maximizedWindow = nullptr;
    updateLayout();
}

int MultiWindowManager::windowCount() const
{
    return m_windows.size();
}

void MultiWindowManager::handleWindowDoubleClick()
{
    WindowContainer *container = qobject_cast<WindowContainer *>(sender());
    if (!container)
        return;

    if (m_maximizedWindow == container)
    {
        switchToGrid();
    }
    else
    {
        switchToMaximized(container);
    }
}

void MultiWindowManager::switchToMaximized(WindowContainer *window)
{
    // 隐藏其他所有窗口
    for (WindowContainer *w : m_windows)
    {
        w->hide();
    }

    window->show();
    m_maximizedWindow = window;
    m_layout->addWidget(window, 0, 0);
}

void MultiWindowManager::switchToGrid()
{
    // 显示所有窗口
    for (WindowContainer *w : m_windows)
    {
        w->show();
    }

    m_maximizedWindow = nullptr;
    updateLayout();
}

void MultiWindowManager::calculateGrid(int &rows, int &cols) const
{
    int count = m_windows.size();
    cols = ceil(sqrt(count));
    rows = ceil(static_cast<double>(count) / cols);
}

void MultiWindowManager::updateLayout()
{
    // 清除现有布局
    QLayoutItem *item;
    while ((item = m_layout->takeAt(0)) != nullptr)
    {
        delete item;
    }

    if (m_maximizedWindow)
    {
        m_layout->addWidget(m_maximizedWindow, 0, 0);
        return;
    }

    int rows, cols;
    calculateGrid(rows, cols);

    int index = 0;
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            if (index < m_windows.size())
            {
                m_layout->addWidget(m_windows[index], row, col);
                index++;
            }
        }
    }
}
