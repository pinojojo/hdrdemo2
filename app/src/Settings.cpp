#include "Settings.hpp"
#include <QStandardPaths>

Settings::Settings()
    : settings(std::make_unique<QSettings>("HDRDemo", "HDRDemo2"))
    , defaultSavePath(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
    , defaultExposureTime(10000.0)
{
    load();
}

QString Settings::getDefaultSavePath() const {
    return defaultSavePath;
}

void Settings::setDefaultSavePath(const QString& path) {
    defaultSavePath = path;
}

double Settings::getDefaultExposureTime() const {
    return defaultExposureTime;
}

void Settings::setDefaultExposureTime(double time) {
    defaultExposureTime = time;
}

void Settings::save() {
    settings->setValue("defaultSavePath", defaultSavePath);
    settings->setValue("defaultExposureTime", defaultExposureTime);
    settings->sync();
}

void Settings::load() {
    defaultSavePath = settings->value("defaultSavePath", defaultSavePath).toString();
    defaultExposureTime = settings->value("defaultExposureTime", defaultExposureTime).toDouble();
}