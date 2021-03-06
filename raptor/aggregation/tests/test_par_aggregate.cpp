// Copyright (c) 2015-2017, RAPtor Developer Team
// License: Simplified BSD, http://opensource.org/licenses/BSD-2-Clause

#include "gtest/gtest.h"
#include "mpi.h"
#include "gallery/stencil.hpp"
#include "core/types.hpp"
#include "core/par_matrix.hpp"
#include "gallery/par_matrix_IO.hpp"
#include "aggregation/par_aggregate.hpp"
#include <iostream>
#include <fstream>

using namespace raptor;

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    ::testing::InitGoogleTest(&argc, argv);
    int temp = RUN_ALL_TESTS();
    MPI_Finalize();
    return temp;
} // end of main() //

TEST(TestParAggregate, TestsInAggregation)
{ 
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    FILE* f;
    aligned_vector<int> states;
    aligned_vector<int> off_proc_states;

    ParCSRMatrix* A;
    ParCSRMatrix* S;

    const char* A0_fn = "../../../../test_data/sas_A0.pm";
    const char* S0_fn = "../../../../test_data/sas_S0.pm";
    const char* agg0_fn = "../../../../test_data/sas_agg0.txt";
    const char* weights_fn = "../../../../test_data/weights.txt";

    A = readParMatrix(A0_fn);
    S = readParMatrix(S0_fn);

    aligned_vector<double> weights(S->local_num_rows);
    f = fopen(weights_fn, "r");
    for (int i = 0; i < S->partition->first_local_row; i++)
    {
        fscanf(f, "%lf\n", &weights[0]);
    }
    for (int i = 0; i < S->local_num_rows; i++)
    {
        fscanf(f, "%lf\n", &weights[i]);
    }
    fclose(f);

    mis2(S, states, off_proc_states, false, weights.data());

    aligned_vector<int> py_aggregates(S->local_num_rows);
    f = fopen(agg0_fn, "r");
    for (int i = 0; i < S->partition->first_local_row; i++)
    {
        fscanf(f, "%d\n", &py_aggregates[0]);
    }
    for (int i = 0; i < S->local_num_rows; i++)
    {
        fscanf(f, "%d\n", &py_aggregates[i]);
    }
    fclose(f);

    aligned_vector<int> aggregates;
    int n_aggs = aggregate(A, S, states, off_proc_states, aggregates, 
            false, weights.data());

    // Aggregates returns global indices of original global rows
    // Gather list of all aggregates, in order, holding original global cols
    aligned_vector<int> agg_sizes(num_procs);
    aligned_vector<int> agg_displ(num_procs+1);
    aligned_vector<int> agg_list;
    aligned_vector<int> total_agg_list;
    int global_col, local_col;
    MPI_Allgather(&n_aggs, 1, MPI_INT, agg_sizes.data(), 1, MPI_INT, MPI_COMM_WORLD);
    agg_displ[0] = 0;
    int first_n = 0;
    for (int i = 0; i < num_procs; i++)
    {
        agg_displ[i+1] = agg_displ[i] + agg_sizes[i];
        if (i < rank) first_n += agg_sizes[i];
    }
    int total_size = agg_displ[num_procs];
    for (int i = 0; i < S->local_num_rows; i++)
    {
        if (states[i] > 0)
            agg_list.push_back(A->local_row_map[i]);
    }
    total_agg_list.resize(total_size);
    MPI_Allgatherv(agg_list.data(), n_aggs, MPI_INT, total_agg_list.data(), 
            agg_sizes.data(), agg_displ.data(), MPI_INT, MPI_COMM_WORLD);

    // Map aggregates[i] (global_col) to original global col that
    // now holds py_aggregates[i]
    for (int i = 0; i < A->local_num_rows; i++)
    {
        global_col = aggregates[i];
        ASSERT_EQ(total_agg_list[py_aggregates[i]], global_col);
    }
    
    delete A;
    delete S;

} // end of TEST(TestParSplitting, TestsInRuge_Stuben) //



