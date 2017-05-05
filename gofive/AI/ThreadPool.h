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
        : running(false)
        , num_working(0)
    { };

    ~ThreadPool()
    {
        if (running) {
            stop();
        }
    };

    ThreadPool(const ThreadPool&) = delete;
    void operator=(const ThreadPool&) = delete;

    void start();

    void stop();

    void run(Task t, bool prior = true);

    void wait();

    static int num_thread;

private:

    void threadFunc();

    void work(Task t);

    Task take();

private:
    atomic<int> num_working;
    mutex mutex_condition;
    mutex mutex_priority_queue;
    mutex mutex_queue;
    condition_variable notEmpty_task;
    //condition_variable notEmpty_origin_task;
    vector<thread> threads;
    deque<Task> task_priority_queue;
    deque<Task> task_queue;
    bool running;
};

#endif // !THREADPOOL_H
