#ifndef THREADPOOL_H
#define THREADPOOL_H
#include "utils.h"
#include "TreeNode.h"
#include <mutex>
#include <condition_variable>
#include <deque>


struct Task
{
    TreeNode *node;
};

class ThreadPool
{
public:

    ThreadPool(int num)
        : num_(num)
        , maxQueueSize_(0)
        , running_(false)
    {
    }

    ~ThreadPool()
    {
        if (running_) {
            stop();
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    void operator=(const ThreadPool&) = delete;

    void setMaxQueueSize(int maxSize)
    {
        maxQueueSize_ = maxSize;
    }

    void start()
    {
        running_ = true;
        threads_.reserve(num_);
        for (int i = 0; i<num_; i++) {
            threads_.push_back(std::thread(std::bind(&ThreadPool::threadFunc, this)));
        }
    }

    void stop()
    {
        {
            std::unique_lock<std::mutex> ul(con_mutex_);
            running_ = false;
            notEmpty_.notify_all();
        }

        for (auto &iter : threads_) {
            iter.join();
        }
    }

    void run(const Task &t)
    {
        if (threads_.empty()) {//ÎÞÏß³Ì
            work(t);
        }
        else {
            std::unique_lock<std::mutex> ul(con_mutex_);
            while (isFull()) {
                notFull_.wait(ul);
            }
            queue_mutex_.lock();
            queue_.push_back(t);
            queue_mutex_.unlock();
            notEmpty_.notify_one();
        }
    }

private:
    bool isFull() const
    {
        return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
    }

    void threadFunc()
    {
        while (running_) {
            Task task = take();
            if (task.node) {
                work(task);
            }
        }
    }

    void work(const Task &t)
    {

    }

    Task take()
    {
        std::unique_lock<std::mutex> ul(con_mutex_);
        while (queue_.empty() && running_) {
            notEmpty_.wait(ul);
        }
        Task task;
        queue_mutex_.lock();
        if (!queue_.empty()) {
            task = queue_.front();
            queue_.pop_front();
            if (maxQueueSize_ > 0) {
                notFull_.notify_one();
            }
        }
        queue_mutex_.unlock();
        return task;
    }

private:
    int num_;
    std::mutex con_mutex_;
    std::mutex queue_mutex_;
    std::condition_variable notEmpty_;
    std::condition_variable notFull_;
    std::vector<std::thread> threads_;
    std::deque<Task> queue_;
    size_t maxQueueSize_;
    bool running_;
};

#endif // !THREADPOOL_H
