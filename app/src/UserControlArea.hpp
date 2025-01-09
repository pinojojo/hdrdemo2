#pragma once

#include <QBoxLayout>

#include <QtWidgets>

#include "Utils.hpp"

#include "Global.hpp"
#include "logwidget.hpp"

#include "Common.h"
#include "USBCamera.hpp"
class MaskMouseDrawModeControl : public QWidget
{
    Q_OBJECT

public:
    // ctor
    MaskMouseDrawModeControl(QWidget *parent = nullptr) : QWidget(parent)
    {
        QVBoxLayout *vbox = new QVBoxLayout(this);
        QHBoxLayout *hbox1 = new QHBoxLayout();

        QSpinBox *globalIntensitySpinBox = new QSpinBox();
        drawButton = new QPushButton("绘制调光区域");
        drawButton->setCheckable(true);
        drawButton->setChecked(false);
        drawButton->setIcon(QIcon::fromTheme(":/icons8_radar_plot_2.svg"));
        clearButton = new QPushButton("清除调光区域");
        hbox1->addWidget(drawButton, 1);
        hbox1->addWidget(clearButton, 1);

        // 绘制局部调光
        connect(drawButton, &QPushButton::clicked, [this, globalIntensitySpinBox]
                {
                    FrameRenderer* ref= GlobalResourceManager::getInstance().getRefFrameRenderer();

                    if(ref == nullptr)
                    {
                        Log::warn("No reference frame renderer");
                        return;
                    }


                    if (drawButton->isChecked())
                    {
                        // 开始绘制
                        drawButton->setChecked(true);
                        drawButton->setText("完成绘制");
                        ref->enterDrawMode(true);
                        emit drawMode(true);
                    }
                    else
                    {
                        // 完成绘制,从参考FrameRenderer中获取Mask数据
                        MaskData maskData;
                        maskData.continuousMode = false;
                        ref->enterDrawMode(false);
                        maskData.polygons = ref->getMaskPolygons();
                        maskData.globalBackgroundIntensity = globalIntensitySpinBox->value() / 255.0f;
                        GlobalResourceManager::getInstance().maskWindow->onMaskDataChanged(maskData); 
                        
                        drawButton->setChecked(false);
                        drawButton->setText("绘制局部调光");
                        emit drawMode(false);
                    } });

        // 全局调光值
        connect(clearButton, &QPushButton::clicked, [this, globalIntensitySpinBox]
                { 
                    
                    // 产生一个MaskData数据并交给MaskWindow处理
                    MaskData maskData;
                    maskData.continuousMode = false;
                    maskData.polygons = {};
                    maskData.globalBackgroundIntensity = globalIntensitySpinBox->value() / 255.0f;
                    GlobalResourceManager::getInstance().maskWindow->onMaskDataChanged(maskData);

                    FrameRenderer* ref= GlobalResourceManager::getInstance().getRefFrameRenderer();
                    if(ref != nullptr)
                    {
                        ref->clearMask();
                    }

                    emit clearButtonClicked(); });

        vbox->addLayout(hbox1);

        QHBoxLayout *hbox2 = new QHBoxLayout();
        hbox2->setContentsMargins(0, 0, 0, 0);
        QLabel *label = new QLabel("全局调光值");
        globalIntensitySpinBox->setRange(0, 255);
        hbox2->addWidget(label, 1);
        hbox2->addWidget(globalIntensitySpinBox, 1);
        vbox->addLayout(hbox2);
        //  globalIntensitySpinBox 行为,值发生变化时
        connect(globalIntensitySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this, globalIntensitySpinBox]
                {
                    MaskData maskData;
                    maskData.continuousMode = false;
                    //maskData.polygons = GlobalResourceManager::getInstance().frameRenderer->getMaskPolygons();
                    maskData.globalBackgroundIntensity = globalIntensitySpinBox->value() / 255.0f;
                    GlobalResourceManager::getInstance().maskWindow->onMaskDataChanged(maskData); });

        vbox->addStretch(1);

        setLayout(vbox);
    }

signals:
    void drawMode(bool enabled);
    void clearButtonClicked();

private:
    QPushButton *drawButton;  // 绘制
    QPushButton *clearButton; // 清除
    QPushButton *applyMask;   // 应用Mask
};

class UserControlArea : public QWidget
{
    Q_OBJECT

public:
    UserControlArea(QWidget *parent = nullptr)
        : QWidget(parent),
          frameRateQueryTimer(new QTimer(this))
    {
        QVBoxLayout *vbox = new QVBoxLayout(this);

        // DMD工作模式
        DMDWorkModeComboBox = new QComboBox();
        DMDWorkModeComboBox->addItem("正常模式");
        DMDWorkModeComboBox->addItem("编码模式");
        addRow(vbox, "DMD工作模式", DMDWorkModeComboBox, true);
        connect(DMDWorkModeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index)
                {
                    // Mask宽度和高度设为不可编辑
                    maskWindowWidthSpinBox->setEnabled(index == 0);
                    maskWindowHeightSpinBox->setEnabled(index == 0);

                    GlobalResourceManager::getInstance().maskWindow->onDMDWorkModeChanged(DMDWorkMode(index)); });

        // 显示掩膜窗口
        showMaskWindowButton = new QPushButton("显示Mask窗口");
        showMaskWindowButton->setCheckable(true);
        addRow(vbox, "Mask窗口", showMaskWindowButton, true);
        connect(showMaskWindowButton, &QPushButton::clicked, [this]
                {
                    if (showMaskWindowButton->isChecked())
                    {
                        maskWindowProperty.visible = true;
                        emit maskWindowPropertyChanged(maskWindowProperty);
                        showMaskWindowButton->setText("隐藏Mask窗口");
                    }
                    else
                    {
                        maskWindowProperty.visible = false;
                        emit maskWindowPropertyChanged(maskWindowProperty);
                        showMaskWindowButton->setText("显示Mask窗口");
                    } });
        // 两个按钮（置于1920 置于0） 并排 等分宽度
        {
            QHBoxLayout *hbox = new QHBoxLayout();
            QPushButton *moveTo1920Button = new QPushButton("置于1920");
            QPushButton *moveTo0Button = new QPushButton("置于0");
            hbox->addWidget(moveTo0Button, 1);
            hbox->addWidget(moveTo1920Button, 1);
            vbox->addLayout(hbox);

            connect(moveTo1920Button, &QPushButton::clicked, [this]
                    {
                                   maskWindowOffsetXSpinBox ->setValue(1920);
                                    emit maskWindowPropertyChanged(maskWindowProperty); });

            connect(moveTo0Button, &QPushButton::clicked, [this]
                    {
                                      maskWindowOffsetXSpinBox ->setValue(0);
                                    emit maskWindowPropertyChanged(maskWindowProperty); });
        }

        // 掩膜窗口宽度
        maskWindowWidthSpinBox = new QSpinBox();
        maskWindowWidthSpinBox->setRange(0, 2000);
        maskWindowWidthSpinBox->setValue(maskWindowProperty.width);
        maskWindowWidthSpinBox->setSingleStep(1);
        addRow(vbox, "Mask窗口宽度", maskWindowWidthSpinBox, true);
        connect(maskWindowWidthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value)
                {
                    maskWindowProperty.width = value;
                    emit maskWindowPropertyChanged(maskWindowProperty); });

        // 掩膜窗口高度
        maskWindowHeightSpinBox = new QSpinBox();
        maskWindowHeightSpinBox->setRange(0, 2000);
        maskWindowHeightSpinBox->setValue(maskWindowProperty.height);
        maskWindowHeightSpinBox->setSingleStep(1);
        addRow(vbox, "Mask窗口高度", maskWindowHeightSpinBox, true);
        connect(maskWindowHeightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value)
                {
                    maskWindowProperty.height = value;
                    emit maskWindowPropertyChanged(maskWindowProperty); });

        // 掩膜窗口X偏移
        maskWindowOffsetXSpinBox = new QSpinBox();
        maskWindowOffsetXSpinBox->setRange(-4096, 4096);
        maskWindowOffsetXSpinBox->setValue(maskWindowProperty.offsetX);
        maskWindowOffsetXSpinBox->setSingleStep(1);
        addRow(vbox, "Mask窗口X偏移", maskWindowOffsetXSpinBox, true);
        connect(maskWindowOffsetXSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value)
                {
                    maskWindowProperty.offsetX = value;
                    emit maskWindowPropertyChanged(maskWindowProperty); });

        // 掩膜窗口Y偏移
        maskWindowOffsetYSpinBox = new QSpinBox();
        maskWindowOffsetYSpinBox->setRange(-4096, 4096);
        maskWindowOffsetYSpinBox->setValue(maskWindowProperty.offsetY);
        maskWindowOffsetYSpinBox->setSingleStep(1);
        addRow(vbox, "Mask窗口Y偏移", maskWindowOffsetYSpinBox, true);
        connect(maskWindowOffsetYSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value)
                {
                    maskWindowProperty.offsetY = value;
                    emit maskWindowPropertyChanged(maskWindowProperty); });

        // 分割线
        {
            QFrame *line = new QFrame();
            line->setFrameShape(QFrame::HLine);
            line->setFrameShadow(QFrame::Sunken);
            vbox->addWidget(line);
        }

        // Mask微调项 （是否FlipX FlipY 旋转角度）
        flipXMaskButton = new QPushButton("X轴镜像");
        flipXMaskButton->setCheckable(true);
        flipXMaskButton->setChecked(false);
        flipYMaskButton = new QPushButton("Y轴镜像");
        flipYMaskButton->setCheckable(true);
        flipYMaskButton->setChecked(false);
        {
            QHBoxLayout *hbox = new QHBoxLayout();
            hbox->setContentsMargins(0, 0, 0, 0);
            hbox->addWidget(flipXMaskButton, 1);
            hbox->addWidget(flipYMaskButton, 1);
            vbox->addLayout(hbox);
        }
        connect(flipXMaskButton, &QPushButton::clicked, [this]
                { GlobalResourceManager::getInstance().maskWindow->onFlipped(flipXMaskButton->isChecked(), flipYMaskButton->isChecked()); });
        connect(flipYMaskButton, &QPushButton::clicked, [this]
                { GlobalResourceManager::getInstance().maskWindow->onFlipped(flipXMaskButton->isChecked(), flipYMaskButton->isChecked()); });

        translateMaskXSpinBox = new QSpinBox();
        translateMaskXSpinBox->setRange(-1024, 1024);
        translateMaskXSpinBox->setValue(0);
        translateMaskXSpinBox->setSingleStep(1);
        translateMaskXSpinBox->setSuffix("px");
        addRow(vbox, "X平移", translateMaskXSpinBox, true);

        translateMaskYSpinBox = new QSpinBox();
        translateMaskYSpinBox->setRange(-1024, 1024);
        translateMaskYSpinBox->setValue(0);
        translateMaskYSpinBox->setSingleStep(1);
        translateMaskYSpinBox->setSuffix("px");
        addRow(vbox, "Y平移", translateMaskYSpinBox, true);

        connect(translateMaskXSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value)
                { GlobalResourceManager::getInstance().maskWindow->onTranslated(value, translateMaskYSpinBox->value()); });
        connect(translateMaskYSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value)
                { GlobalResourceManager::getInstance().maskWindow->onTranslated(translateMaskXSpinBox->value(), value); });

        rotateMaskSpinBox = new QDoubleSpinBox();
        rotateMaskSpinBox->setRange(-180.0, 180.0);
        rotateMaskSpinBox->setValue(0);
        rotateMaskSpinBox->setSingleStep(0.2);
        rotateMaskSpinBox->setSuffix("°");
        rotateMaskSpinBox->setDecimals(1);
        addRow(vbox, "旋转角度", rotateMaskSpinBox, true);
        connect(rotateMaskSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double value)
                { GlobalResourceManager::getInstance().maskWindow->onRotated(value); });

        lumOffsetMaskSpinBox = new QSpinBox();
        lumOffsetMaskSpinBox->setRange(-255, 255);
        lumOffsetMaskSpinBox->setValue(0);
        lumOffsetMaskSpinBox->setSingleStep(1);
        addRow(vbox, "亮度偏移", lumOffsetMaskSpinBox, true);
        connect(lumOffsetMaskSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value)
                { GlobalResourceManager::getInstance().maskWindow->onLumOffsetChanged(value); });

        inverseMaskButton = new QPushButton("全局反相");
        inverseMaskButton->setCheckable(true);
        inverseMaskButton->setChecked(false);
        addRow(vbox, "全局反相", inverseMaskButton, true);
        connect(inverseMaskButton, &QPushButton::clicked, [this]
                { GlobalResourceManager::getInstance().maskWindow->onGlobalInverse(inverseMaskButton->isChecked()); });

        onlyRedChannelButton = new QPushButton("只显示红色通道");
        onlyRedChannelButton->setCheckable(true);
        onlyRedChannelButton->setChecked(false);
        addRow(vbox, "通道控制", onlyRedChannelButton, true);
        connect(onlyRedChannelButton, &QPushButton::clicked, [this]
                { GlobalResourceManager::getInstance().maskWindow->onOnlyRedChannel(onlyRedChannelButton->isChecked()); });

        // 分割线
        {
            QFrame *line = new QFrame();
            line->setFrameShape(QFrame::HLine);
            line->setFrameShadow(QFrame::Sunken);
            vbox->addWidget(line);
        }

        // 创建 QTabWidget
        QTabWidget *tabWidget = new QTabWidget();
        tabWidget->setMinimumHeight(130);

        // 手动模式控件
        maskDrawModeControl = new MaskMouseDrawModeControl();
        connect(maskDrawModeControl, &MaskMouseDrawModeControl::drawMode, this, &UserControlArea::onEnterDrawMode);
        connect(maskDrawModeControl, &MaskMouseDrawModeControl::clearButtonClicked, this, &UserControlArea::onClearMask);

        // 将手动模式控件添加到 QTabWidget
        tabWidget->addTab(maskDrawModeControl, "手动调光模式");

        // 自动模式控件
        QWidget *autoModeControl = new QWidget();
        QVBoxLayout *autoModeLayout = new QVBoxLayout(autoModeControl);

        {
            QSpinBox *clipMinSpinBox = new QSpinBox();
            QSpinBox *clipMaxSpinBox = new QSpinBox();
            QDoubleSpinBox *gammaSpinBox = new QDoubleSpinBox();
            QDoubleSpinBox *intensitySpinBox = new QDoubleSpinBox();
            // 第一行 ClipMin + spinbox
            {
                QHBoxLayout *hbox1 = new QHBoxLayout();
                QLabel *label1 = new QLabel("ClipMin");
                clipMinSpinBox->setRange(0, 255);
                clipMinSpinBox->setValue(0);
                hbox1->addWidget(label1, 1);
                hbox1->addWidget(clipMinSpinBox, 1);
                autoModeLayout->addLayout(hbox1);
                // 行为：当ClipMin改变时，组合一个TransferFunction并交给maskWindow处理
                connect(clipMinSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this, clipMinSpinBox, clipMaxSpinBox, gammaSpinBox, intensitySpinBox]
                        {
                            //  确保ClipMin 比 ClipMax 至少小 1
                            if (clipMinSpinBox->value() >= clipMaxSpinBox->value())
                            {
                                clipMinSpinBox->setValue(clipMaxSpinBox->value() - 1);
                            }

                            TransferFunction transferFunction;
                            transferFunction.min = clipMinSpinBox->value() / 255.0f;
                            transferFunction.max = clipMaxSpinBox->value() / 255.0f;
                            transferFunction.gamma = gammaSpinBox->value();
                            transferFunction.intensity = intensitySpinBox->value();
                            GlobalResourceManager::getInstance().maskWindow->onTransferFunctionChanged(transferFunction); });
            }

            // 第二行 ClipMax + spinbox
            {
                QHBoxLayout *hbox1 = new QHBoxLayout();
                QLabel *label1 = new QLabel("ClipMax");
                clipMaxSpinBox->setRange(0, 255);
                clipMaxSpinBox->setValue(255);
                hbox1->addWidget(label1, 1);
                hbox1->addWidget(clipMaxSpinBox, 1);
                autoModeLayout->addLayout(hbox1);

                // 行为：当ClipMax改变时，组合一个TransferFunction并交给maskWindow处理
                connect(clipMaxSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this, clipMinSpinBox, clipMaxSpinBox, gammaSpinBox, intensitySpinBox]
                        {
                            //  确保ClipMax 比 ClipMin 至少大 1
                            if (clipMaxSpinBox->value() <= clipMinSpinBox->value())
                            {
                                clipMaxSpinBox->setValue(clipMinSpinBox->value() + 1);
                            }
                           

                            TransferFunction transferFunction;
                            transferFunction.min = clipMinSpinBox->value() / 255.0f;
                            transferFunction.max = clipMaxSpinBox->value() / 255.0f;
                            transferFunction.gamma = gammaSpinBox->value();
                            transferFunction.intensity = intensitySpinBox->value();
                            GlobalResourceManager::getInstance().maskWindow->onTransferFunctionChanged(transferFunction); });
            }

            // 新行： "gamma值" + spnbox 等分
            {
                QHBoxLayout *hbox1 = new QHBoxLayout();
                QLabel *label1 = new QLabel("Gamma校正");
                gammaSpinBox->setRange(0.01, 10.0);
                gammaSpinBox->setValue(1.0);
                gammaSpinBox->setSingleStep(0.01);
                hbox1->addWidget(label1, 1);
                hbox1->addWidget(gammaSpinBox, 1);
                autoModeLayout->addLayout(hbox1);

                // 行为：当gamma改变时，组合一个TransferFunction并交给maskWindow处理
                connect(gammaSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, clipMinSpinBox, clipMaxSpinBox, gammaSpinBox, intensitySpinBox]
                        {
                            TransferFunction transferFunction;
                            transferFunction.min = clipMinSpinBox->value() / 255.0f;
                            transferFunction.max = clipMaxSpinBox->value() / 255.0f;
                            transferFunction.gamma = gammaSpinBox->value();
                            transferFunction.intensity = intensitySpinBox->value();
                            GlobalResourceManager::getInstance().maskWindow->onTransferFunctionChanged(transferFunction); });
            }

            // 新行： "强度" + double spinbox(0~1) 等分
            {
                QHBoxLayout *hbox1 = new QHBoxLayout();
                QLabel *label1 = new QLabel("强度系数");
                intensitySpinBox->setRange(0.0, 1.0);
                intensitySpinBox->setValue(1.0);
                intensitySpinBox->setSingleStep(0.1);
                hbox1->addWidget(label1, 1);
                hbox1->addWidget(intensitySpinBox, 1);
                autoModeLayout->addLayout(hbox1);

                // 行为：当强度改变时，组合一个TransferFunction并交给maskWindow处理
                connect(intensitySpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, clipMinSpinBox, clipMaxSpinBox, gammaSpinBox, intensitySpinBox]
                        {
                            TransferFunction transferFunction;
                            transferFunction.min = clipMinSpinBox->value() / 255.0f;
                            transferFunction.max = clipMaxSpinBox->value() / 255.0f;
                            transferFunction.gamma = gammaSpinBox->value();
                            transferFunction.intensity = intensitySpinBox->value();
                            GlobalResourceManager::getInstance().maskWindow->onTransferFunctionChanged(transferFunction); });
            }

            // 新行："强度偏移" int型，从-100到+100，步进为1，原始为0
            {
                QHBoxLayout *hbox1 = new QHBoxLayout();
                QLabel *label1 = new QLabel("强度偏移");
                QSpinBox *intensityOffsetSpinBox = new QSpinBox();
                intensityOffsetSpinBox->setRange(-100, 100);
                intensityOffsetSpinBox->setValue(0);
                hbox1->addWidget(label1, 1);
                hbox1->addWidget(intensityOffsetSpinBox, 1);
                autoModeLayout->addLayout(hbox1);

                // 行为：当强度偏移改变时，组合一个TransferFunction并交给maskWindow处理
                connect(intensityOffsetSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this, clipMinSpinBox, clipMaxSpinBox, gammaSpinBox, intensitySpinBox, intensityOffsetSpinBox]
                        {
                            TransferFunction transferFunction;
                            transferFunction.min = clipMinSpinBox->value() / 255.0f;
                            transferFunction.max = clipMaxSpinBox->value() / 255.0f;
                            transferFunction.gamma = gammaSpinBox->value();
                            transferFunction.intensity = intensitySpinBox->value();
                            transferFunction.offset = (float)intensityOffsetSpinBox->value() / 255;
                            GlobalResourceManager::getInstance().maskWindow->onTransferFunctionChanged(transferFunction); });
            }
        }

        autoModeLayout->addStretch();

        // 将自动模式控件添加到 QTabWidget
        tabWidget->addTab(autoModeControl, "自动调光模式");
        tabWidget->setTabIcon(0, QIcon(":/icons8_sign_up.svg"));
        tabWidget->setTabIcon(1, QIcon(":/icons8_camera_automation.svg"));

        // 将 QTabWidget 添加到布局
        vbox->addWidget(tabWidget);

        // 连接 QTabWidget 的 currentChanged 信号到 onMaskModeChanged 槽
        connect(tabWidget, &QTabWidget::currentChanged, this, &UserControlArea::onMaskModeChanged);

        setLayout(vbox);
    }

public slots:
    void onMaskModeChanged(int index)
    {
        if (index == 0)
        {
            Log::info("进入手动调光模式");
            GlobalResourceManager::getInstance().maskWindow->onContinuousMode(false); // mask window 释放消费权
        }
        else if (index == 1)
        {
            Log::info("进入自动调光模式");
            GlobalResourceManager::getInstance().maskWindow->onContinuousMode(true); // mask window 重新占据消费权
        }
    }

    void onEnterDrawMode(bool enabled)
    {

        FrameRenderer *ref = GlobalResourceManager::getInstance().getRefFrameRenderer();

        if (ref == nullptr)
        {
            Log::warn("No reference frame renderer");
        }
        else
        {
        }

        emit drawMode(enabled);
    }

    void onClearMask()
    {
        emit clearMask();
    }

signals:
    void drawMode(bool enabled);
    void clearMask();
    void maskWindowPropertyChanged(const MaskWindowProperty &property);

private:
    void addRow(QVBoxLayout *vbox, const QString &labelText, QWidget *inputWidget, bool expandInputWidget = false)
    {
        QHBoxLayout *hbox = new QHBoxLayout();
        QLabel *label = new QLabel(labelText);
        hbox->addWidget(label, 1); // 添加标签，占据1份宽度，靠左对齐

        QWidget *wrapperWidget = new QWidget();                      // 创建新的QWidget
        QHBoxLayout *wrapperLayout = new QHBoxLayout(wrapperWidget); // 为新的QWidget设置布局
        wrapperLayout->setContentsMargins(0, 0, 0, 0);               // 设置布局的margin为0
        wrapperLayout->addWidget(inputWidget);                       // 将inputWidget添加到新的QWidget中

        if (expandInputWidget)
        {
            wrapperWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            hbox->addWidget(wrapperWidget, 1); // 添加新的QWidget，占据1份宽度，靠右对齐
        }
        else
        {
            wrapperWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            hbox->addWidget(wrapperWidget, 1, Qt::AlignLeft); // 添加新的QWidget，占据1份宽度，靠右对齐
        }

        vbox->addLayout(hbox);
    }

private:
    QPushButton *flipXMaskButton;      // Mask X轴翻转
    QPushButton *flipYMaskButton;      // Mask Y轴翻转
    QDoubleSpinBox *rotateMaskSpinBox; // Mask 旋转角度
    QPushButton *inverseMaskButton;    // Mask 反色
    QPushButton *onlyRedChannelButton; // 只在红色通道显示
    QSpinBox *translateMaskXSpinBox;   // Mask X平移
    QSpinBox *translateMaskYSpinBox;   // Mask Y平移
    QSpinBox *lumOffsetMaskSpinBox;    // Mask 亮度偏置

    QComboBox *DMDWorkModeComboBox;     // DMD工作模式
    QPushButton *showMaskWindowButton;  // 显示掩膜窗口
    QSpinBox *maskWindowWidthSpinBox;   // 掩膜窗口宽度
    QSpinBox *maskWindowHeightSpinBox;  // 掩膜窗口高度
    QSpinBox *maskWindowOffsetXSpinBox; // 掩膜窗口X偏移
    QSpinBox *maskWindowOffsetYSpinBox; // 掩膜窗口Y偏移
    MaskWindowProperty maskWindowProperty;

    QSpinBox *frameRateSpinBox; // 帧率 不可编辑

    QTimer *frameRateQueryTimer;

    QComboBox *maskModeComboBox;                   // 掩膜模式 手动绘制 灰度自适应
    MaskMouseDrawModeControl *maskDrawModeControl; // 掩膜绘制模式控制
};
