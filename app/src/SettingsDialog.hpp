#ifndef SETTINGSDIALOG_HPP
#define SETTINGSDIALOG_HPP

#include <QDialog>
#include <QLineEdit>
#include <QDoubleSpinBox>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private slots:
    void browseDefaultSavePath();
    void accept() override;

private:
    QLineEdit *defaultSavePathEdit;
    QDoubleSpinBox *defaultExposureTimeSpinBox;

    const int LABEL_WIDTH = 120;
};

#endif // SETTINGSDIALOG_HPP