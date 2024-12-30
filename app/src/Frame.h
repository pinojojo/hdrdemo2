#ifndef FRAME_H
#define FRAME_H

#include <vector>

namespace lzx
{
    class Frame
    {
    public:
        // 无参 构造函数
        Frame() : m_width(0), m_height(0), m_channels(0), m_bitDepth(8)
        {
        }

        // 带参，全0填充
        Frame(int width, int height, int channels, int bitDepth = 8)
            : m_width(width),
              m_height(height),
              m_channels(channels),
              m_bitDepth(bitDepth),
              m_data(width * height * channels * (bitDepth > 8 ? 2 : 1), 0)
        {
        }

        // 带参
        Frame(int width, int height, int channels, const std::vector<unsigned char> &data, int bitDepth = 8)
            : m_width(width),
              m_height(height),
              m_channels(channels),
              m_bitDepth(bitDepth), m_data(data)
        {
        }

        ~Frame() = default;

        int width() const { return m_width; }
        int height() const { return m_height; }
        int channels() const { return m_channels; }
        int bitDepth() const { return m_bitDepth; }
        size_t sn() const { return m_sequenceNumber; }
        void setSequenceNumber(size_t sn) { m_sequenceNumber = sn; }

        void fill(const std::vector<unsigned char> &color)
        {
            size_t bytesPerPixelComponent = (m_bitDepth > 8) ? 2 : 1;
            if (color.size() != m_channels * bytesPerPixelComponent)
            {
                return;
            }

            for (int i = 0; i < m_width * m_height; i++)
            {
                for (int j = 0; j < m_channels; j++)
                {
                    for (size_t b = 0; b < bytesPerPixelComponent; b++)
                    {
                        m_data[i * m_channels * bytesPerPixelComponent + j * bytesPerPixelComponent + b] =
                            color[j * bytesPerPixelComponent + b];
                    }
                }
            }
        }

        void fill(const unsigned char *color)
        {
            size_t bytesPerPixel = m_channels * (m_bitDepth > 8 ? 2 : 1);
            memcpy(m_data.data(), color, m_width * m_height * bytesPerPixel);
        }

        const unsigned char *data() const { return m_data.data(); }

        static size_t sequenceNumber() { return s_sequenceNumber; }

        // Function to get a pointer to the internal buffer, used for writing data to the buffer
        unsigned char *buffer()
        {
            return m_data.data();
        }

        // Function to get the size of the internal buffer
        size_t bufferSize() const
        {
            return m_data.size();
        }

        // const function to get a pointer to the internal buffer, used for reading data from the buffer
        const unsigned char *buffer() const
        {
            return m_data.data();
        }

    private:
        int m_width;
        int m_height;
        int m_channels;
        int m_bitDepth;
        std::vector<unsigned char> m_data;

        static size_t s_sequenceNumber;

        size_t m_sequenceNumber = 0;
    };
}

#endif