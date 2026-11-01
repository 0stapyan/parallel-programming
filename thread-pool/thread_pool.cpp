#include "thread_pool.h"

using Clock = std::chrono::steady_clock;

ThreadPool::ThreadPool(int workerCount, std::chrono::seconds cycleInterval)
    : workerCount_(workerCount), cycleInterval_(cycleInterval) {
    for (int i = 0; i < workerCount_; ++i) {
        workers_.emplace_back(&ThreadPool::worker, this);
    }
    scheduler_ = std::thread(&ThreadPool::runCycles, this);
}

ThreadPool::~ThreadPool() {
    // Default shutdown finishes the tasks already running in the current cycle.
    stop(StopMode::FinishQueued);
}

bool ThreadPool::addTask(Task task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // Discarded if the pool is stopping, paused, or a cycle is running
        // (tasks that arrive mid-cycle are dropped in this variant).
        if (stopping_ || paused_ || cycleActive_) {
            tasksDiscarded_++;
            return false;
        }
        incoming_.push(std::move(task));
    }
    return true;
}

void ThreadPool::pause() {
    std::lock_guard<std::mutex> lock(mutex_);
    paused_ = true;
}

void ThreadPool::resume() {
    std::lock_guard<std::mutex> lock(mutex_);
    paused_ = false;
}

void ThreadPool::stop(StopMode mode) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) return;
        stopping_ = true;
        dropRemaining_ = (mode == StopMode::Immediate);
        if (dropRemaining_) {
            // Drop whatever has not started yet; running tasks still finish.
            std::queue<Task>().swap(running_);
            std::queue<Task>().swap(incoming_);
        }
    }
    workAvailable_.notify_all();
    cycleDone_.notify_all();

    if (scheduler_.joinable()) scheduler_.join();
    for (std::thread& worker : workers_) {
        if (worker.joinable()) worker.join();
    }
}

void ThreadPool::runCycles() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!stopping_) {
        // Wait for the cycle interval, but wake up early if the pool is stopping.
        cycleDone_.wait_for(lock, cycleInterval_, [this] { return stopping_; });
        if (stopping_) break;
        if (paused_ || incoming_.empty()) continue;

        // Start a cycle: the incoming buffer becomes the running buffer, and a
        // fresh incoming buffer collects tasks for the next cycle (double buffering).
        running_.swap(incoming_);
        queueLengthSum_ += static_cast<long long>(running_.size());
        queueSamples_++;
        cycleActive_ = true;
        workAvailable_.notify_all();

        // Wait until the running buffer is drained and all workers are idle.
        cycleDone_.wait(lock, [this] {
            return stopping_ || (running_.empty() && busyWorkers_ == 0);
        });
        cycleActive_ = false;
    }
}

void ThreadPool::worker() {
    while (true) {
        Task task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            auto waitStart = Clock::now();
            workAvailable_.wait(lock, [this] {
                return stopping_ || (cycleActive_ && !running_.empty());
            });
            totalWaitMicros_ += std::chrono::duration_cast<std::chrono::microseconds>(
                                    Clock::now() - waitStart).count();

            if (stopping_ && (dropRemaining_ || running_.empty())) {
                return;
            }
            if (running_.empty()) {
                continue;  // woken for stop with tasks left only when finishing queued
            }

            task = std::move(running_.front());
            running_.pop();
            busyWorkers_++;
        }

        auto taskStart = Clock::now();
        task();
        totalTaskMicros_ += std::chrono::duration_cast<std::chrono::microseconds>(
                                Clock::now() - taskStart).count();
        tasksExecuted_++;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            busyWorkers_--;
            if (running_.empty() && busyWorkers_ == 0) {
                cycleDone_.notify_one();  // the cycle is finished
            }
        }
    }
}

PoolStats ThreadPool::stats() {
    std::lock_guard<std::mutex> lock(mutex_);
    PoolStats s;
    s.threadsCreated = workerCount_;
    s.tasksExecuted = tasksExecuted_.load();
    s.tasksDiscarded = tasksDiscarded_.load();

    long long waits = s.tasksExecuted;
    s.averageWaitSeconds = waits > 0 ? (totalWaitMicros_.load() / 1e6) / workerCount_ : 0.0;
    s.averageTaskSeconds = s.tasksExecuted > 0
                               ? (totalTaskMicros_.load() / 1e6) / s.tasksExecuted : 0.0;
    s.averageQueueLength = queueSamples_ > 0
                               ? static_cast<double>(queueLengthSum_) / queueSamples_ : 0.0;
    return s;
}
