#ifndef DEVICEFINDERDIALOG_HPP
#define DEVICEFINDERDIALOG_HPP

#include <QDialog>
#include <QListWidget>
#include <QPushButton>

class DeviceFinderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceFinderDialog(QWidget *parent = nullptr);
    ~DeviceFinderDialog();

signals:
    void deviceSelected(int device);

private slots:
    void searchDevices();
    void selectDevice();

private:
    QListWidget *deviceList;
    QPushButton *searchButton;
    QPushButton *selectionButton;
};

#endif // DEVICEFINDERDIALOG_HPP
