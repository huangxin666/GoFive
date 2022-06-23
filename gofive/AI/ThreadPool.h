#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__
#include <mutex>
#include <condition_variable>
#include <deque>
#include <thread>
#include <atomic>
#include <vector>
#include <functional>
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

    void start(int num)
    {
        num_thread = num;
        running = true;
        threads.reserve(num_thread);
        for (int i = 0; i < num_thread; i++) {
            threads.push_back(std::thread(std::bind(&ThreadPool::threadFunc, this)));
        }
    }

    void stop()
    {
        {
            unique_lock<std::mutex> ul(mutex_condition);
            running = false;
            notEmpty_task.notify_all();
        }

        for (auto &iter : threads) {
            iter.join();
        }
    }

    void run(Task t, bool prior = true)
    {
        if (!threads.empty()) {
            if (prior)
            {
                mutex_priority_queue.lock();
                task_priority_queue.push_back(t);
                mutex_priority_queue.unlock();
                notEmpty_task.notify_one();
            }
            else
            {
                mutex_queue.lock();
                task_queue.push_back(t);
                if (num_working.load() == 0 && task_queue.size() == 1)
                {
                    notEmpty_task.notify_one();
                }
                mutex_queue.unlock();
            }
        }
    }

    void wait()
    {
        while (true)
        {
            if (task_priority_queue.size() + task_queue.size() == 0 && num_working.load() == 0)
            {
                break;
            }
            this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

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

    void threadFunc()
    {
        while (running) {
            Task task = take();
            if (task) {
                task();
                num_working--;
            }
        }
    }

    //void work(Task t);

    Task take()
    {
        unique_lock<std::mutex> ul(mutex_condition);
        while (task_queue.empty() && task_priority_queue.empty() && running) {
            notEmpty_task.wait(ul);
        }
        Task task = NULL;
        //优先解决task_priority_queue里面的
        mutex_priority_queue.lock();
        if (!task_priority_queue.empty()) {
            task = task_priority_queue.front();
            task_priority_queue.pop_front();//先进先出
                                            //task = task_priority_queue.back();
                                            //task_priority_queue.pop_back();//先进后出
            num_working++;
            mutex_priority_queue.unlock();
        }
        else//若task_priority_queue为空
        {
            mutex_priority_queue.unlock();
            mutex_queue.lock();
            if (num_working.load() == 0 && !task_queue.empty())//task_priority_queue为空并且没有线程在工作
            {
                task = task_queue.front();
                task_queue.pop_front();
                num_working++;
            }
            mutex_queue.unlock();
        }

        return task;
    }

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
