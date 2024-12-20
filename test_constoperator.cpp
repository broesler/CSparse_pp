/*==============================================================================
 *     File: test_constoperator.cpp
 *  Created: 2024-12-20 12:05
 *   Author: Bernie Roesler
 *
 *  Description: A MWE to test the const vs non-const operator() function in the
 *  CSCMatrix class.
 *
 *============================================================================*/

#include "minimal_csc.h"

int main() {
    CSCMatrix A(5, 3); // Non-const object

    A(0, 0) = 1.0;          // Calls double& operator()
    double& ref = A(0, 0);  // Calls double& operator()
    ref = 5;

    std::cout << "-----\nrunning A(0, 0) == 5: ";
    if (A(0, 0) == 5) {  // calls double& operator()!
        std::cout << "    Assignment successful!" << std::endl;
    }
    std::cout << "-----" << std::endl;

    const CSCMatrix B = A;  // Const object

    double val = B(0, 0);   // Calls const double operator() const
    // B(0, 0) = 3; // Compile error: assignment of read-only location ‘B.CSCMatrix::operator()(int, int)’

    std::cout << "-----\nrunning B(0, 0) == 5: ";
    if (B(0, 0) == 5) {  // calls double& operator()!
        std::cout << "    Assignment successful!" << std::endl;
    }
    std::cout << "-----" << std::endl;

    std::cout << A(0, 0) << std::endl; // Output 5
    std::cout << B(0, 0) << std::endl; // Output 5

    const CSCMatrix C(2, 2);
    // C(0, 0) = 3; // Compile error: assignment of read-only location ‘C.CSCMatrix::operator()(int, int)’
    double val2 = C(0, 0); // Calls const double operator() const

    std::cout << "val  = " << val << std::endl;
    std::cout << "val2 = " << val2 << std::endl;

    return 0;
}


/*==============================================================================
 *============================================================================*/
