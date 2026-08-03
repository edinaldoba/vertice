#ifndef MATRIZ_H
#define MATRIZ_H

#include <cblas.h>
#include <lapacke.h>

// Macro para facilitar o acesso: mat_at(A, 1, 0) acessa linha 1, coluna 0
#define mat_at(m, i, j) (m).data[(i) * (m).cols + (j)]


typedef struct {
   int rows;
   int cols;
   double *data;
} Matrix;


// Gerenciamento de Memória
Matrix mat_new( int r, int c );      // No lugar de mat_new
void mat_free( Matrix m );         // No lugar de mat_free

// Inicialização e Preenchimento
void mat_fill( Matrix m, double value );
Matrix mat_ones( int r, int c );         // Mantido conforme solicitado
Matrix mat_zeros( int r, int c );        // Mantido conforme solicitado

// Operações Matemáticas
void mat_mul( Matrix A, Matrix B, Matrix C ); // A nossa mat_mul

// multiplicação por escalar
void mat_mul_esc_inplace( Matrix A, double escalar );

void mat_mul_esc( Matrix A, double escalar, Matrix C );

void mat_add_inplace( Matrix A, Matrix B );

void mat_add( Matrix A, Matrix B, Matrix C );

void mat_sub_inplace( Matrix A, Matrix B );

void mat_sub( Matrix A, Matrix B, Matrix C );

void mat_mul_by_transpose( Matrix A, Matrix C );

void mat_transpose_by_mul( Matrix A, Matrix C );

int mat_eigen_symm( Matrix A, Matrix autovalores, Matrix autovetores );

void mat_centralizar_na_origem( Matrix X, Matrix media );

void mat_projetar_pca_2d( Matrix X, Matrix autovetores, Matrix X_2d );

int mat_homography_dlt( Matrix src, Matrix dst, Matrix H );

void mat_apply_homography( Matrix src, Matrix H, Matrix dst );




#endif
