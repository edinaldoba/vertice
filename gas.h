#ifndef GAS_H
#define GAS_H

#include <glib.h>
#include <omp.h>


typedef struct {
	int n_pop, n_gen, n_tor;
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


#define GAS_NUM_SEMENTES 4
void gas_gerar_sementes( guint32 *sementes );

int gas_comparar_objetivo_max( const void* a, const void* b );

int gas_comparar_objetivo_min( const void* a, const void* b );

double F5( const double *x, const int n_dim );

double F6( const double *x, const int n_dim );

double F10( const double *x, const int n_dim );

GasPopulacao gas_pipeline( const GasParametros *par, const GasLimites *lim, double(gas_avaliar)(const double*,const int),
									int(gas_comparar)(const void* a, const void* b) );




#endif
