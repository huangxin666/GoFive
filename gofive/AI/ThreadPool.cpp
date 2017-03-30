#include "ThreadPool.h"

int ThreadPool::num_thread = 2;
void ThreadPool::start()
{
    running_ = true;
    threads_.reserve(num_thread);
    for (int i = 0; i < num_thread; i++) {
        threads_.push_back(std::thread(std::bind(&ThreadPool::threadFunc, this)));
    }
}

void ThreadPool::stop()
{
    {
        unique_lock<std::mutex> ul(mutex_condition);
        running_ = false;
        notEmpty_task.notify_all();
    }

    for (auto &iter : threads_) {
        iter.join();
    }
}

void ThreadPool::run(Task t, bool origin)
{
    if (!threads_.empty()) {
        if (origin)
        {
            mutex_origin_queue.lock();
            queue_origin_task.push_back(t);
            if (num_working.load() == 0 && queue_origin_task.size() == 1)
            {
                notEmpty_task.notify_one();
            }
            mutex_origin_queue.unlock();

        }
        else
        {
            mutex_queue.lock();
            queue_task.push_back(t);
            if (queue_task.size() > GameTreeNode::maxTaskNum)
            {
                GameTreeNode::maxTaskNum = queue_task.size();
            }
            mutex_queue.unlock();
            notEmpty_task.notify_one();
        }
    }
}

void ThreadPool::threadFunc()
{
    while (running_) {
        Task task = take();
        if (task.node) {
            work(task);
            num_working--;
        }
    }
}

void ThreadPool::work(Task t)
{
    t.node->buildChild(true);//递归
    //t.node->buildChild(false);//不递归
    //size_t len = t.node->childs.size();
    //if (len > 0)
    //{
    //    Task task = t;
    //    for (size_t i = 0; i < len; ++i)
    //    {
    //        task.node = t.node->childs[i];
    //        run(task, false);
    //    }
    //}
}

Task ThreadPool::take()
{
    unique_lock<std::mutex> ul(mutex_condition);
    while (queue_origin_task.empty() && queue_task.empty() && running_) {
        notEmpty_task.wait(ul);
    }
    Task task = { NULL };
    //优先解决queue_task里面的
    mutex_queue.lock();
    if (!queue_task.empty()) {
        /*task = queue_task.front();
        queue_task.pop_front();*/
        task = queue_task.back();
        queue_task.pop_back();
        num_working++;
        mutex_queue.unlock();
    }
    else//若queue_task为空
    {
        mutex_queue.unlock();
        mutex_origin_queue.lock();
        if (num_working.load() == 0 && !queue_origin_task.empty())//queue_task为空并且没有线程在工作
        {
            task = queue_origin_task.front();
            queue_origin_task.pop_front();
            num_working++;
        }
        mutex_origin_queue.unlock();
    }



    return task;
}
