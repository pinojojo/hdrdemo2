#include "PlayerOne.hpp"

#include <PlayerOneCamera.h>

#include "logwidget.hpp"

#include "Frame.h"

std::map<int, std::string> PlayerOne::getALLCameraIDName()
{
    using namespace std;

    map<int, string> cameraIDName;

    int camera_count = POAGetCameraCount();

    for (int i = 0; i < camera_count; ++i)
    {
        POACameraProperties cameraProp;

        POAErrors error = POAGetCameraProperties(i, &cameraProp);

        if (error != POA_OK)
        {
            Log::error(QString("POAGetCameraProperties failed with error code %1").arg(error));
            continue;
        }

        cameraIDName.insert(pair<int, string>(cameraProp.cameraID, string(cameraProp.cameraModelName)));
    }

    return cameraIDName;
}

struct PlayerOne::Impl
{
    std::string label;
    int cameraId = -1;
    void *handle = nullptr;
    bool streaming = false;
    std::unique_ptr<std::thread> grabThread;
    double exposureTime = 0.0; // us
    int width;
    int height;
    int channels;
    int bitDepth;
    std::unique_ptr<lzx::TripleBuffer<lzx::Frame>> tripleBuffer;

    // ctor
    Impl() : tripleBuffer(std::make_unique<lzx::TripleBuffer<lzx::Frame>>())
    {
    }

    void grabFunction()
    {
        while (streaming)
        {
            POABool pIsReady = POA_FALSE;

            while (pIsReady == POA_FALSE)
            {
                POAImageReady(cameraId, &pIsReady);

                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }

            lzx::Frame frame(width, height, channels, bitDepth);

            long exposureUs = exposureTime;
            POAErrors error = POAGetImageData(this->cameraId,
                                              frame.buffer(),
                                              frame.bufferSize(),
                                              exposureUs / 1000 + 500);

            if (error != POA_OK)
            {
                Log::error(QString("POAGetImageData failed with error code %1").arg(error));
                break;
            }

            tripleBuffer->produce(std::move(frame));
        }
    }
};

PlayerOne::PlayerOne()
{
    // 构造impl
    impl = std::make_unique<Impl>();
}

std::string PlayerOne::label()
{
    return impl->label;
}

bool PlayerOne::open()
{
    std::map<int, std::string> allCameraIDName = getALLCameraIDName();

    if (allCameraIDName.empty())
    {
        Log::error("No PlayerOne camera found");
        return false;
    }

    Log::info("PlayerOne camera found");

    impl->cameraId = allCameraIDName.begin()->first;
    impl->label = allCameraIDName.begin()->second;

    // 开启
    POAErrors error = POAOpenCamera(impl->cameraId);

    if (error != POA_OK)
    {
        Log ::error(QString("POAOpenCamera failed with error code %1").arg(error));
        return false;
    }

    // 初始化
    error = POAInitCamera(impl->cameraId);

    if (error != POA_OK)
    {
        Log::error(QString("POAInitCamera failed with error code %1").arg(error));
        return false;
    }

    // 设置图像格式
    POAImgFormat imgFormat = POA_RAW16;
    error = POASetImageFormat(impl->cameraId, imgFormat);

    if (error != POA_OK)
    {
        Log::error(QString("POASetImageFormat failed with error code %1").arg(error));
        return false;
    }

    // 获取图像大小
    error = POAGetImageSize(impl->cameraId, &impl->width, &impl->height);
    if (error != POA_OK)
    {
        Log::error(QString("POAGetImageSize failed with error code %1").arg(error));
        return false;
    }

    return true;
}

bool PlayerOne::close()
{
    POAErrors error = POACloseCamera(impl->cameraId);

    if (error != POA_OK)
    {
        Log::error(QString("POACloseCamera failed with error code %1").arg(error));
        return false;
    }

    return true;
}

bool PlayerOne::start()
{
    if (impl->streaming)
    {
        Log::error("Already streaming");
        return false;
    }

    POAErrors error = POAStartExposure(impl->cameraId, POA_FALSE); // continuously exposure

    if (error != POA_OK)
    {
        Log::error(QString("POAStartExposure failed with error code %1").arg(error));
        return false;
    }

    impl->streaming = true;

    impl->grabThread = std::make_unique<std::thread>(&PlayerOne::Impl::grabFunction, impl.get());

    Log::info("PlayerOne Start streaming thread");

    return true;
}

bool PlayerOne::stop()
{
    if (!impl->streaming)
    {
        Log::error("Not streaming");
        return false;
    }

    POAErrors error = POAStopExposure(impl->cameraId);

    if (error != POA_OK)
    {
        Log::error(QString("POAStopExposure failed with error code %1").arg(error));
        return false;
    }

    impl->streaming = false;

    return true;
}

bool PlayerOne::snap()
{
    return false;
}

bool PlayerOne::streaming()
{
    return impl->streaming;
}

bool PlayerOne::getFrame(unsigned char *buffer, int &width, int &height, int &channels, int &bitDepth)
{
    return false;
}

bool PlayerOne::set(const std::string &name, double value)
{
    return false;
}

bool PlayerOne::set(const std::string &name, int value)
{
    return false;
}

bool PlayerOne::set(const std::string &name, bool value)
{
    return false;
}

bool PlayerOne::set(const std::string &name, const std::string &value)
{
    return false;
}

bool PlayerOne::get(const std::string &name, double &value)
{
    // 曝光时间 us
    if (name == "ExposureTime")
    {
        POAConfigValue exposValue;

        POABool boolValue;

        POAErrors error = POAGetConfig(impl->cameraId, POA_EXPOSURE, &exposValue, &boolValue);

        if (error != POA_OK)
        {
            Log::error(QString("POAGetConfig failed with error code %1").arg(error));
            return false;
        }

        value = exposValue.intValue;

        return true;
    }
    return false;
}

bool PlayerOne::get(const std::string &name, int &value)
{
    return false;
}

bool PlayerOne::get(const std::string &name, bool &value)
{
    return false;
}

bool PlayerOne::get(const std::string &name, std::string &value)
{
    return false;
}
