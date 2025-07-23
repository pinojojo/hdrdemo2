#include "Settings.hpp"
#include <QStandardPaths>

Settings::Settings()
    : settings(std::make_unique<QSettings>("HDRDemo", "HDRDemo2"))
    , defaultSavePath(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
    , defaultExposureTime(10000.0)
    , flipX(false)           // 初始化布尔成员
    , referenceFlipX(false)  // 初始化布尔成员
    , flipY(false)           // 初始化布尔成员
    , referenceFlipY(false)  // 初始化布尔成员
{
    load();
}

QString Settings::getDefaultSavePath() const {
    return defaultSavePath;


}

void Settings::setDefaultSavePath(const QString& path) {
    defaultSavePath = path;
    save(); // 自动保存
}

double Settings::getDefaultExposureTime() const {
    return defaultExposureTime;
}

void Settings::setDefaultExposureTime(double time) {
    defaultExposureTime = time;
    save(); // 自动保存
}

// 添加翻转设置的访问器方法
bool Settings::isFlipX() const {
    return flipX;
}

void Settings::setFlipX(bool flip) {
    flipX = flip;
    save(); // 自动保存
}

bool Settings::isFlipY() const {
    return flipY;
}

void Settings::setFlipY(bool flip) {
    flipY = flip;
    save(); // 自动保存
}

bool Settings::isReferenceFlipX() const {
    return referenceFlipX;
}

void Settings::setReferenceFlipX(bool flip) {
    referenceFlipX = flip;
    save(); // 自动保存
}

bool Settings::isReferenceFlipY() const {
    return referenceFlipY;
}

void Settings::setReferenceFlipY(bool flip) {
    referenceFlipY = flip;
    save(); // 自动保存
}



void Settings::save() {
    settings->setValue("defaultSavePath", defaultSavePath);
    settings->setValue("defaultExposureTime", defaultExposureTime);
    settings->setValue("flipX", flipX);
    settings->setValue("referenceFlipX", referenceFlipX);
    settings->setValue("flipY", flipY);
    settings->setValue("referenceFlipY", referenceFlipY);
    settings->sync();
}

void Settings::load() {
    defaultSavePath = settings->value("defaultSavePath", defaultSavePath).toString();
    defaultExposureTime = settings->value("defaultExposureTime", defaultExposureTime).toDouble();
    flipX = settings->value("flipX", false).toBool();
    referenceFlipX = settings->value("referenceFlipX", false).toBool();
    flipY = settings->value("flipY", false).toBool();
    referenceFlipY = settings->value("referenceFlipY", false).toBool();
    
}