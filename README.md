# CSparse++: A C++ Library for Sparse Matrix Computations

This library is a re-write of the original CSparse library by Tim
Davis, within the 
[SuiteSparse repo](https://github.com/DrTimothyAldenDavis/SuiteSparse), 
following Davis' book "Direct Methods for Sparse Linear Systems" (2006).

My main motivation for creating this library was to simultaneously learn more
about the mechanics of sparse matrix computations, and learn C++.

The original library is written in a terse C style, with many clever memory
optimizations. I have chosen to optimize this code for readability and
(self-)education, rather than for performance, though I have tried to keep the
API as similar to that of the original library as possible. I have included
comments where function names have changed, and where I have made significant
changes to the code.
