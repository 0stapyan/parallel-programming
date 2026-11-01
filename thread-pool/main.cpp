// Demonstrates the thread pool (variant 4): 4 workers, one queue executed in
// cycles. Tasks are generated on a separate thread, each sleeping a random
// 6-12 seconds. Tasks that arrive while a cycle is running are discarded.
//
// A short cycle interval is used here so the demo finishes quickly; the
// assignment specifies 45 seconds, which is the default of the ThreadPool.
#include <atomic>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>

#include "thread_pool.h"

namespace {

std::mutex coutMutex;

void log(const std::string& message) {
    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << message << "\n";
}

}  // namespace

int main() {
    const int workerCount = 4;
    const auto cycleInterval = std::chrono::seconds(45);  // as required by the assignment
    const auto demoDuration = std::chrono::seconds(100);

    ThreadPool pool(workerCount, cycleInterval);
    log("Pool started: " + std::to_string(workerCount) + " workers, cycle every " +
        std::to_string(cycleInterval.count()) + "s.");

    // Task generator runs on its own thread, separate from the pool.
    std::atomic<bool> generating{true};
    std::thread generator([&] {
        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> duration(6, 12);
        int id = 1;
        while (generating) {
            int seconds = duration(rng);
            int taskId = id++;
            bool accepted = pool.addTask([taskId, seconds] {
                log("  Task " + std::to_string(taskId) + " running for " +
                    std::to_string(seconds) + "s");
                std::this_thread::sleep_for(std::chrono::seconds(seconds));
                log("  Task " + std::to_string(taskId) + " done");
            });
            log(std::string("Task ") + std::to_string(taskId) +
                (accepted ? " queued" : " discarded (cycle running)"));
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    });

    std::this_thread::sleep_for(demoDuration);

    log("Stopping task generation and finishing the current cycle...");
    generating = false;
    generator.join();
    pool.stop(StopMode::FinishQueued);

    PoolStats s = pool.stats();
    std::cout << "\n--- Statistics ---\n";
    std::cout << "Worker threads created: " << s.threadsCreated << "\n";
    std::cout << "Tasks executed:         " << s.tasksExecuted << "\n";
    std::cout << "Tasks discarded:        " << s.tasksDiscarded << "\n";
    std::cout << "Average queue length:   " << s.averageQueueLength << " tasks per cycle\n";
    std::cout << "Average task time:      " << s.averageTaskSeconds << " s\n";
    std::cout << "Average worker wait:    " << s.averageWaitSeconds << " s\n";
    return 0;
}
