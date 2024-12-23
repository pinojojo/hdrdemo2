#include "spinlock.hpp"
#include <vector>
class ThreadSafeImage
{
private:
    std::vector<unsigned char> data;
    std::atomic<bool> updated = false;
    NonBlockingSpinLock lock;

public:
    // ctor
    ThreadSafeImage(int width, int height, int channels = 3)
    {
        data.resize(width * height * channels);
    }

    // try produce
    // if the image is locked, return false
    // otherwise, copy the image data to the buffer and return true
    unsigned char *tryLockForProduce()
    {
        return tryLock();
    }

    void unlcokForProduce()
    {
        updated.store(true, std::memory_order_release);
        unlock();
    }

    // try consume the image
    // if the image is not updated, return false
    // otherwise, copy the image data to the buffer and return true
    unsigned char *tryLockForConsume()
    {
        // check updated flag
        if (!updated.load(std::memory_order_acquire))
        {
            return nullptr;
        }

        // copy the image data
        return tryLock();
    }

    void unlockForConsume()
    {
        updated.store(false, std::memory_order_release);
        unlock();
    }

    // resize
    void resize(int width, int height, int channels = 3)
    {
        lock.lock();
        data.resize(width * height * channels);
        lock.unlock();
    }

    size_t size()
    {
        return data.size();
    }

private:
    // try get the pointer of the image data
    // if the image is locked, return nullptr
    // otherwise, return the pointer of the image data
    unsigned char *tryLock()
    {
        if (lock.tryLock())
        {
            return data.data();
        }
        else
        {
            return nullptr;
        }
    }
    // unlock the image
    void unlock()
    {
        lock.unlock();
    }
};