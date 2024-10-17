/*==============================================================================
 *     File: sparsematrix.cpp
 *  Created: 2024-10-16 20:51
 *   Author: Bernie Roesler
 *
 *  Description: Implements some methods of the SparseMatrix class.
 *
 *============================================================================*/

#include <iostream>

#include "sparsematrix.h"


/*------------------------------------------------------------------------------
 *         Printing
 *----------------------------------------------------------------------------*/
/** Print the matrix
 *
 * @param os          the output stream, defaults to std::cout
 * @param verbose     if True, print all non-zeros and their coordinates
 * @param threshold   if `nnz > threshold`, print only the first and last
 *        3 entries in the matrix. Otherwise, print all entries.
 */
// void SparseMatrix::print(std::ostream& os, bool verbose, csint threshold) const
// {
//     csint nnz_ = nnz();
//     os << format_ << " matrix" << std::endl;
//     os << "        with " << nnz_ << " stored elements "
//         << "and shape (" << M_ << ", " << N_ << ")>" << std::endl;

//     if (verbose) {
//         if (nnz_ < threshold) {
//             // Print all elements
//             print_elems_(os, 0, nnz_);
//         } else {
//             // Print just the first and last 3 non-zero elements
//             print_elems_(os, 0, 3);
//             os << "..." << std::endl;
//             print_elems_(os, nnz_ - 3, nnz_);
//         }
//     }
// }


// std::ostream& operator<<(std::ostream& os, const SparseMatrix& A)
// {
//     A.print(os, true);  // verbose printing assumed
//     return os;
// }




/*==============================================================================
 *============================================================================*/
