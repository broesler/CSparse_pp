//==============================================================================
//    File: minimal_csc.h
// Created: 2024-12-20 12:33
//  Author: Bernie Roesler
//
//  Description: The header file for the minimal working example of a CSCMatrix
//  with indexing and assignment operators.
//
//==============================================================================

#ifndef _MINIMAL_CSC_H_
#define _MINIMAL_CSC_H_

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

// typedef std::uint64_t csint;
typedef unsigned int csint;


class CSCMatrix
{
    // Private members
    std::vector<double> v_;  // numerical values, size nzmax
    std::vector<csint> i_;   // row indices, size nzmax
    std::vector<csint> p_;   // column pointers (CSC size N_);
    csint M_ = 0;            // number of rows
    csint N_ = 0;            // number of columns
    bool has_canonical_format_ = false;

    public:
        // ---------- Constructors
        CSCMatrix();

        CSCMatrix(csint M, csint N, csint nzmax=0);  // allocate dims + nzmax

        // Access an element by index, but do not change its value
        const double operator()(csint i, csint j) const;  // v = A(i, j)
        double& operator()(csint i, csint j);             // A(i, j) = v

        double& insert(csint i, csint j, double v, csint p);
};


#endif

//==============================================================================
//==============================================================================
