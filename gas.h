#ifndef GAS_H
#define GAS_H

#include <glib.h>
#include "comum.h"


typedef struct {
   int n_pop, n_gen, n_tor, n_obj;
   int max_geracoes;
   int limiar;
   double p_rec, p_mut, peso_disp, toleracia;
   GRand *rand; // <- Ponteiro para o gerador de números aleatórios
} GasParametros;

typedef struct {
   double *ini, *fim;
   int n_dim;
} GasLimites;

typedef struct {
   double *x, fitness;
} GasPopulacao;

typedef struct {
   double *x;
} GasGenitores;


// ============================================================================
// ASSINATURAS DE FUNÇÕES
// ============================================================================
void gas_liberar_populacao( GasPopulacao *pop, const int n_pop );

GasLimites *gas_limites( const int nrow, const int ncol, const int n_obj );

void gas_limites_liberar( GasLimites *lim, int n_obj );

GasPopulacao *gas_pipeline( const ImagemCinza *img, const GasParametros *par, const GasLimites *lim );




#endif
