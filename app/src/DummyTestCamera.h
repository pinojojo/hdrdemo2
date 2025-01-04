#ifndef DUMMY_TEST_CAMERA_HPP
#define DUMMY_TEST_CAMERA_HPP

#include "ICamera.hpp"
#include <vector>

#include "TripleBuffer.h"
#include "Frame.h"

namespace lzx
{
    class DummyTestCamera : public ICamera
    {
    public:
        DummyTestCamera(int bitDepth = 16);
        virtual ~DummyTestCamera();

        // 实现ICamera的虚函数
        virtual std::string label() override { return "DummyTest"; }
        virtual bool open() override;
        virtual bool close() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool snap() override;
        virtual bool streaming() override { return m_isStreaming; }
        virtual bool getFrame(unsigned char *buffer, int &width, int &height, int &channels, int &bitDepth) override;

        // 实现一些参数设置和获取
        virtual bool set(const std::string &name, int value) override;
        virtual bool get(const std::string &name, int &value) override;

    private:
        bool m_isOpened;
        bool m_isStreaming;
        std::vector<unsigned char> m_testPattern;
        int m_width;
        int m_height;
        int m_channels;
        int m_bitDepth;

        TripleBuffer<Frame> m_frameBuffer;

        void generateTestPattern(); // 生成测试图案
    };
}

#endif