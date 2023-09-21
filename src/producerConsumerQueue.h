#ifndef _PRODUCERCONSUMERQUEUE_H_
#define _PRODUCERCONSUMERQUEUE_H_

#include <mutex>
#include <limits>
#include <queue>
#include <condition_variable>
#include <chrono>

using namespace std::chrono_literals;

template <typename Type>
class ProducerConsumerQueue
{
public:
    ProducerConsumerQueue() {}
    ~ProducerConsumerQueue()
    {
    }

    // 清空队列，唤醒所有等待条件变量的线程
    void stop()
    {
        std::lock_guard<std::mutex> lock(mu_);
        q_.clear();
        not_empty_.notify_one();
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(mu_);
        q_.clear();
    }

    void push(const Type &value)
    {
        std::lock_guard<std::mutex> lock(mu_);
        q_.push(value);
        not_empty_.notify_one();
    }

    // block until success
    Type get()
    {
        std::unique_lock<std::mutex> lock(mu_);
        not_empty_.wait(lock, [this]
                        { return !this->q_.empty(); });
        Type item;
        tryGetInternal(&item);
        return item;
    }

    bool get(Type *item, const int64_t timeout_ms = 0)
    {
        std::unique_lock<std::mutex> lock(mu_);
        if (not_empty_.wait_for(lock, timeout_ms * 1ms, [this]
                                { return !this->q_.empty(); }))
        {
            return tryGetInternal(item);
        }
        return false;
    }

    // not block
    bool tryGet(Type *item)
    {
        std::unique_lock<std::mutex> lock(mu_);
        return tryGetInternal(item);
    }

private:
    // Lock-free function
    bool tryGetInternal(Type *item)
    {
        if (q_.empty())
        {
            return false;
        }
        *item = q_.front();
        q_.pop();
        return true;
    }

    // Lock-free function
    bool TryPushInternal(const Type &item)
    {
        q_.push_back(item);
        not_empty_.notify_one();
        return true;
    }

    std::queue<Type> q_;
    // 访问q_的锁
    std::mutex mu_;
    std::condition_variable not_empty_;
};

#endif // _PRODUCERCONSUMERQUEUE_H_