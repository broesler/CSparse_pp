/*==============================================================================
 *     File: coo.cpp
 *  Created: 2024-10-01 21:07
 *   Author: Bernie Roesler
 *
 *  Description: Implements the sparse matrix classes
 *
 *============================================================================*/

#include <cassert>
#include <sstream>
#include <numeric>

#include "csparse.h"


/*------------------------------------------------------------------------------
 *     Constructors
 *----------------------------------------------------------------------------*/
COOMatrix::COOMatrix() {};

/** Construct a COOMatrix from arrays of values and coordinates.
 *
 * The entries are *not* sorted in any order, and duplicates are allowed. Any
 * duplicates will be summed.
 *
 * The matrix shape `(M, N)` will be inferred from the maximum indices given.
 *
 * @param v the values of the entries in the matrix
 * @param i, j the non-negative integer row and column indices of the values
 * @return a new COOMatrix object
 */
COOMatrix::COOMatrix(
    const std::vector<double>& v,
    const std::vector<csint>& i,
    const std::vector<csint>& j,
    const std::array<csint, 2>& shape
    )
    : v_(v),
      i_(i),
      j_(j),
      M_(shape[0] ? shape[0] : *std::max_element(i_.begin(), i_.end()) + 1),
      N_(shape[1] ? shape[1] : *std::max_element(j_.begin(), j_.end()) + 1)
{}  // TODO add input checking here if any indices > M-1, N-1.


/** Allocate a COOMatrix for a given shape and number of non-zeros.
 *
 * @param M, N  integer dimensions of the rows and columns
 * @param nzmax integer capacity of space to reserve for non-zeros
 */
COOMatrix::COOMatrix(csint M, csint N, csint nzmax)
    : v_(nzmax),
      i_(nzmax),
      j_(nzmax),
      M_(M),
      N_(N) 
{}


/** Read a COOMatrix matrix from a file.
 *
 * The file is expected to be in "triplet format" `(i, j, v)`, where `(i, j)`
 * are the index coordinates, and `v` is the value to be assigned.
 *
 * @param fp    a reference to the file stream.
 * @throws std::runtime_error if file format is not in triplet format
 */
COOMatrix::COOMatrix(std::istream& fp)
{
    csint i, j;
    double v;

    while (fp) {
        std::string line;
        std::getline(fp, line);
        if (!line.empty()) {
            std::stringstream ss(line);
            if (!(ss >> i >> j >> v))
                throw std::runtime_error("File is not in (i, j, v) format!");
            else
                assign(i, j, v);
        }
    }
}

/*------------------------------------------------------------------------------
 *         Accessors
 *----------------------------------------------------------------------------*/
csint COOMatrix::nnz() const { return v_.size(); }
csint COOMatrix::nzmax() const { return v_.capacity(); }

std::array<csint, 2> COOMatrix::shape() const
{
    return std::array<csint, 2> {M_, N_};
}

const std::vector<csint>& COOMatrix::row() const { return i_; }
const std::vector<csint>& COOMatrix::column() const { return j_; }
const std::vector<double>& COOMatrix::data() const { return v_; }


/** Assign a value to a pair of indices.
 *
 * Note that there is no argument checking other than for positive indices.
 * Assigning to an index that is outside of the dimensions of the matrix will
 * just increase the size of the matrix accordingly.
 *
 * Duplicate entries are also allowed to ease incremental construction of
 * matrices from files, or, e.g., finite element applications. Duplicates will be
 * summed upon compression to sparse column/row form.
 *
 * @param i, j  integer indices of the matrix
 * @param v     the value to be assigned
 *
 * @see cs_entry Davis p 12.
 */
void COOMatrix::assign(csint i, csint j, double v)
{
    assert ((i >= 0) && (j >= 0));

    i_.push_back(i);
    j_.push_back(j);
    v_.push_back(v);

    assert(v_.size() == i_.size());
    assert(v_.size() == j_.size());

    M_ = std::max(M_, i+1);
    N_ = std::max(N_, j+1);
}


/** Convert a coordinate format matrix to a compressed sparse column matrix.
 *
 * The columns are not guaranteed to be sorted, and duplicates are allowed.
 *
 * @return a copy of the `COOMatrix` in CSC format.
 */
CSCMatrix COOMatrix::tocsc() const
{
    csint k, p, nnz_ = nnz();
    std::vector<double> data(nnz_);
    std::vector<csint> indices(nnz_), indptr(N_ + 1), ws(N_);

    // Compute number of elements in each column
    for (k = 0; k < nnz_; k++)
        ws[j_[k]]++;

    // Column pointers are the cumulative sum of the counts, starting with 0
    std::partial_sum(ws.begin(), ws.end(), indptr.begin() + 1);

    // Also copy the cumulative sum back into the workspace for iteration
    ws = indptr;

#ifdef DEBUG
    std::cout << "i_ = [";
    for (auto x : i_) std::cout << x << ", ";
    std::cout << "]" << std::endl;

    std::cout << "j_ = [";
    for (auto x : j_) std::cout << x << ", ";
    std::cout << "]" << std::endl;

    std::cout << "v_ = [";
    for (auto x : v_) std::cout << x << ", ";
    std::cout << "]" << std::endl;
#endif

    for (k = 0; k < nnz_; k++) {
        // A(i, j) is the pth entry in the CSC matrix
        p = ws[j_[k]]++;     // "pointer" to the current element's column
        indices[p] = i_[k];
        data[p] = v_[k];
    }

#ifdef DEBUG
    std::cout << "the data are:" << std::endl; 

    std::cout << "ws = [";
    for (auto x : ws) std::cout << x << ", ";
    std::cout << "]" << std::endl;

    std::cout << "indptr = [";
    for (auto x : indptr) std::cout << x << ", ";
    std::cout << "]" << std::endl;

    std::cout << "indices = [";
    for (auto x : indices) std::cout << x << ", ";
    std::cout << "]" << std::endl;

    std::cout << "data = [";
    for (auto x : data) std::cout << x << ", ";
    std::cout << "]" << std::endl;
#endif

    return CSCMatrix {data, indices, indptr, this->shape()};
}


/*------------------------------------------------------------------------------
       Math Operations
----------------------------------------------------------------------------*/
/** Transpose the matrix as a copy.
 *
 * @return new COOMatrix object with transposed rows and columns.
 */
COOMatrix COOMatrix::T() const
{
    return COOMatrix(this->v_, this->j_, this->i_);
}


/*------------------------------------------------------------------------------
 *         Printing
 *----------------------------------------------------------------------------*/
/** Print elements of the matrix between `start` and `end`.
 *
 * @param os          the output stream, defaults to std::cout
 * @param start, end  print the all elements where `p ∈ [start, end]`, counting
 *        column-wise.
 */
void COOMatrix::print_elems_(std::ostream& os, csint start, csint end) const
{
    for (csint k = start; k < end; k++)
        os << "(" << i_[k] << ", " << j_[k] << "): " << v_[k] << std::endl;
}


/** Print the matrix
 *
 * @param os          the output stream, defaults to std::cout
 * @param verbose     if True, print all non-zeros and their coordinates
 * @param threshold   if `nnz > threshold`, print only the first and last
 *        3 entries in the matrix. Otherwise, print all entries.
 */
void COOMatrix::print(std::ostream& os, bool verbose, csint threshold) const
{
    csint nnz_ = nnz();
    os << "<" << format_desc_ << " matrix" << std::endl;
    os << "        with " << nnz_ << " stored elements "
        << "and shape (" << M_ << ", " << N_ << ")>" << std::endl;

    if (verbose) {
        if (nnz_ < threshold) {
            // Print all elements
            print_elems_(os, 0, nnz_);
        } else {
            // Print just the first and last 3 non-zero elements
            print_elems_(os, 0, 3);
            os << "..." << std::endl;
            print_elems_(os, nnz_ - 3, nnz_);
        }
    }
}

std::ostream& operator<<(std::ostream& os, const COOMatrix& A)
{
    A.print(os, true);  // verbose printing assumed
    return os;
}


/*==============================================================================
 *============================================================================*/