#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <functional>

class ThreadPool {
private:
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;
    std::mutex mtx;
    std::condition_variable cv;
    std::condition_variable doneCv;
    bool stop;
    int activeTasks;

public:
    ThreadPool(int threadCount);

    void addTask(std::function<void()> task);
    void wait();

private:
    void worker();

public:
    ~ThreadPool();
};