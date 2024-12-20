/*==============================================================================
 *     File: minimal_csc.cpp
 *  Created: 2024-12-20 12:43
 *   Author: Bernie Roesler
 *
 *  Description: Implements the compressed sparse column matrix class
 *
 *============================================================================*/

#include "minimal_csc.h"

/*------------------------------------------------------------------------------
 *     Constructors
 *----------------------------------------------------------------------------*/
CSCMatrix::CSCMatrix() {};


/** Allocate a CSCMatrix for a given shape and number of non-zeros.
 *
 * @param M, N  integer dimensions of the rows and columns
 * @param nzmax integer capacity of space to reserve for non-zeros
 */
CSCMatrix::CSCMatrix(csint M, csint N, csint nzmax)
    : v_(nzmax),
      i_(nzmax),
      p_(N + 1),
      M_(M),
      N_(N)
{}


/** Return the value of the requested element.
 *
 * This function takes O(log M) time if the columns are sorted, and O(M) time
 * if they are not.
 *
 * @param i, j the row and column indices of the element to access.
 *
 * @return the value of the element at `(i, j)`.
 */
const double CSCMatrix::operator()(csint i, csint j) const
{
    std::cout << "const operator()" << std::endl;
    // Assert indices are in-bounds
    assert(i >= 0 && i < M_);
    assert(j >= 0 && j < N_);

    if (has_canonical_format_) {
        // Binary search for t <= i
        auto start = i_.begin() + p_[j];
        auto end = i_.begin() + p_[j+1];

        auto t = std::lower_bound(start, end, i);

        // Check that we actually found the index t == i
        if (t != end && *t == i) {
            return v_[std::distance(i_.begin(), t)];
        } else {
            return 0.0;
        }

    } else {
        // NOTE this code assumes that columns are *not* sorted, and that
        // duplicate entries may exist, so it will search through *every*
        // element in a column.
        double out = 0.0;

        for (csint p = p_[j]; p < p_[j+1]; p++) {
            if (i_[p] == i) {
                out += v_[p];  // sum duplicate entries
            }
        }

        return out;
    }
}



/** Return a reference to the value of the requested element for use in
 * assignment, e.g. `A(i, j) = 56.0`.
 *
 * This function takes O(log M) time if the columns are sorted, and O(M) time
 * if they are not.
 *
 * @param i, j the row and column indices of the element to access.
 *
 * @return a reference to the value of the element at `(i, j)`.
 */
double& CSCMatrix::operator()(csint i, csint j)
{
    std::cout << "non-const operator()" << std::endl;
    // Assert indices are in-bounds
    assert(i >= 0 && i < M_);
    assert(j >= 0 && j < N_);

    if (has_canonical_format_) {
        // Binary search for t <= i
        auto start = i_.begin() + p_[j];
        auto end = i_.begin() + p_[j+1];

        auto t = std::lower_bound(start, end, i);

        auto k = std::distance(i_.begin(), t);

        // Check that we actually found the index t == i
        if (t != end && *t == i) {
            return v_[k];
        } else {
            // Value does not exist, so add a place-holder here.
            return this->insert(i, j, 0.0, k);
        }

    } else {
        // Linear search for the element
        csint k;
        bool found = false;

        for (csint p = p_[j]; p < p_[j+1]; p++) {
            if (i_[p] == i) {
                if (!found) {
                    k = p;  // store the index of the element
                    found = true;
                } else {
                    v_[k] += v_[p];  // accumulate duplicates here
                    v_[p] = 0;       // zero out duplicates
                }
            }
        }

        if (found) {
            return v_[k];
        } else {
            // Columns are not sorted, so we can just insert the element at the
            // beginning of the column.
            return this->insert(i, j, 0.0, p_[j]);
        }
    }
}


/** Insert a single element at a specified location.
 *
 * @param i, j the row and column indices of the element to access.
 * @param v the value to be assigned.
 * @param p the pointer to the column in the matrix.
 *
 * @return a reference to the inserted value.
 */
double& CSCMatrix::insert(csint i, csint j, double v, csint p)
{
    i_.insert(i_.begin() + p, i);
    v_.insert(v_.begin() + p, v);

    // Increment all subsequent pointers
    for (csint k = j + 1; k < p_.size(); k++) {
        p_[k]++;
    }

    return v_[p];
}


/*==============================================================================
 *============================================================================*/
