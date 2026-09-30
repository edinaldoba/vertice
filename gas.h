#ifndef GAS_H
#define GAS_H

#include <glib.h>
#include "comum.h"


typedef struct {
   int n_pop, n_gen, n_tor, n_obj;
   int max_geracoes;
   int limiar;
   double p_rec, p_mut, peso_disp, toleracia, alfa;
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

void gas_mapear_ancoras( const ImagemCinza *img, MapeamentoGabarito *info, IndiceMatriz *ancora, int tentativa );




#endif
