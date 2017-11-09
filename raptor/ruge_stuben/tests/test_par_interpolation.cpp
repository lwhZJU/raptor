// EXPECT_EQ and ASSERT_EQ are macros
// EXPECT_EQ test execution and continues even if there is a failure
// ASSERT_EQ test execution and aborts if there is a failure
// The ASSERT_* variants abort the program execution if an assertion fails 
// while EXPECT_* variants continue with the run.


#include "gtest/gtest.h"
#include "core/types.hpp"
#include "core/par_matrix.hpp"
#include "gallery/par_matrix_IO.hpp"
#include "ruge_stuben/par_cf_splitting.hpp"
#include "ruge_stuben/par_interpolation.hpp"
#include "gallery/diffusion.hpp"
#include "gallery/par_stencil.hpp"
#include "tests/par_compare.hpp"
#include <iostream>
#include <fstream>

using namespace raptor;

ParCSRMatrix* form_Prap(ParCSRMatrix* A, ParCSRBoolMatrix* S, char* filename, int* first_row_ptr, int* first_col_ptr)
{
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    int first_row, first_col;
    FILE* f;
    ParCSRMatrix* P_rap;
    std::vector<int> proc_sizes(num_procs);
    std::vector<int> splitting;
    if (A->local_num_rows)
    {
        splitting.resize(A->local_num_rows);
    }
    MPI_Allgather(&A->local_num_rows, 1, MPI_INT, proc_sizes.data(), 1, MPI_INT,
            MPI_COMM_WORLD);
    first_row = 0;
    for (int i = 0; i < rank; i++)
    {
        first_row += proc_sizes[i];
    }
    f = fopen(filename, "r");
    int cf;
    for (int i = 0; i < first_row; i++)
    {
        fscanf(f, "%d\n", &cf);
    }
    for (int i = 0; i < A->local_num_rows; i++)
    {
        fscanf(f, "%d\n", &splitting[i]);
    }
    fclose(f);

    // Get off proc states
    S->comm->communicate(splitting.data());
    P_rap = direct_interpolation(A, S, splitting, S->comm->recv_data->int_buffer);
    MPI_Allgather(&P_rap->on_proc_num_cols, 1, MPI_INT, proc_sizes.data(), 1, 
                MPI_INT, MPI_COMM_WORLD);
    first_col = 0;
    for (int i = 0; i < rank; i++)
    {
        first_col += proc_sizes[i];
    }

    *first_row_ptr = first_row;
    *first_col_ptr = first_col;

    return P_rap;
}

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    ::testing::InitGoogleTest(&argc, argv);
    int temp = RUN_ALL_TESTS();
    MPI_Finalize();
    return temp;
} // end of main() //


TEST(TestParInterpolation, TestsInRuge_Stuben)
{ 
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    int first_row, first_col;


    ParCSRMatrix* A;
    ParCSRMatrix* S;
    ParCSRBoolMatrix* S_bool;
    ParCSRMatrix* P;
    ParCSRMatrix* P_rap;


    // TEST LEVEL 0
    A = readParMatrix((char *)"../../../../test_data/rss_A0.mtx", MPI_COMM_WORLD, 1, 1);
    S = readParMatrix((char *)"../../../../test_data/rss_S0.mtx", MPI_COMM_WORLD, 1, 1);
    S_bool = new ParCSRBoolMatrix(S);
    P_rap = form_Prap(A, S_bool, (char *) "../../../../test_data/rss_cf0", 
            &first_row, &first_col);
    P = readParMatrix((char *)"../../../../test_data/rss_P0.mtx", MPI_COMM_WORLD, 1, 0, 
        P_rap->local_num_rows, P_rap->on_proc_num_cols, first_row, first_col);
    compare(P, P_rap);
    delete P_rap;
    delete P;
    delete S_bool;
    delete S;
    delete A;

    // TEST LEVEL 1
    A = readParMatrix((char *)"../../../../test_data/rss_A1.mtx", MPI_COMM_WORLD, 1, 0);
    S = readParMatrix((char *)"../../../../test_data/rss_S1.mtx", MPI_COMM_WORLD, 1, 0);
    S_bool = new ParCSRBoolMatrix(S);
    P_rap = form_Prap(A, S_bool, (char *)"../../../../test_data/rss_cf1", 
            &first_row, &first_col);
    P = readParMatrix((char *)"../../../../test_data/rss_P1.mtx", MPI_COMM_WORLD, 1, 0, 
        P_rap->local_num_rows, P_rap->on_proc_num_cols, first_row, first_col);
    compare(P, P_rap);


    delete P_rap;
    delete P;
    delete S_bool;
    delete S;
    delete A;

} // end of TEST(TestParInterpolation, TestsInRuge_Stuben) //

