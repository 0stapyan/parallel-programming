#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

// A task is any callable with no arguments. In this variant a task simply
// sleeps for a random number of seconds to simulate work.
using Task = std::function<void()>;

// Statistics collected while the pool runs (variant 4 uses unlimited queues).
struct PoolStats {
    int threadsCreated = 0;        // worker threads started over the pool's life
    long long tasksExecuted = 0;
    long long tasksDiscarded = 0;  // rejected because they arrived mid-cycle
    double averageWaitSeconds = 0; // average time a worker spent idle, waiting
    double averageQueueLength = 0; // average queue size sampled at cycle starts
    double averageTaskSeconds = 0; // average task execution time
};

// How to stop the pool when it is destroyed or stopped explicitly.
enum class StopMode {
    FinishQueued,  // run everything already queued, then stop
    Immediate,     // drop queued tasks; running tasks still finish
};

// Variant 4: a pool of worker threads with one queue. The queue is executed in
// cycles every 45 seconds (double buffering): tasks added during a cycle are
// discarded, not carried over. The pool can be paused and resumed, and stopped
// in either of the two modes above. All shared state is guarded by one mutex and
// two condition variables.
class ThreadPool {
public:
    ThreadPool(int workerCount, std::chrono::seconds cycleInterval);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Thread-safe. Returns false if the task was discarded (pool stopping, paused
    // past a cycle, or a cycle is currently running).
    bool addTask(Task task);

    void pause();
    void resume();

    // Stops the pool. FinishQueued runs the current buffer first; Immediate drops it.
    void stop(StopMode mode);

    PoolStats stats();

private:
    void runCycles();          // the scheduler thread: triggers a cycle every interval
    void worker();             // a worker thread: runs tasks from the active buffer

    const int workerCount_;
    const std::chrono::seconds cycleInterval_;

    std::vector<std::thread> workers_;
    std::thread scheduler_;

    std::mutex mutex_;
    std::condition_variable workAvailable_;  // wakes workers when a cycle starts
    std::condition_variable cycleDone_;      // wakes the scheduler when a cycle ends

    std::queue<Task> incoming_;   // tasks arriving between cycles
    std::queue<Task> running_;    // the buffer currently being executed
    bool cycleActive_ = false;
    int busyWorkers_ = 0;

    bool paused_ = false;
    bool stopping_ = false;
    bool dropRemaining_ = false;

    // Statistics
    std::atomic<long long> tasksExecuted_{0};
    std::atomic<long long> tasksDiscarded_{0};
    std::atomic<long long> totalWaitMicros_{0};
    std::atomic<long long> totalTaskMicros_{0};
    long long queueSamples_ = 0;
    long long queueLengthSum_ = 0;
};

#endif
