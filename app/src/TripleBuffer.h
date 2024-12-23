#pragma once

#include <atomic>
#include <array>
#include <optional>

namespace lzx
{
    template <typename T>
    class TripleBuffer
    {
    private:
        std::array<T, 3> buffers;                // 每个缓冲区存储一个T类型对象
        std::atomic<int> currentProduceIndex{0}; // 当前生产索引
        std::atomic<int> currentConsumeIndex{2}; // 当前消费索引
        std::atomic<int> lastConsumedIndex{-1};  // 最后消费的索引
        int reservedIndex{1};                    // 保留索引

    public:
        // 生产者使用移动语义添加数据
        void produce(T &&data)
        {
            int produceIndex = currentProduceIndex.load(std::memory_order_acquire);
            buffers[produceIndex] = std::move(data); // 使用 std::move

            // 尝试交换缓冲区
            int nextIndex = (produceIndex + 1) % 3;
            if (nextIndex != currentConsumeIndex.load(std::memory_order_acquire))
            {
                reservedIndex = produceIndex;
                currentProduceIndex.store(nextIndex, std::memory_order_release);
            }
        }

        // 生产者使用复制语义添加数据
        void produce(const T &data)
        {
            int produceIndex = currentProduceIndex.load(std::memory_order_acquire);
            buffers[produceIndex] = data; // 使用复制语义

            // 尝试交换缓冲区
            int nextIndex = (produceIndex + 1) % 3;
            if (nextIndex != currentConsumeIndex.load(std::memory_order_acquire))
            {
                reservedIndex = produceIndex;
                currentProduceIndex.store(nextIndex, std::memory_order_release);
            }
        }

        // 消费者获取数据指针的方法
        T *consume()
        {
            int consumeIndex = currentConsumeIndex.load(std::memory_order_acquire);
            if (consumeIndex == currentProduceIndex.load(std::memory_order_acquire))
            {
                return nullptr; // 没有新数据可供消费
            }

            lastConsumedIndex.store(consumeIndex, std::memory_order_release); // 更新最后消费的缓冲区索引
            return &buffers[consumeIndex];
        }

        // 消费完成的方法
        void consumeDone()
        {
            int consumedIndex = lastConsumedIndex.load(std::memory_order_acquire);
            if (consumedIndex == -1)
            {
                return; // 没有正在消费的缓冲区
            }

            // 尝试交换缓冲区
            int nextIndex = (consumedIndex + 1) % 3;
            if (nextIndex != currentProduceIndex.load(std::memory_order_acquire))
            {
                currentConsumeIndex.store(nextIndex, std::memory_order_release);
                lastConsumedIndex.store(-1, std::memory_order_release); // 重置最后消费的缓冲区索引
            }
        }
    };
}