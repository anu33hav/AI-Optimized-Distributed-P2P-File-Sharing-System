#include "peer/threadPool.h"
using namespace std;

ThreadPool::ThreadPool(int threadCount) : stop(false), activeTasks(0) {
    for (int i = 0; i < threadCount; i++) {
        threads.push_back(thread(&ThreadPool::worker, this));
    }
}

void ThreadPool::addTask(function<void()> task) {
    {
        lock_guard<mutex> lock(mtx);
        tasks.push(task);
    }

    // notify any thread
    cv.notify_one();
}

void ThreadPool::wait() {

    unique_lock<mutex> lock(mtx);
    // no task, go to sleep
    doneCv.wait(lock, [this]() {
        return tasks.empty() && activeTasks == 0;
    });
}

void ThreadPool::worker() {

    while (true) {
        function<void()> task;

        {
            unique_lock<mutex> lock(mtx);
            // sleep till 
            cv.wait(lock, [this]() {
                return stop || !tasks.empty();
            });
            // no tasks
            if (stop && tasks.empty()) {
                return;
            }

            task = tasks.front();
            tasks.pop();
            activeTasks++;
        }
        
        // execute task
        task();

        {
            lock_guard<mutex> lock(mtx);
            activeTasks--;
            if (tasks.empty() && activeTasks == 0) {
                doneCv.notify_all();
            }
        }
    }
}

ThreadPool::~ThreadPool() {

    {
        lock_guard<mutex> lock(mtx);
        stop = true;
    }

    cv.notify_all();

    for (auto &t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}