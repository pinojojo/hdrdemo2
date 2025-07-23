#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <QString>
#include <QSettings>
#include <memory>

class Settings {
public:
    static Settings& getInstance() {
        static Settings instance;
        return instance;
    }

    // 获取默认保存路径
    QString getDefaultSavePath() const;
    void setDefaultSavePath(const QString& path);

    // 获取默认曝光时间
    double getDefaultExposureTime() const;
    void setDefaultExposureTime(double time);

    // 获取是否flipx
    bool isFlipX() const;
    void setFlipX(bool value);

    // 获取referecer 相机是否flipx
    bool isReferenceFlipX() const;
    void setReferenceFlipX(bool value);

    // 获取是否flipY
    bool isFlipY() const;
    void setFlipY(bool value);

    // 获取reference 相机是否flipY
    bool isReferenceFlipY() const;
    void setReferenceFlipY(bool value);

    // 保存和加载设置
    void save();
    void load();

private:
    Settings();
    ~Settings() = default;
    
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;

    std::unique_ptr<QSettings> settings;
    
    // 设置项
    QString defaultSavePath;
    double defaultExposureTime;
    bool flipX;
    bool referenceFlipX;
    bool flipY;
    bool referenceFlipY;
};

#endif // SETTINGS_HPP