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
};

#endif // SETTINGS_HPP