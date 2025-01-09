#include "mainwindow.hpp"

#include <QMenu>
#include <QMenuBar>

#include "SettingsDialog.hpp"

#include "Common.h"
#include "Global.hpp"
#include "CameraViewPanel.h"

#include "Settings.hpp"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      deviceFinderDialog(new DeviceFinderDialog(this)),
      deviceFinderAction(new QAction(tr("查询相机"), this)),

      logWidget(new LogWidget(this)),
      userControlArea(new UserControlArea(this))
{
    // Set window title
    setWindowTitle("高动态视觉测量系统");

    // 添加action到菜单
    QMenu *menu = menuBar()->addMenu(tr("工具"));
    menu->addAction(deviceFinderAction);

    // 全局设置菜单
    settingsAction = new QAction(tr("系统设置"), this);
    settingsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma)); // Ctrl+,
    settingsAction->setShortcutContext(Qt::ApplicationShortcut);
    settingsAction->setStatusTip(tr("打开系统设置"));

    openSaveFolderAction = new QAction(tr("打开保存文件夹"), this);
    openSaveFolderAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O)); // Ctrl+Shift+O
    openSaveFolderAction->setStatusTip(tr("打开默认保存文件夹"));
    menu->addAction(openSaveFolderAction);
    connect(openSaveFolderAction, &QAction::triggered, this, &MainWindow::openSaveFolder);

    // GLSL编辑器菜单
    glslEditorAction = new QAction(tr("GLSL编辑器"), this);
    glslEditorAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G)); // Ctrl+G
    glslEditorAction->setStatusTip(tr("打开GLSL编辑器"));
    menu->addAction(glslEditorAction);
    connect(glslEditorAction, &QAction::triggered, this, &MainWindow::openGLSLEditor);

    menu->addAction(settingsAction);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettings);

    // 连接action的触发信号到槽函数
    connect(deviceFinderAction, &QAction::triggered, this, &MainWindow::toggleDeviceFinder);
    deviceFinderDialog->hide(); // Initially hidden

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

    if (0)
    {

        multiWindowManager->addWindow(new CameraViewPanel("MVS", rightPanel, true));
        multiWindowManager->addWindow(new CameraViewPanel("PlayerOne", rightPanel));
    }
    else
    {
        multiWindowManager->addWindow(new CameraViewPanel("test8", rightPanel));
        multiWindowManager->addWindow(new CameraViewPanel("test16", rightPanel));
    }

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

void MainWindow::openSaveFolder()
{
    QString path = Settings::getInstance().getDefaultSavePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
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

void MainWindow::openSettings()
{

    SettingsDialog dialog(this);
    dialog.exec();
}

void MainWindow::openGLSLEditor()
{
    if (!glslEditor)
    {
        glslEditor = new GLSLEditor(nullptr); // 使用nullptr作为父对象使其成为独立窗口
        glslEditor->setAttribute(Qt::WA_DeleteOnClose);
    }
    glslEditor->show();
    glslEditor->raise();
    glslEditor->activateWindow();
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