#ifndef THREADPOOL_H
#define THREADPOOL_H
#include "GameTree.h"
#include <mutex>
#include <condition_variable>
#include <deque>
#include <thread>
#include <atomic>
using namespace std;

class ThreadPool
{
public:

    ThreadPool(int num)
        : num_thread(num)
        , running_(false)
        , num_working(0)
    { };

    ~ThreadPool()
    {
        if (running_) {
            stop();
        }
    };

    ThreadPool(const ThreadPool&) = delete;
    void operator=(const ThreadPool&) = delete;

    inline size_t getTaskNum()
    {
        return queue_task.size() + queue_origin_task.size();
    }
    inline int getWorkNum()
    {
        return num_working.load();
    }

    void start();

    void stop();

    void run(Task t,bool origin=true);

private:

    void threadFunc();

    void work(Task t);

    Task take();

private:
    int num_thread;
    atomic<int> num_working;
    mutex mutex_condition;
    mutex mutex_queue;
    mutex mutex_origin_queue;
    condition_variable notEmpty_task;
    //condition_variable notEmpty_origin_task;
    vector<thread> threads_;
    deque<Task> queue_task;
    deque<Task> queue_origin_task;
    bool running_;
};

#endif // !THREADPOOL_H
