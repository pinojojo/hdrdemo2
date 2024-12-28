#include "SettingsDialog.hpp"
#include "Settings.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QDialogButtonBox>
SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("系统设置");
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    // 默认保存路径
    auto *pathLayout = new QHBoxLayout;
    auto *pathLabel = new QLabel("默认保存路径：");
    pathLabel->setFixedWidth(LABEL_WIDTH); // 固定标签宽度以对齐
    defaultSavePathEdit = new QLineEdit;
    defaultSavePathEdit->setFixedWidth(250);
    auto *browseButton = new QPushButton("浏览...");
    browseButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    pathLayout->addWidget(pathLabel);
    pathLayout->addWidget(defaultSavePathEdit);
    pathLayout->addWidget(browseButton);
    pathLayout->addStretch(); // 添加弹性空间
    mainLayout->addLayout(pathLayout);

    // 默认曝光时间
    auto *exposureLayout = new QHBoxLayout;
    auto *exposureLabel = new QLabel("默认曝光时间(μs)：");
    exposureLabel->setFixedWidth(LABEL_WIDTH); // 与上面的标签相同宽度
    defaultExposureTimeSpinBox = new QDoubleSpinBox;
    defaultExposureTimeSpinBox->setRange(0, 1000000);
    defaultExposureTimeSpinBox->setDecimals(0);
    defaultExposureTimeSpinBox->setFixedWidth(100);
    exposureLayout->addWidget(exposureLabel);
    exposureLayout->addWidget(defaultExposureTimeSpinBox);
    exposureLayout->addStretch(); // 添加弹性空间
    mainLayout->addLayout(exposureLayout);

    // 添加一些垂直空间
    mainLayout->addSpacing(10);

    // 确定取消按钮
    auto *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText("确定");
    buttonBox->button(QDialogButtonBox::Cancel)->setText("取消");
    mainLayout->addWidget(buttonBox);

    // 连接信号
    connect(browseButton, &QPushButton::clicked,
            this, &SettingsDialog::browseDefaultSavePath);
    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);

    // 加载当前设置
    auto &settings = Settings::getInstance();
    defaultSavePathEdit->setText(settings.getDefaultSavePath());
    defaultExposureTimeSpinBox->setValue(settings.getDefaultExposureTime());
}

void SettingsDialog::browseDefaultSavePath()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "选择默认保存路径",
        defaultSavePathEdit->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty())
    {
        defaultSavePathEdit->setText(dir);
    }
}

void SettingsDialog::accept()
{
    auto &settings = Settings::getInstance();
    settings.setDefaultSavePath(defaultSavePathEdit->text());
    settings.setDefaultExposureTime(defaultExposureTimeSpinBox->value());
    settings.save();
    QDialog::accept();
}