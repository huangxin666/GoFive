#include "ThreadPool.h"

int ThreadPool::num_thread = 2;
void ThreadPool::start()
{
    running = true;
    threads.reserve(num_thread);
    for (int i = 0; i < num_thread; i++) {
        threads.push_back(std::thread(std::bind(&ThreadPool::threadFunc, this)));
    }
}

void ThreadPool::stop()
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

void ThreadPool::run(Task t, bool prior)
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

void ThreadPool::wait()
{
    while (true)
    {
        if (task_priority_queue.size() + task_queue.size() == 0 && num_working.load() == 0)
        {
            break;
        }
        this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void ThreadPool::threadFunc()
{
    while (running) {
        Task task = take();
        if (task) {
            task();
            num_working--;
        }
    }
}

Task ThreadPool::take()
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
