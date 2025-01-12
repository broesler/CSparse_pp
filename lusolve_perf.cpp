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


// Define function prototypes here to make them visible to main()
// See:
// <https://stackoverflow.com/questions/69558521/friend-function-name-undefined>
std::vector<double> lsolve(const CSCMatrix& L, const std::vector<double>& b);
std::vector<double> usolve(const CSCMatrix& U, const std::vector<double>& b);
std::vector<double> lsolve_opt(const CSCMatrix& L, const std::vector<double>& b);
std::vector<double> usolve_opt(const CSCMatrix& U, const std::vector<double>& b);


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
    using lusolve_prototype = std::function<
        std::vector<double>(const CSCMatrix&, const std::vector<double>&)
    >;

    const std::map<std::string, lusolve_prototype> lusolve_funcs = {
        {"lsolve", lsolve},
        {"usolve", usolve},
        {"lsolve_opt", lsolve_opt},
        {"usolve_opt", usolve_opt}
    };

    // Store the results
    std::map<std::string, TimeStats> times;

    // const std::vector<int> Ns = {10};
    // const std::vector<int> Ns = {10, 100, 1000};
    const std::vector<int> Ns = {10, 20, 50, 100, 200, 500, 1000, 2000, 5000};
    const float density = 0.4;  // density of the sparse matrix

    // Time sampling
    const int N_repeats = 7;
    const int N_samples = 100;  // adjust for total time ~0.2 s (for 1e6 samples)

    // Initialize the results struct
    for (const auto& name : std::views::keys(lusolve_funcs)) {
        times[name] = TimeStats(Ns.size());
    }

    for (const int N : Ns) {
        if (VERBOSE)
            std::cout << "Running N = " << N << "..." << std::endl;

        // Create a large, square sparse matrix
        CSCMatrix A = COOMatrix::random(N, N, density, SEED).tocsc();

        // Ensure all diagonal elements are non-zero so that L is non-singular
        for (int i = 0; i < N; i++) {
            A.assign(i, i, 1.0);
        }

        // Take the lower triangular
        CSCMatrix L = A.band(-N, 0);
        CSCMatrix U = L.T();

        // Create a dense column vector that is the sum of the rows of L
        std::vector<double> bL = L.sum_rows();
        std::vector<double> bU = U.sum_rows();

        // TODO get times vs increasing sparsity of b. The optimized functions
        // are O(n + f), whereas the original functions are O(|L|) or O(|U|).
        // The optimized functions should be faster for sparse b, and the 
        // functions should be identical for dense b.
        zero_random_indices(bL, (size_t)(0.8 * N), SEED);
        zero_random_indices(bU, (size_t)(0.8 * N), SEED);

        for (const auto& [name, lusolve_func] : lusolve_funcs) {
            Stats ts = timeit(
                lusolve_func,
                N_repeats,
                N_samples,
                name.starts_with("l") ? L : U,
                name.starts_with("l") ? bL : bU
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

    write_json_results(filename, density, Ns, times);

    if (VERBOSE)
        std::cout << "done." << std::endl;

    return EXIT_SUCCESS;
};

/*==============================================================================
 *============================================================================*/
