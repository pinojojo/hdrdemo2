#ifndef USBCAMERA_HPP
#define USBCAMERA_HPP

#include "ICamera.hpp"
#include "Frame.h"
#include "TripleBuffer.h"
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>

std::vector<std::string> EnumUSBDevices();

class USBCamera : public lzx::ICamera
{
public:
    USBCamera(const std::string &label);
    USBCamera(int id);
    virtual ~USBCamera();

    std::string label() override;
    bool open() override;
    bool close() override;
    bool start() override;
    bool stop() override;
    bool snap() override;

    virtual bool streaming() override;
    virtual bool getFrame(unsigned char *buffer, int &width, int &height, int &channels, int &bitDepth) override;

    bool set(const std::string &name, double value) override;
    bool set(const std::string &name, int value) override;
    bool set(const std::string &name, bool value) override;
    bool set(const std::string &name, const std::string &value) override;
    bool get(const std::string &name, double &value) override;
    bool get(const std::string &name, int &value) override;
    bool get(const std::string &name, bool &value) override;
    bool get(const std::string &name, std::string &value) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;

    size_t m_lastGotFrameCount = 0;
};

#endif
