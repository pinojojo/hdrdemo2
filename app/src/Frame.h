#ifndef FRAME_H
#define FRAME_H

#include <vector>

namespace lzx
{
    class Frame
    {
    public:
        Frame() : m_width(0), m_height(0), m_channels(0)
        {
            s_sequenceNumber++;
            m_sequenceNumber = s_sequenceNumber;
        }

        Frame(int width, int height, int channels)
            : m_width(width), m_height(height), m_channels(channels), m_data(width * height * channels, 0)
        {
            s_sequenceNumber++;
            m_sequenceNumber = s_sequenceNumber;
        }

        Frame(int width, int height, int channels, const std::vector<unsigned char> &data)
            : m_width(width), m_height(height), m_channels(channels), m_data(data)
        {
            s_sequenceNumber++;
            m_sequenceNumber = s_sequenceNumber;
        }

        ~Frame() = default;

        int width() const { return m_width; }
        int height() const { return m_height; }
        int channels() const { return m_channels; }
        int sn() const { return m_sequenceNumber; }

        void fill(const std::vector<unsigned char> &color)
        {
            if (color.size() != m_channels)
            {
                return;
            }
            else
            {
                for (int i = 0; i < m_width * m_height; i++)
                {
                    for (int j = 0; j < m_channels; j++)
                    {
                        m_data[i * m_channels + j] = color[j];
                    }
                }
            }
        }

        void fill(const unsigned char *color)
        {
            memcpy(m_data.data(), color, m_width * m_height * m_channels);
        }

        const unsigned char *data() const { return m_data.data(); }

        static size_t sequenceNumber() { return s_sequenceNumber; }

        // Function to get a pointer to the internal buffer
        unsigned char *buffer()
        {
            return m_data.data();
        }

        // Function to get the size of the internal buffer
        size_t bufferSize() const
        {
            return m_data.size();
        }

        // Function to get a const pointer to the internal buffer (for read-only access)
        const unsigned char *buffer() const
        {
            return m_data.data();
        }

    private:
        int m_width;
        int m_height;
        int m_channels;
        std::vector<unsigned char> m_data;

        static size_t s_sequenceNumber;

        size_t m_sequenceNumber = 0;
    };
}

#endif