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

    ThreadPool()
        : running_(false)
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

    void start();

    void stop();

    void run(Task t, bool origin = true);

    void wait();

    static int num_thread;

private:

    void threadFunc();

    void work(Task t);

    Task take();

private:
    atomic<int> num_working;
    mutex mutex_condition;
    mutex mutex_queue;
    mutex mutex_origin_queue;
    mutex mutex_map;
    condition_variable notEmpty_task;
    //condition_variable notEmpty_origin_task;
    vector<thread> threads_;
    deque<Task> task_priority_queue;
    deque<Task> task_queue;
    bool running_;
};

#endif // !THREADPOOL_H
