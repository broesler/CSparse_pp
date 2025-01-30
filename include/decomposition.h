//==============================================================================
//     File: decomposition.h
//  Created: 2025-01-27 13:01
//   Author: Bernie Roesler
//
//  Description: Implements the symbolic factorization for a sparse matrix.
//
//==============================================================================

#ifndef _DECOMPOSITION_H_
#define _DECOMPOSITION_H_

namespace cs {

/*------------------------------------------------------------------------------
 *         Enums 
 *----------------------------------------------------------------------------*/
enum class AMDOrder
{
    Natural,
    APlusAT,
    ATimesA
};


enum class LeafStatus {
    NotLeaf,
    FirstLeaf,
    SubsequentLeaf
};


/*------------------------------------------------------------------------------
 *         Structs 
 *----------------------------------------------------------------------------*/
// See cs_symbolic aka css
struct Symbolic
{
    std::vector<csint> p_inv,     // inverse row permutation for QR, fill-reducing permutation for Cholesky
                       q,         // fill-reducting column permutation for LU and QR
                       parent,    // elimination tree
                       cp,        // column pointers for Cholesky
                       leftmost,  // leftmost[i] = min(find(A(i,:))) for QR
                       m2;        // # of rows for QR, after adding fictitious rows

    double lnz,   // # entries in L for LU or Cholesky, in V for QR
           unz;   // # entries in U for LU, in R for QR
};


/*------------------------------------------------------------------------------
 *          Cholesky Decomposition
 *----------------------------------------------------------------------------*/
// ---------- Helpers
/** Post-order a tree non-recursively.
 *
 * @param parent  the parent vector of the elimination tree
 *
 * @return post  the post-order of the elimination tree
 */
std::vector<csint> post(const std::vector<csint>& parent);

/** Depth-first search in a tree.
 *
 * @param j  the starting node
 * @param[in,out] head  the head of the linked list
 * @param next  the next vector of the linked list
 * @param[in,out] postorder  the post-order of the elimination tree
 */
void tdfs(
    csint j,
    std::vector<csint>& head,
    const std::vector<csint>& next,
    std::vector<csint>& postorder
);

// NOTE firstdesc and rowcnt are *not* officially part of CSparse, but are in
// the book for demonstrative purposes.
/** Find the first descendent of a node in a tree.
 *
 * @note The *first descendent* of a node `j` is the smallest postordering of
 * any descendant of `j`.
 *
 * @param parent  the parent vector of the elimination tree
 * @param post  the post-order of the elimination tree
 *
 * @return first  the first descendent of each node in the tree
 * @return level  the level of each node in the tree
 */
std::pair<std::vector<csint>, std::vector<csint>> firstdesc(
    const std::vector<csint>& parent,
    const std::vector<csint>& postorder
);

/** Compute the least common ancestor of j_prev and j, if j is a leaf of the ith
 * row subtree.
 *
 * @param i  the row index
 * @param j  the column index
 * @param first  the first descendant of each node in the tree
 * @param maxfirst  the maximum first descendant of each node in the tree
 * @param prevleaf  the previous leaf of each node in the tree
 * @param ancestor  the ancestor of each node in the tree
 *
 * @return q lca(jprev, j)
 * @return jleaf  the leaf status of j:
 *                  0 (not a leaf), 1 (first leaf), 2 (subsequent leaf)
 *
 * @see cs_leaf Davis p 48.
 */
std::pair<csint, LeafStatus> least_common_ancestor(
    csint i,
    csint j,
    const std::vector<csint>& first,
    std::vector<csint>& maxfirst,
    std::vector<csint>& prevleaf,
    std::vector<csint>& ancestor
);

// ---------- Matrix operations
/** Compute the elimination tree of A.
  *
  * @param ata  if True, compute the elimination tree of A^T A
  *
  * @return parent  the parent vector of the elimination tree
  */
std::vector<csint> etree(const CSCMatrix& A, bool ata=false);

/** Compute the reachability set for the *k*th row of *L*, the Cholesky faxtcor
 * of this matrix.
 *
 * @param k  the row index
 * @param parent  the parent vector of the elimination tree
 *
 * @return xi  the reachability set of the *k*th row of *L* in topological order
 */
std::vector<csint> ereach(
    const CSCMatrix& A,
    csint k,
    const std::vector<csint>& parent
);

/** Count the number of non-zeros in each row of the Cholesky factor L of A.
 *
 * @param parent  the parent vector of the elimination tree
 * @param postorder  the post-order of the elimination tree
 *
 * @return rowcount  the number of non-zeros in each row of L
 */
std::vector<csint> rowcnt(
    const CSCMatrix& A,
    const std::vector<csint>& parent,
    const std::vector<csint>& postorder
);

/** Count the number of non-zeros in each column of the Cholesky factor L of A.
 *
 * @param parent  the parent vector of the elimination tree
 * @param postorder  the post-order of the elimination tree
 *
 * @return colcount  the number of non-zeros in each column of L
 */
std::vector<csint> counts(
    const CSCMatrix& A,
    const std::vector<csint>& parent,
    const std::vector<csint>& postorder
);

/** Count the number of non-zeros in each row of the Cholesky factor L of A.
  *
  * @return rowcount  the number of non-zeros in each row of L
  */
std::vector<csint> chol_rowcounts(const CSCMatrix& A);

/** Count the number of non-zeros in each column of the Cholesky factor L of A.
  *
  * @return colcount  the number of non-zeros in each column of L
  */
std::vector<csint> chol_colcounts(const CSCMatrix& A);

/** Compute the symbolic Cholesky factorization of a sparse matrix.
 *
 * @note This function assumes that `A` is symmetric and positive definite.
 *
 * @param A the matrix to factorize
 * @param order the ordering method to use:
 *       - 0: natural ordering
 *       - 1: amd(A + A.T())
 *       - 2: amd(??)  FIXME
 *       - 3: amd(A^T A)
 *
 * @return the Symbolic factorization
 *
 * @see cs_schol
 */
Symbolic symbolic_cholesky(const CSCMatrix& A, AMDOrder order);

/** Compute the up-looking Cholesky factorization of a sparse matrix.
 *
 * @note This function assumes that `A` is symmetric and positive definite.
 *
 * @param A the matrix to factorize
 * @param S the Symbolic factorization of `A`
 *
 * @return the Cholesky factorization of `A`
 */
CSCMatrix chol(const CSCMatrix& A, const Symbolic& S);

/** Update the Cholesky factor for \f$ A = A + σ w w^T \f$.
 *
 * @param L  the Cholesky factor of A
 * @param σ  +1 for an update, or -1 for a downdate
 * @param C  the update vector, as the first column in a CSCMatrix
 * @param parent  the elimination tree of A
 *
 * @return L  the updated Cholesky factor of A
 */
CSCMatrix& chol_update(
    CSCMatrix& L,
    int sigma,
    const CSCMatrix& C,
    const std::vector<csint>& parent
);


}  // namespace cs

#endif // _DECOMPOSITION_H_

//==============================================================================
//==============================================================================
