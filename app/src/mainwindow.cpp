#include "mainwindow.hpp"

#include <QMenu>
#include <QMenuBar>

#include "Common.h"
#include "Global.hpp"
#include "CameraViewPanel.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      deviceFinderDialog(new DeviceFinderDialog(this)),
      deviceFinderAction(new QAction(tr("查询相机"), this)),
      maskWindowToggleAction(new QAction(tr("打开Mask窗口"), this)),
      logWidget(new LogWidget(this)),
      userControlArea(new UserControlArea(this))
{
    // Set window title
    setWindowTitle("高动态视觉测量系统");

    // 添加action到菜单
    QMenu *menu = menuBar()->addMenu(tr("工具"));
    menu->addAction(deviceFinderAction);
    maskWindowToggleAction->setCheckable(true);
    menu->addAction(maskWindowToggleAction);
    // 连接action的触发信号到槽函数
    connect(deviceFinderAction, &QAction::triggered, this, &MainWindow::toggleDeviceFinder);
    deviceFinderDialog->hide(); // Initially hidden
    connect(maskWindowToggleAction, &QAction::triggered, this, &MainWindow::toggleMaskWindow);

    // 创建 Splitter
    splitter = new QSplitter(this);

    // 创建左侧面板和布局
    leftPanel = new QWidget();
    leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->addWidget(userControlArea);
    leftLayout->addStretch();
    leftLayout->setContentsMargins(0, 0, 0, 1);
    leftPanel->setMaximumWidth(270);
    splitter->addWidget(leftPanel);

    // 右侧面板
    rightPanel = new QWidget();
    rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    splitter->addWidget(rightPanel);

    multiWindowManager = new MultiWindowManager(rightPanel);
    multiWindowManager->addWindow(new CameraViewPanel("hiki", rightPanel));
    multiWindowManager->addWindow(new CameraViewPanel("test", rightPanel));

    // 创建一个垂直的 QSplitter，并添加 FrameRenderer 和 LogWidget
    QSplitter *rightSplitter = new QSplitter(Qt::Vertical, rightPanel);
    rightSplitter->addWidget(multiWindowManager);
    rightSplitter->addWidget(logWidget);
    rightSplitter->setStretchFactor(0, 1);
    rightSplitter->setStretchFactor(1, 0);

    // 将 QSplitter 添加到右侧面板的布局
    rightLayout->addWidget(rightSplitter);

    // connections
    connect(userControlArea, &UserControlArea::maskWindowPropertyChanged, GlobalResourceManager::getInstance().maskWindow, &MaskWindow::onPropertyChanged);

    // 将 Splitter 设置为主窗口的中心部件
    setCentralWidget(splitter);
}

void MainWindow::closeEvent(QCloseEvent *event)
{

    // 关闭Mask窗口
    if (GlobalResourceManager::getInstance().maskWindow != nullptr)
    {
        GlobalResourceManager::getInstance().maskWindow->close();
    }

    // 停止相机
    if (GlobalResourceManager::getInstance().camera != nullptr)
    {
        GlobalResourceManager::getInstance().camera->stop();
    }
}

void MainWindow::toggleMaskWindow()
{
    if (maskWindowToggleAction->isChecked())
    {
        GlobalResourceManager::getInstance().maskWindow->show();
    }
    else
    {
        GlobalResourceManager::getInstance().maskWindow->hide();
    }
}

void MainWindow::toggleDeviceFinder()
{
    if (deviceFinderDialog->isVisible())
    {
        deviceFinderDialog->hide();
    }
    else
    {
        deviceFinderDialog->show();
    }
}