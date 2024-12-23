#include "devicefinderdialog.hpp"

#include <QVBoxLayout>

#include "logwidget.hpp"

#include "USBCamera.hpp"

#include "Global.hpp"

DeviceFinderDialog::DeviceFinderDialog(QWidget *parent)
    : QDialog(parent),
      deviceList(new QListWidget(this)),
      searchButton(new QPushButton(tr("搜索"), this)),
      selectionButton(new QPushButton(tr("选择"), this))
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(deviceList);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(searchButton);
    buttonLayout->addWidget(selectionButton);
    layout->addLayout(buttonLayout);

    selectionButton->setEnabled(false); // Initially disabled

    connect(searchButton, &QPushButton::clicked, this, &DeviceFinderDialog::searchDevices);
    connect(selectionButton, &QPushButton::clicked, this, &DeviceFinderDialog::selectDevice);
    connect(deviceList, &QListWidget::itemSelectionChanged, [this]
            { selectionButton->setEnabled(deviceList->currentItem() != nullptr); });
}

DeviceFinderDialog::~DeviceFinderDialog()
{
    delete deviceList;
    delete searchButton;
    delete selectionButton;
}

void DeviceFinderDialog::searchDevices()
{
    // Clear the device list
    deviceList->clear();

    // Enumerate USB devices and add them to the list
    auto devices = EnumUSBDevices();
    for (auto &device : devices)
    {
        deviceList->addItem(device.c_str());
    }
}

void DeviceFinderDialog::selectDevice()
{
    // get the selected device id
    auto device = deviceList->currentRow();

    // emit the signal
    emit deviceSelected(device);

    GlobalResourceManager::getInstance().camera = std::make_unique<USBCamera>(device);

    Log::info(QString("Device %1 selected").arg(device).toStdString().c_str());

    // close the dialog
    close();
}