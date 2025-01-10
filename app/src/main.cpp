#include <QApplication>
#include <QWidget>
#include <QScreen>
#include <QMessageBox>
#include <QThread>

#include <oclero/qlementine/style/QlementineStyle.hpp>
#include <oclero/qlementine/utils/WidgetUtils.hpp>

#include "mainwindow.hpp"

#include "GrayMappingWidget.h"
#include "MultiWindowManager.h"
#include "CameraControllerBar.h"

#include "NeonButton.h"

#define USE_QLEMENTINE_STYLE

// #define TEST_MY_WIDGET

int main(int argc, char *argv[])
{

    QApplication qApplication(argc, argv);

#ifdef USE_QLEMENTINE_STYLE
    auto *const style = new oclero::qlementine::QlementineStyle(&qApplication);
    style->setAnimationsEnabled(true);

    style->setThemeJsonPath(QStringLiteral(":/themes/dark.json"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons8_view_module.ico")));
    qApplication.setStyle(style);
#endif

    auto window = std::make_unique<MainWindow>();
    window->setMinimumSize(1400, 800);
    window->show();

#ifdef TEST_MY_WIDGET
    // 创建主窗口
    MultiWindowManager multi;

    auto mappingWidget = new GrayMappingWidget();
    multi.addWindow(mappingWidget);

    QLabel *label1 = new QLabel("Another Window");
    multi.addWindow(label1);

    CameraControllerBar *cameraControllerBar = new CameraControllerBar();
    multi.addWindow(cameraControllerBar);

    NeonButton *neonButton = new NeonButton();
    neonButton->setText("Neon Button");
    neonButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    neonButton->startGlowing();
    multi.addWindow(neonButton);

    multi.resize(800, 600);
    multi.show();
#endif

    return qApplication.exec();
}