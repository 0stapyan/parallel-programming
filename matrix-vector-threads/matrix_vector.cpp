#include "matrix_vector.h"

#include <random>
#include <thread>

namespace {

std::mt19937& rng() {
    static thread_local std::mt19937 engine(std::random_device{}());
    return engine;
}

int randomValue() {
    std::uniform_int_distribution<int> dist(1, 100);
    return dist(rng());
}

// Multiplies the rows [startRow, endRow) and writes them into result.
// Each thread owns a disjoint range of rows, so no locking is needed.
void multiplyRows(const Matrix& matrix, const Vector& vec, Vector& result,
                  int startRow, int endRow) {
    int n = static_cast<int>(vec.size());
    for (int i = startRow; i < endRow; ++i) {
        long long sum = 0;
        for (int j = 0; j < n; ++j) {
            sum += static_cast<long long>(matrix[i][j]) * vec[j];
        }
        result[i] = static_cast<int>(sum);
    }
}

}  // namespace

Matrix generateMatrix(int n) {
    Matrix matrix(n, Vector(n));
    for (auto& row : matrix) {
        for (int& value : row) {
            value = randomValue();
        }
    }
    return matrix;
}

Vector generateVector(int n) {
    Vector vec(n);
    for (int& value : vec) {
        value = randomValue();
    }
    return vec;
}

Vector multiplySequential(const Matrix& matrix, const Vector& vec) {
    int n = static_cast<int>(matrix.size());
    Vector result(n, 0);
    multiplyRows(matrix, vec, result, 0, n);
    return result;
}

Vector multiplyParallel(const Matrix& matrix, const Vector& vec, int threadCount) {
    int n = static_cast<int>(matrix.size());
    Vector result(n, 0);

    if (threadCount < 1) threadCount = 1;
    if (threadCount > n) threadCount = n;   // no point in more threads than rows

    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    int chunk = n / threadCount;
    int remainder = n % threadCount;

    int startRow = 0;
    for (int t = 0; t < threadCount; ++t) {
        // Spread the remainder one row per thread, so chunks differ by at most 1
        int rows = chunk + (t < remainder ? 1 : 0);
        int endRow = startRow + rows;
        threads.emplace_back(multiplyRows, std::cref(matrix), std::cref(vec),
                             std::ref(result), startRow, endRow);
        startRow = endRow;
    }

    for (auto& thread : threads) {
        thread.join();
    }
    return result;
}
