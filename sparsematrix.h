//==============================================================================
//    File: sparsematrix.h
// Created: 2024-10-16 20:35
//  Author: Bernie Roesler
//
//  Description: Defines the abstract base class SparseMatrix.
//
//==============================================================================

#ifndef _SPARSE_MATRIX_H_
#define _SPARSE_MATRIX_H_

typedef std::size_t csint;


class SparseMatrix
{
    public:
        virtual ~SparseMatrix() {};

        virtual csint nnz() const = 0;
        virtual csint nzmax() const = 0;
        virtual std::array<csint, 2> shape() const = 0;
        // virtual SparseMatrix T() const;

        virtual void print(
            std::ostream& os=std::cout,
            bool verbose=false,
            csint threshold=1000
        ) const = 0;

        // NOTE fails unless we use a SparseMatrix *ptr = new CSCMatrix(A)
        friend std::ostream& operator<<(std::ostream& os, const SparseMatrix& A)
        {
            A.print(os, true);  // verbose printing assumed
            return os;
        }
};

// std::ostream& operator<<(std::ostream&, const SparseMatrix&);






#endif  // _SPARSE_MATRIX_H_

//==============================================================================
//==============================================================================
