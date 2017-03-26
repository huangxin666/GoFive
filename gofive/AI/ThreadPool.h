#ifndef THREADPOOL_H
#define THREADPOOL_H
#include "utils.h"
#include "TreeNode.h"
#include <mutex>
#include <condition_variable>
#include <deque>


struct Task
{
    //bool *hasSearch;//用于记录是否完成一条线路
    //ChildInfo* bestChild;//需要加锁？
    TreeNode *node;//任务需要计算的节点
    //int currentScore;//节点对应的最开始节点的分数，用来计算bestChild
    int index;//节点对应的最开始节点的索引
};

class ThreadPool
{
public:

    ThreadPool(int num)
        : num_(num)
        //, maxQueueSize_(0)
        , running_(false)
        , work_num(0)
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

    /*void setMaxQueueSize(int maxSize)
    {
        maxQueueSize_ = maxSize;
    }*/

    void start()
    {
        running_ = true;
        threads_.reserve(num_);
        for (int i = 0; i < num_; i++) {
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

    void run(Task t)
    {
        if (threads_.empty()) {//无线程
            work(t);
        }
        else {
            std::unique_lock<std::mutex> ul(con_mutex_);
            /*while (isFull()) {
                notFull_.wait(ul);
            }*/
            queue_mutex_.lock();
            queue_.push_back(t);
            queue_mutex_.unlock();
            notEmpty_.notify_one();
        }
    }

    size_t getTaskNum()
    {
        return queue_.size();
    }
    int getWorkNum()
    {
        return work_num;
    }

private:
    /*bool isFull() const
    {
        return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
    }*/

    void threadFunc()
    {
        while (running_) {
            Task task = take();
            if (task.node) {
                work(task);
                work_num_mutex_.lock();
                work_num--;
                work_num_mutex_.unlock();
            }
        }
    }

    void work(Task t)
    {

        t.node->buildChild(false);//不递归
        size_t len = t.node->childs.size();
        if (len > 0)
        {
            Task task = t;
            for (size_t i = 0; i < len; ++i)
            {
                task.node = t.node->childs[i];
                run(task);
            }
        }
    }

    Task take()
    {
        std::unique_lock<std::mutex> ul(con_mutex_);
        while (queue_.empty() && running_) {
            notEmpty_.wait(ul);
        }
        Task task = { NULL,-1 };
        queue_mutex_.lock();
        if (!queue_.empty()) {
            task = queue_.front();
            queue_.pop_front();

            /*if (maxQueueSize_ > 0) {
                notFull_.notify_one();
            }*/
        }
        else
        {
            queue_mutex_.unlock();
            return task;
        }
        queue_mutex_.unlock();
        work_num_mutex_.lock();
        work_num++;
        work_num_mutex_.unlock();
        return task;
    }

private:
    int num_;
    int work_num;
    std::mutex con_mutex_;
    std::mutex queue_mutex_;
    std::mutex work_num_mutex_;
    std::condition_variable notEmpty_;
    //std::condition_variable notFull_;
    std::vector<std::thread> threads_;
    std::deque<Task> queue_;
    //size_t maxQueueSize_;
    bool running_;
};

#endif // !THREADPOOL_H
