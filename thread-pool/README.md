# Thread Pool

A thread pool written from scratch in C++ with `std::mutex` and `std::condition_variable`. Worker threads are created once and reused, so the cost of starting a thread is not paid per task.

## Behaviour

The pool has a fixed number of worker threads and one task queue, and it executes the queue in **cycles**: every 45 seconds the tasks collected so far are run, while new tasks are gathered for the next cycle (double buffering). Tasks that arrive while a cycle is running are **discarded**. Each task here sleeps for a random 6–12 seconds to simulate work.

- **Worker threads** wait on a condition variable until a cycle starts, then take tasks from the active buffer until it is empty.
- **The scheduler thread** wakes every cycle interval, swaps the incoming buffer into the running buffer, wakes the workers, and waits until the buffer is drained and all workers are idle before ending the cycle.
- Initialising and destroying the pool, and adding and taking tasks, are all guarded by one mutex, so every operation is thread-safe.
- The code that generates tasks runs on a different thread from the pool itself.

### Control

- **Pause / resume** — while paused, the pool starts no new cycle and rejects new tasks.
- **Two shutdown modes:**
  - *FinishQueued* (the default, also used by the destructor) runs the current buffer to completion, then stops.
  - *Immediate* drops everything not yet started; tasks already running still finish.

### Statistics

When it stops, the pool reports the number of worker threads created, how many tasks ran and how many were discarded, the average queue length per cycle, the average task execution time, and the average time a worker spent waiting.

## Build and run

```bash
cmake -B build && cmake --build build
./build/thread_pool
```

The demo generates tasks for a while, then stops the pool and prints the statistics. It uses a 45-second cycle as specified; shorten `cycleInterval` in `main.cpp` to watch several cycles quickly.

## Correctness

The pool was checked under `-fsanitize=thread` (ThreadSanitizer) with no data races reported, and both shutdown modes and pause/resume were verified with separate tests.

## Files

| File | Purpose |
| --- | --- |
| `thread_pool.h/.cpp` | The pool: workers, scheduler, synchronization, statistics |
| `main.cpp` | Task generator and demo that prints the statistics |
