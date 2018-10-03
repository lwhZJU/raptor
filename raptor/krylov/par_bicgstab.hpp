#ifndef RAPTOR_KRYLOV_PAR_BICGSTAB_HPP
#define RAPTOR_KRYLOV_PAR_BICGSTAB_HPP

#include "core/types.hpp"
#include "core/par_matrix.hpp"
#include "core/par_vector.hpp"
#include "multilevel/par_multilevel.hpp"
#include "aggregation/par_smoothed_aggregation_solver.hpp"
#include <vector>

using namespace raptor;

void BiCGStab(ParCSRMatrix* A, ParVector& x, ParVector& b, aligned_vector<double>& res, double tol = 1e-05, int max_iter = -1);

void Pre_BiCGStab(ParCSRMatrix* A, ParMultilevel *ml, ParVector& x, ParVector& b, aligned_vector<double>& res,
                  double tol = 1e-05, int max_iter = -1);

void SeqInner_BiCGStab(ParCSRMatrix* A, ParVector& x, ParVector& b, aligned_vector<double>& res, double tol = 1e-05, int max_iter = -1);

void SeqNorm_BiCGStab(ParCSRMatrix* A, ParVector& x, ParVector& b, aligned_vector<double>& res, double tol = 1e-05, int max_iter = -1);

void SeqInnerSeqNorm_BiCGStab(ParCSRMatrix* A, ParVector& x, ParVector& b, aligned_vector<double>& res, double tol = 1e-05, int max_iter = -1);

void PI_BiCGStab(ParCSRMatrix* A, ParVector& x, ParVector& b, aligned_vector<double>& res, MPI_Comm &inner_comm,
                 MPI_Comm &root_comm, double frac, int inner_color, int root_color, int inner_root, int procs_in_group,
                 int part_global, double tol = 1e-05, int max_iter = -1);

void PrePI_BiCGStab(ParCSRMatrix* A, ParMultilevel* ml, ParVector& x, ParVector& b, aligned_vector<double>& res,
                 aligned_vector<double>& sAs_inner_list, aligned_vector<double>& AsAs_inner_list, MPI_Comm &inner_comm,
                 MPI_Comm &root_comm, double frac, int inner_color, int root_color, int inner_root, int procs_in_group,
                 int part_global, double tol = 1e-05, int max_iter = -1);
#endif
