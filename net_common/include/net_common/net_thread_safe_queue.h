#pragma once
#include <deque>
#include <mutex>

namespace net
{
template <typename T>
class thread_safe_queue
{
public:
    thread_safe_queue() = default;
    thread_safe_queue(const thread_safe_queue<T>&) = delete;
    virtual ~thread_safe_queue() { clear(); }
public:
    const T& front()
    {
        std::scoped_lock lock(muxQueue);
        return deqQueue.front();
    }

    const T& back()
    {
        std::scoped_lock lock(muxQueue);
        return deqQueue.back();
    }

    void push_back(const T& item)
    {
        std::scoped_lock lock(muxQueue);
        deqQueue.emplace_back(std::move(item));

        // Notify any threads waiting on data.
        std::scoped_lock lockBlocking(muxBlocking);
        cvBlocking.notify_one();
    }

    void push_front(const T& item)
    {
        std::scoped_lock lock(muxQueue);
        deqQueue.emplace_front(std::move(item));

        // Notify any threads waiting on data.
        std::scoped_lock lockBlocking(muxBlocking);
        cvBlocking.notify_one();
    }

    bool empty()
    {
        std::scoped_lock lock(muxQueue);
        return deqQueue.empty();
    }

    size_t count()
    {
        std::scoped_lock lock(muxQueue);
        return deqQueue.size();
    }

    void clear()
    {
        std::scoped_lock lock(muxQueue);
        deqQueue.clear();
    }

    T pop_front()
    {
        std::scoped_lock lock(muxQueue);
        auto t = std::move(deqQueue.front());
        deqQueue.pop_front();
        return t;
    }

    T pop_back()
    {
        std::scoped_lock lock(muxQueue);
        auto t = std::move(deqQueue.back());
        deqQueue.pop_back();
        return t;
    }

    // Wait until the queue is not empty, then pop the front item.
    void wait()
    {
        while (empty())
        {
            std::unique_lock<std::mutex> lock(muxBlocking);
            cvBlocking.wait(lock);
        }
    }
protected:
    // Mutex to protect the double-ended queue.
    std::mutex muxQueue;
    // Double-ended queue to hold the data.
    std::deque<T> deqQueue;
    // Condition variable to block the thread until the queue has data.
    std::condition_variable cvBlocking;
    // Mutex to protect the condition variable.
    std::mutex muxBlocking;
};
} // namespace net