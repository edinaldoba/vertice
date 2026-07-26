/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Não esqueça de incluir para usar o memcpy
#include <math.h>
#include <stdbool.h>

#include "gas.h"



typedef struct {
	double *x;
} GasGenitores;


void gas_gerar_sementes( guint32 *sementes ) {
    // 1. Pega os ciclos/tempo monotônico do processador em alta precisão (64 bits)
    gint64 ciclos_cpu = g_get_monotonic_time();

    // 2. Separa a parte alta e baixa do inteiro de 64 bits
    guint32 baixa = (guint32)(ciclos_cpu & 0xFFFFFFFF);
    guint32 alta  = (guint32)(ciclos_cpu >> 32);

    // Imprime para manter a rastreabilidade se precisar reproduzir a execução
    g_print( "Semente Monotonica (Ciclos): %" G_GINT64_FORMAT "\n", ciclos_cpu );

    // 3. Monta o array de sementes multiplicando por constantes de dispersão
    sementes[0] = baixa;
    sementes[1] = alta ^ 0x9E3779B9; // Proporção Áurea de 32-bit
    sementes[2] = baixa ^ 0x6789A;
    sementes[3] = (baixa + alta) ^ 0xBCDEF;
}


static GasPopulacao *gas_alocar_populacao( const int n_pop, const int n_dim ) {
   g_return_val_if_fail( n_pop > 0 && n_dim > 0, NULL );

   GasPopulacao *pop = g_new0( GasPopulacao, n_pop );
   for ( int i = 0; i < n_pop; i++ ) {
      pop[i].x = g_new0( double, n_dim );
   }
   return pop;
}

static GasGenitores *gas_alocar_genitores( const int n_gen, const int n_dim ) {
   g_return_val_if_fail( n_gen > 0 && n_dim > 0, NULL );

   GasGenitores *gen = g_new0( GasGenitores, n_gen );
   for ( int i = 0; i < n_gen; i++ ) {
      gen[i].x = g_new0( double, n_dim );
   }
   return gen;
}

static void gas_liberar_populacao( GasPopulacao *pop, const int n_pop ) {
   for ( int i = 0; i < n_pop; i++ ) {
      g_free( pop[i].x );
   }
   g_free( pop );
}

static void gas_liberar_genitores( GasGenitores *gen, const int n_gen ) {
   for ( int i = 0; i < n_gen; i++ ) {
      g_free( gen[i].x );
   }
   g_free( gen );
}



static void gas_populacao_inicial( GasPopulacao *pop, const GasParametros *par, const GasLimites *lim ) {
   g_return_if_fail( par->n_pop > 0 && lim->n_dim > 0 );

   for ( int i = 0; i < par->n_pop; i++ ) {
      for ( int j = 0; j < lim->n_dim; j++ ) {
         pop[i].x[j] = g_rand_double_range( par->rand, lim->ini[j], lim->fim[j] );
      }
   }
}



static void gas_torneio( const GasPopulacao *pop, GasGenitores *gen, const int n_dim, const GasParametros *par,
                         int(gas_comparar)(const void* a, const void* b) )
{
   g_return_if_fail( pop != NULL && gen != NULL );

   for ( int i = 0; i < par->n_gen; i++ ) {
      int rnd1 = g_rand_int_range( par->rand, 0, par->n_pop );

      for ( int k = 0; k < par->n_tor - 1; k++ ) {
         int rnd2 = g_rand_int_range( par->rand, 0, par->n_pop );

         if ( gas_comparar( &pop[rnd2], &pop[rnd1] ) == 1 ) {
            rnd1 = rnd2;
         }
      }
      memcpy( gen[i].x, pop[rnd1].x, n_dim * sizeof(double) );
   }
}



static void gas_crossover_aritmetico( GasPopulacao *pop, const GasGenitores *gen, const int n_dim, const GasParametros *par ) {
   g_return_if_fail( pop != NULL && gen != NULL );

   for ( int i = 0; i < par->n_gen - 1; i += 2 ) {
      double rnd = g_rand_double_range( par->rand, 0.0, 1.0 );

      if ( rnd < par->p_rec ) {
         double a = g_rand_double_range( par->rand, 0.0, 1.0 );

         for ( int j = 0; j < n_dim; j++ ) {
            pop[i].x[j] = a * gen[i].x[j] + ( 1.0 - a ) * gen[i + 1].x[j];
            pop[i + 1].x[j] = a * gen[i + 1].x[j] + ( 1.0 - a ) * gen[i].x[j];
         }

      } else {
         memcpy( pop[i].x, gen[i].x, n_dim * sizeof(double) );
         memcpy( pop[i + 1].x, gen[i + 1].x, n_dim * sizeof(double) );
      }
   }
}


// static void gas_mutacao_direcional( GasPopulacao *pop, const double *coef_disp, const int n_dim, const GasParametros *par ) {
//    g_return_if_fail( pop != NULL && coef_disp != NULL );
//
//    double fator_escala = 1.0 / sqrt( n_dim );
//
//    for ( int i = 0; i < par->n_gen; i++ ) {
//       double rnd_mut = g_rand_double_range( par->rand, 0.0, 1.0 );
//
//       if ( rnd_mut < par->p_mut ) {
//          for ( int j = 0; j < n_dim; j++ ) {
//             double rnd_dir = g_rand_double_range( par->rand, -1.0, 1.0 );
//             pop[i].x[j] = pop[i].x[j] + rnd_dir * coef_disp[j] * fator_escala;
//          }
//       }
//    }
// }


// GG, eu adaptei o meu coeficiente de dispersão lindo e maravilhoso na mutação creep. Ficou perfeito!
static void gas_mutacao_creep( GasPopulacao *pop, const double *coef_disp, const GasLimites *lim, const GasParametros *par ) {
   g_return_if_fail( pop != NULL && coef_disp != NULL );

   for ( int i = 0; i < par->n_gen; i++ ) {
      double rnd_mut = g_rand_double_range( par->rand, 0.0, 1.0 );

      if ( rnd_mut < par->p_mut ) {
         double fator_escala = 1.0 / sqrt( lim->n_dim );

         for ( int j = 0; j < lim->n_dim; j++ ) {
            double rnd_step = g_rand_double_range( par->rand, 0.0, 1.0 );

            if ( g_rand_boolean( par->rand ) ) {
               pop[i].x[j] = pop[i].x[j] + rnd_step * fmin( lim->fim[j] - pop[i].x[j], coef_disp[j] * fator_escala );

            } else {
               pop[i].x[j] = pop[i].x[j] - rnd_step * fmin( pop[i].x[j] - lim->ini[j], coef_disp[j] * fator_escala );
            }
         }
      }
   }
}

static void gas_coeficiente_dispersao( const GasPopulacao *pop, double *coef_disp,
                                       const GasParametros *par, const int n_dim )
{
   g_return_if_fail( pop != NULL && coef_disp != NULL );

   for ( int j = 0; j < n_dim; j++ ) {
      double inv_n_pop = 1.0 / par->n_pop;
      double soma = 0.0;

      for ( int i = 0; i < par->n_pop; i++ ) {
         soma += pop[i].x[j];
      }
      double media = soma * inv_n_pop;
      soma = 0.0;

      for ( int i = 0; i < par->n_pop; i++ ) {
         double diff = pop[i].x[j] - media;
         soma += diff * diff;
      }
      coef_disp[j] = par->peso_disp * sqrt( soma * inv_n_pop );
   }
}



int gas_comparar_objetivo_max( const void* a, const void* b ) {
   const GasPopulacao *arg1 = ( const GasPopulacao * )a;
   const GasPopulacao *arg2 = ( const GasPopulacao * )b;
   if ( arg1->fitness < arg2->fitness ) return -1;
   if ( arg1->fitness > arg2->fitness ) return 1;
   return 0;
}

int gas_comparar_objetivo_min( const void* a, const void* b ) {
   const GasPopulacao *arg1 = ( const GasPopulacao * )a;
   const GasPopulacao *arg2 = ( const GasPopulacao * )b;
   if ( arg1->fitness < arg2->fitness ) return 1;
   if ( arg1->fitness > arg2->fitness ) return -1;
   return 0;
}


double F5( const double *x, const int n_dim ) {
   // Validação de segurança no padrão da GLib
   g_return_val_if_fail( x != NULL && n_dim > 0, 0.0 );

   // 1. Transformamos a1 e a2 em uma única matriz 2D.
   // 2. O modificador 'static const' é crucial aqui: ele diz ao compilador para alocar
   //    essa matriz na memória apenas uma vez (no segmento de dados), em vez de empurrar
   //    50 números inteiros para a pilha (stack) a cada milissegundo que a função for chamada.
   static const double a[2][25] = {
      {-32,-16,  0, 16, 32,-32,-16,  0, 16, 32,-32,-16,  0, 16, 32,-32,-16,  0, 16, 32,-32,-16,  0, 16, 32},
      {-32,-32,-32,-32,-32,-16,-16,-16,-16,-16,  0,  0,  0,  0,  0, 16, 16, 16, 16, 16, 32, 32, 32, 32, 32}
   };

   const double K = 500.0;
   double soma = 0.0;

   // Proteção para garantir que o laço não tente ler uma 3ª dimensão inexistente na matriz 'a'
   int dim_max = (n_dim < 2) ? n_dim : 2;

   for ( int j = 0; j < 25; j++ ) {
      double soma_potencias = 0.0;

      // Aproveitando o n_dim para iterar sobre as dimensões de forma flexível e expansível
      for ( int d = 0; d < dim_max; d++ ) {
         soma_potencias += pow( x[d] - a[d][j], 6.0 );
      }

      // Uso explícito de '1.0' e '(double)' para evitar conversões implícitas
      soma += 1.0 / ( (double)j + soma_potencias );
   }

   return 1.0 / ( (1.0 / K) + soma );
}


double F6( const double *x, const int n_dim ) {
   g_return_val_if_fail( x != NULL && n_dim > 0, 0.0 );

   double soma = 0.0;

   // 1. Multiplicação direta em vez de pow(x[j], 2)
   for ( int j = 0; j < n_dim; j++ ) {
      soma += x[j] * x[j];
   }

   // 2. Fragmentação da equação para evitar múltiplos pow() e melhorar a leitura
   double temp_sin = sin( sqrt(soma) );
   double numerador = ( temp_sin * temp_sin ) - 0.5;

   double temp_denom = 1.0 + 0.001 * soma;
   double denominador = temp_denom * temp_denom;

   return 0.5 - ( numerador / denominador );
}

double F10( const double *x, const int n_dim ) { // Função de Rastrigin I
   g_return_val_if_fail( x != NULL && n_dim > 0, 0.0 );

   const double A = 10.0;
   double soma = 0.0;

   // Iteramos sobre as dimensões (n_dim) em vez de usar um 'ndim' global
   for ( int j = 0; j < n_dim; j++ ) {
      // 1. Substituímos pow(x[j], 2) pela multiplicação direta x[j] * x[j]
      // 2. Utilizamos a constante G_PI nativa da GLib (que já possui precisão máxima)
      // 3. Garantimos que 2.0 seja tratado como double
      soma += (x[j] * x[j]) - A * cos( 2.0 * G_PI * x[j] );
   }

   return (A * n_dim) + soma;
}


static double gas_max( const double *array, int tam ) {
   g_return_val_if_fail( array != NULL, 0.0 );

   gdouble max_val = array[0];

   for (int i = 1; i < tam; i++) {
      max_val = MAX( max_val, array[i] );
   }

   return max_val;
}





static void gas_display_gnuplot( const GasLimites *lim, int geracao ) {
   FILE *p_plot;

   // 1. Gráfico de Evolução
   p_plot = fopen( "gnuplot/plotEvolucao.txt", "w" );
   if ( p_plot ) {
      fprintf( p_plot, "reset\n" );
      fprintf( p_plot, "set terminal wxt size 920,600 enhanced font 'Verdana,16' persist\n" );
      fprintf( p_plot, "set grid\n" );
      fprintf( p_plot, "set xrange [0:%d]\n", geracao );
      fprintf( p_plot, "set xlabel 'Geração'\n" );
      fprintf( p_plot, "set ylabel 'Avaliação'\n" );
      fprintf( p_plot, "plot 'E.pts' title 'Evolução da Avaliação do Mais Apto' with lines lt 3 lw 2\n" );
      fclose( p_plot );
   }

   // 2. Gráfico de Dispersão
   p_plot = fopen( "gnuplot/plotDispersao.txt", "w" );
   if ( p_plot ) {
      fprintf( p_plot, "reset\n" );
      fprintf( p_plot, "set terminal wxt size 900,600 enhanced font 'Verdana,16' persist\n" );
      fprintf( p_plot, "set grid\n" );
      fprintf( p_plot, "set xrange [0:%d]\n", geracao );
      fprintf( p_plot, "set xlabel 'Geração'\n" );
      fprintf( p_plot, "set ylabel 'Dispersão'\n" );
      fprintf( p_plot, "plot 'D.pts' title 'Evolução do Coeficiente de Dispersão' with lines lt 3 lw 2\n" );
      fclose( p_plot );
   }

   // 3. Animação dos Pontos (Limpo, Inteligente e Profissional)
   p_plot = fopen( "gnuplot/plotPontos.txt", "w" );
   if ( p_plot ) {
      fprintf( p_plot, "reset\n" );
      fprintf( p_plot, "set terminal wxt size 800,800 enhanced font 'Verdana,16' persist\n" );
      fprintf( p_plot, "set grid\n" );
      fprintf( p_plot, "set xrange [%.1f:%.1f]\n", lim->ini[0], lim->fim[0] );
      fprintf( p_plot, "set yrange [%.1f:%.1f]\n", lim->ini[1], lim->fim[1] );
      fprintf( p_plot, "set size ratio -1\n" );
      fprintf( p_plot, "set pointsize 2\n" );

      // Usamos o laço nativo do gnuplot para iterar sobre os arquivos .pts gerados
      fprintf( p_plot, "do for [i=0:%d] {\n", geracao );
      fprintf( p_plot, "    plot sprintf('geracao_%%d.pts', i) title sprintf('Geração: %%d', i) with points pt 1\n" );
      fprintf( p_plot, "    pause 0.015\n" );
      fprintf( p_plot, "}\n" );

      fclose( p_plot );
   }
}



static void gas_gravar_pontos( const GasPopulacao *pop, const int n_pop, const int geracao ) {
   char arquivo[256];
   snprintf( arquivo, sizeof(arquivo), "gnuplot/geracao_%d.pts", geracao );
   FILE *p_geracao = fopen( arquivo, "w" );
   if ( p_geracao ) {
      for ( int i = 0; i < n_pop; i++ ) {
         fprintf( p_geracao, "%.8f %.8f\n", pop[i].x[0], pop[i].x[1] );
      }
      fclose( p_geracao );
   }
}


static void gas_display_terminal( const GasPopulacao *pop, const int n_dim, const double dispersao_max, const int geracao ) {
   printf( "Geração: %d\n", geracao );
   printf( "Mais Apto: " );
   for ( int i = 0; i < n_dim; i++ ) {
      printf( "%.8f  ", pop->x[i] );
   }
   printf( "\nAvaliação do Mais Apto: %.8f\n", pop->fitness );
   printf( "Coeficiente de Dispersão: %.8f\n\n", dispersao_max );
}





GasPopulacao gas_pipeline( const GasParametros *par, const GasLimites *lim, double(gas_avaliar)(const double*,const int),
                           int(gas_comparar)(const void* a, const void* b) )
{
   double *coef_disp = g_new0( double, lim->n_dim );
   GasPopulacao *pop = gas_alocar_populacao( par->n_pop, lim->n_dim );
   GasGenitores *gen = gas_alocar_genitores( par->n_gen, lim->n_dim );

   int geracao = 0;
   double dispersao_max;

   gas_populacao_inicial( pop, par, lim );

   for ( int i = 0; i < par->n_pop; i++ ) {
      pop[i].fitness = gas_avaliar( pop[i].x, lim->n_dim );
   }
   qsort( pop, par->n_pop, sizeof(GasPopulacao), gas_comparar );
   gas_coeficiente_dispersao( pop, coef_disp, par, lim->n_dim );

   //--------------- FEEDBACK VISUAL ------------------------//
   FILE *p_dispersao = fopen( "gnuplot/D.pts", "w" );
   FILE *p_fitness   = fopen( "gnuplot/E.pts", "w" );
   dispersao_max = gas_max(coef_disp, lim->n_dim);
   fprintf( p_dispersao, "%d %.8f\n", geracao, dispersao_max );
   fprintf( p_fitness  , "%d %.8f\n", geracao, pop[par->n_pop - 1].fitness );
   gas_display_terminal( &pop[par->n_pop - 1], lim->n_dim, dispersao_max, geracao );
   gas_gravar_pontos( pop, par->n_pop, geracao );
   //-------------------------------------------------------//

   do {
      /****************************** ALGORITMOS GENÉTICOS ******************************/
      geracao = geracao + 1;

      gas_torneio( pop, gen, lim->n_dim, par, gas_comparar );
      gas_crossover_aritmetico( pop, gen, lim->n_dim, par );
      gas_mutacao_creep( pop, coef_disp, lim, par );

      for ( int i = 0; i < par->n_pop; i++ ) {
         pop[i].fitness = gas_avaliar( pop[i].x, lim->n_dim );
      }
      qsort( pop, par->n_pop, sizeof(GasPopulacao), gas_comparar );
      gas_coeficiente_dispersao( pop, coef_disp, par, lim->n_dim );
      dispersao_max = gas_max( coef_disp, lim->n_dim );

      //--------------- FEEDBACK VISUAL ------------------------//
      fprintf( p_dispersao, "%d %.8f\n", geracao, dispersao_max );
      fprintf( p_fitness  , "%d %.8f\n", geracao, pop[par->n_pop - 1].fitness );
      gas_display_terminal( &pop[par->n_pop - 1], lim->n_dim, dispersao_max, geracao );
      gas_gravar_pontos( pop, par->n_pop, geracao );
      //--------------------------------------------------------//

   } while ( dispersao_max > par->toleracia && geracao < 1000 ); // <--- FIM DO LOOP WHILE

   //--------------- FEEDBACK VISUAL ------------------------//
   gas_display_gnuplot( lim, geracao );
   if ( p_fitness )   fclose( p_fitness );
   if ( p_dispersao ) fclose( p_dispersao );
   //--------------------------------------------------------//

   GasPopulacao melhor;
   melhor.fitness = pop[par->n_pop - 1].fitness;
   melhor.x = g_new0( double, lim->n_dim );
   memcpy( melhor.x, pop[par->n_pop - 1].x, lim->n_dim * sizeof(double) );

   //-------- Liberar memória ----------------
   g_free( coef_disp );
   gas_liberar_populacao( pop, par->n_pop );
   gas_liberar_genitores( gen, par->n_gen );

   return melhor;
}

