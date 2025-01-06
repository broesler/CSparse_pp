/*==============================================================================
 *     File: gaxpy_perf.cpp
 *  Created: 2025-01-06 14:07
 *   Author: Bernie Roesler
 *
 *  Davis, Exercise 2.27: Test gaxpy performance for column-major, row-major,
 *  and column-major block operations.
 *
 *  To compare their performance, we will use a large, sparse matrix and two
 *  dense matrices. We will plot the time taken for each operation as a function
 *  of the number of columns in the dense matrices.
 *
 *============================================================================*/

#include <chrono>
#include <functional>
#include <map>
#include <unordered_map>

#include "csparse.h"

using namespace std;

// Define a struct to hold the results of the performance tests
struct TestResults {
    std::vector<csint> Ns;
    std::vector<double> times;
};


int main()
{
    // For each N:
    //      1. Create large, non-square, random matrices
    //      2. Compute the result 3 ways, and time each

    // Test with single matrix size
    int M =  9;  // number of rows in sparse matrix and added dense matrix
    int N = 10;  // number of columns in sparse matrix
    int K =  8;  // number of columns in multiplied dense matrix

    // Create a large, sparse matrix
    CSCMatrix A = COOMatrix::random(M, N, 0.1).tocsc();

    // Create a compatible random, dense matrix
    COOMatrix X = COOMatrix::random(N, K, 0.1);
    COOMatrix Y = COOMatrix::random(M, K, 0.1);

    // Convert to dense column-major order
    std::vector<double> X_col = X.toarray('F');
    std::vector<double> Y_col = Y.toarray('F');

    // Use same matrices in row-major order
    std::vector<double> X_row = X.toarray('C');
    std::vector<double> Y_row = Y.toarray('C');

    // NOTE These lines work (gaxpy_col recognized)
    std::vector<double> C = gaxpy_col(A, X_col, Y_col);
    print_vec(C);

    // Run the tests
    using GaxpyFunc = std::function<
        std::vector<double>(
            const CSCMatrix&,
            const std::vector<double>&,
            const std::vector<double>&
        )
    >;

    // FIXME gaxpy_col, etc. gives "undefined identifier" error
    std::map<string, GaxpyFunc> gaxpy_funcs = {
        {"gaxpy_col", gaxpy_col},
        {"gaxpy_row", gaxpy_row},
        {"gaxpy_block", gaxpy_block}
    };

    std::map<string, TestResults> res;

    for (const auto& pair : gaxpy_funcs) {
        std::string name = pair.first;
        auto gaxpy_func = pair.second;

        // Compute and time the function
        auto start = std::chrono::high_resolution_clock::now();

        std::vector<double> Y_out = gaxpy_func(A, X_col, Y_col);

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;

        // Store results
        res[name].Ns.push_back(N);
        res[name].times.push_back(elapsed.count());
    }

    return EXIT_SUCCESS;
};

/*==============================================================================
 *============================================================================*/
