#ifndef MATRIX_VECTOR_H
#define MATRIX_VECTOR_H

#include <vector>

using Matrix = std::vector<std::vector<int>>;
using Vector = std::vector<int>;

// Fills an n x n matrix and an n-element vector with random integers in [1, 100].
Matrix generateMatrix(int n);
Vector generateVector(int n);

// result[i] = sum over j of matrix[i][j] * vec[j]. Every row is independent,
// which is what makes the problem easy to split across threads.
Vector multiplySequential(const Matrix& matrix, const Vector& vec);

// Splits the rows evenly across threadCount threads and multiplies in parallel.
Vector multiplyParallel(const Matrix& matrix, const Vector& vec, int threadCount);

#endif
