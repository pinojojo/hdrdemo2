#include "DummyTestCamera.h"
#include <cstring>
#include <cmath>
#include <QString>

#include "logwidget.hpp"

namespace lzx
{
    DummyTestCamera::DummyTestCamera()
        : m_isOpened(false), m_isStreaming(false), m_width(640), m_height(480), m_channels(1)
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
        return true;
    }

    bool DummyTestCamera::start()
    {
        Log::info("camera start");
        if (!m_isOpened)
            return false;

        m_isStreaming = true;
        return true;
    }

    bool DummyTestCamera::stop()
    {
        Log::info("camera stop");
        if (!m_isStreaming)
            return true;

        m_isStreaming = false;
        return true;
    }

    bool DummyTestCamera::snap()
    {
        Log::info("camera snap");
        if (!m_isStreaming)
            return false;

        return true;
    }

    bool DummyTestCamera::getFrame(unsigned char *buffer, int &width, int &height, int &channels)
    {
        Log::info("camera getFrame");
        if (!m_isStreaming || !buffer)
            return false;

        width = m_width;
        height = m_height;
        channels = m_channels;

        // 复制测试图案到缓冲区
        std::memcpy(buffer, m_testPattern.data(), m_testPattern.size());
        return true;
    }

    void DummyTestCamera::generateTestPattern()
    {
        // 为测试图案分配内存
        m_testPattern.resize(m_width * m_height * m_channels);

        // 生成一个简单的渐变测试图案
        for (int y = 0; y < m_height; ++y)
        {
            for (int x = 0; x < m_width; ++x)
            {
                int index = (y * m_width + x);

                // 创建一个彩色渐变图案
                m_testPattern[index] = static_cast<unsigned char>((float)x / m_width * 255); // R
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
        return false;
    }
}