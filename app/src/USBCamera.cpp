#include "USBCamera.hpp"
#include <MvCameraControl.h> // Hikvision SDK

#include "logwidget.hpp"
#include "Global.hpp"

#include <sstream>

static MV_CC_DEVICE_INFO_LIST stDeviceList;

std::string mvsErrorCode(int code)
{
    switch (code)
    {
    case MV_OK:
        return "Success";
    case MV_E_HANDLE:
        return "Invalid handle";
    case MV_E_SUPPORT:
        return "Unsupported function";
    case MV_E_BUFOVER:
        return "Buffer overflow";
    case MV_E_CALLORDER:
        return "Incorrect function call order";
    case MV_E_PARAMETER:
        return "Invalid parameter";
    case MV_E_RESOURCE:
        return "Insufficient resources";
    default:
        return "Unknown error";
    }
}

std::vector<std::string> EnumUSBDevices()
{
    const int maxAttempts = 5; // Max attempts to enum devices
    const int delayMilliseconds = 100;

    std::vector<std::string> devicesInfo;
    int nRet = MV_OK;

    // Try to enum devices for maxAttempts times
    for (int attempt = 0; attempt < maxAttempts; ++attempt)
    {
        memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
        nRet = MV_CC_EnumDevices(MV_USB_DEVICE, &stDeviceList);
        if (MV_OK == nRet)
            break;
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMilliseconds));
    }

    if (stDeviceList.nDeviceNum == 0)
    {
        Log::error("No device found");
        return devicesInfo;
    }

    for (int i = 0; i < stDeviceList.nDeviceNum; ++i)
    {
        std::stringstream ss;
        ss << "Device " << i << ": " << stDeviceList.pDeviceInfo[i]->SpecialInfo.stUsb3VInfo.chVendorName << " "
           << stDeviceList.pDeviceInfo[i]->SpecialInfo.stUsb3VInfo.chModelName << " "
           << stDeviceList.pDeviceInfo[i]->SpecialInfo.stUsb3VInfo.chSerialNumber;
        devicesInfo.push_back(ss.str());

        Log::info(ss.str().c_str());
    }

    return devicesInfo;
}

struct USBCamera::Impl
{
    std::string label;
    int id = 0; // Device id default to 0 to open the first device
    void *handle = nullptr;

    bool streaming = false;
    bool lowLatencyMode = false;
    std::unique_ptr<std::thread> thread;

    void grabFunction()
    {
        MV_FRAME_OUT stOutFrame = {0}; // 帧数据
        while (streaming)
        {
            int nRet = MV_CC_GetImageBuffer(this->handle, &stOutFrame, 1000);
            if (MV_OK == nRet)
            {
                int width = stOutFrame.stFrameInfo.nWidth;
                int height = stOutFrame.stFrameInfo.nHeight;
                int channels = stOutFrame.stFrameInfo.nFrameLen / (width * height);

                qDebug() << "width: " << width << "height: " << height << "channels: " << channels;

                if (stOutFrame.stFrameInfo.enPixelType == PixelType_Gvsp_Mono8)
                {
                    qDebug() << "PixelType_Gvsp_Mono8 Got";
                    lzx::Frame frame(width, height, channels);
                    frame.fill(stOutFrame.pBufAddr);
                    GlobalResourceManager::getInstance().tripleBuffer->produce(std::move(frame));
                }
                else
                {
                    Log::error("Unsupported pixel format");
                }
            }
            MV_CC_FreeImageBuffer(this->handle, &stOutFrame);
        }
    }
};

USBCamera::USBCamera(const std::string &label)
    : impl(std::make_unique<Impl>())
{
    impl->label = label;
}

USBCamera::USBCamera(int id)
    : impl(std::make_unique<Impl>())
{
    impl->id = id;
    Log::info(QString("Device %1 Instance Created.").arg(id).toStdString().c_str());
}

USBCamera::~USBCamera()
{
}

std::string USBCamera::label()
{
    return impl->label;
}

bool USBCamera::open()
{
    // 自动枚举一次设备
    if (stDeviceList.nDeviceNum == 0)
    {
        EnumUSBDevices();
        if (stDeviceList.nDeviceNum == 0)
        {
            Log::error("No device found");
            return false;
        }
    }

    if (impl->id >= 0 && impl->id < stDeviceList.nDeviceNum)
    {
        MV_CC_DEVICE_INFO *di = stDeviceList.pDeviceInfo[impl->id];

        // Check handle is not null
        if (impl->handle != nullptr)
        {
            Log::error("Handle is not null");
            return false;
        }

        // Handle
        int nRet = MV_CC_CreateHandle(&impl->handle, di);
        if (MV_OK != nRet)
        {
            Log::error("Create handle failed");
            return false;
        }

        // Open the device
        nRet = MV_CC_OpenDevice(impl->handle);
        if (MV_OK != nRet)
        {
            Log::error("Open device failed");
            return false;
        }

        // Set trigger mode to off
        nRet = MV_CC_SetEnumValue(impl->handle, "TriggerMode", 0);
        if (MV_OK != nRet)
        {
            Log::error("Set trigger mode failed");
            return false;
        }

        // Get Pixel Format
        MVCC_ENUMVALUE stParam = {0};
        nRet = MV_CC_GetEnumValue(impl->handle, "PixelFormat", &stParam);
        if (MV_OK != nRet)
        {
            Log::error("Get pixel format failed");
            return false;
        }
        else
        {
            Log::info(QString("Pixel format: %1").arg(stParam.nCurValue).toStdString().c_str());
        }

        // Set Default Pixel Format to Mono8
        nRet = MV_CC_SetEnumValue(impl->handle, "PixelFormat", PixelType_Gvsp_Mono8);
        if (MV_OK != nRet)
        {
            Log::error("Set pixel format failed");
            return false;
        }
        else
        {
            Log::info("Set pixel format to Mono8");
        }

        // 设置不控制帧率
        nRet = MV_CC_SetBoolValue(impl->handle, "AcquisitionFrameRateEnable", false);
        if (MV_OK != nRet)
        {
            Log::error("Set AcquisitionFrameRateEnable failed");
            return false;
        }

        int maxWidth = 0;
        int maxHeight = 0;
        {
            // 读取最大Width和Height
            MVCC_INTVALUE stParam = {0};
            nRet = MV_CC_GetIntValue(impl->handle, "WidthMax", &stParam);
            if (MV_OK != nRet)
            {
                Log::error("Get WidthMax failed");
                return false;
            }
            else
            {
                Log::info(QString("WidthMax: %1").arg(stParam.nCurValue).toStdString().c_str());
                maxWidth = stParam.nCurValue;
            }

            nRet = MV_CC_GetIntValue(impl->handle, "HeightMax", &stParam);
            if (MV_OK != nRet)
            {
                Log::error("Get HeightMax failed");
                return false;
            }
            else
            {
                Log::info(QString("HeightMax: %1").arg(stParam.nCurValue).toStdString().c_str());
                maxHeight = stParam.nCurValue;
            }
        }

        // 设置最佳ROI
        int bestWidth = 1024;
        int bestHeight = 768;
        {
            // 确保最佳宽度和高度不超过最大值，不然就返回错误
            if (bestWidth > maxWidth || bestHeight > maxHeight)
            {
                Log::error("Best width and height exceed maximum value");
                return false;
            }

            // 计算最佳OffsetX和OffsetY使得ROI居中
            int offsetX = (maxWidth - bestWidth) / 2;
            int offsetY = (maxHeight - bestHeight) / 2;

            // 确保计算出的OffsetX和OffsetY是4的倍数
            offsetX = offsetX - offsetX % 4;
            offsetY = offsetY - offsetY % 4;
            Log::info(QString("Best OffsetX: %1, OffsetY: %2").arg(offsetX).arg(offsetY).toStdString().c_str());

            // 设置ROI
            nRet = MV_CC_SetIntValue(impl->handle, "Width", bestWidth);
            if (MV_OK != nRet)
            {
                Log::error("Set Width failed");
                return false;
            }

            nRet = MV_CC_SetIntValue(impl->handle, "Height", bestHeight);
            if (MV_OK != nRet)
            {
                Log::error("Set Height failed");
                return false;
            }

            nRet = MV_CC_SetIntValue(impl->handle, "OffsetX", offsetX);
            if (MV_OK != nRet)
            {
                Log::error("Set OffsetX failed");
                return false;
            }

            nRet = MV_CC_SetIntValue(impl->handle, "OffsetY", offsetY);
            if (MV_OK != nRet)
            {
                Log::error("Set OffsetY failed");
                return false;
            }

            Log::info(QString("Set ROI: OffsetX: %1, OffsetY: %2, Width: %3, Height: %4")
                          .arg(offsetX)
                          .arg(offsetY)
                          .arg(bestWidth)
                          .arg(bestHeight)
                          .toStdString()
                          .c_str());
        }

        {
            // 读取机内当前 ROI 数据
            struct ROIData
            {
                int nOffsetX;
                int nOffsetY;
                int nWidth;
                int nHeight;
            };

            ROIData roiData;

            MVCC_INTVALUE stParam = {0};
            nRet = MV_CC_GetIntValue(impl->handle, "OffsetX", &stParam);
            if (MV_OK != nRet)
            {
                Log::error("Get OffsetX failed");
                return false;
            }
            else
            {
                roiData.nOffsetX = stParam.nCurValue;
            }

            nRet = MV_CC_GetIntValue(impl->handle, "OffsetY", &stParam);
            if (MV_OK != nRet)
            {
                Log::error("Get OffsetY failed");
                return false;
            }
            else
            {
                roiData.nOffsetY = stParam.nCurValue;
            }

            nRet = MV_CC_GetIntValue(impl->handle, "Width", &stParam);
            if (MV_OK != nRet)
            {
                Log::error("Get Width failed");
                return false;
            }
            else
            {
                roiData.nWidth = stParam.nCurValue;
            }

            nRet = MV_CC_GetIntValue(impl->handle, "Height", &stParam);
            if (MV_OK != nRet)
            {
                Log::error("Get Height failed");
                return false;
            }
            else
            {
                roiData.nHeight = stParam.nCurValue;
            }

            Log::info(QString("ROI: OffsetX: %1, OffsetY: %2, Width: %3, Height: %4")
                          .arg(roiData.nOffsetX)
                          .arg(roiData.nOffsetY)
                          .arg(roiData.nWidth)
                          .arg(roiData.nHeight)
                          .toStdString()
                          .c_str());
        }

        return true;
    }
    else
    {
        Log::error("Invalid device id");
        return false;
    }
}

bool USBCamera::close()
{
    if (impl->handle != nullptr)
    {
        // Close the device
        int nRet = MV_CC_CloseDevice(impl->handle);
        if (MV_OK != nRet)
        {
            Log::error("Close device failed");
            return false;
        }

        // Destroy handle
        nRet = MV_CC_DestroyHandle(impl->handle);
        if (MV_OK != nRet)
        {
            Log::error("Destroy handle failed");
            return false;
        }

        impl->handle = nullptr;

        return true;
    }
    else
    {
        Log::error("Handle is null");
        return false;
    }
}

bool USBCamera::start()
{
    if (impl->handle == nullptr)
    {
        Log::error("Handle is null");
        return false;
    }
    else
    {

        // 开启取流
        int nRet = MV_CC_StartGrabbing(impl->handle);
        if (MV_OK != nRet)
        {
            Log::error("Start grabbing failed");
            return false;
        }

        // 线程
        impl->streaming = true;
        impl->thread = std::make_unique<std::thread>(&Impl::grabFunction, impl.get());
        Log::info("Start streaming thread");
        return true;
    }
}

bool USBCamera::stop()
{
    if (impl->handle == nullptr || impl->thread == nullptr || !impl->streaming)
    {
        Log::error("Not streaming");
        return false;
    }
    else
    {
        // 停止并等待线程结束
        if (impl->thread)
        {
            impl->streaming = false;
            impl->thread->join();
            impl->thread.reset();
        }

        // 停止取流
        if (impl->handle != nullptr)
        {
            int nRet = MV_CC_StopGrabbing(impl->handle);
            if (MV_OK != nRet)
            {
                Log::error("Stop grabbing failed");
                return false;
            }
        }

        Log::info("Camera Stoppped");
        return true;
    }
}

bool USBCamera::snap()
{
    return false;
}

bool USBCamera::set(const std::string &name, double value)
{
    if (name == "ExposureTime")
    {
        MVCC_FLOATVALUE stParam = {0};
        stParam.fCurValue = value;
        int nRet = MV_CC_SetFloatValue(impl->handle, "ExposureTime", stParam.fCurValue);
        if (MV_OK != nRet)
        {
            Log::error("Set exposure time failed");
            return false;
        }
        else
        {
            return true;
        }
    }
    else if (name == "Gain")
    {
        MVCC_FLOATVALUE stParam = {0};
        stParam.fCurValue = value;
        int nRet = MV_CC_SetFloatValue(impl->handle, "Gain", stParam.fCurValue);
        if (MV_OK != nRet)
        {
            Log::error("Set Gain failed");
            return false;
        }
        else
        {
            return true;
        }
    }
    return false;
}

bool USBCamera::set(const std::string &name, int value)
{

    if (name == "ExposureAuto")
    {
        MVCC_ENUMVALUE stParam = {0};
        stParam.nCurValue = value; // 0: manual, 1: auto once, 2: auto continuous
        int nRet = MV_CC_SetEnumValue(impl->handle, "ExposureAuto", stParam.nCurValue);
        if (MV_OK != nRet)
        {
            Log::error("Set ExposureAuto failed");
            return false;
        }
        else
        {
            return true;
        }
    }
    else if (name == "Width")
    {

        int nRet = MV_CC_SetIntValue(impl->handle, "Width", value);
        if (MV_OK != nRet)
        {
            Log::error("Set Width failed");
            return false;
        }
        else
        {
            return true;
        }
    }
    else if (name == "Height")
    {
        MVCC_INTVALUE stParam = {0};
        stParam.nCurValue = value;
        int nRet = MV_CC_SetIntValue(impl->handle, "Height", stParam.nCurValue);
        if (MV_OK != nRet)
        {
            Log::error("Set Height failed");
            return false;
        }
        else
        {
            return true;
        }
    }
    else if (name == "OffsetX")
    {

        int nRet = MV_CC_SetIntValue(impl->handle, "OffsetX", value);
        if (MV_OK != nRet)
        {
            Log::error("Set OffsetX failed");
            return false;
        }
        else
        {
            return true;
        }
    }
    else if (name == "OffsetY")
    {

        int nRet = MV_CC_SetIntValue(impl->handle, "OffsetY", value);
        if (MV_OK != nRet)
        {
            Log::error("Set OffsetY failed");
            return false;
        }
        else
        {
            return true;
        }
    }

    Log::error("Unsupported parameter");
    return false;
}

bool USBCamera::set(const std::string &name, bool value)
{
    if (name == "LowLatencyMode")
    {
        impl->lowLatencyMode = value; // 影响数据的去向

        if (impl->lowLatencyMode)
        {
            // 关闭取流线程,但是不关闭取流,数据的获取将在OpenGL Window的loop中直接更新纹理，避免任何形式的Buffer的使用以降低延迟
            if (impl->streaming)
            {
                impl->streaming = false;
                impl->thread->join();
                impl->thread.reset();
            }
        }
        return true;
    }

    Log::error("Unsupported parameter");
    return false;
}

bool USBCamera::set(const std::string &name, const std::string &value)
{

    return false;
}

bool USBCamera::get(const std::string &name, double &value)
{

    int nRet = 0;
    if (name == "ExposureTime")
    {
        MVCC_FLOATVALUE stParam = {0};
        nRet = MV_CC_GetFloatValue(impl->handle, "ExposureTime", &stParam);
        if (MV_OK != nRet)
        {
            Log::error("Get exposure time failed");
            return false;
        }
        else
        {
            value = stParam.fCurValue;
            return true;
        }
    }
    else if (name == "ResultingFrameRate")
    {
        MVCC_FLOATVALUE stParam = {0};
        nRet = MV_CC_GetFloatValue(impl->handle, "ResultingFrameRate", &stParam);
        if (MV_OK != nRet)
        {
            Log::error("Get ResultingFrameRate failed");
            return false;
        }
        else
        {
            value = stParam.fCurValue;
            return true;
        }
    }
    else if (name == "Gain")
    {
        MVCC_FLOATVALUE stParam = {0};
        nRet = MV_CC_GetFloatValue(impl->handle, "Gain", &stParam);
        if (MV_OK != nRet)
        {
            Log::error("Get Gain failed");
            return false;
        }
        else
        {
            value = stParam.fCurValue;
            return true;
        }
    }

    return false;
}

bool USBCamera::get(const std::string &name, int &value)
{
    int nRet = 0;

    if (name == "Width")
    {
        MVCC_INTVALUE stParam = {0};
        nRet = MV_CC_GetIntValue(impl->handle, "Width", &stParam);
        if (MV_OK != nRet)
        {
            Log::error("Get Width failed");
            return false;
        }
        else
        {
            value = stParam.nCurValue;
            return true;
        }
    }
    else if (name == "Height")
    {
        MVCC_INTVALUE stParam = {0};
        nRet = MV_CC_GetIntValue(impl->handle, "Height", &stParam);
        if (MV_OK != nRet)
        {
            Log::error("Get Height failed");
            return false;
        }
        else
        {
            value = stParam.nCurValue;
            return true;
        }
    }
    else if (name == "OffsetX")
    {
        MVCC_INTVALUE stParam = {0};
        nRet = MV_CC_GetIntValue(impl->handle, "OffsetX", &stParam);
        if (MV_OK != nRet)
        {
            Log::error("Get OffsetX failed");
            return false;
        }
        else
        {
            value = stParam.nCurValue;
            return true;
        }
    }
    else if (name == "OffsetY")
    {
        MVCC_INTVALUE stParam = {0};
        nRet = MV_CC_GetIntValue(impl->handle, "OffsetY", &stParam);
        if (MV_OK != nRet)
        {
            Log::error("Get OffsetY failed");
            return false;
        }
        else
        {
            value = stParam.nCurValue;
            return true;
        }
    }

    return false;
}

bool USBCamera::get(const std::string &name, bool &value)
{
    return false;
}

bool USBCamera::get(const std::string &name, std::string &value)
{
    return false;
}
