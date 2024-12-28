#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QLabel>
#include <QSplitter>
#include <QPushButton>
#include <QVBoxLayout>
#include <QAction>

#include "framerenderer.hpp"

#include "devicefinderdialog.hpp"
#include "MaskWindow.hpp"
#include "UserControlArea.hpp"

#include "logwidget.hpp"

#include "GLSLEditor.hpp"

#include "MultiWindowManager.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

    void closeEvent(QCloseEvent *event) override;

private slots:
    void toggleDeviceFinder();
    void toggleMaskWindow();
    void openSettings();
    void openGLSLEditor();

private:
    QSplitter *splitter;
    MultiWindowManager *multiWindowManager;
    QWidget *leftPanel;
    QVBoxLayout *leftLayout;
    QWidget *rightPanel;
    QVBoxLayout *rightLayout;
    QAction *deviceFinderAction;
    DeviceFinderDialog *deviceFinderDialog;
    LogWidget *logWidget;
    QAction *logWidgetAction;
    QAction *maskWindowToggleAction;
    UserControlArea *userControlArea;
    QAction *settingsAction;
    QAction *glslEditorAction;
    GLSLEditor *glslEditor = nullptr;
};

#endif // MAINWINDOW_HPP
