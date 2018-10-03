// Copyright (c) 2015, Raptor Developer Team, University of Illinois at Urbana-Champaign
// License: Simplified BSD, http://opensource.org/licenses/BSD-2-Clause
#include "krylov/partial_inner.hpp"

using namespace raptor;

/********************************************************************** 
 Performs approximate inner product using contiguous half of processes
 *********************************************************************/
data_t half_inner_contig(ParVector &x, ParVector &y, int half, int part_global){
    /* x : ParVector for calculating inner product
     * y : ParVector for calculating inner product
     * half : which half of processes to use in inner product
     *    0 : first half
     *    1 : second half
     * part_global : number of values being used in inner product
     */

    int rank, num_procs, comm_rank, inner_root, recv_root, color;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    data_t inner_prod = 0.0;

    // Check if single process
    if (num_procs <= 1){
	if (x.local_n != y.local_n){
            printf("Error. Dimensions do not match.\n");
	    exit(-1);
	}
	inner_prod = x.local.inner_product(y.local);
	return inner_prod;
    }

    // Make sure that half_procs is even or adjust
    int half_procs = num_procs/2;
    if (num_procs % 2 != 0) half_procs++;

    // Using color to determine which processes communicate
    // for the partial inner product
    // 0: participates in the inner product
    // 1: does not participate in the inner product
    if (half){
        if (rank >= half_procs) color = 0;
	else color = 1;
	inner_root = half_procs;
	recv_root = 0;
    }
    else{	
	if (rank < half_procs) color = 0;
	else color = 1;
        inner_root = 0;
	recv_root = half_procs;
    }

    // Communicator for Inner Product and Communicator for Receiving Half
    MPI_Comm inner_comm, recv_comm;
    int inner_comm_size, recv_comm_size;
    if (!(color)){
	MPI_Comm_split(MPI_COMM_WORLD, color, rank, &inner_comm);
        MPI_Comm_size(inner_comm, &inner_comm_size);
    }
    else{
	MPI_Comm_split(MPI_COMM_WORLD, color, rank, &recv_comm);
        MPI_Comm_size(recv_comm, &recv_comm_size);
    }

    if (x.local_n != y.local_n){
        printf("Error. Dimensions do not match.\n");
	exit(-1);
    }

    // Perform Inner Product on Half
    if (!(color)){
        if (x.local_n){
            inner_prod = x.local.inner_product(y.local);
        }
        if (inner_comm_size > 1) MPI_Allreduce(MPI_IN_PLACE, &inner_prod, 1, MPI_DATA_T, MPI_SUM, inner_comm);
    }

    //printf("rank %d inner_prod %lg\n", rank, inner_prod);
    // Send partial inner product from used partition to a single process
    if (rank == inner_root) MPI_Send(&inner_prod, 1, MPI_DATA_T, recv_root, 1, MPI_COMM_WORLD);
    if (rank == recv_root) MPI_Recv(&inner_prod, 1, MPI_DATA_T, inner_root, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Broadcast for Receiving Half
    if (color && recv_comm_size > 1){
        MPI_Bcast(&inner_prod, 1, MPI_DATA_T, 0, recv_comm);
    }

    // Delete communicators
    if (!(color)) MPI_Comm_free(&inner_comm);
    else MPI_Comm_free(&recv_comm);

    // Return partial inner_prod scaled by global percentage
    return ((1.0*x.global_n)/part_global) * inner_prod;
}


/***************************************************************** 
 Performs sequential inner product for testing reproducibility 
 ****************************************************************/
data_t sequential_inner(ParVector &x, ParVector &y){
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    data_t inner_prod = 0.0;

    // Check if single process
    if (num_procs <= 1){
	if (x.local_n != y.local_n){
            printf("Error. Dimensions do not match.\n");
	    exit(-1);
	}
	inner_prod = x.local.inner_product(y.local);
	return inner_prod;
    }

    if (rank > 1)
    {
        MPI_Recv(&inner_prod, 1, MPI_DATA_T, rank-1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    for(int i=0; i<x.local_n; i++){
        inner_prod += x.local[i] * y.local[i];
    }

    if (rank < num_procs-1)
    {
        MPI_Send(&inner_prod, 1, MPI_DATA_T, rank+1, 1, MPI_COMM_WORLD);
    }

    MPI_Bcast(&inner_prod, 1, MPI_DATA_T, num_procs-1, MPI_COMM_WORLD);

    return inner_prod;
}


/***************************************************************** 
 Performs sequential vector norm with parallel vector 
 ****************************************************************/
data_t sequential_norm(ParVector &x, index_t p){
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    data_t norm;

    norm = sequential_inner(x, x);

    return pow(norm, 1./p);
}


void create_partial_inner_comm(MPI_Comm &inner_comm, MPI_Comm &root_comm, double frac, ParVector &x, int &my_inner_color,
                               int &my_root_color, int &inner_root, int &procs_in_group, int &part_global)
{
    /*     inner_comm : MPI communicator containing the processes performing the partial inner product
     *      root_comm : MPI communicator containing the root process for each inner_comm group
     *           frac : fraction of processes to use in inner product calculation
     *              x : ParVector to create communicators for
     * my_inner_color : corresponds to the group of processes included in each inner product
     *  my_root_color : corresponds to processes in the root communicator
     *     inner_root : root for each processes' inner_comm
     * procs_in_group : average number of processes in each group - rounded up if uneven
     *    part_global : number of values being used in the inner product
     */

    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    if (num_procs <= 1) 
    {
       inner_comm = MPI_COMM_NULL;
       root_comm = MPI_COMM_NULL;
       part_global = x.local_n;
       return; 
    }

    int inner_comm_size, recv_comm_size;

    // Calculate number of processes in group
    procs_in_group = num_procs * frac; 
    if (num_procs % 2) procs_in_group++;

    // Split into contigous groups by color and get root for each group
    my_inner_color = rank / procs_in_group;
    inner_root = my_inner_color * procs_in_group;
    if (rank == inner_root) my_root_color = 0;
    else my_root_color = 1;

    // Split processes into communicators for inner products
    MPI_Comm_split(MPI_COMM_WORLD, my_inner_color, rank, &inner_comm);

    // Split processes into root communicator
    MPI_Comm_split(MPI_COMM_WORLD, my_root_color, rank, &root_comm);

    // Get number of values being used in the inner product calculation
    part_global = x.local_n;
    MPI_Allreduce(MPI_IN_PLACE, &part_global, 1, MPI_INT, MPI_SUM, inner_comm);

    return;
}

/***************************************************************** 
 Performs approximate inner product using half of the processes
 ****************************************************************/
data_t half_inner(MPI_Comm &inner_comm, ParVector &x, ParVector &y) {
    /*  inner_comm : MPI communicator containing the processes to use in the inner product 
     *           x : ParVector for calculating inner product
     *           y : ParVector for calculating inner product
     */

    int rank, num_procs, inner_comm_size, comm_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    MPI_Request req;
    MPI_Status stat;

    data_t inner_prod = 0.0;

    // Check if single process
    if (num_procs <= 1 || inner_comm == MPI_COMM_NULL){
        if (x.local_n != y.local_n){
                printf("Error. Dimensions do not match.\n");
            exit(-1);
        }
        inner_prod = x.local.inner_product(y.local);
        return inner_prod;
    }
    
    // Get communicator sizes
    MPI_Comm_size(inner_comm, &inner_comm_size);
    //MPI_Comm_size(recv_comm, &recv_comm_size);

    if (x.local_n != y.local_n){
        printf("Error. Dimensions do not match.\n");
	exit(-1);
    }

    // Perform your half of inner product
    if (x.local_n) inner_prod = x.local.inner_product(y.local);
    if (inner_comm_size > 1) MPI_Allreduce(MPI_IN_PLACE, &inner_prod, 1, MPI_DATA_T, MPI_SUM, inner_comm);

    // Return partial inner_prod plus old half 
    return inner_prod;
}

data_t half_inner_communicate(MPI_Comm &inner_comm, data_t my_half, int my_root, int other_root) {
    /* inner_comm : MPI communicator containing the processes used in my inner product portion
     *    my_half : my computed portion of the inner product to be communicated
     *    my_root : rank corresponding to root of each procs inner_comm
     * other_root : rank corresponding to root of other inner_comm
     */

     int rank, num_procs, inner_comm_size, comm_rank;
     MPI_Comm_rank(MPI_COMM_WORLD, &rank);
     MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
     MPI_Request req;
     MPI_Status stat;

     // Holds non-computed half of inner product
     data_t other_half;

     // Exchange old halves between inner_comm roots
     if (rank == my_root) {
         MPI_Isend(&my_half, 1, MPI_DATA_T, other_root, 1, MPI_COMM_WORLD, &req);
         MPI_Irecv(&other_half, 1, MPI_DATA_T, other_root, 1, MPI_COMM_WORLD, &req);
         MPI_Wait(&req, &stat);
     }

     // Broadcast old inner_prod to each inner_comm
     MPI_Bcast(&other_half, 1, MPI_DATA_T, 0, inner_comm);

     return other_half;
}

data_t partial_inner_communicate(MPI_Comm &inner_comm, MPI_Comm &root_comm, data_t my_inner, int my_index,
                                 std::vector<int> &roots, bool am_root) {
    /* inner_comm : MPI communicator containing the processes used in my inner product portion
     *  root_comm : MPI communicator containing the root processes for all inner_comm communicators
     *   my_inner : my computed portion of the inner product to be communicated
     *   my_index : index of my root in roots
     *      roots : std::vector containing the ranks of the root processes for each inner_comm
     *    am_root : bool that tells whether a process is a root for an inner_comm 
     */

     int rank, num_procs, inner_comm_size, comm_rank;
     MPI_Comm_rank(MPI_COMM_WORLD, &rank);
     MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
     MPI_Request req;
     MPI_Status stat;

     // Contains old off process portion of inner product
     data_t old_part;

     // Holds non-computed half of inner product
     std::vector<data_t> old_buffer(roots.size());

     // Set your inner_comm value in buffer before broadcasts
     old_buffer[my_index] = my_inner;    
 
     // Exchange old halves between inner_comm roots
     for (int i = 0; i < roots.size(); i++){
         if( am_root ) MPI_Bcast(&(old_buffer[i]), 1, MPI_DATA_T, i, root_comm);
     }

     // Broadcast buffer to each inner_comm
     MPI_Bcast(&(old_buffer[0]), old_buffer.size(), MPI_DATA_T, 0, inner_comm);

     // Each process calculates the sum of old_buffer
     for (int i = 0; i < old_buffer.size(); i++) {
         if ( i != my_index) old_part += old_buffer[i];
     }

     return old_part;
}

void create_partial_inner_comm_v2(MPI_Comm &inner_comm, MPI_Comm &root_comm, double frac, ParVector &x, int &my_index,
                                  std::vector<int> &roots, bool &am_root)
{
    /*     inner_comm : MPI communicator containing the processes performing the partial inner product
     *      root_comm : MPI communicator containing the root process for each inner_comm group
     *           frac : fraction of processes to use in inner product calculation
     *              x : ParVector to create communicators for
     *       my_index : index of my root in roots
     *          roots : std::vector containing the ranks of the root processes for each inner_comm
     *        am_root : bool that tells whether a process is a root for an inner_comm 
     */

    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    if (num_procs <= 1) 
    {
       inner_comm = MPI_COMM_NULL;
       root_comm = MPI_COMM_NULL;
       return; 
    }

    int my_root_color;
    // Number of groups to iterate through for partial inner products
    int groups = 1 / frac;

    // Calculate number of processes in group
    int procs_in_group = num_procs * frac;
    int extra = num_procs % groups;
 
    roots.resize(groups);
   
    int root_to_add = 0;
    // Create list of roots for other inner_comm communicators 
    for (int i = 0; i < groups; i++) {
        if (rank >= root_to_add) my_index = i;
        roots[i] = root_to_add;
        root_to_add += procs_in_group;
        if (i < extra) root_to_add++; 
    }

    int inner_root = roots[my_index];
    // Split into contigous groups by color and get root for each group
    if (rank == inner_root) {
        my_root_color = 0;
        am_root = true;
    }
    else {
        my_root_color = 1;
        am_root = false;
    }

    // Split processes into communicators for inner products
    MPI_Comm_split(MPI_COMM_WORLD, my_index, rank, &inner_comm);

    // Split processes into root communicator
    MPI_Comm_split(MPI_COMM_WORLD, my_root_color, rank, &root_comm);

    return;
}

/***************************************************************** 
 Performs approximate inner product using part of the processes
 ****************************************************************/
data_t partial_inner(MPI_Comm &inner_comm, MPI_Comm &root_comm, ParVector &x, ParVector &y, int my_color,
                     int send_color, int inner_root, int procs_in_group, int part_global) {
    /*     inner_comm : MPI communicator containing the processes to use in the inner product 
     *      root_comm : MPI communicator containing the root processes for each inner_comm communicator
     *              x : ParVector for calculating inner product
     *              y : ParVector for calculating inner product
     *       my_color : color corresponding to the processes' communicator 
     *     send_color : color calculating the inner product and sending to the other processes
     *     inner_root : root of the processes' inner_comm
     * procs_in_group : average number of processes in group - rounded up if uneven
     *    part_global : number of values being used in inner product for scaling inner product accordingly
     */

    int rank, num_procs, inner_comm_size, comm_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    MPI_Request req;
    MPI_Status stat;

    data_t inner_prod = 0.0;

    // Check if single process or inner_comm undefined - then default to full inner product
    if (num_procs <= 1 || inner_comm == MPI_COMM_NULL){
        return y.inner_product(x); 
    }
    
    // Get communicator sizes
    MPI_Comm_size(inner_comm, &inner_comm_size);

    if (x.local_n != y.local_n){
        printf("Error. Dimensions do not match.\n");
	    exit(-1);
    }

    // Perform partial inner product - if proc belongs to inner_comm
    // and corresponds to the half performing the inner product this iteration
    if (my_color == send_color){
        if (x.local_n){
            inner_prod = x.local.inner_product(y.local);
        }
        if (inner_comm_size > 1) MPI_Allreduce(MPI_IN_PLACE, &inner_prod, 1, MPI_DATA_T, MPI_SUM, inner_comm);
        // Scale inner product by global percentage before sending
        inner_prod = ((1.0*x.global_n)/part_global) * inner_prod;
    }

    // Broadcast inner product to group roots
    if (rank == inner_root) {
        MPI_Bcast(&inner_prod, 1, MPI_DATA_T, send_color, root_comm);
    }

    // Broadcast from group roots to each group
    if (my_color != send_color){
        if (inner_comm_size > 1) MPI_Bcast(&inner_prod, 1, MPI_DATA_T, 0, inner_comm);
    }

    // Return partial inner_prod scaled by global percentage
    return inner_prod;
}
