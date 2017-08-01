#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__
#include <mutex>
#include <condition_variable>
#include <deque>
#include <thread>
#include <atomic>
#include <vector>
using namespace std;

typedef std::function<void()> Task;

class ThreadPool
{
public:

    ThreadPool()
        : running(false)
        , num_working(0)
    { };

    ~ThreadPool()
    {
        if (running)
        {
            stop();
        }
    };

    ThreadPool(const ThreadPool&) = delete;
    void operator=(const ThreadPool&) = delete;

    void start(int num);

    void stop();

    void run(Task t, bool prior = true);

    void wait();

    static ThreadPool* getInstance()
    {
        static ThreadPool* instance = NULL;
        if (instance == NULL)
        {
            instance = new ThreadPool();
        }
        return instance;
    }

    int num_thread;

private:

    void threadFunc();

    //void work(Task t);

    Task take();

private:
    atomic<int> num_working;
    mutex mutex_condition;
    mutex mutex_priority_queue;
    mutex mutex_queue;
    condition_variable notEmpty_task;
    vector<thread> threads;
    deque<Task> task_priority_queue;
    deque<Task> task_queue;
    bool running;
};

#endif // !THREADPOOL_H
