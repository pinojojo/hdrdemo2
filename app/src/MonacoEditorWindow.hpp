#ifndef MONACOEDITORWINDOW_HPP
#define MONACOEDITORWINDOW_HPP

#include <QMainWindow>
#include <QtWebEngineWidgets/QWebEngineView>
#include <QVBoxLayout>

class MonacoEditorWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MonacoEditorWindow(QWidget *parent = nullptr);
    ~MonacoEditorWindow() = default;

private:
    QWebEngineView *webView;
    void initializeMonaco();
};

#endif // MONACOEDITORWINDOW_HPP