#include <atomic>
#include <thread>

class NonBlockingSpinLock
{
private:
    std::atomic_flag lock_ = ATOMIC_FLAG_INIT;

public:
    // 尝试获取锁，如果锁被占用，立即返回 false
    bool tryLock()
    {
        return !lock_.test_and_set(std::memory_order_acquire);
    }

    // 阻塞版本的 tryLock
    void lock()
    {
        while (!tryLock())
        {
            std::this_thread::yield();
        }
    }

    void unlock()
    {
        lock_.clear(std::memory_order_release);
    }
};
