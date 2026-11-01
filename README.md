# Parallel Programming

Two small C++ projects about threads and synchronization: measuring the speedup of a parallel computation, and a thread pool built from scratch with the standard library’s synchronization primitives.

| Project | What it shows |
| --- | --- |
| [Matrix-vector multiplication](matrix-vector-threads) | Splitting an independent computation across `std::thread`s and benchmarking the speedup against the number of threads and the data size |
| [Thread pool](thread-pool) | A worker-thread pool with one queue executed in cycles, built with `std::mutex` and `std::condition_variable`, with pause/resume and two shutdown modes |

## Build

Each project uses CMake:

```bash
cd matrix-vector-threads && cmake -B build && cmake --build build && ./build/benchmark
cd ../thread-pool       && cmake -B build && cmake --build build && ./build/thread_pool
```

## License

[MIT](LICENSE)
