// Benchmarks sequential vs parallel matrix-vector multiplication.
//
// For every matrix size and thread count required by the assignment it measures
// the execution time and prints a table and a CSV file (results.csv) that the
// charts in the report are built from.
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <algorithm>
#include <functional>
#include <iomanip>
#include <vector>

#include "matrix_vector.h"

namespace {

long long timeMicros(const std::function<void()>& work) {
    auto start = std::chrono::high_resolution_clock::now();
    work();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

}  // namespace

int main() {
    const std::vector<int> sizes = {50, 100, 500, 1000, 10000};

    // The assignment asks for a fixed set of thread counts relative to the number
    // of logical cores. On a typical 8-core machine this is 4, 8, 16, 32, 64, 128;
    // 1 thread is added as the baseline.
    unsigned int cores = std::thread::hardware_concurrency();
    if (cores == 0) cores = 8;
    std::cout << "Logical cores detected: " << cores << "\n\n";

    // Thread counts from the assignment: half the cores, the number of cores,
    // and 2/4/8/16 times the cores, with 1 as the baseline. Duplicates and
    // zeros (possible when there are very few cores) are removed.
    std::vector<int> threadCounts;
    for (int t : {1, static_cast<int>(cores) / 2, static_cast<int>(cores),
                  static_cast<int>(cores) * 2, static_cast<int>(cores) * 4,
                  static_cast<int>(cores) * 8, static_cast<int>(cores) * 16}) {
        if (t >= 1 && std::find(threadCounts.begin(), threadCounts.end(), t) == threadCounts.end()) {
            threadCounts.push_back(t);
        }
    }

    std::ofstream csv("results.csv");
    csv << "n,sequential_us";
    for (int t : threadCounts) csv << ",parallel_" << t;
    csv << "\n";

    std::cout << "All times in microseconds.\n\n";
    std::cout << "    n | sequential |";
    for (int t : threadCounts) std::cout << " p" << t << "\t";
    std::cout << "\n";

    for (int n : sizes) {
        Matrix matrix = generateMatrix(n);
        Vector vec = generateVector(n);

        long long seq = timeMicros([&] { volatile auto r = multiplySequential(matrix, vec); (void)r; });

        csv << n << "," << seq;
        std::cout << std::setw(5) << n << " | " << std::setw(10) << seq << " |";

        for (int t : threadCounts) {
            long long par = timeMicros([&] {
                volatile auto r = multiplyParallel(matrix, vec, t);
                (void)r;
            });
            csv << "," << par;
            std::cout << " " << par << "\t";
        }
        csv << "\n";
        std::cout << "\n";
    }

    std::cout << "\nResults written to results.csv\n";
    return 0;
}
