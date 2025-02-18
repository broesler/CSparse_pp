#!/usr/bin/env python3
# =============================================================================
#     File: householder_experiment.py
#  Created: 2025-02-12 16:06
#   Author: Bernie Roesler
#
"""
Experiment on p 79 of Davis. Test full Q vs Householder vectors.
"""
# =============================================================================

import matplotlib.pyplot as plt
import numpy as np

from scipy import linalg as la, sparse
from scipy.sparse import linalg as spla

from pathlib import Path

# -----------------------------------------------------------------------------
#         Load the Data
# -----------------------------------------------------------------------------
data_path = Path('../../data/')
filename = Path('west0479')  # TODO change indices in file to 0-based
full_path = data_path / filename

dtypes = np.dtype([
    ('rows', np.int32),
    ('cols', np.int32),
    ('vals', np.float64)
])

data = np.genfromtxt(full_path, delimiter=' ', dtype=dtypes)

A = sparse.coo_matrix((data['vals'], (data['rows'], data['cols']))).tocsc()

assert A.shape[0] == A.shape[1] == 479

# -----------------------------------------------------------------------------
#         Compute the QR decomposition of A with scipy
# -----------------------------------------------------------------------------
# Use LU to get the permutation vector
# (inefficient! should have a dedicated colamd interface)
# 'NATURAL', 'MMD_ATA', ''MMD_AT_PLUS_A', 'COLAMD'
permc_spec = 'NATURAL'
lu = spla.splu(A, permc_spec=permc_spec)
q = lu.perm_c

Aq = A[:, q].toarray()

# Get the QR decomposition
Q, R = la.qr(Aq)

# Get the Householder vectors
(Qraw, tau), _ = la.qr(Aq, mode='raw')
V = np.tril(Qraw, -1) + np.eye(Aq.shape[0])
R_ = np.triu(Qraw)

np.testing.assert_allclose(R, R_)

# TODO check that these are correct? Book states "Q 38,070 vs V 3,906"
# It seems like COLAMD is not working correctly. The householder_explicit.m
# file computes nnz values that are similar to the book with q = colamd(A).
print(f"{permc_spec = }")
# print(f"density of A: {A.nnz / (A.shape[0] * A.shape[1]):.4g}")
# print("A.nnz:", A.nnz)

print("Q.nnz:", sparse.csc_matrix(Q).nnz)

print("V.nnz:", sparse.csc_matrix(V).nnz)
print("R.nnz:", sparse.csc_matrix(R).nnz)

# -----------------------------------------------------------------------------
#         Plots
# -----------------------------------------------------------------------------
fig, axs = plt.subplots(num=1, nrows=2, ncols=2, clear=True)
fig.set_size_inches(6.4, 6.4, forward=True)
fig.suptitle(f"{permc_spec} Ordering")

for ax, M, title in zip(axs.flat, [A, V, Q, R], ['A', 'V', 'Q', 'R']):
    ax.spy(M, markersize=1)
    ax.set_title(title)

plt.show()

# =============================================================================
# =============================================================================
