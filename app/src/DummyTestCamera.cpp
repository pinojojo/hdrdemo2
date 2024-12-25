#include "DummyTestCamera.h"
#include <cstring>
#include <cmath>
#include <QString>

#include "logwidget.hpp"

namespace lzx
{
    DummyTestCamera::DummyTestCamera()
        : m_isOpened(false), m_isStreaming(false), m_width(640), m_height(480), m_channels(1), m_bitDepth(16)
    {
        Log::info("camera ctor");
        generateTestPattern();
    }

    DummyTestCamera::~DummyTestCamera()
    {
        Log::info("camera dtor");
        if (m_isStreaming)
            stop();
        if (m_isOpened)
            close();
    }

    bool DummyTestCamera::open()
    {
        Log::info("camera open");
        if (m_isOpened)
            return true;

        m_isOpened = true;

        notifyStateChanged("open", "true");

        return true;
    }

    bool DummyTestCamera::close()
    {
        Log::info("camera close");
        if (!m_isOpened)
            return true;

        if (m_isStreaming)
            stop();

        m_isOpened = false;

        notifyStateChanged("open", "false");
        return true;
    }

    bool DummyTestCamera::start()
    {
        Log::info("camera start");
        if (!m_isOpened)
            return false;

        m_isStreaming = true;
        notifyStateChanged("stream", "true");
        return true;
    }

    bool DummyTestCamera::stop()
    {
        Log::info("camera stop");
        if (!m_isStreaming)
            return true;

        m_isStreaming = false;
        notifyStateChanged("stream", "false");
        return true;
    }

    bool DummyTestCamera::snap()
    {
        Log::info("camera snap");
        if (!m_isStreaming)
            return false;

        return true;
    }

    bool DummyTestCamera::getFrame(unsigned char *buffer, int &width, int &height, int &channels, int &bitDepth)
    {
        if (!m_isOpened || !m_isStreaming || buffer == nullptr)
        {
            Log::error(QString("camera getFrame failed: opened %1 streaming %2 buffer %3")
                           .arg(m_isOpened)
                           .arg(m_isStreaming)
                           .arg(buffer != nullptr));
            return false;
        }

        width = m_width;
        height = m_height;
        channels = m_channels;
        bitDepth = m_bitDepth;

        // 创建临时缓冲区
        std::vector<unsigned char> tempBuffer = m_testPattern;
        auto pattern = reinterpret_cast<uint16_t *>(tempBuffer.data());

        // 在临时缓冲区中绘制动态图案
        static int frameCount = 0;
        frameCount++;

        // 创建一个移动的圆形图案
        int centerX = m_width / 2 + static_cast<int>(100 * sin(frameCount * 0.01));
        int centerY = m_height / 2 + static_cast<int>(100 * cos(frameCount * 0.01));
        int radius = 20;
        uint16_t maxValue = (1 << m_bitDepth) - 1; // 16位最大值 65535

        // 绘制一个实心圆
        for (int y = -radius; y <= radius; y++)
        {
            for (int x = -radius; x <= radius; x++)
            {
                if (x * x + y * y <= radius * radius)
                {
                    int drawX = centerX + x;
                    int drawY = centerY + y;

                    // 确保绘制位置在图像范围内
                    if (drawX >= 0 && drawX < m_width && drawY >= 0 && drawY < m_height)
                    {
                        // 创建渐变效果
                        float distance = sqrt(x * x + y * y) / radius;
                        uint16_t value = static_cast<uint16_t>(maxValue * (1.0f - distance));
                        pattern[drawX + drawY * m_width] = value * (sin(frameCount * 0.1) + 1.0f) * 0.5f;
                    }
                }
            }
        }

        // 将临时缓冲区复制到输出缓冲区
        std::memcpy(buffer, tempBuffer.data(), tempBuffer.size());
        return true;
    }
    void DummyTestCamera::generateTestPattern()
    {
        // 为16位图像分配内存 (2字节/像素)
        m_testPattern.resize(m_width * m_height * sizeof(uint16_t));
        uint16_t *pattern = reinterpret_cast<uint16_t *>(m_testPattern.data());

        // 生成16位渐变测试图案
        const uint16_t maxValue = (1 << m_bitDepth) - 1; // 16位最大值 65535

        for (int y = 0; y < m_height; ++y)
        {
            for (int x = 0; x < m_width; ++x)
            {
                int index = y * m_width + x;

                // 创建一个水平渐变
                float gradientX = static_cast<float>(x) / m_width;
                // 创建一个垂直渐变
                float gradientY = static_cast<float>(y) / m_height;

                // 组合渐变，创建一个对角线渐变效果
                float gradient = (gradientX + gradientY) / 2.0f;

                // 将渐变值映射到16位范围
                pattern[index] = static_cast<uint16_t>(gradient * maxValue);

                // 可以添加一些测试图案，比如中心十字或网格
                // 在图像中心绘制十字
                int centerX = m_width / 2;
                int centerY = m_height / 2;
                int lineWidth = 2;

                if ((abs(x - centerX) < lineWidth) || (abs(y - centerY) < lineWidth))
                {
                    pattern[index] = maxValue; // 白色十字
                }

                // 添加网格线
                if ((x % 64 == 0) || (y % 64 == 0))
                {
                    pattern[index] = maxValue / 2; // 灰色网格
                }
            }
        }
    }

    bool DummyTestCamera::set(const std::string &name, int value)
    {
        QString msg = "set " + QString::fromStdString(name) + " " + QString::number(value);
        Log::info(msg);

        if (name == "width")
        {
            m_width = value;
            generateTestPattern();
            return true;
        }
        else if (name == "height")
        {
            m_height = value;
            generateTestPattern();
            return true;
        }
        return false;
    }

    bool DummyTestCamera::get(const std::string &name, int &value)
    {

        if (name == "width")
        {
            value = m_width;
            return true;
        }
        else if (name == "height")
        {
            value = m_height;
            return true;
        }
        else if (name == "fps")
        {
            value = 30;
            return true;
        }
        return false;
    }
}