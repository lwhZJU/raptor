#include <core/types.hpp>
#include <core/matrix.hpp>

using namespace raptor;

int main(int argc, char* argv[])
{
    COOMatrix* A = new COOMatrix(25, 25, 1);
    A->add_value(0, 1, 1.0);
    A->add_value(21, 9, 2.0);
    A->add_value(1, 0, 1.0);
    A->add_value(5, 3, 2.0);
    A->add_value(0, 0, 1.0);
    A->add_value(7, 11, 1.0);
    A->add_value(0, 0, 0.5);

    COOMatrix* B = new COOMatrix(25, 5, 1);
    B->add_value(0, 0, 1);
    B->add_value(1, 0, 1);
    B->add_value(2, 0, 1);
    B->add_value(3, 0, 1);
    B->add_value(4, 1, 1);
    B->add_value(5, 1, 1);
    B->add_value(6, 1, 1);
    B->add_value(7, 1, 1);
    B->add_value(8, 1, 1);
    B->add_value(9, 1, 1);
    B->add_value(10, 2, 1);
    B->add_value(11, 2, 1);
    B->add_value(12, 2, 1);
    B->add_value(13, 2, 1);
    B->add_value(14, 2, 1);
    B->add_value(15, 2, 1);
    B->add_value(16, 3, 1);
    B->add_value(17, 3, 1);
    B->add_value(18, 3, 1);
    B->add_value(19, 3, 1);
    B->add_value(20, 4, 1);
    B->add_value(21, 4, 1);
    B->add_value(22, 4, 1);
    B->add_value(23, 4, 1);
    B->add_value(24, 4, 1);
    

    printf("Original COO Matrix:\n");
    A->print();

    printf("\nCOO converted to CSR Matrix:\n");
    CSRMatrix* Acsr = new CSRMatrix(A);
    Acsr->print();

    printf("\nCOO converted to CSC Matrix:\n");
    CSCMatrix* Acsc = new CSCMatrix(A);
    Acsc->print();

    printf("\nCSR converted to CSC Matrix: \n");
    CSCMatrix* Acsr_csc = new CSCMatrix(Acsr);
    Acsr_csc->print();

    printf("\nCSC converted to CSR Matrix: \n");
    CSRMatrix* Acsc_csr = new CSRMatrix(Acsc);
    Acsc_csr->print();

    printf("\nSorted COO Matrix:\n");
    A->sort();
    A->print();

    printf("\nSorted CSR Matrix:\n");
    Acsr->sort();
    Acsr->print();

    printf("\nSorted CSC Matrix:\n");
    Acsc->sort();
    Acsc->print();

    printf("\nCreate X, B and Initialize X = 1.0\n");                
    Vector x(A->n_cols);
    Vector b(A->n_rows);
    x.set_const_value(1.0);
   
    printf("\nVector B = A(COO) * X\n");        
    A->mult(x, b);
    b.print("B");

    printf("\nVector B = A(CSR) * X\n");                
    b.set_const_value(1.0);
    Acsr->mult(x, b);
    b.print("B");

    printf("\nVector B = A(CSC) * X\n");        
    b.set_const_value(1.0);
    Acsc->mult(x, b);
    b.print("B");

    printf("\nChange a couple values in B\n");                
    b[2] += 0.1;
    b[4] += 0.1;
    Vector r(A->n_rows);
    
    printf("\nVector R = B - A(COO) * X\n");                
    A->residual(x, b, r);
    r.print("R");

    CSRMatrix* Bcsr = new CSRMatrix(B);
    CSCMatrix* Bcsc = new CSCMatrix(B);
    CSRMatrix* Ccsr;

    printf("\nC = A*B: (CSR, CSR, CSR)\n");
    Ccsr = Acsr->mult(Bcsr);
    Ccsr->print();
    delete Ccsr;

    printf("\nC = A*B: (CSR, CSC, CSR)\n");
    Ccsr = Acsr->mult(Bcsc);
    Ccsr->print();
    delete Ccsr;

    printf("\nC = A_T*B: (CSC, CSC, CSR)\n");
    Ccsr = Acsr->mult_T(Bcsc);
    Ccsr->print();
    delete Ccsr;

    printf("\nC = A_T*B: (CSC, CSR, CSR)\n");
    Ccsr = Acsr->mult_T(Bcsr);
    Ccsr->print();
    delete Ccsr;

}
   
