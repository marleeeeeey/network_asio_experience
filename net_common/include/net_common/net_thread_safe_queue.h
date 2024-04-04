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
    }

    void push_front(const T& item)
    {
        std::scoped_lock lock(muxQueue);
        deqQueue.emplace_front(std::move(item));
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
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
protected:
    std::mutex muxQueue; // protect the double-ended queue.
    std::deque<T> deqQueue; // double-ended queue.
};
} // namespace net