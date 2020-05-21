/** \file
    \brief Wirehair : Codec Implementation
    \copyright Copyright (c) 2012-2018 Christopher A. Taylor.  All rights reserved.

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

#ifndef WIREHAIR_CODEC_H
#define WIREHAIR_CODEC_H

/** \page Codec Overview

    ----------------------------------------------------------------------------
    Matrix Structure:

    S = Size of original data in bytes.
    N = ceil(S / M) = Count of blocks in the original data.

    (1) Matrix Construction

        A = Original data blocks, N blocks long.
        D = Count of dense/heavy matrix rows (see below), chosen based on N.
        E = N + D blocks = Count of recovery set blocks.
        R = Recovery blocks, E blocks long.
        C = Matrix, with E rows and E columns.
        0 = Dense/heavy rows sum to zero.

        +---------+-------+   +---+   +---+
        |         |       |   |   |   |   |
        |    P    |   M   |   |   |   | A |
        |         |       |   |   |   |   |
        +---------+-----+-+ x | R | = +---+
        |    D    |  J  |0|   |   |   | 0 |
        +---------+-+---+-+   |   |   +---+
        |    0      |  H  |   |   |   | 0 |
        +-----------+-----+   +---+   +---+

        A and B are Ex1 vectors of blocks.
            A has N rows of the original data padded by H zeros.
            R has E rows of encoded blocks.

        C is the ExE hybrid matrix above:
            P is the NxN peeling binary submatrix.
                - Optimized for success of the peeling solver.
            M is the Nx(D+6) mixing binary submatrix.
                - Used to mix the D dense/heavy rows into the peeling rows.
            D is the DxN dense binary submatrix.
                - Used to improve on recovery properties of peeling code.
            J is a DxD random-looking invertible submatrix.
            H is the 6x18 heavy GF(256) submatrix.
                - Used to improve on recovery properties of dense code.
            0 is a Dx6 zero submatrix.

        C matrices for each value of N are precomputed offline and used
        based on the length of the input data, which guarantees that C
        is invertible.

    (2) Generating Matrix P

        The Hamming weight of each row of P is a random variable with a
        distribution chosen to optimize the operation of the peeling
        solver (see below).
        For each row of the matrix, this weight is determined and 1 bits
        are then uniformly distributed over the N columns.
        The GeneratePeelRowWeight() function determines row weights.

    (3) Generating Matrix M

        Rows of M are generated with a constant weight of 3 and 1 bits are
        uniformly distributed over the H columns.

    (4) Generating Matrix D

        Matrix D is generated with a Shuffle-2 Code, a novel invention.
        Shuffle-2 codes produce random matrices that offer possibly the
        fastest matrix-matrix multiplication algorithm for this purpose.
        Each bit has a 50% chance of being set.

    (5) Generating Matrix H

        The heavy matrix H is also a novel invention in this context.
        Adding these rows greatly improves invertibility of C while providing
        a constant-time algorithm to solve them.
        H is a 6x18 random byte matrix.
        Each element of H is a byte instead of a bit, representing a number
        in GF(256).

    (6) Matrix Solver

        An optimized sparse technique is used to solve the recovery blocks.


    ----------------------------------------------------------------------------
    Sparse Matrix Solver:

    There are 4 stages to this sparse solver:

    (1) Peeling
        - Opportunistic fast solution for first N rows.
    (2) Compression
        - Setup for Gaussian elimination
    (3) Gaussian Elimination
        - Gaussian elimination on a small square matrix
    (4) Substitution
        - Solves for remaining rows from initial peeling

    See the code comments in Wirehair.cpp for documentation of each step.

    After all of these steps, the row values have been determined and the
    matrix solver is complete.  Let's analyze the complexity of each step:

    (1) Peeling
        - Opportunistic fast solution for first N rows.

        Weight determination : O(k) average
            Column reference update : Amortized O(1) for each column

        If peeling activates,
            Marking as peeled : O(1)

            Reducing weight of rows referencing this column : O(k)
                If other row weight is reduced to 2,
                    Regenerate columns and mark potential Deferred : O(k)
                End
        End

        So peeling is O(1) for each row, and O(N) overall.

    (2) Compression
        - Setup for Gaussian elimination on a wide rectangular matrix

        The dense row multiplication takes O(N / 2 + ceil(N / D) * 2 * (D - 1))
        where D is approximately SQRT(N), so dense row multiplication takes:
        O(N / 2 + ceil(SQRT(N)) * SQRT(N)) = O(1.5*N).

    (3) Gaussian Elimination
        - Gaussian elimination on a (hopefully) small square matrix

        Assume the GE square matrix is SxS, and S = sqrt(N) on
        average thanks to the peeling solver above.

        Gaussian elimination : O(S^3) = O(N^1.5) bit operations

        This algorithm is not bad because the matrix is small and
        it really doesn't contribute much to the run time.

        - Solves for rows of small square matrix

        Assume the GE square matrix is SxS, and S = sqrt(N) on
        average thanks to the peeling solver above.

        Solving inside the GE matrix : O(S^2) = O(N) row ops

    (4) Substitution
        - Solves for remaining rows from initial peeling

        Regenerate peeled matrix rows and substitute : O(N*k) row ops

    So overall, the codec scales roughly linearly in row operations,
    meaning that the throughput is somewhat stable over a wide number of N.

    ----------------------------------------------------------------------------
    Encoding:

    The first N output blocks of the encoder are the same as the
    original data.  After that the encoder will start producing random-
    looking M-byte blocks by generating new rows for P and M and
    multiplying them by B.

    ----------------------------------------------------------------------------
    Decoding:

    Decoding begins by collecting N blocks from the transmitter.  Once
    N blocks are received, the matrix C' (differing in the first N rows
    from the above matrix C) is generated with the rows of P|M that were
    received.  Matrix solving is attempted, failing at the Gaussian
    elimination step if a pivot cannot be found for one of the GE matrix
    columns (see above).

    New rows are received and submitted directly to the GE solver,
    hopefully providing the missing pivot.  Once enough rows have been
    received, back-substitution reconstructs matrix B.

    The first N rows of the original matrix G are then used to fill in
    any blocks that were not received from the original N blocks, and the
    original data is recovered.
*/

#include "WirehairTools.h"

namespace wirehair {


//------------------------------------------------------------------------------
// Peeling Data Structures

struct PeelRowResult
{
    /// Peeling column that is solved by this row.
    /// Set to LIST_TERM if it is deferred for Gaussian elimination
    uint16_t PeelColumn;

    /// Row value is copied yet?
    uint8_t IsCopied;
};

union PeelOverlappingFields
{
    // During peeling:

    /// Final two unmarked column indices
    uint16_t Unmarked[2];

    // After peeling:

    PeelRowResult Result;
};

/// Row in the sparse matrix
struct PeelRow
{
    /// Row PRNG seed that generates the set of columns included in this row
    uint32_t RecoveryId;

    /// Linkage in row list
    uint16_t NextRow;

    /// Row parameters
    PeelRowParameters Params;

    /// Count of columns that have not been marked yet
    uint16_t UnmarkedCount;

    /// Marks left on this row
    PeelOverlappingFields Marks;
};

/// Marks for PeelColumn
enum MarkTypes
{
    MARK_TODO, ///< Unmarked
    MARK_PEEL, ///< Was solved by peeling
    MARK_DEFER ///< Deferred to Gaussian elimination
};

/// Column in the sparse matrix
struct PeelColumn
{
    /// Linkage in column list
    uint16_t Next;

    union
    {
        /// Number of weight-2 rows containing this column
        uint16_t Weight2Refs;

        /// Row that solves the column
        uint16_t PeelRow;

        /// Column that a deferred column is mapped to
        uint16_t GEColumn;
    };

    /// One of the MarkTypes enumeration
    uint8_t Mark;
};

/// List of rows referencing a column
struct PeelRefs
{
    /// Number of rows containing this column
    uint16_t RowCount;

    /// Rows in this reference
    uint16_t Rows[CAT_REF_LIST_MAX];
};


//------------------------------------------------------------------------------
// Codec

class Codec
{
    //--------------------------------------------------------------------------
    // Parameters

    /// Number of bytes in a block
    unsigned _block_bytes = 0;

    /// Number of blocks in the message
    uint16_t _block_count = 0;

    /// Next prime number at or above block count
    uint16_t _block_next_prime = 0;

    /// Enable override for _dense_count, _p_seed, _d_seed
    bool _seed_override = false;

    /// Number of added dense code rows
    uint16_t _dense_count = 0;

    /// Seed for peeled rows of matrix
    uint32_t _p_seed = 0;

    /// Seed for dense rows of matrix
    uint32_t _d_seed = 0;

    /// Number of extra rows to allocate
    uint16_t _extra_count = 0;

    /// Number of stored rows
    uint16_t _row_count = 0;

    /// Number of mix columns
    uint16_t _mix_count = 0;

    /// Next prime number at or above dense count
    uint16_t _mix_next_prime = 0;

    /// Recovery blocks
    uint8_t * GF256_RESTRICT _recovery_blocks = nullptr;

    /// For debugging only: Maximum number of recovery rows
    unsigned _recovery_rows = 0;

    /// Input message blocks
    uint8_t * GF256_RESTRICT _input_blocks = nullptr;

    /// Number of bytes in final block of input
    unsigned _input_final_bytes = 0;

    /// Number of bytes in final block of output
    unsigned _output_final_bytes = 0;

    /// Number of bytes allocated for input, or 0 if referenced
    uint64_t _input_allocated = 0;

#if defined(CAT_ALL_ORIGINAL)
    /// Boolean: Only seen original data block identifiers
    bool _all_original = false;
#endif

    uint8_t * GF256_RESTRICT _copied_original = nullptr;

    /// Boolean: Original blocks are out of order?
    bool _original_out_of_order = false;


    //--------------------------------------------------------------------------
    // Peeling state

    /// Array of N peeling matrix rows
    PeelRow * GF256_RESTRICT _peel_rows = nullptr;

    /// Array of N peeling matrix columns
    PeelColumn * GF256_RESTRICT _peel_cols = nullptr;

    /// List of column references
    PeelRefs * GF256_RESTRICT _peel_col_refs = nullptr;

    /// Tail of peeling solved rows list
    PeelRow * GF256_RESTRICT _peel_tail_rows = nullptr;

    /// Number of bytes allocated for workspace
    uint64_t _workspace_allocated = 0;

    /// Terminator for list (like nullptr in a linked list)
    static const uint16_t LIST_TERM = 0xffff;

    /// Head of peeling solved rows list
    uint16_t _peel_head_rows = 0;

    /// Head of peeling deferred columns list
    uint16_t _defer_head_columns = 0;

    /// Head of peeling deferred rows list
    uint16_t _defer_head_rows = 0;

    /// Count of deferred rows
    uint16_t _defer_count = 0;


    //--------------------------------------------------------------------------
    // Gaussian elimination state

    /// Gaussian elimination compression matrix
    uint64_t * GF256_RESTRICT _compress_matrix = nullptr;

    /// Gaussian elimination matrix
    uint64_t * GF256_RESTRICT _ge_matrix = nullptr;

    /// Number of bytes allocated to GE matrix
    uint64_t _ge_allocated = 0;

    /// Words per row of GE matrix and compression matrix
    unsigned _ge_pitch = 0;

    /// For debugging only: GE matrix rows and columns
    unsigned _ge_rows = 0, _ge_cols = 0;

    /// Pivots for each column of the GE matrix
    uint16_t * GF256_RESTRICT _pivots = nullptr;

    /// Number of pivots in the pivot list
    unsigned _pivot_count = 0;

    /// Map of GE columns to conceptual matrix columns
    uint16_t * GF256_RESTRICT _ge_col_map = nullptr;

    /// Map of GE rows to conceptual matrix rows
    uint16_t * GF256_RESTRICT _ge_row_map = nullptr;

    /// Pivot to resume Triangle() on after it fails
    unsigned _next_pivot = 0;


    //--------------------------------------------------------------------------
    // Heavy submatrix

    /// Heavy rows of GE matrix
    uint8_t * GF256_RESTRICT _heavy_matrix = nullptr;

    /// Bytes per heavy matrix row
    unsigned _heavy_pitch = 0;

    /// Number of heavy matrix columns
    unsigned _heavy_columns = 0;

    /// For debugging: Number of heavy rows
    unsigned _heavy_rows = 0;

    /// First heavy column that is non-zero
    unsigned _first_heavy_column = 0;

    /// First heavy pivot in the list
    unsigned _first_heavy_pivot = 0;

#if defined(CAT_DUMP_CODEC_DEBUG) || defined(CAT_DUMP_GE_MATRIX)
    void PrintGEMatrix();
    void PrintExtraMatrix();
    void PrintCompressMatrix();
    void PrintPeeled();
    void PrintDeferredRows();
    void PrintDeferredColumns();
#endif


    //--------------------------------------------------------------------------
    // Stage (1) Peeling

    /** \page Stage (1) Peeling

        Until N rows are received, the peeling algorithm is executed:

        Columns have 3 states:

        (1) Peeled - Solved by a row during peeling process.
        (2) Deferred - Will be solved by a row during Gaussian Elimination.
        (3) Unmarked - Still deciding.

        Initially all columns are unmarked.

        As a row comes in, the count of columns that are Unmarked is calculated.
        If that count is 1, then the column is marked as Peeled and is solved by
        the received row.  Peeling then goes through all other rows that reference
        that peeled column, reducing their count by 1, potentially causing other
        columns to be marked as Peeled.  This "peeling avalanche" is desired.
    */

    /**
        OpportunisticPeeling()

        Walk forward through rows and solve as many as possible
        before deferring any columns for the GE solver.

        This function accepts a new row from the codec input and
        immediately attempts to solve a column opportunistically
        using the graph-based peeling decoding process.

        The row value is assumed to already be present in the
        _input_blocks data, but this function does take care of
        initializing everything else for a new row, including the
        row ID number and the peeling column generator parameters.

        Returns true on success.
        Returns false if there was not enough space in the reference list.
    */
    bool OpportunisticPeeling(
        const uint16_t row_i, ///< Row index
        const uint32_t row_seed ///< Row PRNG seed
    );

    /**
        FixPeelFailure()

        This function is called by OpportunisticPeeling() if the reference
        list overflows for one of the columns.

        This function unreferences previous columns for a row where, in one
        of the columns, the reference list overflowed.

        This avoids potential data corruption or more severe problems in case
        of unusually distributed peeling matrices.
    */
    void FixPeelFailure(
        PeelRow * GF256_RESTRICT row, ///< The row that failed
        const uint16_t fail_column_i  ///< Column end point
    );

    /**
        PeelAvalancheOnSolve()

        This function is called after a column is solved by a row during
        peeling.  It attempts to find other rows that reference this column
        and resumes opportunistic peeling in an avalanche of solutions.

        OpportunisticPeeling() and PeelAvalancheOnSolve() are split up into
        two functions because I found that the PeelAvalancheOnSolve() function
        can be reused later during GreedyPeeling().
    */
    void PeelAvalancheOnSolve(
        uint16_t column_i ///< Column that was solved
    );

    /**
        SolveWithPeel()

        This function is called exclusively by OpportunisticPeeling()
        to take care of marking columns solved when a row is able to
        solve a column during the peeling process.
    */
    void SolveWithPeel(
        PeelRow * GF256_RESTRICT row, ///< Pointer to row data
        uint16_t row_i,   ///< Row index
        uint16_t column_i ///< Column that this solves
    );

    /**
        GreedyPeeling()

        After the opportunistic peeling solver has completed, no columns have
        been deferred to Gaussian elimination yet.  Greedy peeling will then
        take over and start choosing columns to defer.  It is greedy in that
        the selection of which columns to defer is based on a greedy initial
        approximation of the best column to choose, rather than the best one
        that could be chosen.

        In this algorithm, the column that will cause the largest immediate
        avalanche of peeling solutions is the one that is selected.

        If there is a tie between two or more columns based on just that
        criterion, then of the columns that tied, the one that affects the
        most rows is selected.  This is a better way to choose which columns
        to defer than just selecting the one referenced by the most rows.

        In practice with a well-designed (good distribution) peeling matrix,
        about sqrt(N) + N/150 columns must be deferred to Gaussian elimination
        using this greedy approach.
    */
    void GreedyPeeling();

    /** \page Peeling Solver Output

        After the peeling solver has completed, only Deferred columns remain
        to be solved.  Conceptually the matrix can be re-ordered in the order
        of solution so that the matrix resembles this example:

            +-----+---------+-----+
            |   1 | 1       | 721 | <-- Peeled row 1
            | 1   |   1     | 518 | <-- Peeled row 2
            | 1   | 1   1   | 934 | <-- Peeled row 3
            |   1 | 1   1 1 | 275 | <-- Peeled row 4
            +-----+---------+-----+
            | 1   | 1 1   1 | 123 | <-- Deferred rows
            |   1 |   1 1 1 | 207 | <-- Deferred rows
            +-----+---------+-----+
                ^       ^       ^---- Row value (unmodified input data)
                |       \------------ Peeled columns
                \-------------------- Deferred columns

        Re-ordering the actual matrix is not required, but the lower-triangular
        form of the peeled matrix is apparent in the diagram above.
    */


    //--------------------------------------------------------------------------
    // Stage (2) Compression

    /** \page Stage (2) Compression

        If using a naive approach, this is by far the most complex step of
        the matrix solver.  The approach outlined here makes it much easier.
        At this point the matrix has been re-organized into peeled and
        deferred rows and columns:

            +-----------------------+
            | P P P P P | D D | M M |
            +-----------------------+
                        X
            +-----------+-----+-----+
            | 5 2 6 1 3 | 4 0 | 7 8 |
            +-----------+-----+-----+---+   +---+
            | 1         | 0 0 | 1 0 | 0 |   | P |
            | 0 1       | 0 0 | 0 1 | 5 |   | P |
            | 1 0 1     | 1 0 | 1 0 | 3 |   | P |
            | 0 1 0 1   | 0 0 | 0 1 | 4 |   | P |
            | 0 1 0 1 1 | 1 1 | 1 0 | 1 |   | P |
            +-----------+-----+-----+---| = |---|
            | 1 1 0 1 1 | 1 1 | 1 0 | 7 |   | 0 |
            | 1 0 1 1 0 | 1 0 | 0 1 | 8 |   | 0 |
            +-----------+-----+-----+---+   +---+
            | 0 1 1 0 0 | 1 1 | 0 1 | 2 |   | D |
            | 0 1 0 1 0 | 0 1 | 1 0 | 6 |   | D |
            +-----------+-----+-----+---|   |---|
                  ^          ^     ^- Mixing columns
                  |          \------- Deferred columns
                  \-----------------\ Peeled columns intersections
                                    | with deferred rows.

        P = Peeled rows/columns (re-ordered)
        D = Deferred rows/columns (order of deferment)
        M = Mixing columns always deferred for GE
        0 = Dense rows always deferred for GE; they sum to 0

        Since the re-ordered matrix above is in lower triangular form,
        and since the weight of each row is limited to a constant, the cost
        of diagonalizing the peeled matrix is O(n).  Diagonalizing the peeled
        matrix will cause the mix and deferred columns to add up and become
        dense.  These columns are stored in the same order as in the GE matrix,
        in a long vertical matrix with N rows, called the Compression matrix.

        After diagonalizing the peeling matrix, the peeling matrix will be
        the identity matrix.  Peeled column output blocks can be used to store
        the temporary block values generated by this process.  These temporary
        blocks will be overwritten and lost later during Substitution.

        To finish compressing the matrix into a form for Gaussian elimination,
        all of the peeled columns of the deferred/dense rows must be zeroed.
        Wherever a column is set to 1 in a deferred/dense row, the Compression
        matrix row that solves that peeled column is added to the deferred row.
        Essentially the peeled column intersection with the deferred rows are
        multiplied by the peeled matrix to produce initial row values and
        matrix rows for the GE matrix in the lower right.

        This process does not actually produce any final column values because
        at this point it is not certain where those values will end up.
        Instead, a record of what operations were performed needs to be stored
        and followed later after the destination columns are determined
        by Gaussian elimination.
    */

    /**
        SetDeferredColumns()

        This function initializes some mappings between GE columns and
        column values, and it sets bits in the Compression matrix in rows
        that reference the deferred columns.  These bits will get mixed
        throughout the Compression matrix and will make it very dense.

        For each deferred column,
            Set bit for each row affected by this column.
            Map GE column to this column.
            Map this column to the GE column.

        For each mixing column,
            Map GE column to this column.
    */
    void SetDeferredColumns();

    /**
        SetMixingColumnsForDeferredRows()

        This function generates the mixing column bits
        for each deferred row in the GE matrix.  It also
        marks the row's peel_column with LIST_TERM so that
        later it will be easy to check if it was deferred.
    */
    void SetMixingColumnsForDeferredRows();

    /**
        PeelDiagonal()

        This function diagonalizes the peeled rows and columns of the
        matrix.  The result is that the peeled submatrix is the identity
        matrix, and that the other columns of the peeled rows are very
        dense and have temporary block values assigned.

        These dense columns are used to efficiently zero-out the peeled
        columns of the other rows.

        This function is one of the most expensive in the whole codec,
        because its memory access patterns are not cache-friendly.

        For each peeled row in forward solution order,
            Set mixing column bits for the row in the Compression matrix.
            Generate row block value.
            For each row that references this row in the peeling matrix,
                Add Compression matrix row to referencing row.
                If row is peeled,
                    Add row block value.
    */
    void PeelDiagonal();

    /**
        CopyDeferredRows()

        This function copies deferred rows from the Compression matrix
        into their final location in the GE matrix.  It also maps the
        GE rows to the deferred rows.
    */
    void CopyDeferredRows();

    /** \page Dense SubMatrix Structure: Shuffle-2 Codes

    After Compression, the GE matrix is a square matrix that
    looks like this:

        +-----------------+----------------+--------------+
        |  Dense Deferred | Dense Deferred | Dense Mixing | <- About half
        |                 |                |              |
        +-----------------+----------------+--------------+
        |                 | Dense Deferred | Dense Mixing | <- About half
        |        0        |                |              |
        |                 |----------------+--------------+
        |                 | Sparse Deferred| Dense Mixing | <- Last few rows
        +-----------------+----------------+--------------+
                 ^                  ^---- Middle third of the columns
                 \------ Left third of the columns

    The dense rows are generated so that they can quickly be
    eliminated with as few row operations as possible.
    This elimination can be visualized as a matrix-matrix multiplication
    between the peeling submatrix and the deferred/dense submatrix
    intersection with the peeled columns.  Using Shuffle-2 Codes, I
    have been able to achieve this matrix-matrix multiplication in
    just 2.5*N row operations, which is better than any other approach
    I have seen so far.

    I needed to find a way to generate a binary matrix that looks
    random but actually only differs by ~2 bits per row.  I looked at
    using normal Gray codes or more Weyl generators but they both are
    restricted to a subset of the total possibilities.  Instead, the
    standard in-place shuffle algorithm is used to shuffle row and
    column orders to make it look random.  This new algorithm is able
    to generate nearly all possible combinations with approximately
    uniform likelihood, and the generated matrix can be addressed by
    a 32-bit seed, so it is easy to regenerate the same matrix again,
    or generate many new random matrices without reseeding.

    Shuffling generates the first row randomly, and each following
    row is XORed by two columns, one with a bit set and one without.
    The order of XOR pairs is decided by the initial shuffling.
    The order of the generated rows is shuffled separately.

    Example output: A random 17x17 matrix

        10000001111010011
        00111110100101100
        11000001011010011
        00101100111110010
        00111100101101100
        00111100101110100
        11000011010010011
        01111110000101100
        01011111000101100
        00101000111010011
        00101100111110100
        11010011000001011
        00101000111110010
        10100000111010011
        11010111000001101
        11010111000101100
        11000011010001011

    This code I am calling a Perfect Shuffle-2 Code.

    These are "perfect" matrices in that they have the same
    Hamming weight in each row and each column.  The problem is
    that this type of matrix is NEVER invertible, so the perfect
    structure must be destroyed in order to get a good code for
    error correction.

    The resulting code is called a Shuffle-2 code.

    Here is the Shuffle-2 submatrix generation process:

    Split the dense submatrix of the matrix into DxD squares.
    For each DxD square subsubmatrix,
        Shuffle the destination row order.
        Shuffle the bit flip order.
        Generate a random bit string with weight D/2 for the first output row.
        Reshuffle the bit flip order. <- Helps recovery properties a lot!
        Flip two bits for each row of the first half of the outputs.
        Reshuffle the bit flip order. <- Helps recovery properties a lot!
        Flip two bits for each row of the last half of the outputs.

    This effectively destroys the perfection of the code, and makes
    the square matrices invertible about as often as a random GF(2) code,
    so that using these easily generated square matrices does not hurt
    the error correction properties of the code.  Random GF(2) matrices
    are invertible about 30% of the time, and these are invertible about
    15% of the time when D Mod 4 = 2.  Other choices of D are not so good.

    A Shuffle-3 Code would reshuffle 3 times and flip 3 bits per row,
    and a Shuffle-4 Code would reshuffle 4 times and flip 4 bits per row.
    This is probably not the first time that someone has invented this.
    I believe that Moon Ho Lee has come up with something similar and more
    mathematically rigorous, though I believe he was using Shuffle-4 for
    quantum cryptography.  Shuffle-2 is much faster and works for this
    application because columns from different DxD matrices are randomly
    selected for use in the GE matrix.  And furthermore the rank needed
    from the selected columns is usually much less than the row count.

    MultiplyDenseRows() does not actually use memxor() to generate
    any row block values because it is not certain where the values
    will end up, yet.  So instead this multiplication is done again
    in the MultiplyDenseValues() function after Triangle() succeeds.
    */

    /// Multiply dense rows by peeling matrix to generate GE rows,
    /// but no row values are involed yet
    void MultiplyDenseRows();

    /** \page O(1) Heavy Row Structure

    The heavy rows are made up of bytes instead of bits.  Each byte
    represents a number in the Galois field GF(2^^8) defined by the
    generator polynomial 0x15F.

    The heavy rows are designed to make it easier to find pivots in
    just the last few columns of the GE matrix.  This design choice was
    made because it allows a constant-time algorithm to be employed that
    will reduce the fail rate from >70% to <3%.  It is true that with
    heavy loss rates, the earlier columns can be where the pivot is
    needed.  However in my estimation it would be better to increase the
    number of dense rows instead to handle this problem than to increase
    the number of heavy rows.  The result is that we can assume missing
    pivots occur near the end.

    The number of heavy rows required is at least 5.  This is because
    the heavy rows are used to fill in for missing pivots in the GE matrix
    and the miss rate is about 1/2 because it's random and binary.  The odds
    of the last 5 columns all being zero in the binary rows is 1/32.
    And the odds of a random GF(256) matrix not being invertible is also
    around 1/32, therefore it needs at least 5 heavy rows.  With less than 5
    rows, the binary matrix fail rate would dominate the overall rate of
    invertibility.  After 5 heavy rows, less likely problems can be overcome,
    so 6 heavy rows were chosen for the baseline version.

    An important realization is that almost all of the missing pivots occur
    within the last M columns of the GE matrix, even for large matrices.

    So, the heavy matrix is always 6xM, where M is around 12.  Since the heavy
    matrix never gets any larger, the execution time doesn't vary based on N,
    and for large enough N it only lowers throughput imperceptibly while still
    providing a huge reduction in fail rate.

        The overall matrix structure can be visualized as:

        +-----------+-----------+---------------------+
        |           |           |                     |
        |  Dense    |  Dense    |    Dense Mixing     |
        |  Deferred |  Mixing   |    Heavy Overlap    |
        |           |           |                     |
        +-----------+-----------+---------------------+
                    |           |                     |
            Zero    |  Deferred |    Deferred Mixing  |
            -ish    |  Mixing   |    Heavy Overlap    |
                    |           |                     |
        +-----------+-----------+---------------------+
        |                       |                     |
        |   Extra Binary Rows   |   Extra Heavy Rows  | <-- Uninitialized
        |                       |                     |
        +-----------------------+-------------+-------+
                                |             |       |
            Implicitly Zero     |      H      |   I   | <-- 6x6 Identity matrix
            (Not Allocated)     |             |       |
                                +-------------+-------+

        The heavy matrix H is a Cauchy matrix.  Its elements are selected
        from a run of the CM256 codec.
    */

    /// Initialize heavy submatrix as described above.
    void SetHeavyRows();


    //--------------------------------------------------------------------------
    // Stage (3) Gaussian Elimination

    /** \page GE Matrix Construction Optimization

        One more subtle optimization.  Why not?  Depending on how the GE
        matrix is constructed, it can be put in a roughly upper-triangular
        form from the start so it looks like this:

            +-------+-------+
            | D D D | D D 1 |
            | D D D | D 1 M | <- Dense rows
            | D D D | 1 M M |
            +-------+-------+
            | 0 0 1 | M M M |
            | 0 0 0 | M M M | <- Sparse deferred rows
            | 0 0 0 | M M M |
            +-------+-------+
                ^       ^------- Mixing columns
                \--------------- Deferred columns

        In the example above, the top 4 rows are dense matrix rows.
        The last 4 columns are mixing columns and are also dense.
        The lower left sub-matrix is sparse and roughly upper triangular
        because it is the intersection of sparse rows in the generator
        matrix.  This form is achieved by adding deferred rows starting
        from last deferred to first, and adding deferred columns starting
        from last deferred to first.

        Gaussian elimination will proceed from left to right on the
        matrix.  So, having an upper triangular form will prevent left-most
        zeros from being eaten up when a row is eliminated by one above it.
        This reduces row operations.  For example, GE on a 64x64 matrix will
        do on average 100 fewer row operations on this form rather than its
        transpose (where the sparse part is put in the upper right).
    */

    /**
        SetupTriangle()

        This function initializes the state variables to begin doing
        triangularization.  It was split from Triangle() at one point during
        development so that Triangle() can be shared with ResumeSolveMatrix(),
        which saves a lot of code duplication.

        The pivot list is longer than the number of columns to determine so
        that it will be useful for keeping track of extra overhead blocks.
        Initially it only contains non-heavy rows.
    */
    void SetupTriangle();

    /**
        InsertHeavyRows()

        This function converts remaining extra rows to heavy rows.
        It then adds heavy rows to the GE matrix to be used in the solver.
    */
    void InsertHeavyRows();

    /**
        TriangleNonHeavy()

        This function performs triangularization for all of the non-heavy
        columns of the GE matrix.  As soon as the first heavy column needs to
        be determined it converts the extra rows to heavy rows and adds in the
        heavy rows of the matrix to the pivot list.

        Returns true if a pivot could be found.
        Returns false if pivot could not be found.
    */
    bool TriangleNonHeavy();

    /**
        Triangle()

        This function performs normal Gaussian elimination on the GE
        matrix in order to put it in upper triangular form, which would
        solve the system of linear equations represented by the matrix.
        The only wrinkle here is that instead of zeroing the columns as
        it goes, this algorithm will keep a record of which columns were
        added together so that these steps can be followed later to
        produce the column values with xors on the data.

        It uses a pivot array that it swaps row pointers around in, as
        opposed to actually swapping rows in the matrix, which would be
        more expensive.  Heavy rows are kept at the end of the pivot array
        so that they are always selected last.

        Returns true if a solution was found.
        Returns false if more data is needed.
    */
    bool Triangle();


    //--------------------------------------------------------------------------
    // Stage (4) Substitution

    /**
        InitializeColumnValues()

        This function initializes the output block value for each column
        that was solved by Gaussian elimination.  For deferred rows it follows
        the same steps performed earlier in PeelDiagonal() just for those rows.
        The row values were not formed at that point because the destination
        was uncertain.

        For each pivot that solves the GE matrix:
            If GE row is from a dense row:
                Initialize row value to 0.
            Else it is from a deferred row:
                For each peeled column that it references:
                    Add in that peeled column's row value from Compression.

        For each remaining unused row, (happens in the decoder for extra rows):
            If the unused row is a dense row:
                Set the GE row map entry to LIST_TERM so it can be ignored later.
    */
    void InitializeColumnValues();

    /**
        MultiplyDenseValues()

        This function follows the same order of operations as the
        MultiplyDenseRows() function to add in peeling column values
        to the dense rows.  Deferred rows are handled above in the
        InitializeColumnValues() function.

        See MultiplyDenseRows() comments for justification of the
        design of the dense row structure.
    */
    void MultiplyDenseValues();

    /**
        AddSubdiagonalValues()

        This function uses the bits that were left behind by the
        Triangle() function to follow the same order of operations
        to generate the row values for both deferred and dense rows.
        It is aided by the already roughly upper-triangular form
        of the GE matrix, making this function very cheap to execute.
    */
    void AddSubdiagonalValues();

    /**
        Windowed Back-Substitution

        The matrix to diagonalize is in upper triangular form now, where each
        row is random and dense.  Each column has a value assigned to it at this
        point but it is necessary to back-substitute up each column to eliminate
        the upper triangular part of the matrix.  Because the matrix is random,
        the optimal window size for a windowed substitution method is approx:

            w = CEIL[0.85 + 0.85*ln(r)]

            But in practice, the window size is selected from simple heuristic
        rules based on testing.  For this function, the window size stays
        between 3 and 6 so it is fine to use heuristics.

        The matrix may look like:

            +---+---+---+---+
            | A | 1 | 1 | 0 |
            +---+---+---+---+
            | 0 | B | 1 | 1 |
            +---+---+---+---+
            | 0 | 0 | C | 1 |
            +---+---+---+---+
            | 0 | 0 | 0 | D |
            +---+---+---+---+

            To do the back-substitution with a window width of 2 on the above
        matrix, first back-substitute to diagonalize the lower right:

            +---+---+---+---+
            | A | 1 | 1 | 0 |
            +---+---+---+---+
            | 0 | B | 1 | 1 |
            +---+---+---+---+
            | 0 | 0 | C | 0 |
            +---+---+---+---+
            | 0 | 0 | 0 | D |
            +---+---+---+---+

            Then compute the 2-bit window:

            [0 0] = undefined
            [1 0] = C
            [0 1] = D
            [1 1] = C + D

            And substitute up the last two columns to eliminate them:

            B = B + [1 1] = B + C + D
            A = A + [1 0] = A + C

            +---+---+---+---+
            | A | 1 | 0 | 0 |
            +---+---+---+---+
            | 0 | B | 0 | 0 |
            +---+---+---+---+
            | 0 | 0 | C | 0 |
            +---+---+---+---+
            | 0 | 0 | 0 | D |
            +---+---+---+---+

            This operation is performed until the windowing method is no
        longer worthwhile, and the normal back-substitution is used on the
        remaining matrix pivots.
    */

    /**
        BackSubstituteAboveDiagonal()

        This function uses the windowed approach outlined above
        to eliminate all of the bits in the upper triangular half,
        completing solving for these columns.
    */
    void BackSubstituteAboveDiagonal();

    /**
        Substitute()

        This function generates all of the remaining column values.
        At the point this function is called, the GE columns were solved
        by BackSubstituteAboveDiagonal().  The remaining column values
        are solved by regenerating rows in forward order of peeling.

        Note that as each row is generated, that all of the columns
        active in that row have already been solved.  So long as the
        substitution follows in forward solution order, this is guaranteed.

        It is also possible to start from the Compression matrix values
        and substitute into that matrix.  However, because the mixing columns
        are so dense, it is actually faster in every case to just regenerate
        the rows from scratch and throw away those results.
    */
    void Substitute();


    //--------------------------------------------------------------------------
    // Main Driver

    /// This function determines the matrix to use based on the
    /// given message bytes and bytes per block.
    WirehairResult ChooseMatrix(
        uint64_t message_bytes,
        unsigned block_bytes);

    /**
        SolveMatrix()

        This function attempts to solve the matrix, given that the
        matrix is currently square and may be solvable at this point
        without any additional blocks.

        It performs the peeling, compression, and GE steps:

        (1) Peeling:
            GreedyPeeling()

        (2) Compression:

        Allocate GE and Compression matrix now that the size is known:

            AllocateMatrix()

        Produce the Compression matrix:

            SetDeferredColumns()
            SetMixingColumnsForDeferredRows()
            PeelDiagonal()

        Produce the GE matrix:

            CopyDeferredRows()
            MultiplyDenseRows()
            AddInvertibleGF2Matrix()

        (3) Gaussian Elimination
            Triangle()
    */
    WirehairResult SolveMatrix();

    /**
        ResumeSolveMatrix()

        This function resumes solving the matrix if the initial SolveMatrix()
        failed at finding a pivot.  With heavy rows added this is pretty rare, so
        in theory this function could be written inefficiently and it would not
        affect the average run time much.  But that just wouldn't be right.

        Extra rows have been added to both the GE matrix and the heavy matrix.
        Since the heavy matrix does not cover all the columns of the GE matrix,
        the new rows are staged in the GE matrix and then copied into the heavy
        matrix after they get into range of the heavy columns.
    */
    WirehairResult ResumeSolveMatrix(
        const unsigned id, ///< Block ID
        const void * GF256_RESTRICT data ///< Block data
    );

#if defined(CAT_ALL_ORIGINAL)
    /**
        IsAllOriginalData()

        This function verifies that all of the original N data blocks
        have been received.
    */
    bool IsAllOriginalData();
#endif


    //--------------------------------------------------------------------------
    // Memory Management

    void SetInput(const void * GF256_RESTRICT message_in);
    bool AllocateInput();
    void FreeInput();

    bool AllocateMatrix();
    void FreeMatrix();

    bool AllocateWorkspace();
    void FreeWorkspace();

public:
    Codec();
    ~Codec();


    //--------------------------------------------------------------------------
    // Getters

    GF256_FORCE_INLINE uint32_t PSeed() const { return _p_seed; }
    GF256_FORCE_INLINE uint32_t CSeed() const { return _d_seed; }
    GF256_FORCE_INLINE uint32_t BlockCount() const { return _block_count; }


    //--------------------------------------------------------------------------
    // Encoder API

    /// Allow seeds to be overridden
    void OverrideSeeds(
        uint16_t dense_count,
        uint16_t p_seed,
        uint16_t d_seed);

    /// Initialize encoder mode
    WirehairResult InitializeEncoder(
        uint64_t message_bytes,
        unsigned block_bytes);

    /**
        EncodeFeed()

        This function breaks the input message up into blocks
        and opportunistically peels with each one.  After processing
        all of the blocks from the input, it runs the matrix solver
        and if the solver succeeds, it generates the recovery blocks.

        In practice, the solver should always succeed because the
        encoder should be looking up its matrix parameters from a
        table, which guarantees the matrix is invertible.
    */
    WirehairResult EncodeFeed(const void * GF256_RESTRICT message_in);

    /**
        Encode()

        This function encodes a block.  For the first N blocks
        it simply copies the input to the output block.  For other
        block identifiers, it will generate a new random row and
        sum together recovery blocks to produce the new block.
    */
    uint32_t Encode(
        const uint32_t block_id, ///< Block id to generate
        void * GF256_RESTRICT block_out, ///< Block data output
        uint32_t out_buffer_bytes ///< Output buffer bytes
    );


    //--------------------------------------------------------------------------
    // Decoder API

    /// Initialize decoder mode
    WirehairResult InitializeDecoder(
        uint64_t message_bytes,
        unsigned block_bytes
    );

    /**
        DecodeFeed()

        This function accumulates the new block in a large staging buffer.
        As soon as N blocks are collected, SolveMatrix() is run.
        After N blocks, ResumeSolveMatrix() is run.
    */
    WirehairResult DecodeFeed(
        const unsigned block_id,
        const void * GF256_RESTRICT block_in,
        const unsigned block_bytes
    );

    /**
        GenerateRecoveryBlocks()

        This function generates the recovery blocks after the
        Triangle() function succeeds in solving the matrix.

        It performs the final Substitution step:

        (4) Substitution:

            Solves across GE matrix rows:

                InitializeColumnValues()
                MultiplyDenseValues()
                AddSubdiagonalValues()

            Solves up GE matrix columns:

                BackSubstituteAboveDiagonal()

            Solves remaining columns:

                Substitute()
    */
    void GenerateRecoveryBlocks();

    /**
        ReconstructOutput()

        This function reconstructs the output by copying inputs
        that were from the first N blocks, and regenerating the rest.
        This is only done during decoding.

        Precondition: DecodeFeed() has returned success
    */
    WirehairResult ReconstructOutput(
        void * GF256_RESTRICT message_out,
        uint64_t message_bytes);

    /**
        ReconstructBlock()

        This function reconstructs an original block from the recovery
        blocks, which is much slower than copying from the input data, so
        should be done selectively.  This is only done during decoding.

        Precondition: DecodeFeed() has returned success

        Returns Wirehair_Success on success.
        + block_out will be filled with the reconstructed block
        + bytes_out will be the number of bytes in the block

        Returns other codes on error.
    */
    WirehairResult ReconstructBlock(
        const uint16_t block_id, ///< Block identifier
        void * GF256_RESTRICT block_out, ///< Output block memory
        uint32_t* bytes_out ///< Bytes written to output
    );

    /// Transition from decoder to encoder mode
    /// Precondition: DecodeFeed() succeeded with Wirehair_Success
    WirehairResult InitializeEncoderFromDecoder();
};


} // namespace wirehair

#endif // WIREHAIR_CODEC_H
