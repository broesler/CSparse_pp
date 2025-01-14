/*==============================================================================
 *     File: lusolve_perf.cpp
 *  Created: 2025-01-11 09:38
 *   Author: Bernie Roesler
 *
 *  Davis, Exercise 3.8: Test [lu]solve performance for original and optimized
 *  functions.
 *
 *  To compare their performance, we will use a large, sparse matrix and a dense
 *  RHS vector that becomes increasingly sparse. We will plot the time taken for
 *  each operation as a function of the number of columns in the dense matrices.
 *
 *============================================================================*/

#include <chrono>
#include <functional>
#include <fstream>
#include <iostream>
#include <map>
#include <ranges> // for std::views:keys
#include <random>

#include "csparse.h"

using namespace cs;


/** Set a random number of elements in a vector to zero.
 * 
 * @param vec the vector to modify
 * @param N_zeros the number of elements to set to zero
 * @param seed the random seed
 */
void zero_random_indices(
    std::vector<double>& vec,
    size_t N_zeros,
    unsigned int seed=0
) 
{
    // Create list of indices
    std::vector<size_t> idx(vec.size());
    std::iota(idx.begin(), idx.end(), 0);  // Fill with 0,1,2,...

    if (seed == 0) {
        seed = std::random_device()();
    }

    // Shuffle idx
    std::default_random_engine rng(seed);
    std::shuffle(idx.begin(), idx.end(), rng);

    // Set first N_zeros elements to zero
    for (size_t i = 0; i < N_zeros && i < vec.size(); i++) {
        vec[idx[i]] = 0.0;
    }
}


/*------------------------------------------------------------------------------
 *         Main Function
 *----------------------------------------------------------------------------*/
int main()
{
    // Declare constants
    const bool VERBOSE = true;
    const unsigned int SEED = 565656;
    const std::string filename = "./plots/lusolve_perf.json";

    // Run the tests
    using CSCMatrixFunc = std::vector<double>
        (CSCMatrix::*)(const std::vector<double>&) const;

    const std::map<std::string, CSCMatrixFunc> lusolve_funcs = {
        {"lsolve", &CSCMatrix::lsolve},
        {"usolve", &CSCMatrix::usolve},
        {"lsolve_opt", &CSCMatrix::lsolve_opt},
        {"usolve_opt", &CSCMatrix::usolve_opt}
    };

    // Store the results
    std::map<std::string, TimeStats> times;

    const int N = 2000;
    const float density = 0.1;  // density of the sparse matrix

    const std::vector<float> b_densities = {
        // 0.001, 0.002, 0.003, 0.005,
        0.001,
        0.01, 0.02, 0.03, 0.05, 0.1,
        0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0
    };

    // Time sampling (ipython %timeit uses r = 7, n = 100_000)
    const int N_repeats = 3;
    const int N_samples = 100;

    // Initialize the results struct
    for (const auto& name : std::views::keys(lusolve_funcs)) {
        times[name] = TimeStats(b_densities.size());
    }

    // Create a large, square sparse matrix
    CSCMatrix A = COOMatrix::random(N, N, density, SEED).tocsc();

    // Ensure all diagonal elements are non-zero so that L is non-singular
    for (int i = 0; i < N; i++) {
        A.assign(i, i, 1.0);
    }

    // Take the lower triangular
    const CSCMatrix L = A.band(-N, 0);
    const CSCMatrix U = L.T();

    // Create a dense column vector that is the sum of the rows of L
    std::vector<double> bL = L.sum_rows();
    std::vector<double> bU = U.sum_rows();

    // Get times vs increasing sparsity of b. The optimized functions are O(n
    // + f), whereas the original functions are O(|L|) or O(|U|). The optimized
    // functions should be faster for sparse b, and the functions should be
    // identical for dense b.
    for (const float b_dens : b_densities) {
        if (VERBOSE) {
            std::cout << "Running b_dens = " << b_dens << "..." << std::endl;
        }

        // Create the sparse RHS vectors
        zero_random_indices(bL, (size_t)((1 - b_dens) * N), SEED);
        zero_random_indices(bU, (size_t)((1 - b_dens) * N), SEED);

        for (const auto& [name, lusolve_func] : lusolve_funcs) {
            const CSCMatrix& A = name.starts_with("l") ? L : U;
            const std::vector<double>& b = name.starts_with("l") ? bL : bU;

            Stats ts = timeit_member(
                lusolve_func,
                A,
                N_repeats,
                N_samples,
                b
            );

            // Store results
            times[name].means.push_back(ts.mean);
            times[name].std_devs.push_back(ts.std_dev);

            if (VERBOSE) {
                std::cout << name << (name.ends_with("_opt") ? "" : "    ")
                    << " = " << std::format("{:.4e}", ts.mean)
                    << " ± " << std::format("{:.4e}", ts.std_dev) << " s"
                    << std::endl;
            }
        }
    }

    if (VERBOSE)
        std::cout << "done." << std::endl;

    //--------------------------------------------------------------------------
    //        Write the results to a file
    //--------------------------------------------------------------------------
    if (VERBOSE)
        std::cout << "Writing results to '" << filename << "'..." << std::endl;

    // Hack to convert float to int for JSON output
    std::vector<int> b_out(b_densities.size());
    for (size_t i = 0; i < b_densities.size(); i++) {
        b_out[i] = (int)(1000 * b_densities[i]);
    }

    write_json_results(filename, density, b_out, times);

    if (VERBOSE)
        std::cout << "done." << std::endl;

    return EXIT_SUCCESS;
};

/*==============================================================================
 *============================================================================*/
