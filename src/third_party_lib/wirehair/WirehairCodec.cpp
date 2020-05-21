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

#include "WirehairCodec.h"


//------------------------------------------------------------------------------
// Precompiler-conditional console output

#if defined(CAT_DUMP_PIVOT_FAIL)
#define CAT_IF_PIVOT(x) x
#else
#define CAT_IF_PIVOT(x)
#endif

#if defined(CAT_DUMP_CODEC_DEBUG)
#define CAT_IF_DUMP(x) x
#else
#define CAT_IF_DUMP(x)
#endif

#if defined(CAT_DUMP_ROWOP_COUNTERS)
#define CAT_IF_ROWOP(x) x
#else
#define CAT_IF_ROWOP(x)
#endif

#if defined(CAT_DUMP_CODEC_DEBUG) || defined(CAT_DUMP_PIVOT_FAIL) || \
    defined(CAT_DUMP_ROWOP_COUNTERS) || defined(CAT_DUMP_GE_MATRIX)
#include <iostream>
#include <iomanip>
#include <fstream>
using namespace std;
#endif


namespace wirehair {


//------------------------------------------------------------------------------
// Stage (1) Peeling:

bool Codec::OpportunisticPeeling(
    const uint16_t row_i, ///< Row index
    const uint32_t row_seed ///< Row PRNG seed
)
{
    PeelRow *row = &_peel_rows[row_i];

    row->RecoveryId = row_seed;

    row->Params.Initialize(row_seed, _p_seed, _block_count, _mix_count);

    CAT_IF_DUMP(cout << "Row " << row_seed << " in slot " << row_i << " of weight "
        << row->Params.PeelCount << " [a=" << row->Params.PeelAdd << "] : ";)

    PeelRowIterator iter(row->Params, _block_count, _block_next_prime);

    uint16_t unmarked_count = 0;
    uint16_t unmarked[2];

    // Iterate columns in peeling matrix
    do
    {
        const uint16_t column_i = iter.GetColumn();

        CAT_IF_DUMP(cout << column_i << " ";)

        PeelRefs *refs = &_peel_col_refs[column_i];

        // If there was not enough room in the reference list:
        if (refs->RowCount >= CAT_REF_LIST_MAX)
        {
            CAT_IF_DUMP(cout << "OpportunisticPeeling: Failure!  " \
                "Ran out of space for row references.  CAT_REF_LIST_MAX must be increased!" << endl;)

            CAT_DEBUG_BREAK();

            FixPeelFailure(row, column_i);

            return false;
        }

        // Add row reference to column
        refs->Rows[refs->RowCount++] = row_i;

        // If column is unmarked:
        if (_peel_cols[column_i].Mark == MARK_TODO) {
            unmarked[unmarked_count++ & 1] = column_i;
        }
    } while (iter.Iterate());

    CAT_IF_DUMP(cout << endl;)

    // Initialize row state
    row->UnmarkedCount = unmarked_count;

    switch (unmarked_count)
    {
    case 0:
        // Link at head of defer list
        row->NextRow = _defer_head_rows;
        _defer_head_rows = row_i;
        break;

    case 1:
        // Solve only unmarked column with this row
        SolveWithPeel(
            row,
            row_i,
            unmarked[0]);
        break;

    case 2:
        // Remember which two columns were unmarked
        row->Marks.Unmarked[0] = unmarked[0];
        row->Marks.Unmarked[1] = unmarked[1];

        // Increment weight-2 reference count for unmarked columns
        _peel_cols[unmarked[0]].Weight2Refs++;
        _peel_cols[unmarked[1]].Weight2Refs++;
        break;
    }

    return true;
}

void Codec::FixPeelFailure(
    PeelRow * GF256_RESTRICT row, ///< The row that failed
    const uint16_t fail_column_i  ///< Column end point
)
{
    CAT_IF_DUMP(cout << "!!Fixing Peel Failure!! Unreferencing columns, ending at "
        << fail_column_i << " :";)

    PeelRowIterator iter(row->Params, _block_count, _block_next_prime);

    // Iterate columns in peeling matrix
    do
    {
        const uint16_t column = iter.GetColumn();

        if (column == fail_column_i) {
            break;
        }

        CAT_IF_DUMP(cout << " " << column;)

        PeelRefs * GF256_RESTRICT refs = &_peel_col_refs[column];

        // Subtract off row count.
        // This invalidates the row number that was written earlier
        refs->RowCount--;
    } while (iter.Iterate());
    CAT_IF_DUMP(cout << endl;)
}

void Codec::PeelAvalancheOnSolve(
    uint16_t column_i ///< Column that was solved
)
{
    PeelRefs * GF256_RESTRICT refs = &_peel_col_refs[column_i];
    uint16_t ref_row_count = refs->RowCount;
    uint16_t * GF256_RESTRICT ref_rows = refs->Rows;

    // Walk list of peeled rows referenced by this newly solved column
    while (ref_row_count--)
    {
        // Update unmarked row count for this referenced row
        uint16_t ref_row_i = *ref_rows++;
        PeelRow * GF256_RESTRICT ref_row = &_peel_rows[ref_row_i];
        uint16_t unmarked_count = --ref_row->UnmarkedCount;

        // If row may be solving a column now:
        if (unmarked_count == 1)
        {
            uint16_t new_column_i = ref_row->Marks.Unmarked[0];

            // If that is this column:
            if (new_column_i == column_i) {
                new_column_i = ref_row->Marks.Unmarked[1];
            }

            /*
                Rows that are to be deferred will either end up
                here or below where it handles the case of there
                being no columns unmarked in a row.
            */

            // If column is already solved:
            if (_peel_cols[new_column_i].Mark == MARK_TODO)
            {
                SolveWithPeel(
                    ref_row,
                    ref_row_i,
                    new_column_i);
                continue;
            }

            CAT_IF_DUMP(cout << "PeelAvalancheOnSolve: Deferred(1) with column " <<
                column_i << " at row " << ref_row_i << endl;)

            // Link at head of defer list
            ref_row->NextRow = _defer_head_rows;
            _defer_head_rows = ref_row_i;
        }
        else if (unmarked_count == 2)
        {
            // Regenerate the row columns to discover which are unmarked
            PeelRowIterator ref_iter(ref_row->Params, _block_count, _block_next_prime);

            uint16_t store_count = 0;

            // For each column:
            do
            {
                const uint16_t ref_column_i = ref_iter.GetColumn();

                PeelColumn * GF256_RESTRICT ref_col = &_peel_cols[ref_column_i];

                // If column is unmarked:
                if (ref_col->Mark == MARK_TODO)
                {
                    // Store the two unmarked columns in the row
                    ref_row->Marks.Unmarked[store_count++] = ref_column_i;

                    // Increment weight-2 reference count (cannot hurt even if not true)
                    ref_col->Weight2Refs++;
                }
            } while (ref_iter.Iterate());

            /*
                This is a little subtle, but sometimes the avalanche will
                happen here, and sometimes a row will be marked deferred.
            */

            if (store_count <= 1)
            {
                // Insure that this row won't be processed further during this recursion
                ref_row->UnmarkedCount = 0;

                // If row is to be deferred:
                if (store_count == 1)
                {
                    SolveWithPeel(
                        ref_row,
                        ref_row_i,
                        ref_row->Marks.Unmarked[0]);
                    continue;
                }

                CAT_IF_DUMP(cout << "PeelAvalancheOnSolve: Deferred(2) with column " << column_i << " at row " << ref_row_i << endl;)

                // Link at head of defer list
                ref_row->NextRow = _defer_head_rows;
                _defer_head_rows = ref_row_i;
            }
        }
    }
}

void Codec::SolveWithPeel(
    PeelRow * GF256_RESTRICT row, ///< Pointer to row data
    uint16_t row_i,   ///< Row index
    uint16_t column_i ///< Column that this solves
)
{
    CAT_IF_DUMP(cout << "Peel: Solved column " << column_i << " with row " << row_i << endl;)

    PeelColumn * GF256_RESTRICT column = &_peel_cols[column_i];

    // Mark this column as solved
    column->Mark = MARK_PEEL;

    // Remember which column it solves
    row->Marks.Result.PeelColumn = column_i;

    // Link to back of the peeled list
    if (_peel_tail_rows) {
        _peel_tail_rows->NextRow = row_i;
    }
    else {
        _peel_head_rows = row_i;
    }
    row->NextRow = LIST_TERM;
    _peel_tail_rows = row;

    // Indicate that this row hasn't been copied yet
    row->Marks.Result.IsCopied = 0;

    // Attempt to avalanche and solve other columns
    PeelAvalancheOnSolve(column_i);

    // Remember which row solves the column, after done with rows list
    column->PeelRow = row_i;
}

void Codec::GreedyPeeling()
{
    CAT_IF_DUMP(cout << endl << "---- GreedyPeeling ----" << endl << endl;)

    // Initialize list
    _defer_head_columns = LIST_TERM;
    _defer_count = 0;

    const unsigned block_count = _block_count;

    // Until all columns are marked:
    for (;;)
    {
        uint16_t best_column_i = LIST_TERM;
        unsigned best_w2_refs = 0;
        unsigned best_row_count = 0;

        const PeelColumn *column = _peel_cols;

        // For each peel column:
        for (uint16_t column_i = 0; column_i < block_count; ++column_i, ++column)
        {
            // If column is not marked yet:
            if (column->Mark == MARK_TODO)
            {
                const unsigned w2_refs = column->Weight2Refs;

                // If it may have the most weight-2 references:
                if (w2_refs >= best_w2_refs)
                {
                    const unsigned row_count = _peel_col_refs[column_i].RowCount;

                    // If it has the largest row references overall:
                    if (w2_refs > best_w2_refs || row_count >= best_row_count)
                    {
                        // Use that one
                        best_column_i = column_i;
                        best_w2_refs = w2_refs;
                        best_row_count = row_count;
                    }
                }
            }
        }

        // If no column was found:
        if (best_column_i == LIST_TERM) {
            // Peeling is complete
            break;
        }

        // Mark column as deferred
        PeelColumn *best_column = &_peel_cols[best_column_i];
        best_column->Mark = MARK_DEFER;
        ++_defer_count;

        // Add at head of deferred list
        best_column->Next = _defer_head_columns;
        _defer_head_columns = best_column_i;

        CAT_IF_DUMP(cout << "Deferred column " << best_column_i <<
            " for Gaussian elimination, which had " << best_column->Weight2Refs <<
            " weight-2 row references" << endl;)

        // Peel resuming from where this column left off
        PeelAvalancheOnSolve(best_column_i);
    }
}


//------------------------------------------------------------------------------
// Stage (2) Compression

void Codec::SetDeferredColumns()
{
    CAT_IF_DUMP(cout << endl << "---- SetDeferredColumns ----" << endl << endl;)

    PeelColumn * GF256_RESTRICT column;

    // For each deferred column:
    for (uint16_t ge_column_i = 0, defer_i = _defer_head_columns;
        defer_i != LIST_TERM;
        defer_i = column->Next, ++ge_column_i)
    {
        column = &_peel_cols[defer_i];

        CAT_IF_DUMP(cout << "GE column " << ge_column_i <<
            " mapped to matrix column " << defer_i << " :";)

        // Get pointer to this matrix row
        uint64_t *matrix_row_offset = _compress_matrix + (ge_column_i >> 6);

        // Get word mask for this column bit
        const uint64_t ge_mask = (uint64_t)1 << (ge_column_i & 63);

        // Get references for this deferred index
        const PeelRefs * GF256_RESTRICT refs = &_peel_col_refs[defer_i];

        // For each affected row:
        for (unsigned i = 0, count = refs->RowCount; i < count; ++i)
        {
            const uint16_t row_i = refs->Rows[i];

            CAT_IF_DUMP(cout << " " << row_i;)

            matrix_row_offset[_ge_pitch * row_i] |= ge_mask;
        }

        CAT_IF_DUMP(cout << endl;)

        // Set column map for this GE column
        _ge_col_map[ge_column_i] = defer_i;

        // Set reverse mapping also
        column->GEColumn = ge_column_i;
    }

    // Set column map for each mix column:
    for (uint16_t added_i = 0, count = _mix_count; added_i < count; ++added_i)
    {
        CAT_DEBUG_ASSERT((unsigned)_defer_count + (unsigned)added_i < 65536);
        CAT_DEBUG_ASSERT((unsigned)_block_count + (unsigned)added_i < 65536);
        const uint16_t ge_column_i = _defer_count + added_i;
        const uint16_t column_i = _block_count + added_i;

        CAT_IF_DUMP(cout << "GE column(mix) " << ge_column_i <<
            " mapped to matrix column " << column_i << endl;)

        _ge_col_map[ge_column_i] = column_i;
    }
}

void Codec::SetMixingColumnsForDeferredRows()
{
    CAT_IF_DUMP(cout << endl << "---- SetMixingColumnsForDeferredRows ----" << endl << endl;)

    PeelRow * GF256_RESTRICT row;

    // For each deferred row:
    for (uint16_t defer_row_i = _defer_head_rows;
        defer_row_i != LIST_TERM;
        defer_row_i = row->NextRow)
    {
        row = &_peel_rows[defer_row_i];

        CAT_IF_DUMP(cout << "Deferred row " << defer_row_i << " set mix columns :";)

        // Mark it as deferred for the following loop
        row->Marks.Result.PeelColumn = LIST_TERM;

        // Set up mixing column generator
        uint64_t *ge_row = _compress_matrix + _ge_pitch * defer_row_i;

        const unsigned defer_count = _defer_count;
        const RowMixIterator mix(row->Params, _mix_count, _mix_next_prime);

        // Generate mixing column 1
        const unsigned ge_column_i = defer_count + mix.Columns[0];
        ge_row[ge_column_i >> 6] ^= (uint64_t)1 << (ge_column_i & 63);
        CAT_IF_DUMP(cout << " " << ge_column_i;)

        // Generate mixing column 2
        const unsigned ge_column_j = defer_count + mix.Columns[1];
        ge_row[ge_column_j >> 6] ^= (uint64_t)1 << (ge_column_j & 63);
        CAT_IF_DUMP(cout << " " << ge_column_j;)

        // Generate mixing column 3
        const unsigned ge_column_k = defer_count + mix.Columns[2];
        ge_row[ge_column_k >> 6] ^= (uint64_t)1 << (ge_column_k & 63);
        CAT_IF_DUMP(cout << " " << ge_column_k;)

        CAT_IF_DUMP(cout << endl;)
    }
}

void Codec::PeelDiagonal()
{
    CAT_IF_DUMP(cout << endl << "---- PeelDiagonal ----" << endl << endl;)

    /*
        This function optimizes the block value generation by combining the first
        memcpy and memxor operations together into a three-way memxor if possible,
        using the is_copied row member.
    */

    CAT_IF_ROWOP(unsigned rowops = 0;)

    PeelRow * GF256_RESTRICT row;

    // For each peeled row in forward solution order:
    for (uint16_t peel_row_i = _peel_head_rows;
        peel_row_i != LIST_TERM;
        peel_row_i = row->NextRow)
    {
        row = &_peel_rows[peel_row_i];

        // Lookup peeling results
        const uint16_t peel_column_i = row->Marks.Result.PeelColumn;
        uint64_t *ge_row = _compress_matrix + _ge_pitch * peel_row_i;

        CAT_IF_DUMP(cout << "Peeled row " << peel_row_i << " for peeled column " << peel_column_i << " :";)

        const unsigned defer_count = _defer_count;
        const RowMixIterator mix(row->Params, _mix_count, _mix_next_prime);

        // Generate mixing column 1
        const unsigned ge_column_i = defer_count + mix.Columns[0];
        ge_row[ge_column_i >> 6] ^= (uint64_t)1 << (ge_column_i & 63);
        CAT_IF_DUMP(cout << " " << ge_column_i;)

        // Generate mixing column 2
        const unsigned ge_column_j = defer_count + mix.Columns[1];
        ge_row[ge_column_j >> 6] ^= (uint64_t)1 << (ge_column_j & 63);
        CAT_IF_DUMP(cout << " " << ge_column_j;)

        // Generate mixing column 3
        const unsigned ge_column_k = defer_count + mix.Columns[2];
        ge_row[ge_column_k >> 6] ^= (uint64_t)1 << (ge_column_k & 63);
        CAT_IF_DUMP(cout << " " << ge_column_k << endl;)

        // Get pointer to output block
        CAT_DEBUG_ASSERT(peel_column_i < _recovery_rows);
        uint8_t * GF256_RESTRICT temp_block_src = _recovery_blocks + _block_bytes * peel_column_i;

        // If row has not been copied yet:
        if (!row->Marks.Result.IsCopied)
        {
            const uint8_t * GF256_RESTRICT block_src = _input_blocks + _block_bytes * peel_row_i;

            // If this is not the last block:
            if (peel_row_i != _block_count - 1) {
                // Copy it directly to the output block
                memcpy(temp_block_src, block_src, _block_bytes);
            }
            else
            {
                // Copy with zero padding
                memcpy(temp_block_src, block_src, _input_final_bytes);
                CAT_DEBUG_ASSERT(_block_bytes >= _input_final_bytes);
                memset(temp_block_src + _input_final_bytes, 0, _block_bytes - _input_final_bytes);
            }
            CAT_IF_ROWOP(++rowops;)

            CAT_IF_DUMP(cout << "-- Copied from " << peel_row_i <<
                " because has not been copied yet.  Output block = " <<
                (unsigned)temp_block_src[0] << endl;)

            // Note that we do not need to set is_copied here because no
            // further rows reference this one
        }

        CAT_IF_DUMP(cout << "++ Adding to referencing rows:";)

        PeelRefs * GF256_RESTRICT refs = &_peel_col_refs[peel_column_i];
        const uint16_t * GF256_RESTRICT referencingRows = refs->Rows;

        // For each row that references this one:
        for (unsigned i = 0, count = refs->RowCount; i < count; ++i)
        {
            const uint16_t ref_row_i = referencingRows[i];

            // If it references the current row:
            if (ref_row_i == peel_row_i) {
                // Skip this row
                continue;
            }

            CAT_IF_DUMP(cout << " " << ref_row_i;)

            uint64_t * GF256_RESTRICT ge_ref_row = _compress_matrix + _ge_pitch * ref_row_i;

            // Add GE row to referencing GE row
            for (unsigned j = 0; j < _ge_pitch; ++j) {
                ge_ref_row[j] ^= ge_row[j];
            }

            PeelRow * GF256_RESTRICT ref_row = &_peel_rows[ref_row_i];
            const uint16_t ref_column_i = ref_row->Marks.Result.PeelColumn;

            // If row is peeled:
            if (ref_column_i != LIST_TERM)
            {
                // Generate temporary row block value:
                CAT_DEBUG_ASSERT(ref_column_i < _recovery_rows);
                uint8_t * GF256_RESTRICT temp_block_dest = _recovery_blocks + _block_bytes * ref_column_i;

                // If referencing row is already copied to the recovery blocks:
                if (ref_row->Marks.Result.IsCopied) {
                    // Add this row block value to it
                    gf256_add_mem(temp_block_dest, temp_block_src, _block_bytes);
                }
                else
                {
                    const uint8_t * GF256_RESTRICT block_src = _input_blocks + _block_bytes * ref_row_i;

                    // If this is not the last block:
                    if (ref_row_i != _block_count - 1) {
                        // Add this row block value with message block to it (optimization)
                        gf256_addset_mem(temp_block_dest, temp_block_src, block_src, _block_bytes);
                    }
                    else
                    {
                        // Add with zero padding
                        gf256_addset_mem(temp_block_dest, temp_block_src, block_src, _input_final_bytes);

                        CAT_DEBUG_ASSERT(_block_bytes >= _input_final_bytes);
                        memcpy(
                            temp_block_dest + _input_final_bytes,
                            temp_block_src + _input_final_bytes,
                            _block_bytes - _input_final_bytes);
                    }

                    ref_row->Marks.Result.IsCopied = 1;
                }

                CAT_IF_ROWOP(++rowops;)
            } // end if referencing row is peeled

        } // next referencing row

        CAT_IF_DUMP(cout << endl;)

    } // next peeled row

    CAT_IF_ROWOP(cout << "PeelDiagonal used " << rowops << " row ops = "
        << rowops / (double)_block_count << "*N" << endl;)
}

void Codec::CopyDeferredRows()
{
    CAT_IF_DUMP(cout << endl << "---- CopyDeferredRows ----" << endl << endl;)

    // Get GE matrix row starting at dense rows
    uint64_t * GF256_RESTRICT ge_row = _ge_matrix + _ge_pitch * _dense_count;

    // For each deferred row:
    for (uint16_t ge_row_i = _dense_count, defer_row_i = _defer_head_rows;
        defer_row_i != LIST_TERM;
        ++ge_row_i, ge_row += _ge_pitch)
    {
        CAT_IF_DUMP(cout << "Peeled row " << defer_row_i << " for GE row " << ge_row_i << endl;)

        // Get Compress matrix row
        uint64_t * GF256_RESTRICT compress_row = _compress_matrix + _ge_pitch * defer_row_i;

        // Copy Compress row to GE row
        memcpy(ge_row, compress_row, _ge_pitch * sizeof(uint64_t));

        // Set row map for this deferred row
        _ge_row_map[ge_row_i] = defer_row_i;

        // Get next deferred row from peeling solver output
        defer_row_i = _peel_rows[defer_row_i].NextRow;
    }
}

void Codec::MultiplyDenseRows()
{
    CAT_IF_DUMP(cout << endl << "---- MultiplyDenseRows ----" << endl << endl;)

    // Initialize PRNG
    PCGRandom prng;
    prng.Seed(_d_seed);

    const PeelColumn * GF256_RESTRICT column = _peel_cols;
    uint64_t * GF256_RESTRICT temp_row = _ge_matrix + _ge_pitch * (_dense_count + _defer_count);
    const unsigned dense_count = _dense_count;
    uint16_t rows[CAT_MAX_DENSE_ROWS];
    uint16_t bits[CAT_MAX_DENSE_ROWS];

    // For each block of columns:
    for (unsigned column_i = 0, block_count = _block_count;
        column_i < block_count;
        column_i += dense_count, column += dense_count)
    {
        CAT_IF_DUMP(cout << "Shuffled dense matrix starting at column "
            << column_i << ":" << endl;)

        unsigned max_x = dense_count;

        // Handle final columns
        if (column_i + dense_count > _block_count) {
            CAT_DEBUG_ASSERT(_block_count >= column_i);
            max_x = _block_count - column_i;
        }

        // Shuffle row and bit order
        ShuffleDeck16(prng, rows, dense_count);
        ShuffleDeck16(prng, bits, dense_count);

        // Initialize counters
        const unsigned set_count = (dense_count + 1) >> 1;
        const uint16_t * GF256_RESTRICT set_bits = bits;
        const uint16_t * GF256_RESTRICT clr_bits = set_bits + set_count;

        CAT_IF_DUMP(uint64_t disp_row[(CAT_MAX_DENSE_ROWS + 63) / 64] = {};)

        memset(temp_row, 0, _ge_pitch * sizeof(uint64_t));

        // Generate first row
        for (unsigned ii = 0; ii < set_count; ++ii)
        {
            const unsigned bit_i = set_bits[ii];

            // If bit is peeled:
            if (bit_i < max_x)
            {
                if (column[bit_i].Mark == MARK_PEEL)
                {
                    const uint64_t * GF256_RESTRICT ge_source_row = _compress_matrix + _ge_pitch * column[bit_i].PeelRow;

                    // Add temp row value
                    for (unsigned jj = 0, ge_pitch = _ge_pitch; jj < ge_pitch; ++jj) {
                        temp_row[jj] ^= ge_source_row[jj];
                    }
                }
                else
                {
                    const unsigned ge_column_i = column[bit_i].GEColumn;

                    // Set GE bit for deferred column
                    temp_row[ge_column_i >> 6] ^= (uint64_t)1 << (ge_column_i & 63);
                }
            }

            CAT_IF_DUMP(disp_row[bit_i >> 6] ^= (uint64_t)1 << (bit_i & 63);)
        } // next bit

        // Set up generator
        const uint16_t * GF256_RESTRICT row = rows;

        // Store first row
        CAT_IF_DUMP(for (unsigned ii = 0; ii < dense_count; ++ii) {
            cout << ((disp_row[ii >> 6] & ((uint64_t)1 << (ii & 63))) ? '1' : '0');
            cout << " <- going to row " << *row << endl;
        })

        uint64_t * GF256_RESTRICT ge_dest_row = _ge_matrix + _ge_pitch * *row++;

        // Add to destination row
        for (unsigned jj = 0, ge_pitch = _ge_pitch; jj < ge_pitch; ++jj) {
            ge_dest_row[jj] ^= temp_row[jj];
        }

        // Reshuffle bit order: Shuffle-2 Code
        ShuffleDeck16(prng, bits, dense_count);

        const unsigned loop_count = (dense_count >> 1);

        // Generate first half of rows
        for (unsigned ii = 0; ii < loop_count; ++ii)
        {
            const unsigned bit0 = set_bits[ii];
            const unsigned bit1 = clr_bits[ii];

            // Flip bit 1
            if (bit0 < max_x)
            {
                if (column[bit0].Mark == MARK_PEEL)
                {
                    const uint16_t bit0_row = column[bit0].PeelRow;
                    const uint64_t * GF256_RESTRICT ge_source_row = _compress_matrix + _ge_pitch * bit0_row;

                    // Add temp row value
                    for (unsigned jj = 0, ge_pitch = _ge_pitch; jj < ge_pitch; ++jj) {
                        temp_row[jj] ^= ge_source_row[jj];
                    }
                }
                else
                {
                    const unsigned ge_column_i = column[bit0].GEColumn;

                    // Set GE bit for deferred column
                    temp_row[ge_column_i >> 6] ^= (uint64_t)1 << (ge_column_i & 63);
                }
            }

            CAT_IF_DUMP(disp_row[bit0 >> 6] ^= (uint64_t)1 << (bit0 & 63);)

            // Flip bit 2
            if (bit1 < max_x)
            {
                if (column[bit1].Mark == MARK_PEEL)
                {
                    const uint16_t bit1_row = column[bit1].PeelRow;
                    const uint64_t * GF256_RESTRICT ge_source_row = _compress_matrix + _ge_pitch * bit1_row;

                    // Add temp row value
                    for (unsigned jj = 0, ge_pitch = _ge_pitch; jj < ge_pitch; ++jj) {
                        temp_row[jj] ^= ge_source_row[jj];
                    }
                }
                else
                {
                    const unsigned ge_column_i = column[bit1].GEColumn;

                    // Set GE bit for deferred column
                    temp_row[ge_column_i >> 6] ^= (uint64_t)1 << (ge_column_i & 63);
                }
            }

            CAT_IF_DUMP(disp_row[bit1 >> 6] ^= (uint64_t)1 << (bit1 & 63);)

            // Store in row
            CAT_IF_DUMP(for (unsigned jj = 0; jj < dense_count; ++jj) {
                cout << ((disp_row[jj >> 6] & ((uint64_t)1 << (jj & 63))) ? '1' : '0');
                cout << " <- going to row " << *row << endl;
            })

            ge_dest_row = _ge_matrix + _ge_pitch * (*row++);

            for (unsigned jj = 0, ge_pitch = _ge_pitch; jj < ge_pitch; ++jj) {
                ge_dest_row[jj] ^= temp_row[jj];
            }
        } // next row

        // Reshuffle bit order: Shuffle-2 Code
        ShuffleDeck16(prng, bits, dense_count);

        const unsigned second_loop_count = loop_count - 1 + (dense_count & 1);

        // Generate second half of rows
        for (unsigned ii = 0; ii < second_loop_count; ++ii)
        {
            const unsigned bit0 = set_bits[ii];
            const unsigned bit1 = clr_bits[ii];

            // Flip bit 1
            if (bit0 < max_x)
            {
                if (column[bit0].Mark == MARK_PEEL)
                {
                    const uint16_t bit0_row = column[bit0].PeelRow;
                    const uint64_t * GF256_RESTRICT ge_source_row = _compress_matrix + _ge_pitch * bit0_row;

                    // Add temp row value
                    for (unsigned jj = 0, ge_pitch = _ge_pitch; jj < ge_pitch; ++jj) {
                        temp_row[jj] ^= ge_source_row[jj];
                    }
                }
                else
                {
                    const unsigned ge_column_i = column[bit0].GEColumn;

                    // Set GE bit for deferred column
                    temp_row[ge_column_i >> 6] ^= (uint64_t)1 << (ge_column_i & 63);
                }
            }
            CAT_IF_DUMP(disp_row[bit0 >> 6] ^= (uint64_t)1 << (bit0 & 63);)

            // Flip bit 2
            if (bit1 < max_x)
            {
                if (column[bit1].Mark == MARK_PEEL)
                {
                    const uint16_t bit1_row = column[bit1].PeelRow;
                    const uint64_t * GF256_RESTRICT ge_source_row = _compress_matrix + _ge_pitch * bit1_row;

                    // Add temp row value
                    for (unsigned jj = 0, ge_pitch = _ge_pitch; jj < ge_pitch; ++jj) {
                        temp_row[jj] ^= ge_source_row[jj];
                    }
                }
                else
                {
                    const unsigned ge_column_i = column[bit1].GEColumn;

                    // Set GE bit for deferred column
                    temp_row[ge_column_i >> 6] ^= (uint64_t)1 << (ge_column_i & 63);
                }
            }
            CAT_IF_DUMP(disp_row[bit1 >> 6] ^= (uint64_t)1 << (bit1 & 63);)

            // Store in row
            CAT_IF_DUMP(for (unsigned kk = 0; kk < dense_count; ++kk) {
                cout << ((disp_row[kk >> 6] & ((uint64_t)1 << (kk & 63))) ? '1' : '0');
                cout << " <- going to row " << *row << endl;
            })

            ge_dest_row = _ge_matrix + _ge_pitch * (*row++);

            for (unsigned jj = 0, ge_pitch = _ge_pitch; jj < ge_pitch; ++jj) {
                ge_dest_row[jj] ^= temp_row[jj];
            }
        } // next row

        CAT_IF_DUMP(cout << endl;)
    } // next column
}

// This Cauchy matrix is generated by HeavyRowGenerator.cpp
// It has a special property that stacked random binary matrices do not affect
// its inversion rate.  Honestly I haven't looked into why, but it's not a
// common property for Cauchy matrices, and I didn't know this was possible.
static const uint8_t kHeavyMatrix[kHeavyRows][kHeavyCols] = {
    { 0x85, 0xd3, 0x66, 0xf3, 0x38, 0x95, 0x56, 0xad, 0x57, 0xaf, 0x58, 0x48, 0xbc, 0xfa, 0x02, 0xc5, 0x43, 0xe8, },
    { 0xd3, 0x85, 0xf3, 0x66, 0x95, 0x38, 0xad, 0x56, 0xaf, 0x57, 0x48, 0x58, 0xfa, 0xbc, 0xc5, 0x02, 0xe8, 0x43, },
    { 0x82, 0x22, 0x57, 0xaf, 0x56, 0xad, 0x38, 0x95, 0x66, 0xf3, 0x43, 0xe8, 0x02, 0xc5, 0xbc, 0xfa, 0x58, 0x48, },
    { 0x22, 0x82, 0xaf, 0x57, 0xad, 0x56, 0x95, 0x38, 0xf3, 0x66, 0xe8, 0x43, 0xc5, 0x02, 0xfa, 0xbc, 0x48, 0x58, },
    { 0x51, 0x34, 0x56, 0xad, 0x57, 0xaf, 0x66, 0xf3, 0x38, 0x95, 0x02, 0xc5, 0x43, 0xe8, 0x58, 0x48, 0xbc, 0xfa, },
    { 0x34, 0x51, 0xad, 0x56, 0xaf, 0x57, 0xf3, 0x66, 0x95, 0x38, 0xc5, 0x02, 0xe8, 0x43, 0x48, 0x58, 0xfa, 0xbc, },
};

void Codec::SetHeavyRows()
{
    CAT_IF_DUMP(cout << endl << "---- SetHeavyRows ----" << endl << endl;)

    // Skip extra rows
    uint8_t * GF256_RESTRICT heavy_offset = _heavy_matrix + _heavy_pitch * _extra_count;

    uint8_t * GF256_RESTRICT heavy_row = heavy_offset;

    // For each heavy matrix word:
    for (unsigned row_i = 0; row_i < kHeavyRows; ++row_i, heavy_row += _heavy_pitch)
    {
        // NOTE: Each heavy row is a multiple of 4 bytes in size
        for (unsigned col_i = 0; col_i < _heavy_columns; col_i++) {
            heavy_row[col_i] = kHeavyMatrix[row_i][col_i];
        }
    }

#ifdef CAT_IDENTITY_LOWER_RIGHT
    uint8_t * GF256_RESTRICT lower_right = heavy_offset + _heavy_columns - kHeavyRows;

    // Add identity matrix to tie heavy rows to heavy mixing columns
    for (unsigned ii = 0; ii < kHeavyRows; ++ii, lower_right += _heavy_pitch)
    {
        for (unsigned jj = 0; jj < kHeavyRows; ++jj) {
            lower_right[jj] = (ii == jj) ? 1 : 0;
        }
    }
#endif
}


//------------------------------------------------------------------------------
// Stage (3) Gaussian Elimination

void Codec::SetupTriangle()
{
    CAT_IF_DUMP(cout << endl << "---- SetupTriangle ----" << endl << endl;)

    CAT_DEBUG_ASSERT((unsigned)_defer_count + (unsigned)_dense_count < 65536);
    const uint16_t pivot_count = _defer_count + _dense_count;

    // Initialize pivot array to just non-heavy rows
    for (uint16_t pivot_i = 0; pivot_i < pivot_count; ++pivot_i) {
        _pivots[pivot_i] = pivot_i;
    }

    // Set resume point to the first column
    _next_pivot = 0;
    _pivot_count = pivot_count;

    // If heavy rows are used right from the start:
    if (_first_heavy_column <= 0) {
        InsertHeavyRows();
    }
}

void Codec::InsertHeavyRows()
{
    CAT_IF_DUMP(cout << endl << "---- InsertHeavyRows ----" << endl << endl;)

    CAT_IF_DUMP(cout << "Converting remaining extra rows to heavy...";)

    // Initialize index of first heavy pivot
    unsigned first_heavy_pivot = _pivot_count;

    const uint16_t column_count = _defer_count + _mix_count;
    const uint16_t first_heavy_row = _defer_count + _dense_count;

    // For each remaining pivot in the list:
    for (int pivot_j = (int)_pivot_count - 1; pivot_j >= 0; --pivot_j)
    {
        CAT_DEBUG_ASSERT((unsigned)pivot_j < _pivot_count);
        const uint16_t ge_row_j = _pivots[pivot_j];

        // If row is extra:
        if (ge_row_j < first_heavy_row) {
            continue;
        }

        // If pivot is still unused:
        if ((unsigned)pivot_j >= _next_pivot)
        {
            CAT_DEBUG_ASSERT(first_heavy_pivot > 0);
            --first_heavy_pivot;

            // Swap pivot j into last heavy pivot position
            CAT_DEBUG_ASSERT(first_heavy_pivot < _pivot_count);
            _pivots[pivot_j] = _pivots[first_heavy_pivot];
            _pivots[first_heavy_pivot] = ge_row_j;
        }

        CAT_IF_DUMP(cout << " row=" << ge_row_j << ", pivot=" << pivot_j;)

        CAT_DEBUG_ASSERT(ge_row_j >= first_heavy_row);
        uint8_t * GF256_RESTRICT extra_row = _heavy_matrix + _heavy_pitch * (ge_row_j - first_heavy_row);
        const uint64_t * GF256_RESTRICT ge_extra_row = _ge_matrix + _ge_pitch * ge_row_j;

        // Copy heavy columns to heavy matrix row

        for (unsigned ge_column_j = _first_heavy_column; ge_column_j < column_count; ++ge_column_j)
        {
            // Find bit for this column
            const uint64_t column_bit = (ge_extra_row[ge_column_j >> 6] >> (ge_column_j & 63)) & 1;

            // Set extra row byte to 0 or 1
            CAT_DEBUG_ASSERT(ge_column_j >= _first_heavy_column);
            extra_row[ge_column_j - _first_heavy_column] = static_cast<uint8_t>(column_bit);
        }
    }

    CAT_IF_DUMP(cout << endl;)

    // Store first heavy pivot index
    _first_heavy_pivot = first_heavy_pivot;

    // Add heavy rows at the end to cause them to be selected last if given a choice
    for (uint16_t heavy_i = 0; heavy_i < kHeavyRows; ++heavy_i) {
        // Use GE row index after extra count even if not all are used yet
        _pivots[_pivot_count + heavy_i] = first_heavy_row + _extra_count + heavy_i;
    }

    _pivot_count += kHeavyRows;

    CAT_IF_DUMP(cout << "Added heavy rows to the end of the pivots list." << endl;)
}

bool Codec::TriangleNonHeavy()
{
    CAT_IF_DUMP(cout << endl << "---- TriangleNonHeavy ----" << endl << endl;)

    const unsigned pivot_count = _pivot_count;
    const unsigned first_heavy_column = _first_heavy_column;

    unsigned pivot_i = _next_pivot;
    uint64_t ge_mask = (uint64_t)1 << (pivot_i & 63);

    // For the columns that are not protected by heavy rows:
    for (; pivot_i < first_heavy_column; ++pivot_i)
    {
        const unsigned word_offset = pivot_i >> 6;

        bool found = false;

        uint64_t * GF256_RESTRICT ge_matrix_offset = _ge_matrix + word_offset;

        // For each remaining GE row that might be the pivot:
        for (unsigned pivot_j = pivot_i; pivot_j < pivot_count; ++pivot_j)
        {
            // Determine if the row contains the bit we want
            const unsigned ge_row_j = _pivots[pivot_j];

            uint64_t * GF256_RESTRICT ge_row = &ge_matrix_offset[_ge_pitch * ge_row_j];

            // If the bit was not found:
            if (0 == (*ge_row & ge_mask)) {
                continue; // Skip to next
            }

            // Found it!
            found = true;

            CAT_IF_DUMP(cout << "Pivot " << pivot_i << " found on row " << ge_row_j << endl;)

            // Swap out the pivot index for this one
            _pivots[pivot_j] = _pivots[pivot_i];
            CAT_DEBUG_ASSERT(ge_row_j < _pivot_count);
            _pivots[pivot_i] = (uint16_t)ge_row_j;

            // Prepare masked first word
            const uint64_t row0 = (*ge_row & ~(ge_mask - 1)) ^ ge_mask;

            // For each remaining unused row:
            for (unsigned pivot_k = pivot_j + 1; pivot_k < pivot_count; ++pivot_k)
            {
                // Determine if the row contains the bit we want
                const unsigned ge_row_k = _pivots[pivot_k];

                uint64_t * GF256_RESTRICT rem_row = &ge_matrix_offset[_ge_pitch * ge_row_k];

                // If the bit was found:
                if (0 != (*rem_row & ge_mask))
                {
                    // Unroll first word to handle masked word and for speed
                    *rem_row ^= row0;

                    // Add the pivot row to eliminate the bit from this row, preserving previous bits
                    for (unsigned ii = 1, end = _ge_pitch - word_offset; ii < end; ++ii) {
                        rem_row[ii] ^= ge_row[ii];
                    }
                }
            } // next remaining row

            break;
        }

        // If pivot could not be found:
        if (!found)
        {
            _next_pivot = pivot_i;

            CAT_IF_DUMP(cout << "Singular: Pivot " << pivot_i << " of " <<
                (_defer_count + _mix_count) << " not found!" << endl;)

            CAT_IF_PIVOT(if (pivot_i + 16 < (unsigned)(_defer_count + _mix_count)) {
                cout << ">>>>> Singular: Pivot " << pivot_i << " of " <<
                    (_defer_count + _mix_count) << " not found!" << endl << endl;
            })

            return false;
        }

        // Generate next mask
        ge_mask = CAT_ROL64(ge_mask, 1);
    }

    _next_pivot = pivot_i;

    InsertHeavyRows();

    return true;
}


#if defined(CAT_HEAVY_WIN_MULT)

#if defined(CAT_ENDIAN_BIG)

// Flip endianness at compile time if possible
static uint32_t GF256_MULT_LOOKUP[16] = {
    0x00000000, 0x01000000, 0x00010000, 0x01010000, 
    0x00000100, 0x01000100, 0x00010100, 0x01010100, 
    0x00000001, 0x01000001, 0x00010001, 0x01010001, 
    0x00000101, 0x01000101, 0x00010101, 0x01010101, 
};

#else

// Little-endian or unknown bit order:
static uint32_t GF256_MULT_LOOKUP[16] = {
    0x00000000, 0x00000001, 0x00000100, 0x00000101, 
    0x00010000, 0x00010001, 0x00010100, 0x00010101, 
    0x01000000, 0x01000001, 0x01000100, 0x01000101, 
    0x01010000, 0x01010001, 0x01010100, 0x01010101, 
};
#endif

#endif // CAT_HEAVY_WIN_MULT


bool Codec::Triangle()
{
    CAT_IF_DUMP(cout << endl << "---- Triangle ----" << endl << endl;)

    const unsigned first_heavy_column = _first_heavy_column;

    // If next pivot is not heavy:
    if (_next_pivot < first_heavy_column &&
        !TriangleNonHeavy())
    {
        return false;
    }

    const unsigned pivot_count = _pivot_count;
    const unsigned column_count = _defer_count + _mix_count;
    const unsigned first_heavy_row = _defer_count + _dense_count;
    unsigned first_heavy_pivot = _first_heavy_pivot;

    uint64_t ge_mask = (uint64_t)1 << (_next_pivot & 63);

    // For each heavy pivot to determine:
    for (unsigned pivot_i = _next_pivot; pivot_i < column_count;
        ++pivot_i, ge_mask = CAT_ROL64(ge_mask, 1))
    {
        CAT_DEBUG_ASSERT(pivot_i >= first_heavy_column);
        const unsigned heavy_col_i = pivot_i - first_heavy_column;

        const unsigned word_offset = pivot_i >> 6;
        uint64_t * GF256_RESTRICT ge_matrix_offset = _ge_matrix + word_offset;
        bool found = false;
        unsigned pivot_j;

        // For each remaining GE row that might be the pivot:
        for (pivot_j = pivot_i; pivot_j < first_heavy_pivot; ++pivot_j)
        {
            const unsigned ge_row_j = _pivots[pivot_j];
            uint64_t * GF256_RESTRICT ge_row = &ge_matrix_offset[_ge_pitch * ge_row_j];

            // If the bit was not found:
            if (0 == (*ge_row & ge_mask)) {
                continue; // Skip to next
            }

            // Found it!
            found = true;

            CAT_IF_DUMP(cout << "Pivot " << pivot_i << " found on row " << ge_row_j << endl;)

            // Swap out the pivot index for this one
            _pivots[pivot_j] = _pivots[pivot_i];
            CAT_DEBUG_ASSERT(ge_row_j < _pivot_count);
            _pivots[pivot_i] = (uint16_t)ge_row_j;

            // Prepare masked first word
            const uint64_t row0 = (*ge_row & ~(ge_mask - 1)) ^ ge_mask;

            unsigned pivot_k = pivot_j + 1;

            // For each remaining light row:
            for (; pivot_k < first_heavy_pivot; ++pivot_k)
            {
                // Determine if the row contains the bit we want
                unsigned ge_row_k = _pivots[pivot_k];
                uint64_t * GF256_RESTRICT rem_row = &ge_matrix_offset[_ge_pitch * ge_row_k];

                // If the bit was found:
                if (0 != (*rem_row & ge_mask))
                {
                    // Unroll first word to handle masked word and for speed
                    *rem_row ^= row0;

                    // Add the pivot row to eliminate the bit from this row, preserving previous bits
                    for (unsigned ii = 1, end = _ge_pitch - word_offset; ii < end; ++ii) {
                        rem_row[ii] ^= ge_row[ii];
                    }
                }
            } // next remaining row

            // For each remaining heavy row:
            for (; pivot_k < pivot_count; ++pivot_k)
            {
                CAT_DEBUG_ASSERT(_pivots[pivot_k] >= first_heavy_row);
                const unsigned heavy_row_k = _pivots[pivot_k] - first_heavy_row;

                uint8_t * GF256_RESTRICT rem_row = &_heavy_matrix[_heavy_pitch * heavy_row_k];
                const uint8_t code_value = rem_row[heavy_col_i];

                // If the column is non-zero:
                if (0 == code_value) {
                    continue;
                }

                CAT_IF_DUMP(cout << "Eliminating from heavy row " << heavy_row_k << " :";)

                // For each set bit in the binary pivot row, add rem[i] to rem[i+]:
                uint64_t * GF256_RESTRICT pivot_row = &_ge_matrix[_ge_pitch * ge_row_j];

#if !defined(CAT_HEAVY_WIN_MULT)
                for (unsigned ge_column_i = pivot_i + 1; ge_column_i < column_count; ++ge_column_i)
                {
                    const uint64_t mask = (uint64_t)1 << (ge_column_i & 63);
                    const uint64_t word = pivot_row[ge_column_i >> 6];
                    const bool nonzero = 0 != (word & mask);

                    if (nonzero) {
                        rem_row[ge_column_i - first_heavy_column] ^= code_value;
                    }
                }
#else // CAT_HEAVY_WIN_MULT
                const unsigned odd_count = pivot_i & 3;
                unsigned ge_column_i = pivot_i + 1;
                uint64_t temp_mask = ge_mask;

                // Unroll odd columns:
                switch (odd_count)
                {
                case 0: temp_mask = CAT_ROL64(temp_mask, 1);
                        if (0 != (pivot_row[ge_column_i >> 6] & temp_mask))
                        {
                            CAT_DEBUG_ASSERT(ge_column_i >= _first_heavy_column);
                            rem_row[ge_column_i - _first_heavy_column] ^= code_value;
                            CAT_IF_DUMP(cout << " " << ge_column_i;)
                        }
                        ++ge_column_i;

                        // Fall-thru...
                case 1: temp_mask = CAT_ROL64(temp_mask, 1);
                        if (0 != (pivot_row[ge_column_i >> 6] & temp_mask))
                        {
                            CAT_DEBUG_ASSERT(ge_column_i >= _first_heavy_column);
                            rem_row[ge_column_i - _first_heavy_column] ^= code_value;
                            CAT_IF_DUMP(cout << " " << ge_column_i;)
                        }
                        ++ge_column_i;

                        // Fall-thru...
                case 2: temp_mask = CAT_ROL64(temp_mask, 1);
                        if (0 != (pivot_row[ge_column_i >> 6] & temp_mask))
                        {
                            CAT_DEBUG_ASSERT(ge_column_i >= _first_heavy_column);
                            rem_row[ge_column_i - _first_heavy_column] ^= code_value;
                            CAT_IF_DUMP(cout << " " << ge_column_i;)
                        }

                        // Set GE column to next even column
                        CAT_DEBUG_ASSERT(odd_count <= 4);
                        ge_column_i = pivot_i + (4 - odd_count);
                }

                uint32_t * GF256_RESTRICT word = reinterpret_cast<uint32_t*>(rem_row + ge_column_i - first_heavy_column);

                // For remaining aligned columns:
                for (; ge_column_i < column_count; ge_column_i += 4, ++word)
                {
                    // Look up 4 bit window
                    const uint32_t bits = (uint32_t)(pivot_row[ge_column_i >> 6] >> (ge_column_i & 63)) & 15;
#if defined(CAT_ENDIAN_UNKNOWN)
                    const uint32_t window = getLE(GF256_MULT_LOOKUP[bits]);
#else
                    const uint32_t window = GF256_MULT_LOOKUP[bits];
#endif

                    CAT_IF_DUMP(cout << " " << ge_column_i << "x" << hex << setw(8) << setfill('0') << window << dec;)

                    *word ^= window * code_value;
                }
#endif // CAT_HEAVY_WIN_MULT
                CAT_IF_DUMP(cout << endl;)
            } // next heavy row

            break;
        } // next row

        // If not found, then for each remaining heavy pivot:
        // NOTE: The pivot array maintains the heavy rows at the end so that they are always tried last
        if (!found) for (; pivot_j < _pivot_count; ++pivot_j)
        {
            const unsigned ge_row_j = _pivots[pivot_j];
            CAT_DEBUG_ASSERT(ge_row_j >= first_heavy_row);
            const unsigned heavy_row_j = ge_row_j - first_heavy_row;
            CAT_DEBUG_ASSERT(heavy_row_j < _heavy_rows);
            uint8_t * GF256_RESTRICT pivot_row = &_heavy_matrix[_heavy_pitch * heavy_row_j];
            const uint8_t code_value = pivot_row[heavy_col_i];

            // If heavy row doesn't have the pivot:
            if (0 == code_value) {
                continue; // Skip to next
            }

            // Found it! (common case)
            found = true;

            CAT_IF_DUMP(cout << "Pivot " << pivot_i << " found on heavy row " << ge_row_j << endl;)

            // Swap pivot i and j
            _pivots[pivot_j] = _pivots[pivot_i];
            _pivots[pivot_i] = (uint16_t)ge_row_j;

            // If a non-heavy pivot just got moved into heavy pivot list:
            if (pivot_i < first_heavy_pivot)
            {
                // Swap pivot j with first heavy pivot
                const uint16_t temp = _pivots[first_heavy_pivot];
                _pivots[first_heavy_pivot] = _pivots[pivot_j];
                _pivots[pivot_j] = temp;

                // And move the first heavy pivot up one to cover the hole
                ++first_heavy_pivot;
            }

            unsigned pivot_k = pivot_j + 1;

            // If there are any remaining rows:
            if (pivot_k < pivot_count)
            {
                // For each remaining unused row:
                // Note all remaining rows are heavy rows by pivot array organization
                for (; pivot_k < pivot_count; ++pivot_k)
                {
                    const unsigned ge_row_k = _pivots[pivot_k];
                    CAT_DEBUG_ASSERT(ge_row_k >= first_heavy_row);
                    const unsigned heavy_row_k = ge_row_k - first_heavy_row;
                    CAT_DEBUG_ASSERT(heavy_row_k < _heavy_rows);
                    uint8_t * GF256_RESTRICT rem_row = &_heavy_matrix[_heavy_pitch * heavy_row_k];
                    const uint8_t rem_value = rem_row[heavy_col_i];

                    // If the column is zero:
                    if (0 == rem_value) {
                        continue; // Skip it
                    }

                    // x = rem_value / code_value
                    const uint8_t x = gf256_div(rem_value, code_value);

                    // Store value for later
                    rem_row[heavy_col_i] = x;

                    const unsigned offset = heavy_col_i + 1;
                    CAT_DEBUG_ASSERT(_heavy_columns >= offset);

                    // rem[i+] += x * pivot[i+]
                    gf256_muladd_mem(
                        rem_row + offset,
                        x,
                        pivot_row + offset,
                        _heavy_columns - offset);
                } // next remaining row
            }

            break;
        } // next heavy row

        // If pivot could not be found:
        if (!found)
        {
            _next_pivot = pivot_i;
            _first_heavy_pivot = first_heavy_pivot;

            CAT_IF_DUMP(cout << "Singular: Pivot " << pivot_i << " of " <<
                (_defer_count + _mix_count) << " not found!" << endl;)

            CAT_IF_PIVOT(if (pivot_i + 16 < (unsigned)(_defer_count + _mix_count)) {
                cout << ">>>>> Singular: Pivot " << pivot_i << " of " <<
                    (_defer_count + _mix_count) << " not found!" << endl << endl;
            })

            return false;
        }

        CAT_IF_DUMP( PrintGEMatrix(); )
        CAT_IF_DUMP( PrintExtraMatrix(); )
    }

    return true;
}


//------------------------------------------------------------------------------
// Stage (4) Substitution

void Codec::InitializeColumnValues()
{
    CAT_IF_DUMP(cout << endl << "---- InitializeColumnValues ----" << endl << endl;)

    CAT_IF_ROWOP(uint32_t rowops = 0;)

    const uint16_t first_heavy_row = _defer_count + _dense_count;
    const uint16_t column_count = _defer_count + _mix_count;

    uint16_t pivot_i;

    // For each pivot:
    for (pivot_i = 0; pivot_i < column_count; ++pivot_i)
    {
        // Lookup pivot column, GE row, and destination buffer
        const uint16_t dest_column_i = _ge_col_map[pivot_i];
        const uint16_t ge_row_i = _pivots[pivot_i];
        CAT_DEBUG_ASSERT(dest_column_i < _recovery_rows);
        uint8_t * GF256_RESTRICT buffer_dest = _recovery_blocks + _block_bytes * dest_column_i;

        CAT_IF_DUMP(cout << "Pivot " << pivot_i << " solving column " << dest_column_i << " with GE row " << ge_row_i << " : ";)

        // If it is a dense/heavy(non-extra) row,
        if (ge_row_i < _dense_count ||
            ge_row_i >= (first_heavy_row + _extra_count))
        {
            // Dense/heavy rows sum to zero
            memset(buffer_dest, 0, _block_bytes);

            // Store which column solves the dense row
            _ge_row_map[ge_row_i] = dest_column_i;

            CAT_IF_DUMP(cout << "[0]" << endl;)
            CAT_IF_ROWOP(++rowops;)

            continue;
        }

        // Look up row and input value for GE row
        const uint16_t row_i = _ge_row_map[ge_row_i];
        const uint8_t * GF256_RESTRICT combo = _input_blocks + _block_bytes * row_i;
        PeelRow * GF256_RESTRICT row = &_peel_rows[row_i];

        CAT_IF_DUMP(cout << "[" << (unsigned)combo[0] << "]";)

        // If copying from final input block:
        if (row_i == _block_count - 1)
        {
            memcpy(buffer_dest, combo, _input_final_bytes);
            memset(buffer_dest + _input_final_bytes, 0, _block_bytes - _input_final_bytes);

            CAT_IF_ROWOP(++rowops;)

            combo = 0;
        }

        PeelRowIterator iter(row->Params, _block_count, _block_next_prime);

        // Eliminate peeled columns:
        do
        {
            const uint16_t column_i = iter.GetColumn();

            PeelColumn * GF256_RESTRICT column = &_peel_cols[column_i];

            // If column is peeled:
            if (column->Mark == MARK_PEEL)
            {
                CAT_DEBUG_ASSERT(column_i < _recovery_rows);
                const uint8_t * GF256_RESTRICT src = _recovery_blocks + _block_bytes * column_i;

                // If combo unused:
                if (!combo) {
                    gf256_add_mem(buffer_dest, src, _block_bytes);
                }
                else
                {
                    // Use combo
                    gf256_addset_mem(buffer_dest, combo, src, _block_bytes);

                    combo = 0;
                }
                CAT_IF_ROWOP(++rowops;)
            }
        } while (iter.Iterate());

        // If combo still unused:
        if (combo) {
            memcpy(buffer_dest, combo, _block_bytes);
        }

        CAT_IF_DUMP(cout << endl;)
    }

    // For each remaining pivot:
    for (; pivot_i < _pivot_count; ++pivot_i)
    {
        const uint16_t ge_row_i = _pivots[pivot_i];

        // If row is a dense row,
        if (ge_row_i < _dense_count ||
            (ge_row_i >= first_heavy_row && ge_row_i < column_count))
        {
            // Mark it for skipping
            _ge_row_map[ge_row_i] = LIST_TERM;

            CAT_IF_DUMP(cout << "Did not use GE row " << ge_row_i << ", which is a dense row." << endl;)
        }
        else {
            CAT_IF_DUMP(cout << "Did not use deferred row " << ge_row_i << ", which is not a dense row." << endl;)
        }
    }

    CAT_IF_ROWOP(cout << "InitializeColumnValues used " << rowops << " row ops = " << rowops / (double)_block_count << "*N" << endl;)
}

void Codec::MultiplyDenseValues()
{
    CAT_IF_DUMP(cout << endl << "---- MultiplyDenseValues ----" << endl << endl;)

    CAT_IF_ROWOP(uint32_t rowops = 0;)

    // Initialize PRNG
    PCGRandom prng;
    prng.Seed(_d_seed);

    const uint16_t dense_count = _dense_count;
    CAT_DEBUG_ASSERT((unsigned)(_block_count + _mix_count) < _recovery_rows);
    uint8_t * GF256_RESTRICT temp_block = _recovery_blocks + _block_bytes * (_block_count + _mix_count);
    const uint8_t * GF256_RESTRICT source_block = _recovery_blocks;
    const PeelColumn * GF256_RESTRICT column = _peel_cols;
    uint16_t rows[CAT_MAX_DENSE_ROWS];
    uint16_t bits[CAT_MAX_DENSE_ROWS];
    const uint16_t block_count = _block_count;

    // For each block of columns:
    for (uint16_t column_i = 0; column_i < block_count; column_i += dense_count,
        column += dense_count, source_block += _block_bytes * dense_count)
    {
        unsigned max_x = dense_count;

        // Handle final columns
        if (column_i + dense_count > block_count) {
            max_x = _block_count - column_i;
        }

        CAT_IF_DUMP(cout << endl << "For window of columns between " << column_i <<
            " and " << column_i + dense_count - 1 << " (inclusive):" << endl;)

        // Shuffle row and bit order
        ShuffleDeck16(prng, rows, dense_count);
        ShuffleDeck16(prng, bits, dense_count);

        // Initialize counters
        const uint16_t set_count = (dense_count + 1) >> 1;
        uint16_t * GF256_RESTRICT set_bits = bits;
        uint16_t * GF256_RESTRICT clr_bits = set_bits + set_count;
        const uint16_t * GF256_RESTRICT row = rows;

        CAT_IF_DUMP(cout << "Generating first row " << _ge_row_map[*row] << ":";)

        // Generate first row
        const uint8_t * GF256_RESTRICT combo = 0;
        CAT_IF_ROWOP(++rowops;)

        for (unsigned ii = 0; ii < set_count; ++ii)
        {
            unsigned bit_i = set_bits[ii];

            // If bit is peeled:
            if (bit_i < max_x && column[bit_i].Mark == MARK_PEEL)
            {
                CAT_IF_DUMP(cout << " " << column_i + bit_i;)

                const uint8_t * GF256_RESTRICT src = source_block + _block_bytes * bit_i;

                // If no combo used yet:
                if (!combo) {
                    combo = src;
                }
                else if (combo == temp_block)
                {
                    // Else if combo has been used: XOR it in
                    gf256_add_mem(temp_block, src, _block_bytes);

                    CAT_IF_ROWOP(++rowops;)
                }
                else
                {
                    // Else if combo needs to be used: Combine into block
                    gf256_addset_mem(temp_block, combo, src, _block_bytes);

                    CAT_IF_ROWOP(++rowops;)

                    combo = temp_block;
                }
            }
        }

        CAT_IF_DUMP(cout << endl;)

        // If no combo ever triggered:
        if (!combo) {
            memset(temp_block, 0, _block_bytes);
        }
        else
        {
            // Else if never combined two: Just copy it
            if (combo != temp_block)
            {
                memcpy(temp_block, combo, _block_bytes);
                CAT_IF_ROWOP(++rowops;)
            }

            const uint16_t dest_column_i = _ge_row_map[*row];

            // Store in destination column in recovery blocks
            if (dest_column_i != LIST_TERM)
            {
                CAT_DEBUG_ASSERT(dest_column_i < _recovery_rows);
                gf256_add_mem(_recovery_blocks + _block_bytes * dest_column_i, temp_block, _block_bytes);
                CAT_IF_ROWOP(++rowops;)
            }
        }

        ++row;

        // Reshuffle bit order: Shuffle-2 Code
        ShuffleDeck16(prng, bits, dense_count);

        const unsigned loop_count = (dense_count >> 1);
        CAT_DEBUG_ASSERT(loop_count < CAT_MAX_DENSE_ROWS);

        // Generate first half of rows
        for (unsigned ii = 0; ii < loop_count; ++ii)
        {
            CAT_IF_DUMP(cout << "Flipping bits for derivative row " << _ge_row_map[*row] << ":";)

            CAT_DEBUG_ASSERT(ii < CAT_MAX_DENSE_ROWS);
            const unsigned bit0 = set_bits[ii];
            const unsigned bit1 = clr_bits[ii];

            // Add in peeled columns
            if (bit0 < max_x && column[bit0].Mark == MARK_PEEL)
            {
                if (bit1 < max_x && column[bit1].Mark == MARK_PEEL)
                {
                    CAT_IF_DUMP(cout << " " << column_i + bit0 << "+" << column_i + bit1;)

                    gf256_add2_mem(
                        temp_block,
                        source_block + _block_bytes * bit0,
                        source_block + _block_bytes * bit1,
                        _block_bytes);
                }
                else
                {
                    CAT_IF_DUMP(cout << " " << column_i + bit0;)

                    gf256_add_mem(
                        temp_block,
                        source_block + _block_bytes * bit0,
                        _block_bytes);
                }
                CAT_IF_ROWOP(++rowops;)
            }
            else if (bit1 < max_x && column[bit1].Mark == MARK_PEEL)
            {
                CAT_IF_DUMP(cout << " " << column_i + bit1;)

                gf256_add_mem(
                    temp_block,
                    source_block + _block_bytes * bit1,
                    _block_bytes);

                CAT_IF_ROWOP(++rowops;)
            }

            CAT_IF_DUMP(cout << endl;)

            const uint16_t dest_column_i = _ge_row_map[*row++];

            // Store in destination column in recovery blocks
            if (dest_column_i != LIST_TERM)
            {
                CAT_DEBUG_ASSERT(dest_column_i < _recovery_rows);

                gf256_add_mem(
                    _recovery_blocks + _block_bytes * dest_column_i,
                    temp_block,
                    _block_bytes);

                CAT_IF_ROWOP(++rowops;)
            }
        }

        // Reshuffle bit order: Shuffle-2 Code
        ShuffleDeck16(prng, bits, dense_count);

        const unsigned second_loop_count = loop_count - 1 + (dense_count & 1);
        CAT_DEBUG_ASSERT(second_loop_count < CAT_MAX_DENSE_ROWS);

        // Generate second half of rows
        for (unsigned ii = 0; ii < second_loop_count; ++ii)
        {
            CAT_IF_DUMP(cout << "Flipping bits for derivative row " << _ge_row_map[*row] << ":";)

            // Add in peeled columns
            CAT_DEBUG_ASSERT(ii < CAT_MAX_DENSE_ROWS);
            const unsigned bit0 = set_bits[ii];
            const unsigned bit1 = clr_bits[ii];

            if (bit0 < max_x && column[bit0].Mark == MARK_PEEL)
            {
                if (bit1 < max_x && column[bit1].Mark == MARK_PEEL)
                {
                    CAT_IF_DUMP(cout << " " << column_i + bit0 << "+" << column_i + bit1;)

                    CAT_DEBUG_ASSERT(bit0 < _block_count);
                    CAT_DEBUG_ASSERT(bit1 < _block_count);

                    gf256_add2_mem(
                        temp_block,
                        source_block + _block_bytes * bit0,
                        source_block + _block_bytes * bit1,
                        _block_bytes);
                }
                else
                {
                    CAT_IF_DUMP(cout << " " << column_i + bit0;)

                    CAT_DEBUG_ASSERT(bit0 < _block_count);

                    gf256_add_mem(
                        temp_block,
                        source_block + _block_bytes * bit0,
                        _block_bytes);
                }

                CAT_IF_ROWOP(++rowops;)
            }
            else if (bit1 < max_x && column[bit1].Mark == MARK_PEEL)
            {
                CAT_IF_DUMP(cout << " " << column_i + bit1;)

                CAT_DEBUG_ASSERT(bit1 < _block_count);

                gf256_add_mem(
                    temp_block,
                    source_block + _block_bytes * bit1,
                    _block_bytes);

                CAT_IF_ROWOP(++rowops;)
            }

            CAT_IF_DUMP(cout << endl;)

            // Store in destination column in recovery blocks
            const uint16_t dest_column_i = _ge_row_map[*row++];

            if (dest_column_i != LIST_TERM)
            {
                CAT_DEBUG_ASSERT(dest_column_i < _recovery_rows);

                gf256_add_mem(
                    _recovery_blocks + _block_bytes * dest_column_i,
                    temp_block,
                    _block_bytes);

                CAT_IF_ROWOP(++rowops;)
            }
        }
    } // next column

    CAT_IF_ROWOP(cout << "MultiplyDenseValues used " << rowops << " row ops = " << rowops / (double)_block_count << "*N" << endl;)
}

// These are heuristic values.  Choosing better values has little effect on performance.
#define CAT_UNDER_WIN_THRESH_4 (45 + 4) /* Note: Assumes this is higher than CAT_HEAVY_MAX_COLS */
#define CAT_UNDER_WIN_THRESH_5 (65 + 5)
#define CAT_UNDER_WIN_THRESH_6 (85 + 6)
#define CAT_UNDER_WIN_THRESH_7 (138 + 7)

void Codec::AddSubdiagonalValues()
{
    CAT_IF_DUMP(cout << endl << "---- AddSubdiagonalValues ----" << endl << endl;)

    CAT_IF_ROWOP(uint32_t rowops = 0; unsigned heavyops = 0;)

    const unsigned column_count = _defer_count + _mix_count;
    unsigned pivot_i = 0;
    const uint16_t first_heavy_row = _defer_count + _dense_count;

#if defined(CAT_WINDOWED_LOWERTRI)
    const uint16_t first_non_binary_row = first_heavy_row + _extra_count;

    // Build temporary storage space if windowing is to be used
    if (column_count >= CAT_UNDER_WIN_THRESH_5)
    {
        unsigned w, next_check_i;

        // Calculate initial window size
        if (column_count >= CAT_UNDER_WIN_THRESH_7)
        {
            w = 7;
            next_check_i = column_count - CAT_UNDER_WIN_THRESH_7;
        }
        else if (column_count >= CAT_UNDER_WIN_THRESH_6)
        {
            w = 6;
            next_check_i = column_count - CAT_UNDER_WIN_THRESH_6;
        }
        else if (column_count >= CAT_UNDER_WIN_THRESH_5)
        {
            w = 5;
            next_check_i = column_count - CAT_UNDER_WIN_THRESH_5;
        }
        else
        {
            w = 4;
            next_check_i = CAT_UNDER_WIN_THRESH_4;
        }
        uint32_t win_lim = 1 << w;

        CAT_IF_DUMP(cout << "Activating windowed lower triangular elimination with initial window " << w << endl;)

        // Use the first few peel column values as window table space
        // Note that the peeled column values were previously used up until this point,
        // but now they are unused, and so they can be reused for temporary space
        uint8_t * GF256_RESTRICT win_table[128];
        const PeelColumn * GF256_RESTRICT column = _peel_cols;
        uint8_t * GF256_RESTRICT column_src = _recovery_blocks;
        uint32_t jj = 1;

        for (uint32_t count = _block_count; count > 0; --count, ++column, column_src += _block_bytes)
        {
            // If column is peeled:
            if (column->Mark == MARK_PEEL)
            {
                // Reuse the block value temporarily as window table space
                win_table[jj] = column_src;

                CAT_IF_DUMP(cout << "-- Window table entry " << jj << " set to column " << _block_count - count << endl;)

                // If done:
                if (++jj >= win_lim) {
                    break;
                }
            }
        }

        CAT_IF_DUMP(if (jj < win_lim) {
            cout << "!! Not enough space in peeled columns to generate a table.  " \
                "Going back to normal lower triangular elimination." << endl;
        })

        // If enough space was found:
        if (jj >= win_lim) for (;;)
        {
            // Calculate first column in window
            const unsigned final_i = pivot_i + w - 1;

            CAT_IF_DUMP(cout << "-- Windowing from " << pivot_i << " to " << final_i << " (inclusive)" << endl;)

            // Eliminate lower triangular part below windowed bits:

            uint64_t ge_mask = (uint64_t)1 << (pivot_i & 63);

            // For each column:
            for (unsigned src_pivot_i = pivot_i; src_pivot_i < final_i; ++src_pivot_i)
            {
                CAT_DEBUG_ASSERT(_ge_col_map[src_pivot_i] < _recovery_rows);
                uint8_t * GF256_RESTRICT src = _recovery_blocks + _block_bytes * _ge_col_map[src_pivot_i];

                CAT_IF_DUMP(cout << "Back-substituting small triangle from pivot " << src_pivot_i << "[" << (unsigned)src[0] << "] :";)

                const uint64_t * GF256_RESTRICT ge_row = _ge_matrix + (src_pivot_i >> 6);

                // For each row above the diagonal:
                for (unsigned dest_pivot_i = src_pivot_i + 1; dest_pivot_i <= final_i; ++dest_pivot_i)
                {
                    // If row is heavy:
                    const uint16_t dest_row_i = _pivots[dest_pivot_i];

                    // If bit is set in that row:
                    if (ge_row[_ge_pitch * dest_row_i] & ge_mask)
                    {
                        const uint16_t dest_col_i = _ge_col_map[dest_pivot_i];
                        CAT_DEBUG_ASSERT(dest_col_i < _block_count + _mix_count);

                        CAT_DEBUG_ASSERT(dest_col_i < _recovery_rows);
                        uint8_t * GF256_RESTRICT dest = _recovery_blocks + _block_bytes * dest_col_i;

                        // Back-substitute
                        gf256_add_mem(dest, src, _block_bytes);

                        CAT_IF_ROWOP(++rowops;)

                        CAT_IF_DUMP(cout << " " << dest_pivot_i;)
                    }
                } // next pivot above

                CAT_IF_DUMP(cout << endl;)

                ge_mask = CAT_ROL64(ge_mask, 1);
            } // next pivot

            CAT_IF_DUMP(cout << "-- Generating window table with " << w << " bits" << endl;)

            // Generate window table: 2 bits
            CAT_DEBUG_ASSERT(_ge_col_map[pivot_i] < _recovery_rows);
            win_table[1] = _recovery_blocks + _block_bytes * _ge_col_map[pivot_i];
            CAT_DEBUG_ASSERT(_ge_col_map[pivot_i + 1] < _recovery_rows);
            win_table[2] = _recovery_blocks + _block_bytes * _ge_col_map[pivot_i + 1];
            gf256_addset_mem(win_table[3], win_table[1], win_table[2], _block_bytes);
            CAT_IF_ROWOP(++rowops;)

            // Generate window table: 3 bits
            CAT_DEBUG_ASSERT(_ge_col_map[pivot_i + 2] < _recovery_rows);
            win_table[4] = _recovery_blocks + _block_bytes * _ge_col_map[pivot_i + 2];
            gf256_addset_mem(win_table[5], win_table[1], win_table[4], _block_bytes);
            gf256_addset_mem(win_table[6], win_table[2], win_table[4], _block_bytes);
            gf256_addset_mem(win_table[7], win_table[1], win_table[6], _block_bytes);
            CAT_IF_ROWOP(rowops += 3;)

            // Generate window table: 4 bits
            CAT_DEBUG_ASSERT(_ge_col_map[pivot_i + 3] < _recovery_rows);
            win_table[8] = _recovery_blocks + _block_bytes * _ge_col_map[pivot_i + 3];
            for (unsigned ii = 1; ii < 8; ++ii) {
                gf256_addset_mem(win_table[8 + ii], win_table[ii], win_table[8], _block_bytes);
            }
            CAT_IF_ROWOP(rowops += 7;)

            // Generate window table: 5+ bits
            if (w >= 5)
            {
                CAT_DEBUG_ASSERT(_ge_col_map[pivot_i + 4] < _recovery_rows);
                win_table[16] = _recovery_blocks + _block_bytes * _ge_col_map[pivot_i + 4];
                for (unsigned ii = 1; ii < 16; ++ii) {
                    gf256_addset_mem(win_table[16 + ii], win_table[ii], win_table[16], _block_bytes);
                }
                CAT_IF_ROWOP(rowops += 15;)

                if (w >= 6)
                {
                    CAT_DEBUG_ASSERT(_ge_col_map[pivot_i + 5] < _recovery_rows);
                    win_table[32] = _recovery_blocks + _block_bytes * _ge_col_map[pivot_i + 5];
                    for (unsigned ii = 1; ii < 32; ++ii) {
                        gf256_addset_mem(win_table[32 + ii], win_table[ii], win_table[32], _block_bytes);
                    }
                    CAT_IF_ROWOP(rowops += 31;)

                    if (w >= 7)
                    {
                        CAT_DEBUG_ASSERT(_ge_col_map[pivot_i + 6] < _recovery_rows);
                        win_table[64] = _recovery_blocks + _block_bytes * _ge_col_map[pivot_i + 6];
                        for (unsigned ii = 1; ii < 64; ++ii) {
                            gf256_addset_mem(win_table[64 + ii], win_table[ii], win_table[64], _block_bytes);
                        }
                        CAT_IF_ROWOP(rowops += 63;)
                    }
                }
            }

            const unsigned first_word = pivot_i >> 6;
            const unsigned shift0 = pivot_i & 63;
            const unsigned last_word = final_i >> 6;
            const uint16_t * GF256_RESTRICT pivot_row = _pivots + final_i + 1;

            // If not straddling words:
            if (first_word == last_word)
            {
                // For each pivot row:
                for (unsigned ge_below_i = final_i + 1; ge_below_i < column_count; ++ge_below_i)
                {
                    const uint16_t ge_row_i = *pivot_row++;

                    // If pivot row is heavy:
                    if (ge_row_i >= first_non_binary_row) {
                        continue;
                    }

                    // Calculate window bits
                    const uint64_t * GF256_RESTRICT ge_row = _ge_matrix + first_word + _ge_pitch * ge_row_i;
                    const unsigned win_bits = (uint32_t)(ge_row[0] >> shift0) & (win_lim - 1);

                    // If any XOR needs to be performed:
                    if (win_bits != 0)
                    {
                        CAT_IF_DUMP(cout << "Adding window table " << win_bits << " to pivot " << ge_below_i << endl;)

                        // Back-substitute
                        uint8_t * GF256_RESTRICT dest = _recovery_blocks + _block_bytes * _ge_col_map[ge_below_i];
                        gf256_add_mem(dest, win_table[win_bits], _block_bytes);
                        CAT_IF_ROWOP(++rowops;)
                    }
                }
            }
            else // Rare: Straddling case
            {
                const unsigned shift1 = 64 - shift0;

                // For each pivot row:
                for (unsigned ge_below_i = final_i + 1; ge_below_i < column_count; ++ge_below_i)
                {
                    // If pivot row is heavy:
                    const uint16_t ge_row_i = *pivot_row++;
                    if (ge_row_i >= first_non_binary_row) {
                        continue;
                    }

                    // Calculate window bits
                    const uint64_t * GF256_RESTRICT ge_row = _ge_matrix + first_word + _ge_pitch * ge_row_i;
                    const uint32_t win_bits = ( (uint32_t)(ge_row[0] >> shift0) | (uint32_t)(ge_row[1] << shift1) ) & (win_lim - 1);

                    // If any XOR needs to be performed:
                    if (win_bits != 0)
                    {
                        CAT_IF_DUMP(cout << "Adding window table " << win_bits << " to pivot " << ge_below_i << endl;)

                        // Back-substitute
                        uint8_t * GF256_RESTRICT dest = _recovery_blocks + _block_bytes * _ge_col_map[ge_below_i];
                        gf256_add_mem(dest, win_table[win_bits], _block_bytes);
                        CAT_IF_ROWOP(++rowops;)
                    }
                }
            }

            // If column index falls below window size:
            pivot_i += w;
            if (pivot_i >= next_check_i)
            {
                CAT_DEBUG_ASSERT(pivot_i < column_count);
                const unsigned remaining_columns = column_count - pivot_i;

                if (remaining_columns >= CAT_UNDER_WIN_THRESH_6) {
                    w = 6;
                    next_check_i = remaining_columns - CAT_UNDER_WIN_THRESH_6;
                }
                else if (remaining_columns >= CAT_UNDER_WIN_THRESH_5) {
                    w = 5;
                    next_check_i = remaining_columns - CAT_UNDER_WIN_THRESH_5;
                }
                else if (remaining_columns >= CAT_UNDER_WIN_THRESH_4) {
                    w = 4;
                    next_check_i = remaining_columns - CAT_UNDER_WIN_THRESH_4;
                }
                else {
                    break;
                }

                // Update window limit
                win_lim = 1 << w;
            }
        } // next window
    } // end if windowed
#endif // CAT_WINDOWED_LOWERTRI

    // For each row to eliminate:
    for (unsigned ge_column_i = pivot_i + 1; ge_column_i < column_count; ++ge_column_i)
    {
        // Lookup pivot column, GE row, and destination buffer
        const unsigned column_i = _ge_col_map[ge_column_i];
        const uint16_t ge_row_i = _pivots[ge_column_i];
        CAT_DEBUG_ASSERT(column_i < _recovery_rows);
        uint8_t * GF256_RESTRICT dest = _recovery_blocks + _block_bytes * column_i;

        CAT_IF_DUMP(cout << "Pivot " << ge_column_i << " solving column " << column_i << "[" << (unsigned)dest[0] << "] with GE row " << ge_row_i << " :";)

        unsigned ge_limit = ge_column_i;

        // If row is heavy or extra:
        if (ge_row_i >= first_heavy_row)
        {
            const uint16_t heavy_row_i = ge_row_i - first_heavy_row;

            // For each column up to the diagonal:
            const uint8_t * GF256_RESTRICT heavy_row = _heavy_matrix + _heavy_pitch * heavy_row_i;
            for (unsigned sub_i = _first_heavy_column; sub_i < ge_limit; ++sub_i)
            {
                // If column is zero:
                const uint8_t code_value = heavy_row[sub_i - _first_heavy_column];
                if (0 == code_value) {
                    continue; // Skip it
                }

                // Look up data source
                CAT_DEBUG_ASSERT(_ge_col_map[sub_i] < _recovery_rows);
                const uint8_t * GF256_RESTRICT src = _recovery_blocks + _block_bytes * _ge_col_map[sub_i];

                gf256_muladd_mem(dest, code_value, src, _block_bytes);

                CAT_IF_ROWOP(if (code_value == 1) ++rowops; else ++heavyops;)
                CAT_IF_DUMP(cout << " h" << ge_column_i << "=[" << (unsigned)src[0] << "*" << (unsigned)code_value << "]";)
            }

            // If row is not extra:
            if (heavy_row_i >= _extra_count) {
                CAT_IF_DUMP(cout << endl;)
                continue; // Skip binary matrix elimination
            }

            // Limit the binary matrix elimination to non-heavy columns
            if (ge_limit > _first_heavy_column) {
                ge_limit = _first_heavy_column;
            }

            // fall-thru..
        }

        const uint64_t * GF256_RESTRICT ge_row = _ge_matrix + _ge_pitch * ge_row_i;
        uint64_t ge_mask = (uint64_t)1 << (pivot_i & 63);

        // For each GE matrix bit in the row:
        for (unsigned bit_j = pivot_i; bit_j < ge_limit; ++bit_j)
        {
            // If bit is non-zero:
            if (0 != (ge_row[bit_j >> 6] & ge_mask))
            {
                const unsigned column_j = _ge_col_map[bit_j];
                CAT_DEBUG_ASSERT(column_j < _recovery_rows);
                const uint8_t * GF256_RESTRICT src = _recovery_blocks + _block_bytes * column_j;

                // Add pivot for non-zero bit to destination row value
                gf256_add_mem(dest, src, _block_bytes);
                CAT_IF_ROWOP(++rowops;)

                CAT_IF_DUMP(cout << " " << bit_j << "=[" << (unsigned)src[0] << "]";)
            }

            ge_mask = CAT_ROL64(ge_mask, 1);
        }

        CAT_IF_DUMP(cout << endl;)
    }

    CAT_IF_ROWOP(cout << "AddSubdiagonalValues used " << rowops << " row ops = " << rowops / (double)_block_count << "*N and " << heavyops << " heavy ops" << endl;)
}

// These are heuristic values.  Choosing better values has little effect on performance.
#define CAT_ABOVE_WIN_THRESH_4 (20 + 4)
#define CAT_ABOVE_WIN_THRESH_5 (40 + 5)
#define CAT_ABOVE_WIN_THRESH_6 (64 + 6)
#define CAT_ABOVE_WIN_THRESH_7 (128 + 7)

void Codec::BackSubstituteAboveDiagonal()
{
    CAT_IF_DUMP(cout << endl << "---- BackSubstituteAboveDiagonal ----" << endl << endl;)

    CAT_IF_ROWOP(unsigned rowops = 0; unsigned heavyops = 0;)

    const unsigned pivot_count = _defer_count + _mix_count;
    unsigned pivot_i = pivot_count - 1;
    const uint16_t first_heavy_row = _defer_count + _dense_count;
    const unsigned first_heavy_column = _first_heavy_column;

#if defined(CAT_WINDOWED_BACKSUB)
    // Build temporary storage space if windowing is to be used
    if (pivot_i >= CAT_ABOVE_WIN_THRESH_5)
    {
        // Calculate initial window size
        unsigned w, next_check_i;
        if (pivot_i >= CAT_ABOVE_WIN_THRESH_7) {
            w = 7;
            next_check_i = CAT_ABOVE_WIN_THRESH_7;
        }
        else if (pivot_i >= CAT_ABOVE_WIN_THRESH_6) {
            w = 6;
            next_check_i = CAT_ABOVE_WIN_THRESH_6;
        }
        else if (pivot_i >= CAT_ABOVE_WIN_THRESH_5) {
            w = 5;
            next_check_i = CAT_ABOVE_WIN_THRESH_5;
        }
        else {
            w = 4;
            next_check_i = CAT_ABOVE_WIN_THRESH_4;
        }
        uint32_t win_lim = 1 << w;

        CAT_IF_DUMP(cout << "Activating windowed back-substitution with initial window " << w << endl;)

        // Use the first few peel column values as window table space
        // NOTE: The peeled column values were previously used up until this point,
        // but now they are unused, and so they can be reused for temporary space.
        uint8_t * GF256_RESTRICT win_table[128];
        const PeelColumn * GF256_RESTRICT column = _peel_cols;
        uint8_t * GF256_RESTRICT column_src = _recovery_blocks;
        uint32_t jj = 1;

        // For each original data column:
        for (unsigned count = _block_count; count > 0; --count, ++column, column_src += _block_bytes)
        {
            // If column is peeled:
            if (column->Mark == MARK_PEEL)
            {
                // Reuse the block value temporarily as window table space
                win_table[jj] = column_src;

                CAT_IF_DUMP(cout << "-- Window table entry " << jj << " set to column " << _block_count - count << endl;)

                // If done:
                if (++jj >= win_lim) {
                    break;
                }
            }
        }

        CAT_IF_DUMP(if (jj < win_lim) {
            cout << "!! Not enough space in peeled columns to generate a table.  " \
                "Going back to normal back-substitute." << endl;
        })

        // If enough space was found:
        if (jj >= win_lim) for (;;)
        {
            // Calculate first column in window
            const unsigned backsub_i = pivot_i - w + 1;

            CAT_IF_DUMP(cout << "-- Windowing from " << backsub_i << " to " << pivot_i << " (inclusive)" << endl;)

            // Eliminate upper triangular part above windowed bits:

            // For each column:
            uint64_t ge_mask = (uint64_t)1 << (pivot_i & 63);
            for (unsigned src_pivot_i = pivot_i; src_pivot_i > backsub_i; --src_pivot_i)
            {
                CAT_DEBUG_ASSERT(_ge_col_map[src_pivot_i] < _recovery_rows);
                uint8_t * GF256_RESTRICT src = _recovery_blocks + _block_bytes * _ge_col_map[src_pivot_i];

                const uint16_t ge_row_i = _pivots[src_pivot_i];

                // If diagonal element is heavy:
                if (ge_row_i >= first_heavy_row &&
                    src_pivot_i >= first_heavy_column)
                {
                    // Look up row value
                    CAT_DEBUG_ASSERT(ge_row_i >= first_heavy_row);
                    uint16_t heavy_row_i = ge_row_i - first_heavy_row;
                    CAT_DEBUG_ASSERT(src_pivot_i >= first_heavy_column);
                    unsigned heavy_col_i = src_pivot_i - first_heavy_column;
                    CAT_DEBUG_ASSERT(heavy_col_i < _heavy_columns);
                    CAT_DEBUG_ASSERT(heavy_row_i < _heavy_rows);
                    const uint8_t code_value = _heavy_matrix[_heavy_pitch * heavy_row_i + heavy_col_i];

                    // Normalize code value, setting it to 1 (implicitly nonzero)
                    if (code_value != 1) {
                        gf256_div_mem(src, src, code_value, _block_bytes);
                        CAT_IF_ROWOP(++heavyops;)
                    }

                    CAT_IF_DUMP(cout << "Normalized diagonal for heavy pivot " << pivot_i << endl;)
                }

                CAT_IF_DUMP(cout << "Back-substituting small triangle from pivot " << src_pivot_i << "[" << (unsigned)src[0] << "] :";)

                uint64_t * GF256_RESTRICT ge_row = _ge_matrix + (src_pivot_i >> 6);

                // For each row above the diagonal:
                for (unsigned dest_pivot_i = backsub_i; dest_pivot_i < src_pivot_i; ++dest_pivot_i)
                {
                    const uint16_t dest_row_i = _pivots[dest_pivot_i];

                    // If row is heavy:
                    if (dest_row_i >= first_heavy_row && src_pivot_i >= first_heavy_column)
                    {
                        CAT_DEBUG_ASSERT(dest_row_i >= first_heavy_row);
                        const unsigned heavy_row_i = dest_row_i - first_heavy_row;
                        CAT_DEBUG_ASSERT(src_pivot_i >= first_heavy_column);
                        const unsigned heavy_col_i = src_pivot_i - first_heavy_column;
                        const uint8_t code_value = _heavy_matrix[_heavy_pitch * heavy_row_i + heavy_col_i];

                        // If column is zero:
                        if (0 == code_value) {
                            continue; // Skip it
                        }

                        CAT_DEBUG_ASSERT(_ge_col_map[dest_pivot_i] < _recovery_rows);
                        uint8_t * GF256_RESTRICT dest = _recovery_blocks + _block_bytes * _ge_col_map[dest_pivot_i];

                        // Back-substitute
                        gf256_muladd_mem(dest, code_value, src, _block_bytes);

                        CAT_IF_ROWOP(if (code_value == 1) ++rowops; else ++heavyops;)
                        CAT_IF_DUMP(cout << " h" << dest_pivot_i;)
                    }
                    else
                    {
                        // If bit is set in that row:
                        if (ge_row[_ge_pitch * dest_row_i] & ge_mask)
                        {
                            CAT_DEBUG_ASSERT(_ge_col_map[dest_pivot_i] < _recovery_rows);
                            uint8_t * GF256_RESTRICT dest = _recovery_blocks + _block_bytes * _ge_col_map[dest_pivot_i];

                            // Back-substitute
                            gf256_add_mem(dest, src, _block_bytes);

                            CAT_IF_ROWOP(++rowops;)
                            CAT_IF_DUMP(cout << " " << dest_pivot_i;)
                        }
                    }
                } // next pivot above

                ge_mask = CAT_ROR64(ge_mask, 1);

                CAT_IF_DUMP(cout << endl;)
            } // next pivot

            // Normalize the final diagonal element
            const uint16_t ge_row_i = _pivots[backsub_i];

            // If this is a heavy row and column:
            if (ge_row_i >= first_heavy_row &&
                backsub_i >= first_heavy_column)
            {
                CAT_DEBUG_ASSERT(ge_row_i >= first_heavy_row);
                CAT_DEBUG_ASSERT(backsub_i >= first_heavy_column);
                const unsigned heavy_row_i = ge_row_i - first_heavy_row;
                const unsigned heavy_col_i = backsub_i - first_heavy_column;
                CAT_DEBUG_ASSERT(heavy_col_i < _heavy_columns);
                CAT_DEBUG_ASSERT(heavy_row_i < _heavy_rows);

                // Look up row value
                const uint8_t code_value = _heavy_matrix[_heavy_pitch * heavy_row_i + heavy_col_i];

                // Divide by this code value (implicitly nonzero)
                if (code_value != 1)
                {
                    CAT_DEBUG_ASSERT(_ge_col_map[backsub_i] < _recovery_rows);
                    uint8_t * GF256_RESTRICT src = _recovery_blocks + _block_bytes * _ge_col_map[backsub_i];

                    gf256_div_mem(src, src, code_value, _block_bytes);
                    CAT_IF_ROWOP(++heavyops;)
                }
            }

            CAT_IF_DUMP(cout << "-- Generating window table with " << w << " bits" << endl;)

            // Generate window table: 2 bits
            CAT_DEBUG_ASSERT(_ge_col_map[backsub_i] < _recovery_rows);
            win_table[1] = _recovery_blocks + _block_bytes * _ge_col_map[backsub_i];
            CAT_DEBUG_ASSERT(_ge_col_map[backsub_i + 1] < _recovery_rows);
            win_table[2] = _recovery_blocks + _block_bytes * _ge_col_map[backsub_i + 1];
            gf256_addset_mem(win_table[3], win_table[1], win_table[2], _block_bytes);
            CAT_IF_ROWOP(++rowops;)

            // Generate window table: 3 bits
            CAT_DEBUG_ASSERT(_ge_col_map[backsub_i + 2] < _recovery_rows);
            win_table[4] = _recovery_blocks + _block_bytes * _ge_col_map[backsub_i + 2];
            gf256_addset_mem(win_table[5], win_table[1], win_table[4], _block_bytes);
            gf256_addset_mem(win_table[6], win_table[2], win_table[4], _block_bytes);
            gf256_addset_mem(win_table[7], win_table[1], win_table[6], _block_bytes);
            CAT_IF_ROWOP(rowops += 3;)

            // Generate window table: 4 bits
            CAT_DEBUG_ASSERT(_ge_col_map[backsub_i + 3] < _recovery_rows);
            win_table[8] = _recovery_blocks + _block_bytes * _ge_col_map[backsub_i + 3];
            for (unsigned ii = 1; ii < 8; ++ii) {
                gf256_addset_mem(win_table[8 + ii], win_table[ii], win_table[8], _block_bytes);
            }
            CAT_IF_ROWOP(rowops += 7;)

            // Generate window table: 5+ bits
            if (w >= 5)
            {
                CAT_DEBUG_ASSERT(_ge_col_map[backsub_i + 4] < _recovery_rows);
                win_table[16] = _recovery_blocks + _block_bytes * _ge_col_map[backsub_i + 4];
                for (unsigned ii = 1; ii < 16; ++ii) {
                    gf256_addset_mem(win_table[16 + ii], win_table[ii], win_table[16], _block_bytes);
                }
                CAT_IF_ROWOP(rowops += 15;)

                if (w >= 6)
                {
                    CAT_DEBUG_ASSERT(_ge_col_map[backsub_i + 5] < _recovery_rows);
                    win_table[32] = _recovery_blocks + _block_bytes * _ge_col_map[backsub_i + 5];
                    for (unsigned ii = 1; ii < 32; ++ii) {
                        gf256_addset_mem(win_table[32 + ii], win_table[ii], win_table[32], _block_bytes);
                    }
                    CAT_IF_ROWOP(rowops += 31;)

                    if (w >= 7)
                    {
                        CAT_DEBUG_ASSERT(_ge_col_map[backsub_i + 6] < _recovery_rows);
                        win_table[64] = _recovery_blocks + _block_bytes * _ge_col_map[backsub_i + 6];
                        for (unsigned ii = 1; ii < 64; ++ii) {
                            gf256_addset_mem(win_table[64 + ii], win_table[ii], win_table[64], _block_bytes);
                        }
                        CAT_IF_ROWOP(rowops += 63;)
                    }
                }
            }

            // If a row above the window may be heavy:
            if (pivot_i >= first_heavy_column)
            {
                const uint16_t * GF256_RESTRICT pivot_row = _pivots;

                // For each pivot in the window:
                for (unsigned ge_above_i = 0; ge_above_i < backsub_i; ++ge_above_i)
                {
                    const unsigned ge_row_k = *pivot_row++;

                    // If row is not heavy:
                    if (ge_row_k < first_heavy_row) {
                        continue; // Skip it
                    }

                    CAT_DEBUG_ASSERT(_ge_col_map[ge_above_i] < _recovery_rows);
                    uint8_t * GF256_RESTRICT dest = _recovery_blocks + _block_bytes * _ge_col_map[ge_above_i];
                    unsigned ge_column_j = backsub_i;

                    // If the first column of window is not heavy:
                    if (ge_column_j < first_heavy_column)
                    {
                        uint64_t ge_mask2 = (uint64_t)1 << (ge_column_j & 63);
                        const uint64_t * GF256_RESTRICT ge_row = _ge_matrix + _ge_pitch * ge_row_k;

                        // For each non-heavy column in the extra row:
                        for (; ge_column_j < first_heavy_column && ge_column_j <= pivot_i; ++ge_column_j)
                        {
                            const uint64_t ge_word = ge_row[ge_column_j >> 6];
                            const bool nonzero = 0 != (ge_word & ge_mask2);

                            // If column is non-zero:
                            if (nonzero)
                            {
                                CAT_DEBUG_ASSERT(_ge_col_map[ge_column_j] < _recovery_rows);
                                const uint8_t *src = _recovery_blocks + _block_bytes * _ge_col_map[ge_column_j];

                                gf256_add_mem(dest, src, _block_bytes);

                                CAT_IF_ROWOP(++rowops;)
                            }

                            ge_mask2 = CAT_ROL64(ge_mask2, 1);
                        }
                    }

                    CAT_DEBUG_ASSERT(ge_row_k >= first_heavy_row);
                    CAT_DEBUG_ASSERT(ge_column_j >= first_heavy_column);
                    const unsigned heavy_row_i = ge_row_k - first_heavy_row;
                    const unsigned heavy_col_j = ge_column_j - first_heavy_column;
                    CAT_DEBUG_ASSERT(heavy_col_j < _heavy_columns);
                    CAT_DEBUG_ASSERT(heavy_row_i < _heavy_rows);

                    const uint8_t * GF256_RESTRICT heavy_row = &_heavy_matrix[_heavy_pitch * heavy_row_i + heavy_col_j];

                    // For each heavy column:
                    for (; ge_column_j <= pivot_i; ++ge_column_j)
                    {
                        const uint8_t code_value = *heavy_row++;

                        // If zero:
                        if (0 == code_value) {
                            continue; // Skip it
                        }

                        CAT_DEBUG_ASSERT(_ge_col_map[ge_column_j] < _recovery_rows);
                        const uint8_t * GF256_RESTRICT src = _recovery_blocks + _block_bytes * _ge_col_map[ge_column_j];

                        // Back-substitute
                        gf256_muladd_mem(dest, code_value, src, _block_bytes);

                        CAT_IF_ROWOP(if (code_value == 1) ++rowops; else ++heavyops;)
                    } // next column in row
                } // next pivot in window
            } // end if contains heavy

            // Only add window table entries for rows under this limit
            const uint16_t window_row_limit = (pivot_i >= first_heavy_column) ? first_heavy_row : 0x7fff;

            const uint32_t first_word = backsub_i >> 6;
            const uint32_t shift0 = backsub_i & 63;
            const uint32_t last_word = pivot_i >> 6;
            const uint16_t * GF256_RESTRICT pivot_row = _pivots;

            // If not straddling words:
            if (first_word == last_word)
            {
                // For each pivot row:
                for (uint16_t above_pivot_i = 0; above_pivot_i < backsub_i; ++above_pivot_i)
                {
                    const unsigned ge_row_k = *pivot_row++;

                    // If pivot row is heavy:
                    if (ge_row_k >= window_row_limit) {
                        continue; // Skip it
                    }

                    // Calculate window bits
                    const uint64_t * GF256_RESTRICT ge_row = _ge_matrix + first_word + _ge_pitch * ge_row_k;
                    const uint32_t win_bits = (uint32_t)(ge_row[0] >> shift0) & (win_lim - 1);

                    // If any XOR needs to be performed:
                    if (win_bits != 0)
                    {
                        CAT_IF_DUMP(cout << "Adding window table " << win_bits << " to pivot " << above_pivot_i << endl;)

                        CAT_DEBUG_ASSERT(_ge_col_map[above_pivot_i] < _recovery_rows);
                        uint8_t * GF256_RESTRICT dest = _recovery_blocks + _block_bytes * _ge_col_map[above_pivot_i];

                        // Back-substitute
                        gf256_add_mem(dest, win_table[win_bits], _block_bytes);

                        CAT_IF_ROWOP(++rowops;)
                    }
                }
            }
            else // Rare: Straddling case
            {
                const unsigned shift1 = 64 - shift0;

                // For each pivot row,
                for (uint16_t above_pivot_i = 0; above_pivot_i < backsub_i; ++above_pivot_i)
                {
                    const unsigned ge_row_k = *pivot_row++;

                    // If pivot row is heavy:
                    if (ge_row_k >= window_row_limit) {
                        continue; // Skip it
                    }

                    // Calculate window bits
                    const uint64_t * GF256_RESTRICT ge_row = _ge_matrix + first_word + _ge_pitch * ge_row_k;
                    const unsigned win_bits = ( (uint32_t)(ge_row[0] >> shift0) | (uint32_t)(ge_row[1] << shift1) ) & (win_lim - 1);

                    // If any XOR needs to be performed:
                    if (win_bits != 0)
                    {
                        CAT_IF_DUMP(cout << "Adding window table " << win_bits << " to pivot " << above_pivot_i << endl;)

                        CAT_DEBUG_ASSERT(_ge_col_map[above_pivot_i] < _recovery_rows);
                        uint8_t * GF256_RESTRICT dest = _recovery_blocks + _block_bytes * _ge_col_map[above_pivot_i];

                        // Back-substitute
                        gf256_add_mem(dest, win_table[win_bits], _block_bytes);

                        CAT_IF_ROWOP(++rowops;)
                    }
                }
            }

            // If column index falls below window size:
            pivot_i -= w;
            if (pivot_i < next_check_i)
            {
                if (pivot_i >= CAT_ABOVE_WIN_THRESH_6) {
                    w = 6;
                    next_check_i = CAT_ABOVE_WIN_THRESH_6;
                }
                else if (pivot_i >= CAT_ABOVE_WIN_THRESH_5) {
                    w = 5;
                    next_check_i = CAT_ABOVE_WIN_THRESH_5;
                }
                else if (pivot_i >= CAT_ABOVE_WIN_THRESH_4) {
                    w = 4;
                    next_check_i = CAT_ABOVE_WIN_THRESH_4;
                }
                else {
                    break;
                }

                // Update window limit
                win_lim = 1 << w;
            }
        } // next window
    } // end if windowed
#endif // CAT_WINDOWED_BACKSUB

    uint64_t ge_mask = (uint64_t)1 << (pivot_i & 63);

    // For each remaining pivot:
    for (;;)
    {
        // Calculate source
        CAT_DEBUG_ASSERT(_ge_col_map[pivot_i] < _recovery_rows);
        uint8_t * GF256_RESTRICT src = _recovery_blocks + _block_bytes * _ge_col_map[pivot_i];

        const uint16_t ge_row_i = _pivots[pivot_i];

        // If diagonal element is heavy:
        if (ge_row_i >= first_heavy_row &&
            pivot_i >= first_heavy_column)
        {
            // Look up row value
            CAT_DEBUG_ASSERT(ge_row_i >= first_heavy_row);
            const unsigned heavy_row_i = ge_row_i - first_heavy_row;
            CAT_DEBUG_ASSERT(pivot_i >= first_heavy_column);
            const unsigned heavy_col_i = pivot_i - first_heavy_column;
            CAT_DEBUG_ASSERT(heavy_col_i < _heavy_columns);
            CAT_DEBUG_ASSERT(heavy_row_i < _heavy_rows);
            const uint8_t code_value = _heavy_matrix[_heavy_pitch * heavy_row_i + heavy_col_i];

            // Normalize code value, setting it to 1 (implicitly nonzero)
            if (code_value != 1) {
                gf256_div_mem(src, src, code_value, _block_bytes);
                CAT_IF_ROWOP(++heavyops;)
            }

            CAT_IF_DUMP(cout << "Normalized diagonal for heavy pivot " << pivot_i << endl;)
        }

        CAT_IF_DUMP(cout << "Pivot " << pivot_i << "[" << (unsigned)src[0] << "]:";)

        const uint64_t * GF256_RESTRICT ge_row = _ge_matrix + (pivot_i >> 6);

        // For each pivot row above it:
        for (unsigned ge_up_i = 0; ge_up_i < pivot_i; ++ge_up_i)
        {
            const uint16_t up_row_i = _pivots[ge_up_i];

            // If element is heavy:
            if (up_row_i >= first_heavy_row &&
                ge_up_i >= first_heavy_column)
            {
                const unsigned heavy_row_i = up_row_i - first_heavy_row;
                const unsigned heavy_col_i = pivot_i - first_heavy_column;
                const uint8_t code_value = _heavy_matrix[_heavy_pitch * heavy_row_i + heavy_col_i];

                // If column is zero:
                if (!code_value) {
                    continue; // Skip it
                }

                CAT_DEBUG_ASSERT(_ge_col_map[ge_up_i] < _recovery_rows);
                uint8_t * GF256_RESTRICT dest = _recovery_blocks + _block_bytes * _ge_col_map[ge_up_i];

                // Back-substitute
                gf256_muladd_mem(dest, code_value, src, _block_bytes);

                CAT_IF_ROWOP(if (code_value == 1) {
                    ++rowops;
                }
                else {
                    ++heavyops;
                })
                CAT_IF_DUMP(cout << " h" << up_row_i;)
            }
            else
            {
                // If bit is set in that row,
                if (ge_row[_ge_pitch * up_row_i] & ge_mask)
                {
                    CAT_DEBUG_ASSERT(_ge_col_map[ge_up_i] < _recovery_rows);
                    uint8_t * GF256_RESTRICT dest = _recovery_blocks + _block_bytes * _ge_col_map[ge_up_i];

                    // Back-substitute
                    gf256_add_mem(dest, src, _block_bytes);

                    CAT_IF_ROWOP(++rowops;)
                    CAT_IF_DUMP(cout << " " << up_row_i;)
                }
            }
        } // next pivot above

        if (pivot_i <= 0) {
            break;
        }
        --pivot_i;

        ge_mask = CAT_ROR64(ge_mask, 1);

        CAT_IF_DUMP(cout << endl;)
    }

    CAT_IF_ROWOP(cout << "BackSubstituteAboveDiagonal used " << rowops << " row ops = " << rowops / (double)_block_count << "*N and " << heavyops << " heavy ops" << endl;)
}

void Codec::Substitute()
{
    CAT_IF_DUMP(cout << endl << "---- Substitute ----" << endl << endl;)

    CAT_IF_ROWOP(uint32_t rowops = 0;)

    PeelRow * GF256_RESTRICT row;

    // For each column that has been peeled:
    for (uint16_t row_i = _peel_head_rows; row_i != LIST_TERM; row_i = row->NextRow)
    {
        row = &_peel_rows[row_i];

        const uint16_t dest_column_i = row->Marks.Result.PeelColumn;
        CAT_DEBUG_ASSERT(dest_column_i < _recovery_rows);
        uint8_t * GF256_RESTRICT dest = _recovery_blocks + _block_bytes * dest_column_i;

        CAT_IF_DUMP(cout << "Generating column " << dest_column_i << ":";)

        const uint8_t * GF256_RESTRICT input_src = _input_blocks + _block_bytes * row_i;
        CAT_IF_DUMP(cout << " " << row_i << ":[" << (unsigned)input_src[0] << "]";)

        const RowMixIterator mix(row->Params, _mix_count, _mix_next_prime);

        // Set up mixing column generator
        CAT_DEBUG_ASSERT((unsigned)(_block_count + mix.Columns[0]) < _recovery_rows);
        const uint8_t * GF256_RESTRICT src = _recovery_blocks + _block_bytes * (_block_count + mix.Columns[0]);

        // If copying from final block:
        if (row_i != _block_count - 1) {
            gf256_addset_mem(dest, src, input_src, _block_bytes);
        }
        else
        {
            gf256_addset_mem(dest, src, input_src, _input_final_bytes);
            memcpy(
                dest + _input_final_bytes,
                src + _input_final_bytes,
                _block_bytes - _input_final_bytes);
        }
        CAT_IF_ROWOP(++rowops;)

        CAT_DEBUG_ASSERT((unsigned)(_block_count + mix.Columns[1]) < _recovery_rows);
        const uint8_t * GF256_RESTRICT src0 = _recovery_blocks + _block_bytes * (_block_count + mix.Columns[1]);
        CAT_DEBUG_ASSERT((unsigned)(_block_count + mix.Columns[2]) < _recovery_rows);
        const uint8_t * GF256_RESTRICT src1 = _recovery_blocks + _block_bytes * (_block_count + mix.Columns[2]);

        // Add next two mixing columns in
        gf256_add2_mem(dest, src0, src1, _block_bytes);

        CAT_IF_ROWOP(++rowops;)

        // If at least two peeling columns are set:
        if (row->Params.PeelCount >= 2) // common case:
        {
            PeelRowIterator iter(row->Params, _block_count, _block_next_prime);

            const uint16_t column_0 = iter.GetColumn();
            iter.Iterate();
            const uint16_t column_1 = iter.GetColumn();

            // Common case:
            if (column_0 != dest_column_i)
            {
                CAT_DEBUG_ASSERT(column_0 < _recovery_rows);
                const uint8_t * GF256_RESTRICT peel0 = _recovery_blocks + _block_bytes * column_0;

                // Common case:
                if (column_1 != dest_column_i) {
                    CAT_DEBUG_ASSERT(column_1 < _recovery_rows);
                    gf256_add2_mem(dest, peel0, _recovery_blocks + _block_bytes * column_1, _block_bytes);
                }
                else {
                    gf256_add_mem(dest, peel0, _block_bytes);
                }
            }
            else {
                CAT_DEBUG_ASSERT(column_1 < _recovery_rows);
                gf256_add_mem(dest, _recovery_blocks + _block_bytes * column_1, _block_bytes);
            }
            CAT_IF_ROWOP(++rowops;)

            // For each remaining column:
            while (iter.Iterate())
            {
                const uint16_t column_i = iter.GetColumn();
                CAT_DEBUG_ASSERT(column_i < _recovery_rows);
                const uint8_t * GF256_RESTRICT peel_src = _recovery_blocks + _block_bytes * column_i;

                CAT_IF_DUMP(cout << " " << column_i;)

                // If column is not the solved one:
                if (column_i != dest_column_i)
                {
                    gf256_add_mem(dest, peel_src, _block_bytes);
                    CAT_IF_ROWOP(++rowops;)
                    CAT_IF_DUMP(cout << "[" << (unsigned)peel_src[0] << "]";)
                }
                else {
                    CAT_IF_DUMP(cout << "*";)
                }
            }
        } // end if weight 2

        CAT_IF_DUMP(cout << endl;)
    }

    CAT_IF_ROWOP(cout << "Substitute used " << rowops << " row ops = " << rowops / (double)_block_count << "*N" << endl;)
}


//------------------------------------------------------------------------------
// Setup

void Codec::OverrideSeeds(
    uint16_t dense_count,
    uint16_t p_seed,
    uint16_t d_seed)
{
    _dense_count = dense_count;
    _p_seed = p_seed;
    _d_seed = d_seed;
    _seed_override = true;
}

WirehairResult Codec::ChooseMatrix(
    uint64_t message_bytes,
    unsigned block_bytes)
{
    CAT_IF_DUMP(cout << endl << "---- ChooseMatrix ----" << endl << endl;)

    // Validate input
    if (message_bytes < 1 || block_bytes < 1) {
        return Wirehair_InvalidInput;
    }

    // Calculate message block count
    _block_bytes = block_bytes;
    _block_count = static_cast<uint16_t>((message_bytes + _block_bytes - 1) / _block_bytes);
    _block_next_prime = NextPrime16(_block_count);

    // Validate block count
    if (_block_count < CAT_WIREHAIR_MIN_N) {
        return Wirehair_BadInput_SmallN;
    }
    if (_block_count > CAT_WIREHAIR_MAX_N) {
        return Wirehair_BadInput_LargeN;
    }

    CAT_IF_DUMP(cout << "Total message = " << message_bytes << " bytes.  Block bytes = " << _block_bytes << endl;)
    CAT_IF_DUMP(cout << "Block count = " << _block_count << " +Prime=" << _block_next_prime << endl;)

    if (!_seed_override)
    {
        // Pick dense row count, dense row seed, and peel row seed
        _dense_count = GetDenseCount(_block_count);
        _d_seed = GetDenseSeed(_block_count, _dense_count);
        _p_seed = GetPeelSeed(_block_count);
    }

    CAT_IF_DUMP(cout << "Peel seed = " << _p_seed << "  Dense seed = " << _d_seed << endl;)

    _mix_count = _dense_count + kHeavyRows;
    _mix_next_prime = NextPrime16(_mix_count);

    CAT_IF_DUMP(cout << "Mix count = " << _mix_count << " +Prime=" << _mix_next_prime << endl;)

    // Initialize lists
    _peel_head_rows = LIST_TERM;
    _peel_tail_rows = 0;
    _defer_head_rows = LIST_TERM;

    return Wirehair_Success;
}

WirehairResult Codec::SolveMatrix()
{
    // (1) Peeling

    GreedyPeeling();

    CAT_IF_DUMP( PrintPeeled(); )
    CAT_IF_DUMP( PrintDeferredRows(); )
    CAT_IF_DUMP( PrintDeferredColumns(); )

    // (2) Compression

    if (!AllocateMatrix()) {
        return Wirehair_OOM;
    }

    SetDeferredColumns();
    SetMixingColumnsForDeferredRows();
    PeelDiagonal();
    CopyDeferredRows();
    MultiplyDenseRows();
    SetHeavyRows();

    // Add invertible matrix to mathematically tie dense rows to dense mixing columns
    AddInvertibleGF2Matrix(_ge_matrix, _defer_count, _ge_pitch, _dense_count);

#if defined(CAT_DUMP_CODEC_DEBUG) || defined(CAT_DUMP_GE_MATRIX)
    cout << "After Compress:" << endl;
    PrintGEMatrix();
#endif
    CAT_IF_DUMP( PrintCompressMatrix(); )

    // (3) Gaussian Elimination

    SetupTriangle();

    if (!Triangle())
    {
        CAT_IF_DUMP( cout << "After Triangle FAILED:" << endl; )
        CAT_IF_DUMP( PrintGEMatrix(); )
        CAT_IF_DUMP( PrintExtraMatrix(); )

        return Wirehair_NeedMore;
    }

#if defined(CAT_DUMP_CODEC_DEBUG) || defined(CAT_DUMP_GE_MATRIX)
    cout << "After Triangle:" << endl;
    PrintGEMatrix();
    PrintExtraMatrix();
#endif

    return Wirehair_Success;
}

void Codec::GenerateRecoveryBlocks()
{
    InitializeColumnValues();
    MultiplyDenseValues();
    AddSubdiagonalValues();
    BackSubstituteAboveDiagonal();
    Substitute();
}

WirehairResult Codec::ResumeSolveMatrix(
    const unsigned id, ///< Block ID
    const void * GF256_RESTRICT data ///< Block data
)
{
    CAT_IF_DUMP(cout << endl << "---- ResumeSolveMatrix ----" << endl << endl;)

    if (!data) {
        return Wirehair_InvalidInput;
    }

    unsigned row_i, ge_row_i, new_pivot_i;

    // If there is no room for it:
    if (_row_count >= _block_count + _extra_count)
    {
        const uint16_t first_heavy_row = _defer_count + _dense_count;

        new_pivot_i = 0;

        // For each pivot in the list:
        for (unsigned pivot_i = _next_pivot; pivot_i < _pivot_count; ++pivot_i)
        {
            const uint16_t ge_row_k = _pivots[pivot_i];

            // If unused row is extra:
            if (ge_row_k >= first_heavy_row &&
                ge_row_k < (first_heavy_row + _extra_count))
            {
                // Re-use it
                new_pivot_i = (uint16_t)pivot_i;
                break;
            }
        }

        // If nothing was found, return error
        if (!new_pivot_i) {
            return Wirehair_ExtraInsufficient;
        }

        // Look up row indices
        ge_row_i = _pivots[new_pivot_i];
        row_i = _ge_row_map[ge_row_i];
    }
    else
    {
        // Add extra rows to the end of the pivot list
        new_pivot_i = _pivot_count++;
        row_i = _row_count++;
        ge_row_i = _defer_count + _dense_count + row_i - _block_count;
        _ge_row_map[ge_row_i] = (uint16_t)row_i;
        _pivots[new_pivot_i] = (uint16_t)ge_row_i;

        /*
            Before the extra rows are converted to heavy, the new rows
            are added to the end of the pivot list.  And after the extra
            rows are converted to heavy rows, new rows that come in are
            also heavy and should also be at the end of the pivot list.

            So, this doesn't need to change based on what stage of the
            GE solver is running through at this point.
        */
    }

    CAT_IF_DUMP(cout << "Resuming using row slot " << row_i << " and GE row " << ge_row_i << endl;)

    // Update row data needed at this point
    PeelRow * GF256_RESTRICT row = &_peel_rows[row_i];
    row->RecoveryId = id;

    uint8_t * GF256_RESTRICT block_store_dest = _input_blocks + _block_bytes * row_i;

    // Copy new block to input blocks
    if (id != (unsigned)_block_count - 1) {
        memcpy(block_store_dest, data, _block_bytes);
    }
    else
    {
        memcpy(block_store_dest, data, _output_final_bytes);

        memset(
            block_store_dest + _output_final_bytes,
            0,
            _block_bytes - _output_final_bytes);
    }

    // Generate new GE row
    uint64_t * GF256_RESTRICT ge_new_row = _ge_matrix + _ge_pitch * ge_row_i;

    // Clear the row initially before flipping bits on
    memset(ge_new_row, 0, _ge_pitch * sizeof(uint64_t));

    row->Params.Initialize(
        id,
        _p_seed,
        _block_count,
        _mix_count);

    PeelRowIterator iter(row->Params, _block_count, _block_next_prime);
    const RowMixIterator mix(row->Params, _mix_count, _mix_next_prime);

    // Generate mixing bits in GE row
    uint16_t ge_column_i = mix.Columns[0] + _defer_count;
    ge_new_row[ge_column_i >> 6] ^= (uint64_t)1 << (ge_column_i & 63);
    ge_column_i = mix.Columns[1] + _defer_count;
    ge_new_row[ge_column_i >> 6] ^= (uint64_t)1 << (ge_column_i & 63);
    ge_column_i = mix.Columns[2] + _defer_count;
    ge_new_row[ge_column_i >> 6] ^= (uint64_t)1 << (ge_column_i & 63);

    // Generate peeled bits in GE row
    do
    {
        const uint16_t column = iter.GetColumn();

        PeelColumn * GF256_RESTRICT ref_col = &_peel_cols[column];

        // If column is peeled:
        if (ref_col->Mark == MARK_PEEL)
        {
            const unsigned row_k = ref_col->PeelRow;
            const uint64_t * GF256_RESTRICT ge_src_row = _compress_matrix + _ge_pitch * row_k;

            // Add compress row to the new GE row
            for (unsigned ii = 0; ii < _ge_pitch; ++ii) {
                ge_new_row[ii] ^= ge_src_row[ii];
            }
        }
        else
        {
            const unsigned ge_column_k = ref_col->GEColumn;

            // Set bit for this deferred column
            ge_new_row[ge_column_k >> 6] ^= (uint64_t)1 << (ge_column_k & 63);
        }
    } while (iter.Iterate());

    uint64_t ge_mask = 1;

    // For each pivot-found column up to the start of the heavy columns:
    for (uint16_t pivot_j = 0; pivot_j < _next_pivot && pivot_j < _first_heavy_column; ++pivot_j)
    {
        const unsigned word_offset = pivot_j >> 6;
        uint64_t * GF256_RESTRICT rem_row = &ge_new_row[word_offset];

        // If bit is set:
        if (0 != (*rem_row & ge_mask))
        {
            const unsigned ge_row_j = _pivots[pivot_j];
            const uint64_t * GF256_RESTRICT ge_pivot_row = _ge_matrix + word_offset + _ge_pitch * ge_row_j;
            const uint64_t row0 = (*ge_pivot_row & ~(ge_mask - 1)) ^ ge_mask;

            // Unroll first word
            *rem_row ^= row0;

            // Add previous pivot row to new row
            CAT_DEBUG_ASSERT(_ge_pitch >= word_offset);
            for (unsigned ii = 1; ii < _ge_pitch - word_offset; ++ii) {
                rem_row[ii] ^= ge_pivot_row[ii];
            }
        }

        ge_mask = CAT_ROL64(ge_mask, 1);
    }

    // If next pivot is not heavy:
    if (_next_pivot < _first_heavy_column)
    {
        const uint64_t bit = ge_new_row[_next_pivot >> 6] & ((uint64_t)1 << (_next_pivot & 63));

        // If the next pivot was not found on this row:
        if (0 == bit) {
            return Wirehair_NeedMore; // Maybe next time...
        }

        // Swap out the pivot index for this one
        _pivots[new_pivot_i] = _pivots[_next_pivot];
        _pivots[_next_pivot] = (uint16_t)ge_row_i;
    }
    else
    {
        const uint16_t column_count = _defer_count + _mix_count;
        const uint16_t first_heavy_row = _dense_count + _defer_count;
        CAT_DEBUG_ASSERT(ge_row_i >= first_heavy_row);
        const unsigned heavy_row_i = ge_row_i - first_heavy_row;
        CAT_DEBUG_ASSERT(heavy_row_i < _heavy_rows);
        uint8_t * GF256_RESTRICT heavy_row = _heavy_matrix + _heavy_pitch * heavy_row_i;

        // For each heavy column:
        for (unsigned ge_column_j = _first_heavy_column; ge_column_j < column_count; ++ge_column_j)
        {
            CAT_DEBUG_ASSERT(ge_column_j >= _first_heavy_column);
            const unsigned heavy_col_j = ge_column_j - _first_heavy_column;
            const uint8_t bit_j = static_cast<uint8_t>((ge_new_row[ge_column_j >> 6] >> (ge_column_j & 63)) & 1);

            // Copy bit into column byte
            heavy_row[heavy_col_j] = bit_j;
        }

        // For each pivot-found column in the heavy columns:
        for (unsigned pivot_j = _first_heavy_column; pivot_j < _next_pivot; ++pivot_j)
        {
            CAT_DEBUG_ASSERT(pivot_j >= _first_heavy_column);
            const unsigned heavy_col_j = pivot_j - _first_heavy_column;
            CAT_DEBUG_ASSERT(heavy_col_j < _heavy_columns);
            const uint8_t code_value = heavy_row[heavy_col_j];

            // If column is zero:
            if (0 == code_value) {
                continue; // Skip it
            }

            const unsigned ge_row_j = _pivots[pivot_j];

            // If previous row is heavy:
            if (ge_row_j >= first_heavy_row)
            {
                // Calculate coefficient of elimination
                CAT_DEBUG_ASSERT(ge_row_j >= first_heavy_row);
                const unsigned heavy_row_j = ge_row_j - first_heavy_row;
                CAT_DEBUG_ASSERT(heavy_row_j < _heavy_rows);
                const uint8_t * GF256_RESTRICT heavy_pivot_row = _heavy_matrix + _heavy_pitch * heavy_row_j;
                CAT_DEBUG_ASSERT(heavy_col_j < _heavy_columns);
                const uint8_t pivot_code = heavy_pivot_row[heavy_col_j];
                const unsigned start_column = heavy_col_j + 1;

                // heavy[m+] += exist[m+] * (code_value / pivot_code)
                if (pivot_code == 1) {
                    // heavy[m+] += exist[m+] * code_value
                    gf256_muladd_mem(
                        heavy_row + start_column,
                        code_value,
                        heavy_pivot_row + start_column,
                        _heavy_columns - start_column);
                }
                else
                {
                    // eliminator = code_value / pivot_code
                    const uint8_t eliminator = gf256_div(code_value, pivot_code);

                    // Store eliminator for later
                    heavy_row[heavy_col_j] = eliminator;

                    // heavy[m+] += exist[m+] * eliminator
                    gf256_muladd_mem(
                        heavy_row + start_column,
                        eliminator,
                        heavy_pivot_row + start_column,
                        _heavy_columns - start_column);
                }
            }
            else
            {
                const uint64_t * GF256_RESTRICT other_row = _ge_matrix + _ge_pitch * ge_row_j;

                unsigned ge_column_k = pivot_j + 1;
                uint64_t ge_mask_k = (uint64_t)1 << (ge_column_k & 63);

                // For each remaining column:
                for (; ge_column_k < column_count; ++ge_column_k)
                {
                    const uint64_t word = other_row[ge_column_k >> 6];
                    const bool nonzero = 0 != (word & ge_mask_k);

                    // If bit is nonzero:
                    if (nonzero) {
                        // Add in the code value for this column
                        heavy_row[ge_column_k - _first_heavy_column] ^= code_value;
                    }

                    ge_mask_k = CAT_ROL64(ge_mask_k, 1);
                }
            } // end if row is heavy
        } // next column

        CAT_DEBUG_ASSERT(_next_pivot >= _first_heavy_column);
        const unsigned next_heavy_col = _next_pivot - _first_heavy_column;

        // If the next pivot was not found on this heavy row:
        if (!heavy_row[next_heavy_col]) {
            return Wirehair_NeedMore; // Maybe next time...
        }

        // If a non-heavy pivot just got moved into heavy pivot list:
        if (_next_pivot < _first_heavy_pivot)
        {
            // Swap out the pivot index for this one
            _pivots[new_pivot_i] = _pivots[_first_heavy_pivot];
            _pivots[_first_heavy_pivot] = _pivots[_next_pivot];

            // And move the first heavy pivot up one to cover the hole
            ++_first_heavy_pivot;
        }
        else {
            // Swap out the pivot index for this one
            _pivots[new_pivot_i] = _pivots[_next_pivot];
        }

        CAT_DEBUG_ASSERT(ge_row_i < _pivot_count);
        _pivots[_next_pivot] = (uint16_t)ge_row_i;
    }

    // NOTE: Pivot was found and is definitely not set anywhere else
    // so it doesn't need to be cleared from any other GE rows.

    // If just starting heavy columns:
    if (++_next_pivot == _first_heavy_column) {
        InsertHeavyRows();
    }

    // Resume Triangle() at next pivot to determine
    return Triangle() ? Wirehair_Success : Wirehair_NeedMore;
}

#if defined(CAT_ALL_ORIGINAL)

bool Codec::IsAllOriginalData()
{
    // Use some of the recovery blocks memory
    uint8_t * GF256_RESTRICT copied_rows = reinterpret_cast<uint8_t*>(_recovery_blocks);

    // Zero an array to store whether or not each row id needs to be regenerated
    memset(copied_rows, 0, _block_count);

    const PeelRow * GF256_RESTRICT row = _peel_rows;
    unsigned seen_rows = 0;

    // Copy any original message rows that were received:
    for (uint16_t row_i = 0; row_i < _row_count; ++row_i, ++row)
    {
        const uint32_t id = row->RecoveryId;

        // If the row identifier indicates it is part of the original message data:
        // If not already marked copied:
        if (id < _block_count &&
            !copied_rows[id])
        {
            copied_rows[id] = 1;
            ++seen_rows;
        }
    }

    // If all rows were seen, return true
    return seen_rows >= _block_count;
}

#endif // CAT_ALL_ORIGINAL

WirehairResult Codec::ReconstructBlock(
    const uint16_t block_id, ///< Block identifier
    void * GF256_RESTRICT block_out, ///< Output block memory
    uint32_t* bytes_out ///< Bytes written to output
)
{
    CAT_IF_DUMP(cout << endl << "---- ReconstructBlock ----" << endl << endl;)

    // Validate input
    if (!block_out || block_id >= _block_count)
    {
        *bytes_out = 0;
        return Wirehair_InvalidInput;
    }

#if defined(CAT_ALL_ORIGINAL)
    // If decoder received only original data out of order:
    if (_all_original)
    {
        PeelRow * GF256_RESTRICT row = _peel_rows;
        const uint8_t * GF256_RESTRICT src = _input_blocks;

        // For each row that was received:
        for (uint16_t row_i = 0, count = _row_count; row_i < count; ++row_i)
        {
            const uint32_t id = row[row_i].RecoveryId;

            // If this is the id we seek:
            if (id == block_id)
            {
                CAT_IF_DUMP(cout << "Copying received row " << id << endl;)

                const unsigned bytes = (id != (unsigned)_block_count - 1) ? _block_bytes : _output_final_bytes;

                memcpy(block_out, src + row_i * _block_bytes, bytes);

                *bytes_out = (uint32_t)bytes;

                return Wirehair_Success;
            }
        }

        return Wirehair_Error;
    }
#endif

    // Regenerate any single row that got lost:

    uint32_t block_bytes = _block_bytes;

    // For last row, use final byte count
    if (block_id == _block_count - 1) {
        block_bytes = _output_final_bytes;
    }

    CAT_IF_DUMP(cout << "Regenerating row " << block_id << ":";)

    PeelRowParameters params;
    params.Initialize(block_id, _p_seed, _block_count, _mix_count);

    PeelRowIterator iter(params, _block_count, _block_next_prime);
    const RowMixIterator mix(params, _mix_count, _mix_next_prime);

    const uint16_t peel_0 = iter.GetColumn();

    // Remember first column (there is always at least one)
    CAT_DEBUG_ASSERT(peel_0 < _recovery_rows);
    uint8_t * GF256_RESTRICT first = _recovery_blocks + _block_bytes * peel_0;

    CAT_IF_DUMP(cout << " " << peel_0;)

    // If this row references multiple columns:
    if (iter.Iterate())
    {
        const uint16_t peel_1 = iter.GetColumn();

        CAT_IF_DUMP(cout << " " << peel_1;)

        CAT_DEBUG_ASSERT(peel_1 < _recovery_rows);

        // Combine first two columns into output buffer (faster than memcpy + memxor)
        gf256_addset_mem(
            block_out,
            first,
            _recovery_blocks + _block_bytes * peel_1,
            block_bytes);

        // For each remaining peeler column:
        while (iter.Iterate())
        {
            const uint16_t peel_x = iter.GetColumn();

            CAT_IF_DUMP(cout << " " << peel_x;)

            CAT_DEBUG_ASSERT(peel_x < _recovery_rows);

            // Mix in each column
            gf256_add_mem(
                block_out,
                _recovery_blocks + _block_bytes * peel_x,
                block_bytes);
        }

        CAT_DEBUG_ASSERT((unsigned)(_block_count + mix.Columns[0]) < _recovery_rows);

        // Mix first mixer block in directly
        gf256_add_mem(
            block_out,
            _recovery_blocks + _block_bytes * (_block_count + mix.Columns[0]),
            block_bytes);
    }
    else
    {
        CAT_DEBUG_ASSERT((unsigned)(_block_count + mix.Columns[0]) < _recovery_rows);

        // Mix first with first mixer block (faster than memcpy + memxor)
        gf256_addset_mem(
            block_out,
            first,
            _recovery_blocks + _block_bytes * (_block_count + mix.Columns[0]),
            block_bytes);
    }

    CAT_IF_DUMP(cout << " " << (_block_count + mix.Columns[0]);)

    // Combine remaining two mixer columns together:
    CAT_DEBUG_ASSERT((unsigned)(_block_count + mix.Columns[1]) < _recovery_rows);
    const uint8_t * mix0_src = _recovery_blocks + _block_bytes * (_block_count + mix.Columns[1]);
    CAT_IF_DUMP(cout << " " << (_block_count + mix.Columns[1]);)

    CAT_DEBUG_ASSERT((unsigned)(_block_count + mix.Columns[2]) < _recovery_rows);
    const uint8_t * mix1_src = _recovery_blocks + _block_bytes * (_block_count + mix.Columns[2]);
    CAT_IF_DUMP(cout << " " << (_block_count + mix.Columns[2]);)

    gf256_add2_mem(block_out, mix0_src, mix1_src, block_bytes);

    CAT_IF_DUMP(cout << endl;)

    *bytes_out = static_cast<uint32_t>(block_bytes);
    return Wirehair_Success;
}

WirehairResult Codec::ReconstructOutput(
    void * GF256_RESTRICT message_out,
    uint64_t message_bytes)
{
    CAT_IF_DUMP(cout << endl << "---- ReconstructOutput ----" << endl << endl;)

    // Validate input
    if (!message_out) {
        return Wirehair_InvalidInput;
    }
    if (message_bytes != _block_bytes * (_block_count - 1) + _output_final_bytes) {
        CAT_DEBUG_BREAK();
        return Wirehair_InvalidInput;
    }
    uint8_t * GF256_RESTRICT output_blocks = reinterpret_cast<uint8_t *>( message_out );

#if defined(CAT_COPY_FIRST_N)
    // Re-purpose and initialize an array to store whether or not each row id needs to be regenerated
    uint8_t * GF256_RESTRICT copied_original = _copied_original;
    memset(copied_original, 0, _block_count);

    // Copy any original message rows that were received
    const uint8_t * GF256_RESTRICT src = _input_blocks;
    PeelRow * GF256_RESTRICT row = _peel_rows;

    // For each row:
    for (uint16_t row_i = 0; row_i < _row_count; ++row_i, ++row, src += _block_bytes)
    {
        const uint32_t block_id = row->RecoveryId;

        // If the row identifier indicates it is part of the original message data:
        if (block_id < _block_count)
        {
            CAT_IF_DUMP(cout << "Copying received row " << id << endl;)

            uint8_t * GF256_RESTRICT dest = output_blocks + _block_bytes * block_id;
            const unsigned bytes = (block_id != (unsigned)_block_count - 1) ? _block_bytes : _output_final_bytes;

            memcpy(dest, src, bytes);

            copied_original[block_id] = 1;
        }
    }
#endif // CAT_COPY_FIRST_N

    // Regenerate any rows that got lost:

    uint8_t * GF256_RESTRICT dest = output_blocks;
    unsigned block_bytes = _block_bytes;

    // For each block to generate:
    const uint16_t block_count = _block_count;
    for (uint32_t block_id = 0; block_id < block_count; ++block_id, dest += _block_bytes)
    {
#if defined(CAT_COPY_FIRST_N)
        // If already copied, skip it
        if (copied_original[block_id]) {
            continue;
        }
#endif // CAT_COPY_FIRST_N
        // For last row, use final byte count
        if (block_id + 1 == block_count) {
            block_bytes = _output_final_bytes;
        }

        CAT_IF_DUMP(cout << "Regenerating row " << row_i << ":";)

        PeelRowParameters params;
        params.Initialize(block_id, _p_seed, block_count, _mix_count);

        PeelRowIterator iter(params, block_count, _block_next_prime);
        const RowMixIterator mix(params, _mix_count, _mix_next_prime);

        const uint16_t peel_0 = iter.GetColumn();

        // Remember first column (there is always at least one)
        CAT_DEBUG_ASSERT(peel_0 < _recovery_rows);
        const uint8_t * GF256_RESTRICT first = _recovery_blocks + _block_bytes * peel_0;

        CAT_IF_DUMP(cout << " " << peel_0;)

        // If peeler has multiple columns:
        if (iter.Iterate())
        {
            const uint16_t peel_1 = iter.GetColumn();

            CAT_IF_DUMP(cout << " " << peel_1;)

            // Combine first two columns into output buffer (faster than memcpy + memxor)
            CAT_DEBUG_ASSERT(peel_1 < _recovery_rows);
            gf256_addset_mem(dest, first, _recovery_blocks + _block_bytes * peel_1, block_bytes);

            // For each remaining peeler column:
            while (iter.Iterate())
            {
                const uint16_t peel_x = iter.GetColumn();

                CAT_IF_DUMP(cout << " " << peel_x;)

                // Mix in each column
                CAT_DEBUG_ASSERT(peel_x < _recovery_rows);
                gf256_add_mem(dest, _recovery_blocks + _block_bytes * peel_x, block_bytes);
            }

            // Mix first mixer block in directly
            CAT_DEBUG_ASSERT((unsigned)(block_count + mix.Columns[0]) < _recovery_rows);
            gf256_add_mem(dest, _recovery_blocks + _block_bytes * (block_count + mix.Columns[0]), block_bytes);
        }
        else
        {
            // Mix first with first mixer block (faster than memcpy + memxor)
            CAT_DEBUG_ASSERT((unsigned)(block_count + mix.Columns[0]) < _recovery_rows);
            gf256_addset_mem(dest, first, _recovery_blocks + _block_bytes * (block_count + mix.Columns[0]), block_bytes);
        }

        CAT_IF_DUMP(cout << " " << (block_count + mix.Columns[0]);)

        // Combine remaining two mixer columns together:
        CAT_DEBUG_ASSERT((unsigned)(block_count + mix.Columns[1]) < _recovery_rows);
        const uint8_t *mix0_src = _recovery_blocks + _block_bytes * (block_count + mix.Columns[1]);
        CAT_IF_DUMP(cout << " " << (block_count + mix.Columns[1]);)

        CAT_DEBUG_ASSERT((unsigned)(block_count + mix.Columns[2]) < _recovery_rows);
        const uint8_t *mix1_src = _recovery_blocks + _block_bytes * (block_count + mix.Columns[2]);
        CAT_IF_DUMP(cout << " " << (block_count + mix.Columns[2]);)

        gf256_add2_mem(dest, mix0_src, mix1_src, block_bytes);

        CAT_IF_DUMP(cout << endl;)
    } // next row

    return Wirehair_Success;
}


//// Memory Management

Codec::Codec()
{
}

Codec::~Codec()
{
    FreeWorkspace();
    FreeMatrix();
    FreeInput();
}

void Codec::SetInput(const void * GF256_RESTRICT message_in)
{
    FreeInput();

    // Set input blocks to the input message
    _input_blocks = (uint8_t*)message_in;
    _input_allocated = 0;
}

bool Codec::AllocateInput()
{
    CAT_IF_DUMP(cout << endl << "---- AllocateInput ----" << endl << endl;)

    const uint64_t sizeBytes = static_cast<uint64_t>(_block_count + _extra_count) * _block_bytes;

    // If need to allocate more:
    if (_input_allocated < sizeBytes)
    {
        FreeInput();

        // Allocate input blocks
        _input_blocks = SIMDSafeAllocate((size_t)sizeBytes);
        if (!_input_blocks) {
            return false;
        }

        _input_allocated = sizeBytes;
    }

    return true;
}

void Codec::FreeInput()
{
    if (_input_allocated > 0 &&
        _input_blocks != nullptr)
    {
        SIMDSafeFree(_input_blocks);
        _input_blocks = nullptr;
    }

    _input_allocated = 0;
}

bool Codec::AllocateMatrix()
{
    CAT_IF_DUMP(cout << endl << "---- AllocateMatrix ----" << endl << endl;)

    // GE matrix
    const unsigned ge_cols = _defer_count + _mix_count;
    const unsigned ge_rows = _defer_count + _dense_count + _extra_count + 1; // One extra for workspace
    const unsigned ge_pitch = (ge_cols + 63) / 64;
    const unsigned ge_matrix_words = ge_rows * ge_pitch;

    // Compression matrix
    const unsigned compress_rows = _block_count;
    const unsigned compress_matrix_words = compress_rows * ge_pitch;

    // Pivots
    const unsigned pivot_count = ge_cols + _extra_count;
    const unsigned pivot_words = pivot_count * 2 + ge_cols;

    // Heavy
    const unsigned heavy_rows = kHeavyRows + _extra_count;
    const unsigned heavy_cols = _mix_count < kHeavyCols ? _mix_count : kHeavyCols;
    const unsigned heavy_pitch = (heavy_cols + 3 + 3) & ~3; // Round up columns+3 to next multiple of 4
    const unsigned heavy_bytes = heavy_pitch * heavy_rows;

    // Calculate buffer size
    const uint64_t sizeBytes = \
        compress_matrix_words * sizeof(uint64_t)
        + ge_matrix_words * sizeof(uint64_t)
        + heavy_bytes
        + pivot_words * sizeof(uint16_t);

    // If need to allocate more:
    if (_ge_allocated < sizeBytes)
    {
        FreeMatrix();

        uint8_t * GF256_RESTRICT matrix = SIMDSafeAllocate((size_t)sizeBytes);
        if (!matrix) {
            return false;
        }

        _ge_allocated = sizeBytes;
        _compress_matrix = reinterpret_cast<uint64_t *>( matrix );
    }

    // Store pointers
    _ge_pitch = ge_pitch;
    _ge_rows = ge_rows;
    _ge_cols = ge_cols;
    _ge_matrix = _compress_matrix + compress_matrix_words;
    _heavy_pitch = heavy_pitch;
    _heavy_rows = heavy_rows;
    _heavy_columns = heavy_cols;
    _first_heavy_column = _defer_count + _mix_count - heavy_cols;
    _heavy_matrix = reinterpret_cast<uint8_t *>( _ge_matrix + ge_matrix_words );
    _pivots = reinterpret_cast<uint16_t *>( _heavy_matrix + heavy_bytes );
    _ge_row_map = _pivots + pivot_count;
    _ge_col_map = _ge_row_map + pivot_count;

    CAT_IF_DUMP(cout << "GE matrix is " << ge_rows << " x " << ge_cols
        << " with pitch " << ge_pitch << " consuming "
        << ge_matrix_words * sizeof(uint64_t) << " bytes" << endl;)
    CAT_IF_DUMP(cout << "Compress matrix is " << compress_rows
        << " x " << ge_cols << " with pitch " << ge_pitch
        << " consuming " << compress_matrix_words * sizeof(uint64_t)
        << " bytes" << endl;)
    CAT_IF_DUMP(cout << "Allocated " << pivot_count
        << " pivots, consuming " << pivot_words*2 << " bytes" << endl;)
    CAT_IF_DUMP(cout << "Allocated " << kHeavyRows
        << " heavy rows, consuming " << heavy_bytes << " bytes" << endl;)

    // Clear entire Compression matrix
    memset(_compress_matrix, 0, compress_matrix_words * sizeof(uint64_t));

    // Clear entire GE matrix.
    // This clears ge_cols not ge_rows because we just need to clear the upper
    // square matrix, whereas ge_rows also includes some extra rows for data
    // received in excess of N for when the decoder fails and has to resume
    // again.
    // When these extra rows are added we clear the row memory at that point.
    memset(_ge_matrix, 0, ge_cols * ge_pitch * sizeof(uint64_t));

    return true;
}

void Codec::FreeMatrix()
{
    // If memory was allocated:
    if (_compress_matrix != nullptr)
    {
        // Free it now
        SIMDSafeFree(_compress_matrix);
        _compress_matrix = nullptr;
    }

    _ge_allocated = 0;
}

bool Codec::AllocateWorkspace()
{
    CAT_IF_DUMP(cout << endl << "---- AllocateWorkspace ----" << endl << endl;)

    // +1 for temporary space for MultiplyDenseValues()
    const unsigned recovery_rows = _block_count + _mix_count + 1;
    const uint64_t recoverySizeBytes = static_cast<uint64_t>(recovery_rows) * _block_bytes;

    // Count needed rows and columns
    const uint32_t row_count = _block_count + _extra_count;
    const uint32_t column_count = _block_count;

    // Calculate size
    const uint64_t sizeBytes = recoverySizeBytes
        + sizeof(PeelRow) * row_count
        + sizeof(PeelColumn) * column_count
        + sizeof(PeelRefs) * column_count
        + row_count;

    if (_workspace_allocated < sizeBytes)
    {
        FreeWorkspace();

        // Allocate workspace
        _recovery_blocks = SIMDSafeAllocate((size_t)sizeBytes);
        if (!_recovery_blocks) {
            return false;
        }
        _workspace_allocated = sizeBytes;
    }

    // Set pointers
    _peel_rows = reinterpret_cast<PeelRow *>( _recovery_blocks + recoverySizeBytes );
    _peel_cols = reinterpret_cast<PeelColumn *>( _peel_rows + row_count );
    _peel_col_refs = reinterpret_cast<PeelRefs *>( _peel_cols + column_count );
    _copied_original = reinterpret_cast<uint8_t *>( _peel_col_refs + column_count );

    _recovery_rows = recovery_rows;

    CAT_IF_DUMP(cout << "Memory overhead for workspace = " << sizeBytes << " bytes" << endl;)

    // Initialize columns
    for (unsigned ii = 0; ii < _block_count; ++ii)
    {
        _peel_col_refs[ii].RowCount = 0;
        _peel_cols[ii].Weight2Refs = 0;
        _peel_cols[ii].Mark = MARK_TODO;
    }

    return true;
}

void Codec::FreeWorkspace()
{
    if (_recovery_blocks != nullptr)
    {
        SIMDSafeFree(_recovery_blocks);
        _recovery_blocks = nullptr;
    }

    _workspace_allocated = 0;
}


//// Diagnostic

#if defined(CAT_DUMP_CODEC_DEBUG) || defined(CAT_DUMP_GE_MATRIX)

void Codec::PrintGEMatrix()
{
    const unsigned rows = _dense_count + _defer_count;
    const unsigned cols = _defer_count + _mix_count;

    cout << endl << "GE matrix is " << rows << " x " << cols << ":" << endl;

    for (unsigned ii = 0; ii < rows; ++ii)
    {
        for (unsigned jj = 0; jj < cols; ++jj)
        {
            if (_ge_matrix[_ge_pitch * ii + (jj >> 6)] & ((uint64_t)1 << (jj & 63)))
                cout << '1';
            else
                cout << '0';
        }
        cout << endl;
    }
    cout << endl;

    const unsigned heavy_rows = kHeavyRows;
    const unsigned heavy_cols = _heavy_columns;
    cout << "Heavy submatrix is " << heavy_rows << " x " << heavy_cols << ":" << endl;

    for (unsigned ii = 0; ii < heavy_rows; ++ii)
    {
        for (unsigned jj = 0; jj < heavy_cols; ++jj)
        {
            cout << hex << setw(2) << setfill('0') << (unsigned)_heavy_matrix[_heavy_pitch * (ii + _extra_count) + jj] << dec << " ";
        }
        cout << endl;
    }
    cout << endl;
}

void Codec::PrintExtraMatrix()
{
    cout << endl << "Extra rows: " << endl;

    // For each pivot,
    unsigned extra_count = 0;
    const uint16_t column_count = _defer_count + _mix_count;
    const uint16_t first_heavy_row = _defer_count + _dense_count;
    for (uint16_t pivot_i = 0; pivot_i < _pivot_count; ++pivot_i)
    {
        // If row is extra,
        uint16_t ge_row_i = _pivots[pivot_i];
        if (ge_row_i >= first_heavy_row && ge_row_i < first_heavy_row + _extra_count)
        {
            const uint64_t * GF256_RESTRICT ge_row = _ge_matrix + _ge_pitch * ge_row_i;
            CAT_DEBUG_ASSERT(ge_row_i >= first_heavy_row);
            const uint16_t heavy_row_i = ge_row_i - first_heavy_row;
            const uint8_t * GF256_RESTRICT heavy_row = _heavy_matrix + _heavy_pitch * heavy_row_i;

            cout << "row=" << ge_row_i << " : light={ ";

            // For each non-heavy column,
            for (uint16_t ge_column_i = 0; ge_column_i < _first_heavy_column; ++ge_column_i)
            {
                // If column is non-zero,
                const uint64_t ge_mask = (uint64_t)1 << (ge_column_i & 63);
                const bool nonzero = 0 != (ge_row[ge_column_i >> 6] & ge_mask);
                const char ch = '0' + (char)nonzero; // bool is 0 when false, 1 when true
                cout << ch;
            }

            cout << " } heavy=(";

            // For each heavy column,
            for (unsigned ge_column_i = _first_heavy_column; ge_column_i < column_count; ++ge_column_i)
            {
                CAT_DEBUG_ASSERT(ge_column_i >= _first_heavy_column);
                const unsigned heavy_col_i = ge_column_i - _first_heavy_column;
                CAT_DEBUG_ASSERT(heavy_col_i < _heavy_columns);
                const uint8_t code_value = heavy_row[heavy_col_i];

                cout << " " << hex << setfill('0') << setw(2) << (unsigned)code_value << dec;
            }

            cout << " )" << endl;

            ++extra_count;
        }
    }

    cout << "Number of extra rows is " << extra_count << endl << endl;
}

void Codec::PrintCompressMatrix()
{
    const unsigned rows = _block_count;
    const unsigned cols = _defer_count + _mix_count;

    cout << endl << "Compress matrix is " << rows << " x " << cols << ":" << endl;

    for (unsigned ii = 0; ii < rows; ++ii)
    {
        for (unsigned jj = 0; jj < cols; ++jj)
        {
            const uint64_t mask = (uint64_t)1 << (jj & 63);
            const uint64_t word = _compress_matrix[_ge_pitch * ii + (jj >> 6)];
            const bool nonzero = 0 != (word & mask);
            const char ch = '0' + nonzero; // bool is 0 when false, 1 when true
            cout << ch;
        }
        cout << endl;
    }

    cout << endl;
}

void Codec::PrintPeeled()
{
    cout << "Peeled elements :";

    uint16_t row_i = _peel_head_rows;
    while (row_i != LIST_TERM)
    {
        const PeelRow *row = &_peel_rows[row_i];

        cout << " " << row_i << "x" << row->Marks.Result.PeelColumn;

        row_i = row->NextRow;
    }

    cout << endl;
}

void Codec::PrintDeferredRows()
{
    cout << "Deferred rows :";

    uint16_t row_i = _defer_head_rows;
    while (row_i != LIST_TERM)
    {
        const PeelRow *row = &_peel_rows[row_i];

        cout << " " << row_i;

        row_i = row->NextRow;
    }

    cout << endl;
}

void Codec::PrintDeferredColumns()
{
    cout << "Deferred columns :";

    uint16_t column_i = _defer_head_columns;
    while (column_i != LIST_TERM)
    {
        const PeelColumn *column = &_peel_cols[column_i];

        cout << " " << column_i;

        column_i = column->Next;
    }

    cout << endl;
}

#endif // CAT_DUMP_CODEC_DEBUG


//// Encoder Mode

WirehairResult Codec::InitializeEncoder(
    uint64_t message_bytes,
    unsigned block_bytes)
{
    WirehairResult result = ChooseMatrix(message_bytes, block_bytes);

    if (result == Wirehair_Success)
    {
        // Calculate partial final bytes
        unsigned partial_final_bytes = message_bytes % _block_bytes;
        if (partial_final_bytes <= 0) {
            partial_final_bytes = _block_bytes;
        }

        // Encoder-specific
        _input_final_bytes = partial_final_bytes;
        _output_final_bytes = _block_bytes;
        _extra_count = 0;
        _original_out_of_order = false;

        if (!AllocateWorkspace()) {
            result = Wirehair_OOM;
        }
    }

    return result;
}

WirehairResult Codec::EncodeFeed(const void * GF256_RESTRICT message_in)
{
    CAT_IF_DUMP(cout << endl << "---- EncodeFeed ----" << endl << endl;)

    // Validate input
    if (message_in == nullptr) {
        return Wirehair_InvalidInput;
    }

    SetInput(message_in);

    // For each input row:
    for (uint16_t id = 0; id < _block_count; ++id) {
        if (!OpportunisticPeeling(id, id)) {
            return Wirehair_BadPeelSeed;
        }
    }

    // Solve matrix and generate recovery blocks
    WirehairResult result = SolveMatrix();

    if (result == Wirehair_Success) {
        GenerateRecoveryBlocks();
        return Wirehair_Success;
    }
    else if (result == Wirehair_NeedMore) {
        return Wirehair_BadPeelSeed;
    }
    else {
        return result;
    }
}

uint32_t Codec::Encode(
    const uint32_t block_id, ///< Block id to generate
    void * GF256_RESTRICT block_out, ///< Block data output
    uint32_t out_buffer_bytes ///< Bytes in block
)
{
    if (!block_out) {
        return 0;
    }

    unsigned copyBytes;

    // If this is the last block:
    if ((uint16_t)block_id == _block_count - 1) {
        copyBytes = _input_final_bytes;
    }
    else {
        copyBytes = _block_bytes;
    }

    // If not enough space in output buffer:
    if (out_buffer_bytes < copyBytes) {
        return 0;
    }

    uint8_t * GF256_RESTRICT data_out = reinterpret_cast<uint8_t *>( block_out );

#if defined(CAT_COPY_FIRST_N)
    // Note that if the encoder was a decoder the original message blocks may
    // not be available, so we regenerate them each time using the recovery set.
    // This means that wirehair_encode() after conversion will be slow for the
    // set of original blocks.  I noted this in the API comments to help devs.

    // For the original message blocks (id < N):
    if (block_id < _block_count &&
        !_original_out_of_order)
    {
        const uint8_t * GF256_RESTRICT src = _input_blocks + _block_bytes * block_id;

        // Copy from the original file data
        memcpy(data_out, src, copyBytes);
        return copyBytes;
    }
#endif // CAT_COPY_FIRST_N

    CAT_IF_DUMP(cout << "Encode: Generating row " << block_id << ":";)

    PeelRowParameters params;
    params.Initialize(block_id, _p_seed, _block_count, _mix_count);

    PeelRowIterator iter(params, _block_count, _block_next_prime);
    const RowMixIterator mix(params, _mix_count, _mix_next_prime);

    const uint16_t peel_0 = iter.GetColumn();

    // Remember first column (there is always at least one)
    CAT_DEBUG_ASSERT(peel_0 < _recovery_rows);
    const uint8_t * GF256_RESTRICT first = _recovery_blocks + _block_bytes * peel_0;

    CAT_IF_DUMP(cout << " " << peel_0;)

    CAT_DEBUG_ASSERT((unsigned)(_block_count + mix.Columns[0]) < _recovery_rows);
    const uint8_t * GF256_RESTRICT mix0_src = _recovery_blocks + _block_bytes * (_block_count + mix.Columns[0]);

    // If peeler has multiple columns:
    if (iter.Iterate())
    {
        const uint16_t peel_1 = iter.GetColumn();

        CAT_IF_DUMP(cout << " " << peel_1;)

        CAT_DEBUG_ASSERT(peel_1 < _recovery_rows);

        // Combine first two columns into output buffer (faster than memcpy + memxor)
        gf256_addset_mem(
            data_out,
            first,
            _recovery_blocks + _block_bytes * peel_1,
            copyBytes);

        // For each remaining peeler column:
        while (iter.Iterate())
        {
            const uint16_t peel_x = iter.GetColumn();

            CAT_IF_DUMP(cout << " " << peel_x;)

            CAT_DEBUG_ASSERT(peel_x < _recovery_rows);

            // Mix in each column
            gf256_add_mem(
                data_out,
                _recovery_blocks + _block_bytes * peel_x,
                copyBytes);
        }

        CAT_DEBUG_ASSERT((unsigned)(_block_count + mix.Columns[0]) < _recovery_rows);

        // Mix first mixer block in directly
        gf256_add_mem(data_out, mix0_src, copyBytes);
    }
    else
    {
        CAT_DEBUG_ASSERT((unsigned)(_block_count + mix.Columns[0]) < _recovery_rows);

        // Mix first with first mixer block (faster than memcpy + memxor)
        gf256_addset_mem(data_out, first, mix0_src, copyBytes);
    }

    CAT_IF_DUMP(cout << " " << (_block_count + mix.Columns[0]);)

    // Add in remaining 2 mixer columns:

    CAT_DEBUG_ASSERT((unsigned)(_block_count + mix.Columns[1]) < _recovery_rows);
    const uint8_t * GF256_RESTRICT mix1_src = _recovery_blocks + _block_bytes * (_block_count + mix.Columns[1]);
    CAT_IF_DUMP(cout << " " << (_block_count + mix.Columns[1]);)

    CAT_DEBUG_ASSERT((unsigned)(_block_count + mix.Columns[2]) < _recovery_rows);
    const uint8_t * GF256_RESTRICT mix2_src = _recovery_blocks + _block_bytes * (_block_count + mix.Columns[2]);
    CAT_IF_DUMP(cout << " " << (_block_count + mix.Columns[2]);)

    gf256_add2_mem(data_out, mix1_src, mix2_src, copyBytes);

    CAT_IF_DUMP(cout << endl;)

    return copyBytes;
}


//// Decoder Mode

WirehairResult Codec::InitializeDecoder(
    uint64_t message_bytes,
    uint32_t block_bytes)
{
    const WirehairResult result = ChooseMatrix(message_bytes, block_bytes);

    if (result != Wirehair_Success) {
        return result;
    }

    // Calculate partial final bytes
    unsigned partial_final_bytes = message_bytes % _block_bytes;
    if (partial_final_bytes <= 0) {
        partial_final_bytes = _block_bytes;
    }

    // Decoder-specific
    _row_count = 0;
    _output_final_bytes = partial_final_bytes;

    // Hack: Prevents row-based ids from causing partial copies when they
    // happen to be the last block id.  This is only an issue because the
    // shared codec source code happens to be built with more encoder-like
    // semantics
    _input_final_bytes = _block_bytes;

    _extra_count = CAT_MAX_EXTRA_ROWS;
#if defined(CAT_ALL_ORIGINAL)
    _all_original = true;
#endif
    _original_out_of_order = true;

    if (!AllocateInput()) {
        CAT_DEBUG_BREAK();
        return Wirehair_OOM;
    }

    if (!AllocateWorkspace()) {
        CAT_DEBUG_BREAK();
        return Wirehair_OOM;
    }

    return result;
}

WirehairResult Codec::InitializeEncoderFromDecoder()
{
    CAT_DEBUG_ASSERT(_row_count >= _block_count);

#if defined(CAT_ALL_ORIGINAL)
    // If all original data, return success (common case)
    if (_all_original)
    {
        // All of the original data was received, but perhaps not in the
        // original order.  When the original data was received by the
        // DecodeFeed() method, it called OpportunisticPeeling() for each
        // of the original data blocks already.  So the solver is already
        // set up, we just need to run it now to generate the recovery set.

        // Solve matrix and generate recovery blocks
        WirehairResult result = SolveMatrix();

        if (result != Wirehair_Success)
        {
            CAT_DEBUG_BREAK();
            if (result == Wirehair_NeedMore) {
                result = Wirehair_BadPeelSeed;
            }
            return result;
        }

        GenerateRecoveryBlocks();
    }
#endif

    // Set input final bytes to output final bytes
    _input_final_bytes = _output_final_bytes;

    return Wirehair_Success;
}

WirehairResult Codec::DecodeFeed(
    const uint32_t block_id,
    const void * GF256_RESTRICT block_in,
    const unsigned block_bytes)
{
    // Validate input
    if (!block_in) {
        return Wirehair_InvalidInput;
    }

    const bool isFinalBlock = ((block_id + 1) == (uint32_t)_block_count);

    // If this is the last block:
    if (isFinalBlock) {
        // Decoder uses _output_final_bytes to store partial bytes
        const uint32_t final_bytes = _output_final_bytes;

        // If the application did not provide enough bytes:
        if (final_bytes > block_bytes) {
            return Wirehair_InvalidInput;
        }
    }
    else {
        // If the application did not provide the right number of bytes:
        if (_block_bytes != block_bytes) {
            return Wirehair_InvalidInput;
        }
    }

#if defined(CAT_ALL_ORIGINAL)
    // If provided a block of non-original data, mark all original as false
    if (block_id >= _block_count) {
        _all_original = false;
    }
#endif

    const uint16_t row_i = _row_count;

    // If at least N rows stored:
    if (row_i >= _block_count)
    {
        // Resume GE from this row
        const WirehairResult result = ResumeSolveMatrix(block_id, block_in);

        if (result == Wirehair_Success) {
            GenerateRecoveryBlocks();
        }

        return result;
    }

    // If opportunistic peeling failed for this row:
    if (!OpportunisticPeeling(row_i, block_id))
    {
        // This means that there was not enough room in the sparse matrix
        // data structure to store the data in this row.  We will need to
        // wait for another row that references different columns in
        // order to continue
        return Wirehair_NeedMore;
    }

    uint8_t * GF256_RESTRICT dest = _input_blocks + _block_bytes * row_i;

    // If this is the last block id:
    if (isFinalBlock)
    {
        const uint32_t final_bytes = _output_final_bytes;

        // Copy the new row data into the input block area
        memcpy(dest, block_in, final_bytes);

        // Pad with zeros
        memset(dest + final_bytes, 0, _block_bytes - final_bytes);
    }
    else
    {
        // Copy the new row data into the input block area
        memcpy(dest, block_in, _block_bytes);
    }

    ++_row_count;
    CAT_DEBUG_ASSERT(_row_count <= _block_count);

    // If not enough blocks yet:
    if (_row_count != _block_count) {
        return Wirehair_NeedMore;
    }

#if defined(CAT_ALL_ORIGINAL)
    // If all original data, return success (common case)
    if (_all_original) {
        // Check input:
        if (!IsAllOriginalData()) {
            // Application violated precondition that all inputs must be unique
            CAT_DEBUG_BREAK();
            _all_original = false;
            return Wirehair_InvalidInput;
        }
        return Wirehair_Success;
    }
#endif

    // Attempt to solve the matrix
    const WirehairResult result = SolveMatrix();

    // If solve was successful (common):
    if (result == Wirehair_Success) {
        GenerateRecoveryBlocks();
    }

    return result;
}


} // namespace wirehair
