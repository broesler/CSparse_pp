/*==============================================================================
 *     File: test_csparse.cpp
 *  Created: 2024-10-01 21:07
 *   Author: Bernie Roesler
 *
 *  Description: Basic test of my CSparse implementation.
 *
 *============================================================================*/

#define CATCH_CONFIG_MAIN  // tell the compiler to define `main()`

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include <algorithm>  // for std::reverse
#include <iostream>
#include <fstream>
#include <numeric>   // for std::iota
#include <string>
#include <sstream>
#include <ranges>

#include "csparse.h"

using namespace cs;

using Catch::Matchers::AllTrue;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::UnorderedEquals;

constexpr double tol = 1e-14;


COOMatrix davis_21_coo()
{
    // See Davis pp 7-8, Eqn (2.1)
    std::vector<csint>  i = {2,    1,    3,    0,    1,    3,    3,    1,    0,    2};
    std::vector<csint>  j = {2,    0,    3,    2,    1,    0,    1,    3,    0,    1};
    std::vector<double> v = {3.0,  3.1,  1.0,  3.2,  2.9,  3.5,  0.4,  0.9,  4.5,  1.7};
    return COOMatrix {v, i, j};
}

// See: Strang, p 25
// E = [[ 1, 0, 0],
//      [-2, 1, 0],
//      [ 0, 0, 1]]
//
// A = [[ 2, 1, 1],
//      [ 4,-6, 0],
//      [-2, 7, 2]]

// Build matrices with sorted columns
CSCMatrix E_mat()
{
    return COOMatrix(
        std::vector<double> {1, -2, 1, 1},  // vals
        std::vector<csint>  {0,  1, 1, 2},  // rows
        std::vector<csint>  {0,  0, 1, 2}   // cols
    ).tocsc();
}

CSCMatrix A_mat()
{
    return COOMatrix(
        std::vector<double> {2, 4, -2, 1, -6, 7, 1, 2},  // vals
        std::vector<csint>  {0, 1,  2, 0,  1, 2, 0, 2},  // rows
        std::vector<csint>  {0, 0,  0, 1,  1, 1, 2, 2}   // cols
    ).tocsc();
}


/** Compare two matrices for equality.
 *
 * @note This function expects the matrices to be in canonical form.
 *
 * @param C       the matrix to test
 * @param expect  the expected matrix
 */
auto compare_canonical(const CSCMatrix& C, const CSCMatrix& expect)
{
    REQUIRE(C.has_canonical_format());
    REQUIRE(expect.has_canonical_format());
    CHECK(C.nnz() == expect.nnz());
    CHECK(C.shape() == expect.shape());
    CHECK(C.indptr() == expect.indptr());
    CHECK(C.indices() == expect.indices());
    REQUIRE(C.data() == expect.data());
}


/** Compare two matrices for equality.
 *
 * @note This function does not require the matrices to be in canonical form.
 *
 * @param C       the matrix to test
 * @param expect  the expected matrix
 */
auto compare_noncanonical(const CSCMatrix& C, const CSCMatrix& expect)
{
    REQUIRE(C.nnz() == expect.nnz());
    REQUIRE(C.shape() == expect.shape());

    auto [M, N] = C.shape();

    for (csint i = 0; i < M; i++) {
        for (csint j = 0; j < N; j++) {
            REQUIRE(C(i, j) == expect(i, j));
        }
    }
}


// TODO figure out how to use the "spaceship" operator<=> to define all
// of the comparisons in one fell swoop?
// A: May only work if we define a wrapper class on std::vector and define the
//    operator within the class vs. scalars.

/** Return a boolean vector comparing each individual element.
 *
 * @param vec   a vector of doubles.
 * @param c     the value against which to compare
 * @return out  a vector whose elements are vec[i] <=> c.
 */
// std::vector<bool> operator<=>(const std::vector<double>& vec, const double c)
// {
//     std::vector<bool> out(vec.size());

//     for (auto const& v : vec) {
//         if (v < c) {
//             out.push_back(std::strong_ordering::less);
//         } else if (v > c) {
//             out.push_back(std::strong_ordering::greater);
//         } else {
//             out.push_back(std::strong_ordering::equal);
//         }
//     }

//     return out;
// }


/** Return a boolean vector comparing each individual element.
 *
 * @param vec   a vector of doubles
 * @param c     the value against which to compare
 * @param comp  the comparison function for elements of the vector and scalar
 *
 * @return out  a vector whose elements are vec[i] <=> c.
 */
std::vector<bool> compare_vec(
    const std::vector<double>& vec,
    const double c,
    std::function<bool(double, double)> comp
    )
{
    std::vector<bool> out;
    out.reserve(vec.size());
    for (const auto& v : vec) {
        out.push_back(comp(v, c));
    }
    return out;
}


// Create the comparison operators by passing the single comparison function to
// our vector comparison function.
std::vector<bool> operator>=(const std::vector<double>& vec, const double c)
{
    return compare_vec(vec, c, std::greater_equal<double>());
}


std::vector<bool> operator!=(const std::vector<double>& vec, const double c)
{
    return compare_vec(vec, c, std::not_equal_to<double>());
}


std::vector<bool> is_close(
    const std::vector<double>& a,
    const std::vector<double>& b,
    const double tol=1e-15
    )
{
    assert(a.size() == b.size());

    std::vector<bool> out(a.size());
    for (int i = 0; i < a.size(); i++) {
        out[i] = (std::fabs(a[i] - b[i]) < tol);
    }

    return out;
}


/*------------------------------------------------------------------------------
 *         Test Utilities
 *----------------------------------------------------------------------------*/
TEST_CASE("Test vector ops", "[vector]")
{
    std::vector<double> a = {1, 2, 3};

    SECTION("Scale a vector") {
        std::vector<double> expect = {2, 4, 6};

        REQUIRE((2 * a) == expect);
        REQUIRE((a * 2) == expect);
    }

    SECTION("Add two vectors") {
        std::vector<double> b = {4, 5, 6};

        REQUIRE((a + b) == std::vector<double>{5, 7, 9});
    }

    SECTION("Negate a vector") {
        REQUIRE(-a == std::vector<double>{-1, -2, -3});
    }

    SECTION("Subtract two vectors") {
        std::vector<double> b = {4, 5, 6};

        REQUIRE((a - b) == std::vector<double>{-3, -3, -3});
    }
}


TEST_CASE("Test cumsum", "[vector]")
{
    std::vector<csint> a = {1, 1, 1, 1};
    std::vector<csint> c = cumsum(a);
    std::vector<csint> expect = {0, 1, 2, 3, 4};
    REQUIRE(c == expect);
    REQUIRE(&a != &c);
}


TEST_CASE("Test vector permutations", "[vector]")
{
    std::vector<double> b = {0, 1, 2, 3, 4};
    std::vector<csint> p = {2, 0, 1, 4, 3};

    REQUIRE(pvec(p, b) == std::vector<double>{2, 0, 1, 4, 3});
    REQUIRE(ipvec(p, b) == std::vector<double>{1, 2, 0, 4, 3});
    REQUIRE(inv_permute(p) == std::vector<csint>{1, 2, 0, 4, 3});
}


TEST_CASE("Test argsort.", "[vector]")
{
    SECTION("Test vector of doubles") {
        std::vector<double> v = {5.6, 6.9, 42.0, 1.7, 9.0};
        REQUIRE(argsort(v) == std::vector<csint> {3, 0, 1, 4, 2});
    }

    SECTION("Test vector of ints") {
        std::vector<int> v = {5, 6, 42, 1, 9};
        REQUIRE(argsort(v) == std::vector<csint> {3, 0, 1, 4, 2});
    }
}


/*------------------------------------------------------------------------------
 *         Test Matrix Functions
 *----------------------------------------------------------------------------*/
TEST_CASE("Test COOMatrix Constructors", "[COOMatrix]")
{
    SECTION("Empty constructor") {
        COOMatrix A;

        REQUIRE(A.nnz() == 0);
        REQUIRE(A.nzmax() == 0);
        REQUIRE(A.shape() == Shape{0, 0});
    }

    SECTION("Make new from given shape") {
        COOMatrix A(56, 37);
        REQUIRE(A.nnz() == 0);
        REQUIRE(A.nzmax() == 0);
        REQUIRE(A.shape() == Shape{56, 37});
    }

    SECTION("Allocate new from shape and nzmax") {
        int nzmax = 1e4;
        COOMatrix A(56, 37, nzmax);
        REQUIRE(A.nnz() == 0);
        REQUIRE(A.nzmax() >= nzmax);
        REQUIRE(A.shape() == Shape{56, 37});
    }

}


TEST_CASE("COOMatrix from (v, i, j) literals.", "[COOMatrix]")
{
    // See Davis pp 7-8, Eqn (2.1)
    std::vector<csint>  i = {2,    1,    3,    0,    1,    3,    3,    1,    0,    2};
    std::vector<csint>  j = {2,    0,    3,    2,    1,    0,    1,    3,    0,    1};
    std::vector<double> v = {3.0,  3.1,  1.0,  3.2,  2.9,  3.5,  0.4,  0.9,  4.5,  1.7};
    COOMatrix A(v, i, j);

    SECTION("Test attributes") {
        REQUIRE(A.nnz() == 10);
        REQUIRE(A.nzmax() >= 10);
        REQUIRE(A.shape() == Shape{4, 4});
        REQUIRE(A.row() == i);
        REQUIRE(A.column() == j);
        REQUIRE(A.data() == v);
    }

    SECTION("Test printing") {
        std::stringstream s;

        SECTION("Print short") {
            std::string expect =
                "<COOrdinate Sparse matrix\n"
                "        with 10 stored elements and shape (4, 4)>\n";

            A.print(s);  // default verbose=false

            REQUIRE(s.str() == expect);
        }

        SECTION("Print verbose") {
            std::string expect =
                "<COOrdinate Sparse matrix\n"
                "        with 10 stored elements and shape (4, 4)>\n"
                "(2, 2): 3\n"
                "(1, 0): 3.1\n"
                "(3, 3): 1\n"
                "(0, 2): 3.2\n"
                "(1, 1): 2.9\n"
                "(3, 0): 3.5\n"
                "(3, 1): 0.4\n"
                "(1, 3): 0.9\n"
                "(0, 0): 4.5\n"
                "(2, 1): 1.7\n";

            SECTION("Print from function") {
                A.print(s, true);  // FIXME memory leak?
                REQUIRE(s.str() == expect);
            }

            SECTION("Print from operator<< overload") {
                s << A;  // FIXME memory leak?
                REQUIRE(s.str() == expect);
            }
        }

        // Clear the stringstream to prevent memory leaks
        s.str("");
        s.clear();
    }

    SECTION("Assign an existing element to create a duplicate") {
        A.assign(3, 3, 56.0);

        REQUIRE(A.nnz() == 11);
        REQUIRE(A.nzmax() >= 11);
        REQUIRE(A.shape() == Shape{4, 4});
        // REQUIRE_THAT(A(3, 3), WithinAbs(57.0, tol));
    }

    SECTION("Assign a new element that changes the dimensions") {
        A.assign(4, 3, 69.0);

        REQUIRE(A.nnz() == 11);
        REQUIRE(A.nzmax() >= 11);
        REQUIRE(A.shape() == Shape{5, 4});
        // REQUIRE_THAT(A(4, 3), WithinAbs(69.0, tol));
    }

    // Exercise 2.5
    SECTION("Assign a dense submatrix") {
        std::vector<csint> rows = {2, 3, 4};
        std::vector<csint> cols = {4, 5, 6};
        std::vector<double> vals = {1, 2, 3, 4, 5, 6, 7, 8, 9};

        A.assign(rows, cols, vals);

        REQUIRE(A.nnz() == 19);
        REQUIRE(A.nzmax() >= 19);
        REQUIRE(A.shape() == Shape{5, 7});
        // std::cout << "A = " << std::endl << A;  // rows sorted
        // std::cout << "A.compress() = " << std::endl << A.compress();  // cols sorted
    }

    SECTION("Tranpose") {
        COOMatrix A_T = A.transpose();
        COOMatrix A_TT = A.T();

        REQUIRE(A_T.row() == j);
        REQUIRE(A_T.column() == i);
        REQUIRE(A_T.row() == A_TT.row());
        REQUIRE(A_T.column() == A_TT.column());
        REQUIRE(&A != &A_T);
    }

    SECTION("Read from a file") {
        std::ifstream fp("./data/t1");
        COOMatrix F(fp);

        REQUIRE(A.row() == F.row());
        REQUIRE(A.column() == F.column());
        REQUIRE(A.data() == F.data());
    }

    SECTION("Conversion to dense array: Column-major") {
        std::vector<double> expect = {
            4.5, 3.1, 0.0, 3.5,
            0.0, 2.9, 1.7, 0.4,
            3.2, 0.0, 3.0, 0.0,
            0.0, 0.9, 0.0, 1.0
        };

        REQUIRE(A.toarray() == expect);
        REQUIRE(A.toarray('F') == expect);
    }

    SECTION("Conversion to dense array: Row-major") {
        std::vector<double> expect = {
            4.5, 0.0, 3.2, 0.0,
            3.1, 2.9, 0.0, 0.9,
            0.0, 1.7, 3.0, 0.0,
            3.5, 0.4, 0.0, 1.0
        };

        REQUIRE(A.toarray('C') == expect);
    }

    SECTION("Generate random matrix") {
        double density = 0.25;
        csint M = 5, N = 10;
        unsigned int seed = 56;  // seed for reproducibility
        COOMatrix A = COOMatrix::random(M, N, density, seed);

        REQUIRE(A.shape() == Shape{M, N});
        REQUIRE(A.nnz() == (csint)(density * M * N));
    }
}


TEST_CASE("Test CSCMatrix", "[CSCMatrix]")
{
    COOMatrix A = davis_21_coo();
    CSCMatrix C = A.compress();  // unsorted columns

    // std::cout << "C = \n" << C;
    SECTION("Test attributes") {
        std::vector<csint> indptr_expect  = {  0,             3,             6,        8,  10};
        std::vector<csint> indices_expect = {  1,   3,   0,   1,   3,   2,   2,   0,   3,   1};
        std::vector<double> data_expect   = {3.1, 3.5, 4.5, 2.9, 0.4, 1.7, 3.0, 3.2, 1.0, 0.9};

        REQUIRE(C.nnz() == 10);
        REQUIRE(C.nzmax() >= 10);
        REQUIRE(C.shape() == Shape{4, 4});
        REQUIRE(C.indptr() == indptr_expect);
        REQUIRE(C.indices() == indices_expect);
        REQUIRE(C.data() == data_expect);
    }

    SECTION ("Test CSCMatrix printing") {
        std::stringstream s;

        SECTION("Print short") {
            std::string expect =
                "<Compressed Sparse Column matrix\n"
                "        with 10 stored elements and shape (4, 4)>\n";

            C.print(s);  // default verbose=false

            REQUIRE(s.str() == expect);
        }

        SECTION("Print verbose") {
            std::string expect =
                "<Compressed Sparse Column matrix\n"
                "        with 10 stored elements and shape (4, 4)>\n"
                "(1, 0): 3.1\n"
                "(3, 0): 3.5\n"
                "(0, 0): 4.5\n"
                "(1, 1): 2.9\n"
                "(3, 1): 0.4\n"
                "(2, 1): 1.7\n"
                "(2, 2): 3\n"
                "(0, 2): 3.2\n"
                "(3, 3): 1\n"
                "(1, 3): 0.9\n";

            SECTION("Print from function") {
                C.print(s, true);  // FIXME memory leak?
                REQUIRE(s.str() == expect);
            }

            SECTION("Print from operator<< overload") {
                s << C;  // FIXME memory leak?
                REQUIRE(s.str() == expect);
            }
        }

        // Clear the stringstream to prevent memory leaks
        s.str("");
        s.clear();
    }

    SECTION("Test indexing: unsorted, without duplicates") {
        std::vector<csint> indptr = C.indptr();
        std::vector<csint> indices = C.indices();
        std::vector<double> data = C.data();
        csint N = C.shape()[1];

        for (csint j = 0; j < N; j++) {
            for (csint p = indptr[j]; p < indptr[j+1]; p++) {
                REQUIRE(C(indices[p], j) == data[p]);
            }
        }

    }

    SECTION("Test indexing: unsorted, with a duplicate") {
        const CSCMatrix C = A.assign(3, 3, 56.0).compress();

        // NOTE "double& operator()" function is being called when we are
        // trying to compare the value. Not sure why.
        // A: The non-const version is called when C is non-const. If C is
        // const, then the const version is called.
        REQUIRE_THAT(C(3, 3), WithinAbs(57.0, tol));
    }

    // Test the transpose -> use indexing to test A(i, j) == A(j, i)
    SECTION("Transpose") {
        // lambda to test on M == N, M < N, M > N
        auto transpose_test = [](CSCMatrix C) {
            CSCMatrix C_T = C.transpose();

            auto [M, N] = C.shape();

            REQUIRE(C.nnz() == C_T.nnz());
            REQUIRE(M == C_T.shape()[1]);
            REQUIRE(N == C_T.shape()[0]);

            for (csint i = 0; i < M; i++) {
                for (csint j = 0; j < N; j++) {
                    REQUIRE(C(i, j) == C_T(j, i));
                }
            }
        };

        SECTION("Test square matrix M == N") {
            transpose_test(C);  // shape = {4, 4}
        }

        SECTION("Test non-square matrix M < N") {
            transpose_test(A.assign(0, 4, 1.6).compress()); // shape = {4, 5}
        }

        SECTION("Test non-square matrix M > N") {
            transpose_test(A.assign(4, 0, 1.6).compress()); // shape = {5, 4}
        }
    }

    SECTION("Sort rows/columns") {
        // Test on non-square matrix M != N
        C = A.assign(0, 4, 1.6).compress();  // {4, 5}

        auto sort_test = [](const CSCMatrix& Cs) {
            Shape shape_expect = {4, 5};
            std::vector<csint> indptr_expect  = {  0,             3,             6,        8,       10, 11};
            std::vector<csint> indices_expect = {  0,   1,   3,   1,   2,   3,   0,   2,   1,   3,   0};
            std::vector<double> data_expect   = {4.5, 3.1, 3.5, 2.9, 1.7, 0.4, 3.2, 3.0, 0.9, 1.0, 1.6};

            CHECK(Cs.shape() == shape_expect);
            CHECK(Cs.has_sorted_indices());
            CHECK(Cs.indptr() == indptr_expect);
            CHECK(Cs.indices() == indices_expect);
            REQUIRE(Cs.data() == data_expect);
        };

        SECTION("Two transposes") {
            sort_test(C.tsort());
        }

        SECTION("qsort") {
            sort_test(C.qsort());
        }

        SECTION("Efficient two transposes") {
            sort_test(C.sort());
        }
    }

    SECTION("Sum duplicates") {
        C = A.assign(0, 2, 100.0)
             .assign(3, 0, 100.0)
             .assign(2, 1, 100.0)
             .compress()
             .sum_duplicates();

        REQUIRE_THAT(C(0, 2), WithinAbs(103.2, tol));
        REQUIRE_THAT(C(3, 0), WithinAbs(103.5, tol));
        REQUIRE_THAT(C(2, 1), WithinAbs(101.7, tol));
    }

    SECTION("Test droptol") {
        C = davis_21_coo().compress().droptol(2.0);

        REQUIRE(C.nnz() == 6);
        REQUIRE(C.shape() == Shape{4, 4});
        REQUIRE_THAT(C.data() >= 2.0, AllTrue());
    }

    SECTION("Test dropzeros") {
        // Assign explicit zeros
        C = davis_21_coo()
            .assign(0, 1, 0.0)
            .assign(2, 1, 0.0)
            .assign(3, 1, 0.0)
            .compress();

        REQUIRE(C.nnz() == 13);

        C.dropzeros();

        REQUIRE(C.nnz() == 10);
        REQUIRE_THAT(C.data() != 0.0, AllTrue());
    }

    SECTION("Test 1-norm") {
        REQUIRE_THAT(C.norm(), WithinAbs(11.1, tol));
    }

    // Exercise 2.2
    SECTION("Test Conversion to COOMatrix") {
        auto convert_test = [](const COOMatrix& B) {
            // Columns are sorted, but not rows
            std::vector<csint>  expect_i = {  1,   3,   0,   1,   3,   2,   2,   0,   3,   1};
            std::vector<csint>  expect_j = {  0,   0,   0,   1,   1,   1,   2,   2,   3,   3};
            std::vector<double> expect_v = {3.1, 3.5, 4.5, 2.9, 0.4, 1.7, 3.0, 3.2, 1.0, 0.9};

            REQUIRE(B.nnz() == 10);
            REQUIRE(B.nzmax() >= 10);
            REQUIRE(B.shape() == Shape{4, 4});
            REQUIRE(B.row() == expect_i);
            REQUIRE(B.column() == expect_j);
            REQUIRE(B.data() == expect_v);
        };

        SECTION("As constructor") {
            COOMatrix B(C);  // via constructor
            convert_test(B);
        }

        SECTION("As function") {
            COOMatrix B = C.tocoo();  // via member function
            convert_test(B);
        }
    }

    // Exercise 2.16 (inverse)
    SECTION("Test conversion to dense array in column-major format") {
        // Column-major order
        std::vector<double> expect = {
            4.5, 3.1, 0.0, 3.5,
            0.0, 2.9, 1.7, 0.4,
            3.2, 0.0, 3.0, 0.0,
            0.0, 0.9, 0.0, 1.0
        };

        REQUIRE(A.tocsc().toarray() == expect);  // canonical form
        REQUIRE(C.toarray() == expect);          // non-canonical form
    }

    SECTION("Test conversion to dense array in row-major format") {
        // Row-major order
        std::vector<double> expect = {
            4.5, 0.0, 3.2, 0.0,
            3.1, 2.9, 0.0, 0.9,
            0.0, 1.7, 3.0, 0.0,
            3.5, 0.4, 0.0, 1.0
        };

        REQUIRE(A.tocsc().toarray('C') == expect);  // canonical form
        REQUIRE(C.toarray('C') == expect);          // non-canonical form
    }
}


TEST_CASE("Test canonical format", "[CSCMatrix][COOMatrix]")
{
    std::vector<csint> indptr_expect  = {  0,               3,                 6,        8,  10};
    std::vector<csint> indices_expect = {  0,   1,     3,   1,     2,   3,     0,   2,   1,   3};
    std::vector<double> data_expect   = {4.5, 3.1, 103.5, 2.9, 101.7, 0.4, 103.2, 3.0, 0.9, 1.0};

    COOMatrix A = (
        davis_21_coo()        // unsorted matrix
        .assign(0, 2, 100.0)  // assign duplicates
        .assign(3, 0, 100.0)
        .assign(2, 1, 100.0)
        .assign(0, 1, 0.0)    // assign zero entries
        .assign(2, 2, 0.0)
        .assign(3, 1, 0.0)
    );

    REQUIRE(A.nnz() == 16);

    // Convert to canonical format
    CSCMatrix C = A.tocsc();  // as member function

    // Duplicates summed
    REQUIRE(C.nnz() == 10);
    REQUIRE_THAT(C(0, 2), WithinAbs(103.2, tol));
    REQUIRE_THAT(C(3, 0), WithinAbs(103.5, tol));
    REQUIRE_THAT(C(2, 1), WithinAbs(101.7, tol));
    // No non-zeros
    REQUIRE_THAT(C.data() != 0.0, AllTrue());
    // Sorted entries
    REQUIRE(C.indptr() == indptr_expect);
    REQUIRE(C.indices() == indices_expect);
    REQUIRE(C.data() == data_expect);
    // Flags set
    REQUIRE(C.has_sorted_indices());
    REQUIRE(C.has_canonical_format());
    REQUIRE_FALSE(C.is_symmetric());

    SECTION("Test constructor") {
        CSCMatrix B(A);
        REQUIRE(C.indptr() == B.indptr());
        REQUIRE(C.indices() == B.indices());
        REQUIRE(C.data() == B.data());
    }

    SECTION("Test indexing") {
        std::vector<csint> indptr = C.indptr();
        std::vector<csint> indices = C.indices();
        std::vector<double> data = C.data();
        csint N = C.shape()[1];

        for (csint j = 0; j < N; j++) {
            for (csint p = indptr[j]; p < indptr[j+1]; p++) {
                REQUIRE(C(indices[p], j) == data[p]);
            }
        }
    }
}


// Exercise 2.13
TEST_CASE("Test is_symmetric.") {
    std::vector<csint>  i = {0, 1, 2};
    std::vector<csint>  j = {0, 1, 2};
    std::vector<double> v = {1, 2, 3};

    SECTION("Test diagonal matrix") {
        CSCMatrix A = COOMatrix(v, i, j).tocsc();
        REQUIRE(A.is_symmetric());
    }

    SECTION("Test non-symmetric matrix with off-diagonals") {
        CSCMatrix A = COOMatrix(v, i, j)
                       .assign(0, 1, 1.0)
                       .tocsc();
        REQUIRE_FALSE(A.is_symmetric());
    }

    SECTION("Test symmetric matrix with off-diagonals") {
        CSCMatrix A = COOMatrix(v, i, j)
                       .assign(0, 1, 1.0)
                       .assign(1, 0, 1.0)
                       .tocsc();
        REQUIRE(A.is_symmetric());
    }
}


/*------------------------------------------------------------------------------
 *          Math Operations
 *----------------------------------------------------------------------------*/
TEST_CASE("Matrix-(dense) vector multiply + addition.", "[math]")
{
    auto multiply_test = [](
        const CSCMatrix& A,
        const std::vector<double>& x,
        const std::vector<double>& y,
        const std::vector<double>& expect_Ax,
        const std::vector<double>& expect_Axpy
        )
    {
        std::vector<double> zero(y.size());
        REQUIRE_THAT(is_close(A.gaxpy(x, zero),   expect_Ax,   tol), AllTrue());
        REQUIRE_THAT(is_close(A.gaxpy(x, y),      expect_Axpy, tol), AllTrue());
        REQUIRE_THAT(is_close(A.T().gatxpy(x, y), expect_Axpy, tol), AllTrue());
        REQUIRE_THAT(is_close(A.dot(x),            expect_Ax,   tol), AllTrue());
        REQUIRE_THAT(is_close((A * x),             expect_Ax,   tol), AllTrue());
        REQUIRE_THAT(is_close((A * x + y),         expect_Axpy, tol), AllTrue());
    };

    SECTION("Test a non-square matrix.") {
        CSCMatrix A = COOMatrix(
            std::vector<double> {1, 1, 2},
            std::vector<csint>  {0, 1, 2},
            std::vector<csint>  {0, 1, 1}
        ).tocsc();

        std::vector<double> x = {1, 2};
        std::vector<double> y = {1, 2, 3};

        // A @ x + y
        std::vector<double> expect_Ax   = {1, 2, 4};
        std::vector<double> expect_Axpy = {2, 4, 7};

        multiply_test(A, x, y, expect_Ax, expect_Axpy);
    }

    SECTION("Test a symmetric (diagonal) matrix.") {
        CSCMatrix A = COOMatrix(
            std::vector<double> {1, 2, 3},
            std::vector<csint>  {0, 1, 2},
            std::vector<csint>  {0, 1, 2}
        ).compress();

        std::vector<double> x = {1, 2, 3};
        std::vector<double> y = {9, 6, 1};

        // A @ x + y
        std::vector<double> expect_Ax   = {1, 4, 9};
        std::vector<double> expect_Axpy = {10, 10, 10};

        multiply_test(A, x, y, expect_Ax, expect_Axpy);
        REQUIRE_THAT(is_close(A.sym_gaxpy(x, y),  expect_Axpy, tol), AllTrue());
    }

    SECTION("Test an arbitrary non-symmetric matrix.") {
        COOMatrix Ac = davis_21_coo();
        CSCMatrix A = Ac.compress();

        std::vector<double> x = {1, 2, 3, 4};
        std::vector<double> y = {1, 1, 1, 1};

        // A @ x + y
        std::vector<double> expect_Ax   = {14.1, 12.5, 12.4,  8.3};
        std::vector<double> expect_Axpy = {15.1, 13.5, 13.4,  9.3};

        multiply_test(A, x, y, expect_Ax, expect_Axpy);

        // Test COOMatrix
        REQUIRE_THAT(is_close(Ac.dot(x), expect_Ax, tol), AllTrue());
        REQUIRE_THAT(is_close((Ac * x),  expect_Ax, tol), AllTrue());
    }

    SECTION("Test an arbitrary symmetric matrix.") {
        // See Davis pp 7-8, Eqn (2.1)
        std::vector<csint>  i = {  0,   1,   3,   0,   1,   2,   1,   2,   0,   3};
        std::vector<csint>  j = {  0,   0,   0,   1,   1,   1,   2,   2,   3,   3};
        std::vector<double> v = {4.5, 3.1, 3.5, 3.1, 2.9, 1.7, 1.7, 3.0, 3.5, 1.0};
        CSCMatrix A = COOMatrix(v, i, j).compress();

        std::vector<double> x = {1, 2, 3, 4};
        std::vector<double> y = {1, 1, 1, 1};

        // A @ x + y
        std::vector<double> expect_Axpy = {25.7, 15.0, 13.4,  8.5};

        REQUIRE_THAT(is_close(A.sym_gaxpy(x, y), expect_Axpy, tol), AllTrue());
    }
}


// Exercise 2.27
TEST_CASE("Matrix-(dense) matrix multiply + addition.")
{
    CSCMatrix A = davis_21_coo().compress();

    SECTION("Test identity op") {
        std::vector<double> I = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };

        std::vector<double> Z(16, 0);

        CSCMatrix expect = A;

        compare_noncanonical(CSCMatrix(A.gaxpy_col(I, Z), 4, 4), expect);
        compare_noncanonical(CSCMatrix(A.T().gatxpy_col(I, Z), 4, 4), expect);
    }

    SECTION("Test arbitrary square matrix in column-major format") {
        std::vector<double> A_dense = A.toarray();

        // A.T @ A + A in column-major format
        std::vector<double> expect = {
            46.61, 13.49, 14.4 ,  9.79,
            10.39, 14.36,  6.8 ,  3.41,
            17.6 ,  5.1 , 22.24,  0.0 ,
             6.29,  3.91,  0.0 ,  2.81
        };

        std::vector<double> C_col = A.T().gaxpy_col(A_dense, A_dense);
        std::vector<double> C_block = A.T().gaxpy_block(A_dense, A_dense);
        std::vector<double> CT_col = A.gatxpy_col(A_dense, A_dense);
        std::vector<double> CT_block = A.gatxpy_block(A_dense, A_dense);

        REQUIRE_THAT(is_close(C_col, expect, tol), AllTrue());
        REQUIRE_THAT(is_close(C_block, expect, tol), AllTrue());
        REQUIRE_THAT(is_close(CT_col, expect, tol), AllTrue());
        REQUIRE_THAT(is_close(CT_block, expect, tol), AllTrue());
    }

    SECTION("Test arbitrary square matrix in row-major format") {
        std::vector<double> A_dense = A.toarray('C');

        // A.T @ A + A in row-major format
        std::vector<double> expect = {
            46.61, 10.39, 17.6 ,  6.29,
            13.49, 14.36,  5.1 ,  3.91,
            14.4 ,  6.8 , 22.24,  0.0 ,
             9.79,  3.41,  0.0 ,  2.81
        };

        std::vector<double> C = A.T().gaxpy_row(A_dense, A_dense);
        std::vector<double> CT = A.gatxpy_row(A_dense, A_dense);

        REQUIRE_THAT(is_close(C, expect, tol), AllTrue());
        REQUIRE_THAT(is_close(CT, expect, tol), AllTrue());
    }

    SECTION("Test non-square matrix in column-major format.") {
        CSCMatrix Ab = A.slice(0, 4, 0, 3);  // {4, 3}
        std::vector<double> Ac_dense = A.slice(0, 3, 0, 4).toarray();
        std::vector<double> A_dense = A.toarray();

        // Ab @ Ac + A in column-major format
        std::vector<double> expect = {
            24.75, 26.04,  5.27, 20.49,
             5.44, 11.31, 11.73,  1.56,
            27.2 ,  9.92, 12.0 , 11.2 ,
             0.0 ,  3.51,  1.53,  1.36
        };

        REQUIRE_THAT(is_close(Ab.gaxpy_col(Ac_dense, A_dense), expect, tol),
                     AllTrue());
        REQUIRE_THAT(is_close(Ab.gaxpy_block(Ac_dense, A_dense), expect, tol),
                     AllTrue());
        REQUIRE_THAT(is_close(Ab.T().gatxpy_col(Ac_dense, A_dense), expect, tol),
                     AllTrue());
        REQUIRE_THAT(is_close(Ab.T().gatxpy_block(Ac_dense, A_dense), expect, tol),
                     AllTrue());
    }

    SECTION("Test non-square matrix in row-major format.") {
        CSCMatrix Ab = A.slice(0, 4, 0, 3);  // {4, 3}
        std::vector<double> Ac_dense = A.slice(0, 3, 0, 4).toarray('C');
        std::vector<double> A_dense = A.toarray('C');

        // Ab @ Ac + A in row-major format
        std::vector<double> expect = {
            24.75,  5.44, 27.2 ,  0.0 ,
            26.04, 11.31,  9.92,  3.51,
             5.27, 11.73, 12.0 ,  1.53,
            20.49,  1.56, 11.2 ,  1.36
        };

        REQUIRE_THAT(is_close(Ab.gaxpy_row(Ac_dense, A_dense), expect, tol),
                     AllTrue());
        REQUIRE_THAT(is_close(Ab.T().gatxpy_row(Ac_dense, A_dense), expect, tol),
                     AllTrue());
    }
}


TEST_CASE("Matrix-matrix multiply.", "[math]")
{
    SECTION("Test square matrices") {
        // Build matrices with sorted columns
        CSCMatrix E = E_mat();
        CSCMatrix A = A_mat();

        // See: Strang, p 25
        // EA = [[ 2, 1, 1],
        //       [ 0,-8,-2],
        //       [-2, 7, 2]]

        CSCMatrix expect = COOMatrix(
            std::vector<double> {2, -2, 1, -8, 7, 1, -2, 2},  // vals
            std::vector<csint>  {0,  2, 0,  1, 2, 0,  1, 2},  // rows
            std::vector<csint>  {0,  0, 1,  1, 1, 2,  2, 2}   // cols
        ).tocsc();

        auto multiply_test = [](
            const CSCMatrix& C,
            const CSCMatrix& E,
            const CSCMatrix& A,
            const CSCMatrix& expect
        ) {
            auto [M, N] = C.shape();

            REQUIRE(M == E.shape()[0]);
            REQUIRE(N == A.shape()[1]);

            for (csint i = 0; i < M; i++) {
                for (csint j = 0; j < N; j++) {
                    REQUIRE_THAT(C(i, j), WithinAbs(expect(i, j), tol));
                }
            }
        };

        SECTION("Test CSCMatrix::dot (aka cs_multiply)") {
            CSCMatrix C = E * A;
            multiply_test(C, E, A, expect);
        }

        SECTION("Test dot_2x two-pass multiply") {
            CSCMatrix C = E.dot_2x(A);
            multiply_test(C, E, A, expect);
        }
    }

    SECTION("Test arbitrary size matrices") {
        // >>> A
        // ===
        // array([[1, 2, 3, 4],
        //        [5, 6, 7, 8]])
        // >>> B
        // ===
        // array([[ 1,  2,  3],
        //        [ 4,  5,  6],
        //        [ 7,  8,  9],
        //        [10, 11, 12]])
        // >>> A @ B
        // ===
        // array([[ 70,  80,  90],
        //        [158, 184, 210]])

        CSCMatrix A = COOMatrix(
            std::vector<double> {1, 2, 3, 4, 5, 6, 7, 8},  // vals
            std::vector<csint>  {0, 0, 0, 0, 1, 1, 1, 1},  // rows
            std::vector<csint>  {0, 1, 2, 3, 0, 1, 2, 3}   // cols
        ).compress();

        CSCMatrix B = COOMatrix(
            std::vector<double> {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},  // vals
            std::vector<csint>  {0, 0, 0, 1, 1, 1, 2, 2, 2,  3,  3,  3},  // rows
            std::vector<csint>  {0, 1, 2, 0, 1, 2, 0, 1, 2,  0,  1,  2}   // cols
        ).compress();

        CSCMatrix expect = COOMatrix(
            std::vector<double> {70, 80, 90, 158, 184, 210},  // vals
            std::vector<csint>  { 0,  0,  0,   1,   1,   1},  // rows
            std::vector<csint>  { 0,  1,  2,   0,   1,   2}   // cols
        ).compress();

        CSCMatrix C = A * B;
        auto [M, N] = C.shape();

        REQUIRE(M == A.shape()[0]);
        REQUIRE(N == B.shape()[1]);

        for (csint i = 0; i < M; i++) {
            for (csint j = 0; j < N; j++) {
                REQUIRE_THAT(C(i, j), WithinAbs(expect(i, j), tol));
            }
        }
    }
}


// Exercise 2.18
TEST_CASE("Sparse Vector-Vector Multiply", "[math]")
{
    // Sparse column vectors *without* sorting columns.
    CSCMatrix x = COOMatrix(
        std::vector<double> {4.5, 3.1, 3.5, 2.9, 1.7, 0.4},
        std::vector<csint>  {0, 1, 3, 5, 6, 7},
        std::vector<csint>  (6, 0)
    ).compress();

    CSCMatrix y = COOMatrix(
        std::vector<double> {3.2, 3.0, 0.9, 1.0},
        std::vector<csint>  {0, 2, 5, 7},
        std::vector<csint>  (4, 0)
    ).compress();

    double expect = 17.41;

    SECTION("Test unsorted indices") {
        CHECK_THAT(x.T() * y, WithinAbs(expect, tol));
        REQUIRE_THAT(x.vecdot(y), WithinAbs(expect, tol));
    }

    SECTION("Test sorted indices") {
        x.sort();
        y.sort();

        CHECK_THAT(x.T() * y, WithinAbs(expect, tol));
        REQUIRE_THAT(x.vecdot(y), WithinAbs(expect, tol));
    }
}


TEST_CASE("Scaling by a constant", "[math]")
{
    std::vector<csint> i{{0, 0, 0, 1, 1, 1}};  // rows
    std::vector<csint> j{{0, 1, 2, 0, 1, 2}};  // cols

    CSCMatrix A = COOMatrix(
        std::vector<double> {1, 2, 3, 4, 5, 6},
        i, j
    ).compress();

    CSCMatrix expect = COOMatrix(
        std::vector<double> {0.1, 0.2, 0.3, 0.4, 0.5, 0.6},
        i, j
    ).compress();

    auto [M, N] = A.shape();

    // Test operator overloading
    CSCMatrix C = 0.1 * A;

    for (csint i = 0; i < M; i++) {
        for (csint j = 0; j < N; j++) {
            REQUIRE_THAT(C(i, j), WithinAbs(expect(i, j), tol));
        }
    }
}


// Exercise 2.4
TEST_CASE("Scale rows and columns", "[math]")
{
    CSCMatrix A = davis_21_coo().compress();

    // Diagonals of R and C to compute RAC
    std::vector<double> r = {1, 2, 3, 4},
                        c = {1.0, 0.5, 0.25, 0.125};

    // expect_RAC = array([[ 4.5  ,  0.   ,  0.8  ,  0.   ],
    //                     [ 6.2  ,  2.9  ,  0.   ,  0.225],
    //                     [ 0.   ,  2.55 ,  2.25 ,  0.   ],
    //                     [14.   ,  0.8  ,  0.   ,  0.5  ]])

    std::vector<csint> expect_i = {0, 1, 3, 1, 2, 3, 0, 2, 1, 3};
    std::vector<csint> expect_j = {0, 0, 0, 1, 1, 1, 2, 2, 3, 3};
    std::vector<double> expect_v = {4.5, 6.2, 14.0, 2.9, 2.55, 0.8, 0.8, 2.25, 0.225, 0.5};
    CSCMatrix expect_RAC = COOMatrix(expect_v, expect_i, expect_j).compress();

    CSCMatrix RAC = A.scale(r, c);

    auto [M, N] = A.shape();

    for (csint i = 0; i < M; i++) {
        for (csint j = 0; j < N; j++) {
            REQUIRE_THAT(RAC(i, j), WithinAbs(expect_RAC(i, j), tol));
        }
    }
}


TEST_CASE("Matrix-matrix addition.", "[math]")
{
    SECTION("Test non-square matrices") {
        // >>> A
        // ===
        // array([[1, 2, 3],
        //        [4, 5, 6]])
        // >>> B
        // ===
        // array([[1, 1, 1]
        //        [1, 1, 1]])
        // >>> 0.1 * B + 9.0 * C
        // ===
        // array([[9.1, 9.2, 9.3],
        //        [9.4, 9.5, 9.6]])

        std::vector<csint> i{{0, 0, 0, 1, 1, 1}};  // rows
        std::vector<csint> j{{0, 1, 2, 0, 1, 2}};  // cols

        CSCMatrix A = COOMatrix(
            std::vector<double> {1, 2, 3, 4, 5, 6},
            i, j
        ).compress();

        CSCMatrix B = COOMatrix(
            std::vector<double> {1, 1, 1, 1, 1, 1},
            i, j
        ).compress();

        CSCMatrix expect = COOMatrix(
            std::vector<double> {9.1, 9.2, 9.3, 9.4, 9.5, 9.6},
            i, j
        ).compress();

        // Test function definition
        CSCMatrix Cf = add_scaled(A, B, 0.1, 9.0);

        // Test operator overloading
        CSCMatrix C = 0.1 * A + 9.0 * B;
        // std::cout << "C = \n" << C << std::endl;

        compare_noncanonical(C, expect);
        compare_noncanonical(Cf, expect);
    }

    SECTION("Test sparse column vectors") {
        CSCMatrix a = COOMatrix(
            std::vector<double> {4.5, 3.1, 3.5, 2.9, 0.4},
            std::vector<csint>  {0, 1, 3, 5, 7},
            std::vector<csint>  (5, 0)
        ).tocsc();

        CSCMatrix b = COOMatrix(
            std::vector<double> {3.2, 3.0, 0.9, 1.0},
            std::vector<csint>  {0, 2, 5, 7},
            std::vector<csint>  (4, 0)
        ).tocsc();

        CSCMatrix expect = COOMatrix(
            std::vector<double> {7.7, 3.1, 3.0, 3.5, 3.8, 1.4},
            std::vector<csint>  {0, 1, 2, 3, 5, 7},
            std::vector<csint>  (6, 0)
        ).tocsc();

        auto [M, N] = a.shape();

        SECTION("Test operator") {
            CSCMatrix C = a + b;

            REQUIRE(C.shape() == a.shape());

            for (csint i = 0; i < M; i++) {
                for (csint j = 0; j < N; j++) {
                    REQUIRE_THAT(C(i, j), WithinAbs(expect(i, j), tol));
                }
            }
        }

        // Exercise 2.21
        SECTION("Test saxpy") {
            std::vector<csint> expect_w(M);
            for (auto i : expect.indices()) {
                expect_w[i] = 1;
            }

            // Initialize workspaces
            std::vector<csint> w(M);
            std::vector<double> x(M);

            w = saxpy(a, b, w, x);

            REQUIRE(w == expect_w);
        }
    }
}


TEST_CASE("Test matrix permutation", "[permute]")
{
    CSCMatrix A = davis_21_coo().compress();

    SECTION("Test no-op") {
        std::vector<csint> p = {0, 1, 2, 3};
        std::vector<csint> q = {0, 1, 2, 3};

        CSCMatrix C = A.permute(inv_permute(p), q);

        compare_noncanonical(C, A);
        compare_noncanonical(A.permute_rows(p), A);
        compare_noncanonical(A.permute_cols(q), A);
    }

    SECTION("Test row permutation") {
        std::vector<csint> p = {1, 0, 2, 3};
        std::vector<csint> q = {0, 1, 2, 3};

        // See Davis pp 7-8, Eqn (2.1)
        CSCMatrix expect = COOMatrix(
            std::vector<double> {3.0,  3.1,  1.0,  3.2,  2.9,  3.5,  0.4,  0.9,  4.5,  1.7},
            std::vector<csint>  {2,    0,    3,    1,    0,    3,    3,    0,    1,    2},
            std::vector<csint>  {2,    0,    3,    2,    1,    0,    1,    3,    0,    1}
        ).tocsc();

        CSCMatrix C = A.permute(inv_permute(p), q);

        compare_noncanonical(C, expect);
        compare_noncanonical(A.permute_rows(inv_permute(p)), expect);
    }

    SECTION("Test column permutation") {
        std::vector<csint> p = {0, 1, 2, 3};
        std::vector<csint> q = {1, 0, 2, 3};

        // See Davis pp 7-8, Eqn (2.1)
        CSCMatrix expect = COOMatrix(
            std::vector<double> {3.0,  3.1,  1.0,  3.2,  2.9,  3.5,  0.4,  0.9,  4.5,  1.7},
            std::vector<csint>  {2,    1,    3,    0,    1,    3,    3,    1,    0,    2},
            std::vector<csint>  {2,    1,    3,    2,    0,    1,    0,    3,    1,    0}
        ).tocsc();

        CSCMatrix C = A.permute(inv_permute(p), q);

        compare_noncanonical(C, expect);
        compare_noncanonical(A.permute_cols(q), expect);
    }

    SECTION("Test both row and column permutation") {
        std::vector<csint> p = {3, 0, 2, 1};
        std::vector<csint> q = {2, 1, 3, 0};

        // See Davis pp 7-8, Eqn (2.1)
        CSCMatrix expect = COOMatrix(
            std::vector<double> {3.0,  3.1,  1.0,  3.2,  2.9,  3.5,  0.4,  0.9,  4.5,  1.7},
            std::vector<csint>  {2,    3,    0,    1,    3,    0,    0,    3,    1,    2},
            std::vector<csint>  {0,    3,    2,    0,    1,    3,    1,    2,    3,    1}
        ).tocsc();

        std::vector<csint> p_inv = inv_permute(p);
        CSCMatrix C = A.permute(p_inv, q);

        compare_noncanonical(C, expect);
        compare_noncanonical(A.permute_rows(p_inv).permute_cols(q), expect);
    }

    SECTION("Test symperm") {
        // Define a symmetric matrix by zero-ing out below-diagonal entries in A
        A.assign(1, 0, 0.0)
         .assign(2, 1, 0.0)
         .assign(3, 0, 0.0)
         .assign(3, 1, 0.0)
         .dropzeros();

        std::vector<csint> p = {3, 0, 2, 1};

        // See Davis pp 7-8, Eqn (2.1)
        CSCMatrix expect = COOMatrix(
            std::vector<double> {3.0,  1.0,  3.2,  2.9,  0.9,  4.5},
            std::vector<csint>  {2,    0,    1,    3,    0,    1},
            std::vector<csint>  {2,    0,    2,    3,    3,    1}
        ).tocsc();

        CSCMatrix C = A.symperm(inv_permute(p));

        compare_noncanonical(C, expect);
    }

    // Exercise 2.26
    SECTION("Test non-permuted transpose") {
        std::vector<csint> p = {0, 1, 2, 3};
        std::vector<csint> q = {0, 1, 2, 3};

        CSCMatrix expect = A.T();
        CSCMatrix C = A.permute_transpose(inv_permute(p), inv_permute(q));

        compare_noncanonical(C, expect);
    }

    SECTION("Test row-permuted transpose") {
        std::vector<csint> p = {3, 0, 1, 2};
        std::vector<csint> q = {0, 1, 2, 3};

        // See Davis pp 7-8, Eqn (2.1)
        CSCMatrix expect = COOMatrix(
            std::vector<double> {3.0,  3.1,  1.0,  3.2,  2.9,  3.5,  0.4,  0.9,  4.5,  1.7},
            std::vector<csint>  {3,    2,    0,    1,    2,    0,    0,    2,    1,    3},
            std::vector<csint>  {2,    0,    3,    2,    1,    0,    1,    3,    0,    1}
        ).tocsc().T();

        CSCMatrix C = A.permute_transpose(inv_permute(p), inv_permute(q));

        compare_noncanonical(C, expect);
    }

    SECTION("Test column-permuted transpose") {
        std::vector<csint> p = {0, 1, 2, 3};
        std::vector<csint> q = {3, 0, 1, 2};

        // See Davis pp 7-8, Eqn (2.1)
        CSCMatrix expect = COOMatrix(
            std::vector<double> {3.0,  3.1,  1.0,  3.2,  2.9,  3.5,  0.4,  0.9,  4.5,  1.7},
            std::vector<csint>  {2,    1,    3,    0,    1,    3,    3,    1,    0,    2},
            std::vector<csint>  {3,    1,    0,    3,    2,    1,    2,    0,    1,    2}
        ).tocsc().T();

        CSCMatrix C = A.permute_transpose(inv_permute(p), inv_permute(q));

        compare_noncanonical(C, expect);
    }

    SECTION("Test permuted transpose") {
        std::vector<csint> p = {3, 0, 2, 1};
        std::vector<csint> q = {2, 1, 3, 0};

        // See Davis pp 7-8, Eqn (2.1)
        CSCMatrix expect = COOMatrix(
            std::vector<double> {3.0,  3.1,  1.0,  3.2,  2.9,  3.5,  0.4,  0.9,  4.5,  1.7},
            std::vector<csint>  {2,    3,    0,    1,    3,    0,    0,    3,    1,    2},
            std::vector<csint>  {0,    3,    2,    0,    1,    3,    1,    2,    3,    1}
        ).tocsc().T();

        CSCMatrix C = A.permute_transpose(inv_permute(p), inv_permute(q));

        compare_noncanonical(C, expect);
    }
}


// Exercise 2.15
TEST_CASE("Test band function")
{
    csint N = 6;
    csint nnz = N*N;

    // CSCMatrix A = ones((N, N)); // TODO
    std::vector<csint> r(N);
    std::iota(r.begin(), r.end(), 0);  // sequence to repeat

    std::vector<csint> rows;
    rows.reserve(nnz);

    std::vector<csint> cols;
    cols.reserve(nnz);

    std::vector<double> vals(nnz, 1);

    // Repeat {0, 1, 2, 3, 0, 1, 2, 3, ...}
    for (csint i = 0; i < N; i++) {
        for (auto& x : r) {
            rows.push_back(x);
        }
    }

    // Repeat {0, 0, 0, 1, 1, 1, 2, 2, 2, ...}
    for (auto& x : r) {
        for (csint i = 0; i < N; i++) {
            cols.push_back(x);
        }
    }

    CSCMatrix A = COOMatrix(vals, rows, cols).tocsc();

    SECTION("Test main diagonal") {
        int kl = 0,
            ku = 0;

        COOMatrix Ab = A.band(kl, ku).tocoo();

        std::vector<csint> expect_rows = {0, 1, 2, 3, 4, 5};
        std::vector<csint> expect_cols = {0, 1, 2, 3, 4, 5};
        std::vector<double> expect_data(expect_rows.size(), 1);

        CHECK(Ab.nnz() == N);
        CHECK(Ab.row() == expect_rows);
        CHECK(Ab.column() == expect_cols);
        REQUIRE(Ab.data() == expect_data);
    }

    SECTION("Test arbitrary diagonals") {
        int kl = -3,
            ku = 2;

        // CSCMatrix Ab = A.band(kl, ku);
        // std::cout << Ab;
        COOMatrix Ab = A.band(kl, ku).tocoo();

        std::vector<csint> expect_rows = {0, 1, 2, 3, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 2, 3, 4, 5, 3, 4, 5};
        std::vector<csint> expect_cols = {0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5};
        std::vector<double> expect_data(expect_rows.size(), 1);

        CHECK(Ab.nnz() == 27);
        CHECK(Ab.row() == expect_rows);
        CHECK(Ab.column() == expect_cols);
        REQUIRE(Ab.data() == expect_data);
    }
}


// Exercise 2.16
TEST_CASE("Test CSC from dense column-major")
{
    std::vector<double> dense_mat = {
        4.5, 3.1, 0.0, 3.5,
        0.0, 2.9, 1.7, 0.4,
        3.2, 0.0, 3.0, 0.0,
        0.0, 0.9, 0.0, 1.0
    };

    CSCMatrix A {dense_mat, 4, 4};

    CSCMatrix expect_A = davis_21_coo().tocsc();

    CHECK(A.nnz() == expect_A.nnz());
    CHECK(A.indptr() == expect_A.indptr());
    CHECK(A.indices() == expect_A.indices());
    REQUIRE(A.data() == expect_A.data());
}


// Exercise 2.12 "cs_ok"
TEST_CASE("Test validity check")
{
    COOMatrix C = davis_21_coo();
    CSCMatrix A = davis_21_coo().compress();

    constexpr bool SORTED = true;
    constexpr bool VALUES = true;

    REQUIRE(A.is_valid());
    REQUIRE_FALSE(A.is_valid(SORTED));
    REQUIRE_FALSE(A.is_valid(SORTED));

    REQUIRE(A.sort().is_valid(SORTED));
    REQUIRE(A.sort().is_valid(SORTED, VALUES));  // no non-zeros

    // Add explicit non-zeros
    A = C.assign(0, 1, 0.0).compress();
    REQUIRE_FALSE(A.is_valid(!SORTED, VALUES));
}


// Exercise 2.22 "hcat" and "vcat"
TEST_CASE("Test concatentation")
{
    CSCMatrix E = E_mat();
    CSCMatrix A = A_mat();

    SECTION("Test horizontal concatenation") {
        CSCMatrix expect = COOMatrix(
            std::vector<double> {1, -2, 1, 1, 2, 4, -2, 1, -6, 7,  1, 2},
            std::vector<csint>  {0,  1, 1, 2, 0, 1,  2, 0,  1, 2,  0, 2},
            std::vector<csint>  {0,  0, 1, 2, 3, 3,  3, 4,  4, 4,  5, 5}
        ).tocsc();

        CSCMatrix C = hstack(E, A);
        compare_canonical(C, expect);
    }

    SECTION("Test vertical concatenation") {
        CSCMatrix expect = COOMatrix(
            std::vector<double> {1, -2, 1, 1, 2, 4, -2, 1, -6, 7,  1, 2},
            std::vector<csint>  {0,  1, 1, 2, 3, 4,  5, 3,  4, 5,  3, 5},
            std::vector<csint>  {0,  0, 1, 2, 0, 0,  0, 1,  1, 1,  2, 2}
        ).tocsc();

        CSCMatrix C = vstack(E, A);
        compare_canonical(C, expect);
    }
}


// Exercise 2.23 slicing with contiguous indices
TEST_CASE("Test slicing")
{
    CSCMatrix A = davis_21_coo().tocsc();

    SECTION("Test row slicing") {
        CSCMatrix expect = COOMatrix(
            std::vector<double> {3.1, 2.9, 1.7, 3.0, 0.9},
            std::vector<csint>  {  0,   0,   1,   1,   0},
            std::vector<csint>  {  0,   1,   1,   2,   3}
        ).tocsc();

        CSCMatrix C = A.slice(1, 3, 0, A.shape()[1]);
        compare_canonical(C, expect);
    }

    SECTION("Test column slicing") {
        CSCMatrix expect = COOMatrix(
            std::vector<double> {2.9, 1.7, 0.4, 3.2, 3.0},
            std::vector<csint>  {  1,   2,   3,   0,   2},
            std::vector<csint>  {  0,   0,   0,   1,   1}
        ).tocsc();

        CSCMatrix C = A.slice(0, A.shape()[0], 1, 3);
        compare_canonical(C, expect);
    }

    SECTION("Test row and column slicing") {
        CSCMatrix expect = COOMatrix(
            std::vector<double> {2.9, 1.7, 3.0, 0.9},
            std::vector<csint>  {  0,   1,   1,   0},
            std::vector<csint>  {  0,   0,   1,   2}
        ).tocsc();

        CSCMatrix C = A.slice(1, 3, 1, 4);
        compare_canonical(C, expect);
    }
}


// Exercise 2.24 indexing with (possibly) non-contiguous indices
TEST_CASE("Test non-contiguous indexing")
{
    CSCMatrix A = davis_21_coo().tocsc();

    SECTION("Test indexing without duplicates") {
        CSCMatrix C = A.index({2, 0}, {0, 3, 2});

        CSCMatrix expect = COOMatrix(
            std::vector<double> {4.5, 3.2, 3.0},
            std::vector<csint>  {  1,   1,   0},
            std::vector<csint>  {  0,   2,   2}
        ).tocsc();

        compare_canonical(C, expect);
    }

    SECTION("Test indexing with duplicate rows") {
        CSCMatrix C = A.index({2, 0, 1, 1}, {0, 3, 2});

        CSCMatrix expect = COOMatrix(
            std::vector<double> {4.5, 3.1, 3.1, 0.9, 0.9, 3.2, 3.0},
            std::vector<csint>  {  1,   2,   3,   2,   3,   1,   0},
            std::vector<csint>  {  0,   0,   0,   1,   1,   2,   2}
        ).tocsc();

        compare_canonical(C, expect);
    }

    SECTION("Test indexing with duplicate columns") {
        CSCMatrix C = A.index({2, 0}, {0, 3, 2, 0});

        CSCMatrix expect = COOMatrix(
            std::vector<double> {4.5, 3.2, 3.0, 4.5},
            std::vector<csint>  {  1,   1,   0,   1},
            std::vector<csint>  {  0,   2,   2,   3}
        ).tocsc();

        compare_canonical(C, expect);
    }
}


// Exercise 2.25 indexing for assignment
TEST_CASE("Test indexing for single assignment.")
{
    auto test_assignment = [](
        CSCMatrix& A,
        const csint i,
        const csint j,
        const double v,
        const bool is_existing
    )
    {
        csint nnz = A.nnz();

        A.assign(i, j, v);

        if (is_existing) {
            CHECK(A.nnz() == nnz);
        } else {
            CHECK(A.nnz() == nnz + 1);
        }
        REQUIRE(A(i, j) == v);
    };

    SECTION("Canonical format") {
        CSCMatrix A = davis_21_coo().tocsc();

        SECTION("Re-assign existing element") {
            test_assignment(A, 2, 1, 56.0, true);
        }

        SECTION("Add a new element") {
            test_assignment(A, 0, 1, 56.0, false);
        }
    }

    SECTION("Non-canonical format") {
        CSCMatrix A = davis_21_coo().compress();

        SECTION("Re-assign existing element") {
            test_assignment(A, 2, 1, 56.0, true);
        }

        SECTION("Add a new element") {
            test_assignment(A, 0, 1, 56.0, false);
        }
    }

    SECTION("Test multiple assignment") {
        CSCMatrix A = davis_21_coo().tocsc();

        std::vector<csint> rows = {2, 0};
        std::vector<csint> cols = {0, 3, 2};

        SECTION("Dense assignment") {
            std::vector<double> vals = {100, 101, 102, 103, 104, 105};

            A.assign(rows, cols, vals);

            for (csint i = 0; i < rows.size(); i++) {
                for (csint j = 0; j < cols.size(); j++) {
                    REQUIRE(A(rows[i], cols[j]) == vals[i + j * rows.size()]);
                }
            }
        }

        SECTION("Sparse assignment") {
            const CSCMatrix C = CSCMatrix(
                std::vector<double> {100, 101, 102, 103, 104, 105},
                std::vector<csint> {0, 1, 0, 1, 0, 1},
                std::vector<csint> {0, 2, 4, 6},
                Shape{2, 3}
            );

            A.assign(rows, cols, C);

            for (csint i = 0; i < rows.size(); i++) {
                for (csint j = 0; j < cols.size(); j++) {
                    REQUIRE(A(rows[i], cols[j]) == C(i, j));
                }
            }
        }
    }
}


// Exercise 2.29
TEST_CASE("Test adding empty rows and columns to a CSCMatrix.")
{
    CSCMatrix A = davis_21_coo().tocsc();
    int k = 3;  // number of rows/columns to add

    SECTION("Add empty rows to top") {
        CSCMatrix C = A.add_empty_top(k);

        std::vector<csint> expect_indices = A.indices();
        for (auto& x : expect_indices) {
            x += k;
        }

        REQUIRE(C.nnz() == A.nnz());
        REQUIRE(C.shape()[0] == A.shape()[0] + k);
        REQUIRE(C.shape()[1] == A.shape()[1]);
        REQUIRE(C.indptr() == A.indptr());
        REQUIRE(C.indices() == expect_indices);
    }

    SECTION("Add empty rows to bottom") {
        CSCMatrix C = A.add_empty_bottom(k);

        REQUIRE(C.nnz() == A.nnz());
        REQUIRE(C.shape()[0] == A.shape()[0] + k);
        REQUIRE(C.shape()[1] == A.shape()[1]);
        REQUIRE(C.indptr() == A.indptr());
        REQUIRE(C.indices() == A.indices());
    }

    SECTION("Add empty columns to left") {
        CSCMatrix C = A.add_empty_left(k);

        std::vector<csint> expect_indptr(k, 0);
        expect_indptr.insert(
            expect_indptr.end(),
            A.indptr().begin(),
            A.indptr().end()
        );

        REQUIRE(C.nnz() == A.nnz());
        REQUIRE(C.shape()[0] == A.shape()[0]);
        REQUIRE(C.shape()[1] == A.shape()[1] + k);
        REQUIRE(C.indptr() == expect_indptr);
        REQUIRE(C.indices() == A.indices());
    }

    SECTION("Add empty columns to right") {
        CSCMatrix C = A.add_empty_right(k);

        std::vector<csint> expect_indptr = A.indptr();
        std::vector<csint> nnzs(k, A.nnz());
        expect_indptr.insert(
            expect_indptr.end(),
            nnzs.begin(),
            nnzs.end()
        );

        REQUIRE(C.nnz() == A.nnz());
        REQUIRE(C.shape()[0] == A.shape()[0]);
        REQUIRE(C.shape()[1] == A.shape()[1] + k);
        REQUIRE(C.indptr() == expect_indptr);
        REQUIRE(C.indices() == A.indices());
    }
}


TEST_CASE("Sum the rows and columns of a matrix")
{
    CSCMatrix A = davis_21_coo().tocsc();

    SECTION("Sum the rows") {
        std::vector<double> expect = {7.7, 6.9, 4.7, 4.9};

        std::vector<double> sums = A.sum_rows();

        REQUIRE(sums.size() == expect.size());
        REQUIRE(sums == expect);
    }

    SECTION("Sum the columns") {
        std::vector<double> expect = {11.1,  5.0,  6.2,  1.9};

        std::vector<double> sums = A.sum_cols();

        REQUIRE(sums.size() == expect.size());
        REQUIRE(sums == expect);
    }
}

/*------------------------------------------------------------------------------
 *          Matrix Solutions
 *----------------------------------------------------------------------------*/
TEST_CASE("Test triangular solve with dense RHS")
{
    const CSCMatrix L = COOMatrix(
        std::vector<double> {1, 2, 3, 4, 5, 6},
        std::vector<csint>  {0, 1, 1, 2, 2, 2},
        std::vector<csint>  {0, 0, 1, 0, 1, 2}
    ).tocsc();

    const CSCMatrix U = L.T();

    const std::vector<double> expect = {1, 1, 1};

    SECTION("Forward solve L x = b") {
        const std::vector<double> b = {1, 5, 15};  // row sums of L
        const std::vector<double> x = L.lsolve(b);

        REQUIRE(x.size() == expect.size());
        REQUIRE_THAT(is_close(x, expect, tol), AllTrue());
    }

    SECTION("Backsolve L.T x = b") {
        const std::vector<double> b = {7, 8, 6};  // row sums of L.T == col sums of L
        const std::vector<double> x = L.ltsolve(b);

        REQUIRE(x.size() == expect.size());
        REQUIRE_THAT(is_close(x, expect, tol), AllTrue());
    }

    SECTION("Backsolve U x = b") {
        const std::vector<double> b = {7, 8, 6};  // row sums of L.T == col sums of L
        const std::vector<double> x = U.usolve(b);

        REQUIRE(x.size() == expect.size());
        REQUIRE_THAT(is_close(x, expect, tol), AllTrue());
    }

    SECTION("Forward solve U.T x = b") {
        const std::vector<double> b = {1, 5, 15};  // row sums of L
        const std::vector<double> x = U.utsolve(b);

        REQUIRE(x.size() == expect.size());
        REQUIRE_THAT(is_close(x, expect, tol), AllTrue());
    }
}


TEST_CASE("Reachability and DFS")
{
    csint N = 14;  // size of L

    // Define a lower-triangular matrix L with arbitrary non-zeros
    std::vector<csint> rows = {2, 3, 4, 6, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 11, 11, 12, 12, 12, 12, 13, 13};
    std::vector<csint> cols = {0, 1, 2, 1, 2, 4, 1, 3, 5, 5, 6, 7,  6,  9,  8, 10,  8,  9, 10, 11,  9, 12};

    // Add the diagonals
    std::vector<csint> diags(N);
    std::iota(diags.begin(), diags.end(), 0);
    rows.insert(rows.end(), diags.begin(), diags.end());
    cols.insert(cols.end(), diags.begin(), diags.end());

    // All values are 1
    std::vector<double> vals(rows.size(), 1);

    CSCMatrix L = COOMatrix(vals, rows, cols).tocsc();
    CSCMatrix U = L.T();

    // Define the rhs matrix B
    CSCMatrix B(N, 1);

    SECTION("dfs from a single node") {
        // Assign non-zeros to rows 3 and 5 in column 0
        csint j = 3;
        B.assign(j, 0, 1.0);
        std::vector<csint> expect = {13, 12, 11, 8, 3};  // reversed in stack

        std::vector<bool> is_marked(N, false);
        std::vector<csint> xi;  // do not initialize!
        xi.reserve(N);

        xi = L.dfs(j, is_marked, xi);

        REQUIRE(xi == expect);
    }

    SECTION("Reachability from a single node") {
        // Assign non-zeros to rows 3 and 5 in column 0
        B.assign(3, 0, 1.0);
        std::vector<csint> expect = {3, 8, 11, 12, 13};

        std::vector<csint> xi = L.reach(B, 0);

        REQUIRE(xi == expect);
    }

    SECTION("Reachability from multiple nodes") {
        // Assign non-zeros to rows 3 and 5 in column 0
        B.assign(3, 0, 1.0).assign(5, 0, 1.0).to_canonical();
        std::vector<csint> expect = {5, 9, 10, 3, 8, 11, 12, 13};

        std::vector<csint> xi = L.reach(B, 0);

        REQUIRE(xi == expect);
    }

    SECTION("spsolve Lx = b with dense RHS") {
        // Create RHS from sums of rows of L, so that x == ones(N)
        std::vector<double> b = {1., 1., 2., 2., 2., 1., 2., 3., 4., 4., 3., 3., 5., 3.};
        for (int i = 0; i < N; i++) {
            B.assign(i, 0, b[i]);
        }
        std::vector<double> expect(N, 1.0);

        // Use structured bindings to unpack the result
        auto [xi, x] = L.spsolve(B, 0, true);

        REQUIRE(x == expect);
    }

    SECTION("spsolve Lx = b with sparse RHS") {
        // RHS is just B with non-zeros in the first column
        B.assign(3, 0, 1.0);

        std::vector<double> expect = { 0.,  0.,  0.,  1.,  0.,  0.,  0.,  0., -1.,  0.,  0.,  1.,  0.,  0.};

        // Use structured bindings to unpack the result
        auto [xi, x] = L.spsolve(B, 0, true);

        REQUIRE(x == expect);
    }

    SECTION("spsolve Ux = b with sparse RHS") {
        // RHS is just B with non-zeros in the first column
        B.assign(3, 0, 1.0);

        std::vector<double> expect = {0., -1.,  0.,  1.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.};

        auto [xi, x] = U.spsolve(B, 0, false);

        REQUIRE(x == expect);
    }
}


TEST_CASE("Permuted triangular solvers")
{
    // >>> L.toarray()
    // === array([[1, 0, 0, 0, 0, 0],
    //            [2, 2, 0, 0, 0, 0],
    //            [3, 3, 3, 0, 0, 0],
    //            [4, 4, 4, 4, 0, 0],
    //            [5, 5, 5, 5, 5, 0],
    //            [6, 6, 6, 6, 6, 6]])
    // >>> (P @ L).toarray().astype(int)
    // === array([[ 6,  6,  6,  6,  6, *6],
    //            [ 4,  4,  4, *4,  0,  0],
    //            [*1,  0,  0,  0,  0,  0],
    //            [ 2, *2,  0,  0,  0,  0],
    //            [ 5,  5,  5,  5, *5,  0],
    //            [ 3,  3, *3,  0,  0,  0]])
    //
    // Starred elements are the diagonals of the un-permuted matrix

    // Un-permuted L
    const CSCMatrix L = COOMatrix(
        std::vector<double> {1, 2, 3, 4, 5, 6, 2, 3, 4, 5, 6, 3, 4, 5, 6, 4, 5, 6, 5, 6, 6},
        std::vector<csint>  {0, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 2, 3, 4, 5, 3, 4, 5, 4, 5, 5},
        std::vector<csint>  {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 5}
    ).tocsc();

    // TODO use U where rows are constant values like L, not L.T(). Only
    // requires changing b in a few places, and makes it easier to debug the
    // solver.
    const CSCMatrix U = L.T().to_canonical();

    const std::vector<csint> p = {5, 3, 0, 1, 4, 2};
    const std::vector<csint> q = {1, 4, 0, 2, 5, 3};

    // Permute the rows (non-canonical form works too)
    const CSCMatrix PL = L.permute_rows(inv_permute(p)).to_canonical();
    const CSCMatrix PU = U.permute_rows(inv_permute(p)).to_canonical();

    // Permute the columns (non-canonical form works too)
    const CSCMatrix LQ = L.permute_cols(p).to_canonical();
    const CSCMatrix UQ = U.permute_cols(p).to_canonical();

    // Permute both rows and columns
    const CSCMatrix PLQ = L.permute(inv_permute(p), q).to_canonical();
    const CSCMatrix PUQ = U.permute(inv_permute(p), q).to_canonical();

    SECTION("Determine if matrix is lower triangular or not") {
        CHECK(L.is_lower_tri());
        CHECK_FALSE(U.is_lower_tri());
        CHECK(PLQ.is_lower_tri(p, inv_permute(q)));
        REQUIRE_FALSE(PUQ.is_lower_tri(p, inv_permute(q)));
    }

    SECTION("Find diagonals of permuted L") {
        std::vector<csint> expect = {2, 8, 14, 16, 19, 20};
        std::vector<csint> p_diags = PL.find_lower_diagonals();
        CHECK(p_diags == expect);

        // Check that we can get the inverse permutation
        std::vector<csint> p_inv = inv_permute(p);  // {2, 3, 5, 1, 4, 0};
        std::vector<csint> diags;
        for (const auto& p : p_diags) {
            diags.push_back(PL.indices()[p]);
        }
        REQUIRE(diags == p_inv);
    }

    SECTION("Find diagonals of permuted U") {
        std::vector<csint> expect = {0, 2, 5, 6, 13, 15};
        std::vector<csint> p_diags = PU.find_upper_diagonals();
        CHECK(p_diags == expect);

        // Check that we can get the inverse permutation
        std::vector<csint> p_inv = inv_permute(p);  // {2, 3, 5, 1, 4, 0};
        std::vector<csint> diags;
        for (const auto& p : p_diags) {
            diags.push_back(PU.indices()[p]);
        }
        REQUIRE(diags == p_inv);
    }

    SECTION("Find diagonals of non-triangular matrix") {
        const CSCMatrix A = davis_21_coo().tocsc();
        REQUIRE_THROWS(A.find_lower_diagonals());
        REQUIRE_THROWS(A.find_upper_diagonals());
        REQUIRE_THROWS(A.find_tri_permutation());
    }

    SECTION("Find permutation vectors of permuted L") {
        std::vector<csint> expect_p = inv_permute(p);
        std::vector<csint> expect_q = inv_permute(q);

        auto [p_inv, q_inv] = PLQ.find_tri_permutation();

        CHECK(p_inv == expect_p);
        CHECK(q_inv == expect_q);
        compare_noncanonical(L, PLQ.permute(inv_permute(p_inv), q_inv));
        compare_noncanonical(PLQ, L.permute(p_inv, inv_permute(q_inv)));
    }

    SECTION("Find permutation vectors of permuted U") {
        std::vector<csint> expect_p = inv_permute(p);
        std::vector<csint> expect_q = inv_permute(q);

        // NOTE returns *reversed* vectors for an upper triangular matrix!!
        auto [p_inv, q_inv] = PUQ.find_tri_permutation();
        std::reverse(p_inv.begin(), p_inv.end());
        std::reverse(q_inv.begin(), q_inv.end());

        CHECK(p_inv == expect_p);
        CHECK(q_inv == expect_q);
        compare_noncanonical(U, PUQ.permute(inv_permute(p_inv), q_inv));
        compare_noncanonical(PUQ, U.permute(p_inv, inv_permute(q_inv)));
    }

    SECTION("Permuted P L x = b, with unknown P") {
        // Create RHS for Lx = b
        // Set b s.t. x == {1, 2, 3, 4, 5, 6} to see output permutation
        const std::vector<double> b = { 1,  6, 18, 40, 75, 126};
        const std::vector<double> expect = {1, 2, 3, 4, 5, 6};

        // Solve Lx = b
        const std::vector<double> x = L.lsolve(b);
        CHECK_THAT(is_close(x, expect, tol), AllTrue());

        // Solve PLx = b
        const std::vector<double> xp = PL.lsolve_rows(b);

        REQUIRE_THAT(is_close(xp, expect, tol), AllTrue());
    }

    SECTION("Permuted L Q x = b, with unknown Q") {
        // Create RHS for Lx = b
        // Set b s.t. x == {1, 2, 3, 4, 5, 6} to see output permutation
        const std::vector<double> b = { 1,  6, 18, 40, 75, 126};
        const std::vector<double> expect = {1, 2, 3, 4, 5, 6};

        // Solve L Q.T x = b
        const std::vector<double> xp = LQ.lsolve_cols(b);

        REQUIRE_THAT(is_close(xp, expect, tol), AllTrue());
    }

    SECTION("Permuted P U x = b, with unknown P") {
        // Create RHS for Ux = b
        // Set b s.t. x == {1, 2, 3, 4, 5, 6} to see output permutation
        const std::vector<double> b = {91, 90, 86, 77, 61, 36};
        const std::vector<double> expect = {1, 2, 3, 4, 5, 6};

        // Solve Ux = b (un-permuted)
        const std::vector<double> x = U.usolve(b);
        CHECK_THAT(is_close(x, expect, tol), AllTrue());

        // Solve PUx = b
        const std::vector<double> xp = PU.usolve_rows(b);
        REQUIRE_THAT(is_close(xp, expect, tol), AllTrue());
    }

    SECTION("Permuted U Q x = b, with unknown Q") {
        // Create RHS for Ux = b
        // Set b s.t. x == {1, 2, 3, 4, 5, 6} to see output permutation
        const std::vector<double> b = {91, 90, 86, 77, 61, 36};
        const std::vector<double> expect = {1, 2, 3, 4, 5, 6};

        // Solve U Q.T x = b
        const std::vector<double> xp = UQ.usolve_cols(b);

        REQUIRE_THAT(is_close(xp, expect, tol), AllTrue());
    }

    SECTION("Permuted P L Q x = b, with unknown P and Q") {
        // Create RHS for Lx = b
        // Set b s.t. x == {1, 2, 3, 4, 5, 6} to see output permutation
        const std::vector<double> b = { 1,  6, 18, 40, 75, 126};
        const std::vector<double> expect = {1, 2, 3, 4, 5, 6};

        // Solve P L Q x = b
        const std::vector<double> xp = PLQ.tri_solve_perm(b);

        std::cout << "xp = " << xp << std::endl;

        REQUIRE_THAT(is_close(xp, expect, tol), AllTrue());
    }

    // SECTION("Permuted P U Q x = b, with unknown P and Q") {
    //     // Create RHS for Lx = b
    //     // Set b s.t. x == {1, 2, 3, 4, 5, 6} to see output permutation
    //     const std::vector<double> b = {91, 90, 86, 77, 61, 36};
    //     const std::vector<double> expect = {1, 2, 3, 4, 5, 6};

    //     // Solve P L Q x = b
    //     const std::vector<double> xp = PUQ.tri_solve_perm(b);

    //     std::cout << "xp = " << xp << std::endl;

    //     REQUIRE_THAT(is_close(xp, expect, tol), AllTrue());
    // }
}

/*==============================================================================
 *============================================================================*/
