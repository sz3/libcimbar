/** \file
    \brief Wirehair : Heavy Row Generator
    \copyright Copyright (c) 2018 Christopher A. Taylor.  All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of Wirehair nor the names of its contributors may be
      used to endorse or promote products derived from this software without
      specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/

/**
    Generate_HeavyRows()

    This generates a heavy matrix that is very likely to be invertible even
    when its values are perturbed randomly.  The way this matrix is used in
    Wirehair is for the heavy submatrix in the lower right of the matrix that
    the solver is designed to invert.

    For context, before the heavy rows come into play the overall matrix has
    been subject to the peel solver, which has solved most of the rows, leaving
    behind a smaller (about Sqrt(N)) dense binary matrix.  This matrix will be
    solved using Gaussian elimination.  The lower right of the matrix contains
    these GF(256) heavy rows as fallbacks in case the binary rows randomly
    tumble to all zeros.

    When the dense binary matrix selects a binary pivot above the heavy rows,
    the heavy rows are also subject to Gaussian elimination (GE).  This means
    the column needs to be eliminated (set to zero) using the GE algorithm.

    Example:

    GE finds a binary row pivot:

        [1 0 1 0 1 1 1 1 0 0 1 0 1 ...] = (Binary pivot row)
         ^ Column it is solving is a 1.

    It XORs this row into all binary rows below it to zero the column.

    And then it eliminates this column from any heavy rows that are nonzero:

        [1a 56 34 2c ...] = (Heavy row)
         ^^ Column to eliminate.

    (Heavy row) = (Heavy row) xor ( 0x1a * (Binary pivot row) )

    So before the heavy rows are used as a fallback, they are subject
    to this sort of random mixing by their first element.

    With increasing probability, these heavy rows are used for the solution
    and eliminate other heavy rows according to the GE algorithm.
    Finally the remaining rows are nearly all heavy and a small square
    heavy GF(256) matrix is inverted to complete the solution.
*/
bool Generate_HeavyRows();
