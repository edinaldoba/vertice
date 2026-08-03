/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdio.h>
#include <stdlib.h>


#include "matriz.h"


Matrix mat_new( int r, int c ) {
   Matrix m;
   m.rows = r;
   m.cols = c;
   m.data = ( double * )malloc( r * c * sizeof( double ) );
   if ( m.data == NULL ) {
      fprintf( stderr, "Erro: Falha na alocação de memória!\n" );
      exit( 1 );
   }
   return m;
}

void mat_free( Matrix m ) {
   if ( m.data != NULL ) {
      free( m.data );
   }
}


void mat_fill( Matrix m, double value ) {
   for ( int i = 0; i < m.rows * m.cols; i++ ) {
      m.data[i] = value;
   }
}


Matrix mat_ones( int r, int c ) {
   Matrix m = mat_new( r, c );
   mat_fill( m, 1.0 );
   return m;
}

Matrix mat_zeros( int r, int c ) {
   // 1. Centraliza a alocação (reuso de código)
   Matrix m = mat_new( r, c );

   // 2. Garante que todos os elementos sejam 0.0
   mat_fill( m, 0.0 );

   return m;
}


// Sua função agora fica muito mais "limpa"
void mat_mul( Matrix A, Matrix B, Matrix C ) {
   if ( A.cols != B.rows || A.rows != C.rows || B.cols != C.cols ) {
      fprintf( stderr, "Erro: Dimensões incompatíveis para multiplicação!\n" );
      return;
   }
   cblas_dgemm( CblasRowMajor, CblasNoTrans, CblasNoTrans,
                A.rows, B.cols, A.cols,
                1.0, A.data, A.cols,
                B.data, B.cols,
                0.0, C.data, B.cols );
}



/**
 * Multiplica todos os elementos da matriz por um escalar.
 * A operação é feita "in-place" (na própria matriz).
 */
void mat_mul_esc_inplace( Matrix A, double escalar ) {
   // Parâmetros da dscal:
   // 1. N: Número total de elementos (linhas * colunas)
   // 2. alpha: O escalar
   // 3. X: O ponteiro para os dados
   // 4. incX: O "salto" entre elementos (1 significa processar todos seguidamente)
   cblas_dscal( A.rows * A.cols, escalar, A.data, 1 );
}



/**
 * Multiplica a matriz A por um escalar e armazena o resultado em C.
 * (C = A * escalar)
 * C deve estar previamente alocada com as mesmas dimensões de A.
 */
void mat_mul_esc( Matrix A, double escalar, Matrix C ) {
   // 1. Copia os dados de A para o destino C
   cblas_dcopy( A.rows * A.cols, A.data, 1, C.data, 1 );

   // 2. Aplica o escalonamento diretamente no destino C
   cblas_dscal( C.rows * C.cols, escalar, C.data, 1 );
}




/**
 * Soma a matriz A na matriz B (B = A + B).
 * As matrizes devem ter as mesmas dimensões.
 */
void mat_add_inplace( Matrix A, Matrix B ) {
   // cblas_daxpy(N, alpha, X, incX, Y, incY)
   // N: total de elementos
   // alpha: 1.0 (pois queremos 1*A + B)
   cblas_daxpy( A.rows * A.cols, 1.0, A.data, 1, B.data, 1 );
}




/**
 * Soma A e B e armazena em C (C = A + B).
 * C deve estar previamente alocada com as mesmas dimensões.
 */
void mat_add( Matrix A, Matrix B, Matrix C ) {
   // 1. Copia os dados de B para C usando cblas_dcopy (muito rápido)
   cblas_dcopy( B.rows * B.cols, B.data, 1, C.data, 1 );

   // 2. Soma A em C (C = 1*A + C)
   cblas_daxpy( A.rows * A.cols, 1.0, A.data, 1, C.data, 1 );
}



/**
 * Soma a matriz A na matriz B (B = A + B).
 * As matrizes devem ter as mesmas dimensões.
 */
void mat_sub_inplace( Matrix A, Matrix B ) {
   // cblas_daxpy(N, alpha, X, incX, Y, incY)
   // N: total de elementos
   // alpha: 1.0 (pois queremos 1*A + B)
   cblas_daxpy( A.rows * A.cols, -1.0, A.data, 1, B.data, 1 );
}


void mat_sub( Matrix A, Matrix B, Matrix C ) {
   cblas_dcopy( A.rows * A.cols, A.data, 1, C.data, 1 ); // Copia A para C
   cblas_daxpy( B.rows * B.cols, -1.0, B.data, 1, C.data, 1 ); // C = A - B
}


/**
 * Multiplica a matriz A por sua transposta (C = A * A^T).
 * C deve estar previamente alocada com dimensões (A.rows x A.rows).
 */
void mat_mul_by_transpose( Matrix A, Matrix C ) {
   if ( C.rows != A.rows || C.cols != A.rows ) {
      fprintf( stderr, "Erro: C deve ser uma matriz quadrada de tamanho %d x %d!\n", A.rows, A.rows );
      return;
   }

   // Parâmetros dgemm para C = alpha * op(A) * op(B) + beta * C
   // op(A) = A (CblasNoTrans)
   // op(B) = A^T (CblasTrans)
   // M = A.rows, N = A.rows, K = A.cols
   cblas_dgemm( CblasRowMajor, CblasNoTrans, CblasTrans,
                A.rows, A.rows, A.cols,
                1.0, A.data, A.cols,
                A.data, A.cols, // B é a própria matriz A
                0.0, C.data, C.cols );
}

/**
 * Multiplica a transposta de A pela própria matriz A (C = A^T * A).
 * C deve estar previamente alocada com dimensões (A.cols x A.cols).
 */
void mat_transpose_by_mul( Matrix A, Matrix C ) {
   if ( C.rows != A.cols || C.cols != A.cols ) {
      fprintf( stderr, "Erro: C deve ser uma matriz quadrada de tamanho %d x %d!\n", A.cols, A.cols );
      return;
   }

   // op(A) = A^T (CblasTrans)
   // op(B) = A (CblasNoTrans)
   // M = A.cols, N = A.cols, K = A.rows
   cblas_dgemm( CblasRowMajor, CblasTrans, CblasNoTrans,
                A.cols, A.cols, A.rows,
                1.0, A.data, A.cols,
                A.data, A.cols, // B é a própria matriz A
                0.0, C.data, C.cols );
}

/**
 * Calcula os autovalores e autovetores de uma matriz simétrica real A (ex: Matriz de Covariância).
 *
 * @param A Matrix simétrica de entrada (n x n).
 * @param autovalores Matrix (n x 1) que receberá os autovalores em ordem CRESCENTE.
 * @param autovetores Matrix (n x n) que receberá os autovetores nas SUAS COLUNAS.
 * @return int 0 se teve sucesso (mesmo retorno do LAPACK_INFO).
 */
int mat_eigen_symm( Matrix A, Matrix autovalores, Matrix autovetores ) {
   if ( A.rows != A.cols ) {
      fprintf( stderr, "Erro: A matriz deve ser quadrada para cálculo de autovalores!\n" );
      return -1;
   }

   int n = A.rows;

   // 1. O LAPACKE altera a matriz de entrada durante o cálculo.
   // Copiamos os dados de A para a matriz de autovetores para preservar A intacta.
   cblas_dcopy( n * n, A.data, 1, autovetores.data, 1 );

   // 2. Chamada purista do LAPACKE (Divide & Conquer para matrizes simétricas)
   // LAPACK_ROW_MAJOR: Formato dos dados na memória (Row-Major do C)
   // 'V': Computar autovalores E autovetores ('N' seria apenas autovalores)
   // 'U': Apenas a parte triangular superior da matriz é acessada
   int info = LAPACKE_dsyevd( LAPACK_ROW_MAJOR, 'V', 'U',
                              n, autovetores.data, n, autovalores.data );

   if ( info > 0 ) {
      fprintf( stderr, "Erro: O algoritmo LAPACKE_dsyevd falhou em convergir!\n" );
   }

   return info;
}

/**
 * Centraliza a nuvem de pontos na origem (subtrai a média de cada coluna/dimensão).
 * A operação é realizada "in-place" na própria matriz de dados.
 *
 * @param X Matrix de tamanho (n_indivíduos x n_dimensões)
 * @param media Matrix (1 x n_dimensões) alocada que receberá os valores médios
 */
void mat_centralizar_na_origem( Matrix X, Matrix media ) {
   if ( media.rows != 1 || media.cols != X.cols ) {
      fprintf( stderr, "Erro: A matriz média deve ter dimensão 1 x %d!\n", X.cols );
      return;
   }

   int n_ind = X.rows;
   int n_dim = X.cols;

   // 1. Zera a matriz de médias e calcula a soma de cada dimensão (coluna)
   mat_fill( media, 0.0 );
   for ( int i = 0; i < n_ind; i++ ) {
      for ( int j = 0; j < n_dim; j++ ) {
         // Acesso contíguo ao buffer data
         media.data[j] += X.data[i * n_dim + j];
      }
   }

   // 2. Divide pelo número de indivíduos para obter a média de cada dimensão (\mu_j)
   mat_mul_esc_inplace( media, 1.0 / ( double )n_ind );

   // 3. Subtrai o vetor média de CADA LINHA da população usando a BLAS
   // cblas_daxpy executa: Y = alpha * X + Y (aqui: Linha = -1.0 * Media + Linha)
   for ( int i = 0; i < n_ind; i++ ) {
      double *linha_i = &X.data[i * n_dim];
      cblas_daxpy( n_dim, -1.0, media.data, 1, linha_i, 1 );
   }
}

/**
 * Projeta a matriz centralizada X no plano 2D formado pelos dois autovetores principais.
 *
 * @param X Matriz de dados centralizada (n_ind x n_dim).
 * @param autovetores Matriz (n_dim x n_dim) retornada pelo LAPACKE.
 * @param X_2d Matriz de saída (n_ind x 2) previamente alocada.
 */
void mat_projetar_pca_2d( Matrix X, Matrix autovetores, Matrix X_2d ) {
   if ( X_2d.rows != X.rows || X_2d.cols != 2 ) {
      fprintf( stderr, "Erro: A matriz de saída X_2d deve ter tamanho %d x 2!\n", X.rows );
      return;
   }

   int n_ind = X.rows;
   int n_dim = X.cols;

   // 1. Identificando as colunas dos Componentes Principais
   // V1 (Maior variância) está na última coluna: índice (n_dim - 1)
   // V2 (Segunda maior) está na penúltima coluna: índice (n_dim - 2)

   // O ponteiro aponta para o primeiro elemento de cada coluna na matriz Row-Major
   double *v1 = &autovetores.data[n_dim - 1];
   double *v2 = &autovetores.data[n_dim - 2];

   // O ponteiro aponta para a primeira (X) e segunda (Y) colunas de saída
   double *out_x = &X_2d.data[0];
   double *out_y = &X_2d.data[1];

   // 2. Projeta o Eixo X (Multiplica X pelo autovetor V1)
   // incX = n_dim (salta uma linha inteira para pegar o próximo elemento da coluna em V1)
   // incY = 2 (salta duas posições para preencher a coluna X na matriz de saída)
   cblas_dgemv( CblasRowMajor, CblasNoTrans,
                n_ind, n_dim,
                1.0, X.data, n_dim,
                v1, n_dim,
                0.0, out_x, 2 );

   // 3. Projeta o Eixo Y (Multiplica X pelo autovetor V2)
   cblas_dgemv( CblasRowMajor, CblasNoTrans,
                n_ind, n_dim,
                1.0, X.data, n_dim,
                v2, n_dim,
                0.0, out_y, 2 );
}




/**
 * Calcula a Matriz de Homografia 3x3 usando o algoritmo DLT (Direct Linear Transform).
 * Transforma o quadrilátero 'src' no quadrilátero 'dst'.
 *
 * @param src Matriz 4x2 com os 4 pontos de origem (ex: âncoras encontradas pelo GA).
 * @param dst Matriz 4x2 com os 4 pontos de destino (ex: gabarito real perfeito).
 * @param H Matriz 3x3 previamente alocada que receberá os coeficientes da Homografia.
 * @return int 0 se sucesso, >0 se a matriz for singular (pontos colineares).
 */
int mat_homography_dlt( Matrix src, Matrix dst, Matrix H ) {
   if ( src.rows != 4 || src.cols != 2 || dst.rows != 4 || dst.cols != 2 ) {
      fprintf( stderr, "Erro: As matrizes src e dst devem ter dimensões 4x2!\n" );
      return -1;
   }
   if ( H.rows != 3 || H.cols != 3 ) {
      fprintf( stderr, "Erro: A matriz H deve ter dimensão 3x3!\n" );
      return -1;
   }

   // Sistema Linear A * h = B, onde A(8x8) e B(8x1)
   double A_data[64] = { 0.0 };
   double B_data[8]  = { 0.0 };
   int ipiv[8]; // Array de pivoteamento exigido pelo LAPACKE

   // Preenchendo as 8 equações a partir dos 4 pontos
   for ( int i = 0; i < 4; i++ ) {
      double x = src.data[i * 2 + 0];
      double y = src.data[i * 2 + 1];
      double u = dst.data[i * 2 + 0];
      double v = dst.data[i * 2 + 1];

      // Linha par: Equação correspondente à coordenada u
      A_data[( 2 * i ) * 8 + 0] = x;
      A_data[( 2 * i ) * 8 + 1] = y;
      A_data[( 2 * i ) * 8 + 2] = 1.0;
      A_data[( 2 * i ) * 8 + 3] = 0.0;
      A_data[( 2 * i ) * 8 + 4] = 0.0;
      A_data[( 2 * i ) * 8 + 5] = 0.0;
      A_data[( 2 * i ) * 8 + 6] = -x * u;
      A_data[( 2 * i ) * 8 + 7] = -y * u;
      B_data[2 * i] = u;

      // Linha ímpar: Equação correspondente à coordenada v
      A_data[( 2 * i + 1 ) * 8 + 0] = 0.0;
      A_data[( 2 * i + 1 ) * 8 + 1] = 0.0;
      A_data[( 2 * i + 1 ) * 8 + 2] = 0.0;
      A_data[( 2 * i + 1 ) * 8 + 3] = x;
      A_data[( 2 * i + 1 ) * 8 + 4] = y;
      A_data[( 2 * i + 1 ) * 8 + 5] = 1.0;
      A_data[( 2 * i + 1 ) * 8 + 6] = -x * v;
      A_data[( 2 * i + 1 ) * 8 + 7] = -y * v;
      B_data[2 * i + 1] = v;
   }

   // Resolve o sistema linear real A * X = B
   // LAPACK_ROW_MAJOR: Formato C
   // N = 8 (Ordem), NRHS = 1 (colunas do B)
   // O resultado X irá sobrescrever o vetor B_data!
   int info = LAPACKE_dgesv( LAPACK_ROW_MAJOR, 8, 1, A_data, 8, ipiv, B_data, 1 );

   if ( info > 0 ) {
      fprintf( stderr, "Erro: LAPACKE falhou, geometria impossível (pontos colineares)!\n" );
      return info;
   }

   // Monta a matriz H (3x3) final a partir do vetor solução
   for ( int i = 0; i < 8; i++ ) {
      H.data[i] = B_data[i];
   }
   // O fator de escala homogênea (h22) é sempre 1.0
   H.data[8] = 1.0;

   return 0;
}


/**
 * Aplica a Matriz de Homografia H a uma matriz de pontos 2D (Transformação de Perspectiva).
 *
 * @param src Matriz N x 2 contendo os pontos de entrada.
 * @param H Matriz de Homografia 3 x 3.
 * @param dst Matriz N x 2 previamente alocada para os pontos transformados.
 */
void mat_apply_homography( Matrix src, Matrix H, Matrix dst ) {
   if ( src.cols != 2 || dst.cols != 2 || src.rows != dst.rows ) {
      fprintf( stderr, "Erro: Dimensões incompatíveis na projeção!\n" );
      return;
   }

   for ( int i = 0; i < src.rows; i++ ) {
      double x = src.data[i * 2 + 0];
      double y = src.data[i * 2 + 1];

      // Multiplicação por coordenadas homogêneas
      double w = H.data[6] * x + H.data[7] * y + H.data[8];

      // Proteção matemática contra divisão por zero acidental
      if ( w == 0.0 ) w = 1e-8;

      double u = ( H.data[0] * x + H.data[1] * y + H.data[2] ) / w;
      double v = ( H.data[3] * x + H.data[4] * y + H.data[5] ) / w;

      dst.data[i * 2 + 0] = u;
      dst.data[i * 2 + 1] = v;
   }
}
