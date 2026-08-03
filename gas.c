/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Não esqueça de incluir para usar o memcpy
#include <math.h>

#include "gas.h"



static double gas_sum( const double *array, const int tam ) {
   g_return_val_if_fail( array && tam > 0, 0.0 );
   double soma = 0.0;
   for ( int i = 0; i < tam; i++ ) {
      soma += array[i];
   }
   return soma;
}

static double gas_mean( const double *array, const int tam ) {
   g_return_val_if_fail( tam > 0, 0.0 );
   return gas_sum( array, tam ) / tam;
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

void gas_liberar_populacao( GasPopulacao *pop, const int n_pop ) {
   g_return_if_fail( pop );
   for ( int i = 0; i < n_pop; i++ ) {
      if ( pop[i].x != NULL ) {
         g_free( pop[i].x );
      }
   }
   g_free( pop );
}

static void gas_liberar_genitores( GasGenitores *gen, const int n_gen ) {
   g_return_if_fail( gen );
   for ( int i = 0; i < n_gen; i++ ) {
      if ( gen[i].x != NULL ) {
         g_free( gen[i].x );
      }
   }
   g_free( gen );
}



// ============================================================================
// FUNÇÃO QUE CALCULA OS LIMITES DOS 4 QUADRANTES (4 OBJETIVOS)
// ============================================================================
GasLimites *gas_limites( const int nrow, const int ncol, const int n_obj ) {
   // Validação estrita padrão GLib
   // Como esta função desenha limites para 4 quadrantes, travamos n_obj em 4.
   g_return_val_if_fail( nrow > 0 && ncol > 0 && n_obj == 4, NULL );

   int n_dim = 2; // 0: eixo X (colunas), 1: eixo Y (linhas)

   // Calcula os pontos médios e limites máximos da imagem
   double mid_x = ( ncol - 1.0 ) / 2.0;
   double mid_y = ( nrow - 1.0 ) / 2.0;
   double max_x = ncol - 1.0;
   double max_y = nrow - 1.0;

   // Constantes espaciais (Devem ter tamanho constante [4] para inicializar com chaves)
   // k=0 (Topo-Esquerda)   | k=1 (Topo-Direita)
   // ----------------------+----------------------
   // k=3 (Base-Esquerda)   | k=2 (Base-Direita)
   double ini_x[4] = { 0.0,   mid_x, mid_x, 0.0   };
   double fim_x[4] = { mid_x, max_x, max_x, mid_x };

   double ini_y[4] = { 0.0,   0.0,   mid_y, mid_y };
   double fim_y[4] = { mid_y, mid_y, max_y, max_y };

   // Aloca o array principal de limites usando a variável n_obj
   GasLimites *lim = g_new0( GasLimites, n_obj );

   for ( int k = 0; k < n_obj; k++ ) {
      lim[k].n_dim = n_dim;
      lim[k].ini   = g_new0( double, n_dim );
      lim[k].fim   = g_new0( double, n_dim );

      // Preenche os limites da Dimensão 0 (Eixo X)
      lim[k].ini[0] = ini_x[k];
      lim[k].fim[0] = fim_x[k];

      // Preenche os limites da Dimensão 1 (Eixo Y)
      lim[k].ini[1] = ini_y[k];
      lim[k].fim[1] = fim_y[k];
   }

   return lim;
}


void gas_limites_liberar( GasLimites *lim, int n_obj ) {
   if ( !lim ) return;

   for ( int k = 0; k < n_obj; k++ ) {
      if ( lim[k].ini ) g_free( lim[k].ini );
      if ( lim[k].fim ) g_free( lim[k].fim );
   }
   g_free( lim );
}



static void gas_populacao_inicial_uniforme( GasPopulacao *pop, const GasParametros *par, const GasLimites *lim ) {
   g_return_if_fail( pop && par && lim );

   // Aloca um array temporário para armazenar os índices das fatias (bins)
   int *indices = g_new( int, par->n_pop );

   // Processa uma dimensão por vez para garantir a distribuição uniforme em cada eixo
   for ( int j = 0; j < lim->n_dim; j++ ) {

      // 1. Cria fatias ordenadas de 0 até n-1
      for ( int i = 0; i < par->n_pop; i++ ) {
         indices[i] = i;
      }

      // 2. Embaralhamento de Fisher-Yates (Shuffle)
      // Isso garante que a combinação das dimensões seja aleatória (não forme uma linha diagonal)
      for ( int i = par->n_pop - 1; i > 0; i-- ) {
         int k = g_rand_int_range( par->rand, 0, i + 1 );
         int temp = indices[i];
         indices[i] = indices[k];
         indices[k] = temp;
      }

      // 3. Distribui os indivíduos dentro de suas respectivas fatias
      double tamanho_fatia = ( lim->fim[j] - lim->ini[j] ) / ( double )par->n_pop;

      for ( int i = 0; i < par->n_pop; i++ ) {
         int bin = indices[i]; // Qual fatia este indivíduo pegou nesta dimensão?

         // Calcula os limites reais desta fatia específica no espaço de busca
         double inicio_fatia = lim->ini[j] + ( bin * tamanho_fatia );
         double fim_fatia    = inicio_fatia + tamanho_fatia;

         // Sorteia um ponto uniformemente *dentro* da fatia
         pop[i].x[j] = g_rand_double_range( par->rand, inicio_fatia, fim_fatia );
      }
   }

   // Libera a memória temporária
   g_free( indices );
}



static void gas_torneio( const GasPopulacao *pop, GasGenitores *gen, const int n_dim, const GasParametros *par,
                  int( gas_comparar )( const void* a, const void* b ) ) {
   g_return_if_fail( pop && gen && par && gas_comparar );

   for ( int i = 0; i < par->n_gen; i++ ) {
      int rnd1 = g_rand_int_range( par->rand, 0, par->n_pop );

      for ( int k = 0; k < par->n_tor - 1; k++ ) {
         int rnd2 = g_rand_int_range( par->rand, 0, par->n_pop );

         if ( gas_comparar( &pop[rnd2], &pop[rnd1] ) == 1 ) {
            rnd1 = rnd2;
         }
      }
      memcpy( gen[i].x, pop[rnd1].x, n_dim * sizeof( double ) );
   }
}



static void gas_crossover_aritmetico( GasPopulacao *pop, const GasGenitores *gen, const int n_dim, const GasParametros *par ) {
   g_return_if_fail( pop && gen && par );

   for ( int i = 0; i < par->n_gen - 1; i += 2 ) {
      double rnd = g_rand_double_range( par->rand, 0.0, 1.0 );

      if ( rnd < par->p_rec ) {
         double a = g_rand_double_range( par->rand, 0.0, 1.0 );

         for ( int j = 0; j < n_dim; j++ ) {
            pop[i].x[j] = a * gen[i].x[j] + ( 1.0 - a ) * gen[i + 1].x[j];
            pop[i + 1].x[j] = a * gen[i + 1].x[j] + ( 1.0 - a ) * gen[i].x[j];
         }

      } else {
         memcpy( pop[i].x, gen[i].x, n_dim * sizeof( double ) );
         memcpy( pop[i + 1].x, gen[i + 1].x, n_dim * sizeof( double ) );
      }
   }
}




// GG, eu adaptei o meu coeficiente de dispersão lindo e maravilhoso na mutação creep. Ficou perfeito!
static void gas_mutacao_creep( GasPopulacao *pop, const double *coef_disp, const GasLimites *lim, const GasParametros *par ) {
   g_return_if_fail( pop && coef_disp && lim && par );

   for ( int i = 0; i < par->n_gen; i++ ) {
      double rnd_mut = g_rand_double_range( par->rand, 0.0, 1.0 );

      if ( rnd_mut < par->p_mut ) {
         double fator_escala = 1.0 / sqrt( lim->n_dim );

         for ( int j = 0; j < lim->n_dim; j++ ) {
            double rnd_step = g_rand_double_range( par->rand, 0.0, 1.0 );

            if ( g_rand_boolean( par->rand ) ) {
               pop[i].x[j] = pop[i].x[j] + rnd_step * MIN( lim->fim[j] - pop[i].x[j], coef_disp[j] * fator_escala );

            } else {
               pop[i].x[j] = pop[i].x[j] - rnd_step * MIN( pop[i].x[j] - lim->ini[j], coef_disp[j] * fator_escala );
            }
         }
      }
   }
}

static void gas_coeficiente_dispersao( const GasPopulacao *pop, double *coef_disp,
                                       const GasParametros *par, const int n_dim ) {
   g_return_if_fail( pop && coef_disp && par );

   for ( int j = 0; j < n_dim; j++ ) {
      double inn_pop = 1.0 / par->n_pop;
      double soma = 0.0;

      for ( int i = 0; i < par->n_pop; i++ ) {
         soma += pop[i].x[j];
      }
      double media = soma * inn_pop;
      soma = 0.0;

      for ( int i = 0; i < par->n_pop; i++ ) {
         double diff = pop[i].x[j] - media;
         soma += diff * diff;
      }
      coef_disp[j] = par->peso_disp * sqrt( soma * inn_pop );
   }
}



static int gas_comparar_objetivo_max( const void* a, const void* b ) {
   const GasPopulacao *arg1 = ( const GasPopulacao * )a;
   const GasPopulacao *arg2 = ( const GasPopulacao * )b;
   if ( arg1->fitness < arg2->fitness ) return -1;
   if ( arg1->fitness > arg2->fitness ) return 1;
   return 0;
}






// ============================================================================
// FUNÇÃO DE FITNESS LOCAL (ALTAMENTE OTIMIZADA)
// ============================================================================
static double fitness_local( const double *x, const ImagemCinza *img, const int limiar, const int k ) {
   g_return_val_if_fail( x && img && img->image, 0.0 );

   ( void )k;

   int cx = ( int )round( x[0] );
   int cy = ( int )round( x[1] );

   if ( cx < 0 || cx >= img->ncol || cy < 0 || cy >= img->nrow ) return 0.0;

   // Vetores de direção: 0(Leste), 1(Oeste), 2(Sul), 3(Norte)
   int dx[4] = { 1, -1,  0,  0 };
   int dy[4] = { 0,  0,  1, -1 };

   int raio_max = 40;
   double fitness_total = 0.0;

   int centro_eh_preto = ( img->image[cy][cx] < limiar );

   // NOVO: Array para guardar o 'r' da última transição em cada uma das 4 direções
   int ultimo_r[4] = { 0, 0, 0, 0 };

   for ( int dir = 0; dir < 4; dir++ ) {
      int transicoes = 0;
      int estado_atual = centro_eh_preto;

      for ( int r = 1; r <= raio_max; r++ ) {
         int px = cx + ( dx[dir] * r );
         int py = cy + ( dy[dir] * r );

         if ( px < 0 || px >= img->ncol || py < 0 || py >= img->nrow ) break;

         int pixel_escuro = ( img->image[py][px] < limiar );

         if ( pixel_escuro != estado_atual ) {
            transicoes++;
            estado_atual = pixel_escuro;

            // NOVO: Atualiza a distância da transição mais recente
            ultimo_r[dir] = r;
         }
      }

      int pontuacao = 5 - abs( transicoes - 5 );
      if ( pontuacao < 0 ) pontuacao = 0;

      fitness_total += pontuacao;
   }

   double fitness_normalizado = fitness_total / 20.0;

   if ( !centro_eh_preto ) {
      fitness_normalizado *= 0.75;
   }
   // NOVO: Desempate do Platô! Só aplicamos se ele encontrou o alvo (20 pontos)
   else if ( fitness_total == 20.0 ) {

      // Calcula a diferença de distância das bordas opostas
      // Se estiver perfeitamente centralizado, erro_x e erro_y serão 0 (ou no máximo 1 por conta do grid de pixels)
      int erro_x = abs( ultimo_r[0] - ultimo_r[1] ); // Diferença entre Leste e Oeste
      int erro_y = abs( ultimo_r[2] - ultimo_r[3] ); // Diferença entre Sul e Norte

      // Aplicamos uma penalidade minúscula (0.0001 por pixel de assimetria)
      // Ex: Se o candidato está 3 pixels pro lado direito, erro_x = 6. Penalidade = 0.0006.
      // O fitness cai de 1.0000 para 0.9994.
      double penalidade_simetria = ( erro_x + erro_y ) * 0.015;

      // O centro absoluto mantém 1.0000 (ou o mais próximo disso possível)
      fitness_normalizado -= penalidade_simetria;
   }

   return fitness_normalizado;
}



// ============================================================================
// CÁLCULO DA ÁREA DE QUALQUER POLÍGONO (SHOELACE FORMULA / FÓRMULA DE GAUSS)
// ============================================================================
static double gas_calcular_area_ancoras( const GasPopulacao *elite, const int n_ancoras ) {
   g_return_val_if_fail( elite && n_ancoras >= 3, 0.0 );

   double soma = 0.0;

   for ( int i = 0; i < n_ancoras; i++ ) {
      // O operador modulo (%) garante que o próximo vértice após o último seja o primeiro (0)
      int proximo = ( i + 1 ) % n_ancoras;

      // Coordenadas do vértice atual (i) e do próximo (proximo)
      double x_atual   = elite[i].x[0];
      double y_atual   = elite[i].x[1];

      double x_proximo = elite[proximo].x[0];
      double y_proximo = elite[proximo].x[1];

      // Produto cruzado em 2D (Determinante da matriz 2x2)
      soma += ( x_atual * y_proximo ) - ( x_proximo * y_atual );
   }

   // A área é a metade do módulo do determinante acumulado
   return fabs( soma ) / 2.0;
}



// Função auxiliar inline para distância euclidiana (muito rápida)
static inline double gas_distancia( const double p1[2], const double p2[2] ) {
   return hypot( p1[0] - p2[0], p1[1] - p2[1] ); // Nativa do C
}


// ============================================================================
// FUNÇÃO AUXILIAR: ERRO ORTOGONAL VIA PRODUTO ESCALAR NORMALIZADO
// ============================================================================
static double gas_erro_ortogonal( const double p0[2], const double p1[2],
                                    const double p2[2], const double p3[2],
                                    const double top_w, const double bot_w,
                                    const double left_h, const double right_h ) {

   // Vetores partindo de cada vértice
   // Canto 0 (Top-Esq): Vetor para P1 e Vetor para P3
   double dp0 = ( p1[0] - p0[0] ) * ( p3[0] - p0[0] ) + ( p1[1] - p0[1] ) * ( p3[1] - p0[1] );

   // Canto 1 (Top-Dir): Vetor para P0 e Vetor para P2
   double dp1 = ( p0[0] - p1[0] ) * ( p2[0] - p1[0] ) + ( p0[1] - p1[1] ) * ( p2[1] - p1[1] );

   // Canto 2 (Bot-Dir): Vetor para P1 e Vetor para P3
   double dp2 = ( p1[0] - p2[0] ) * ( p3[0] - p2[0] ) + ( p1[1] - p2[1] ) * ( p3[1] - p2[1] );

   // Canto 3 (Bot-Esq): Vetor para P2 e Vetor para P0
   double dp3 = ( p2[0] - p3[0] ) * ( p0[0] - p3[0] ) + ( p2[1] - p3[1] ) * ( p0[1] - p3[1] );

   // O erro é a média dos cossenos absolutos de cada quina.
   // Como a área na função principal é garantida > 100, não há risco de divisão por zero aqui.
   double cos0 = fabs( dp0 ) / ( top_w * left_h );
   double cos1 = fabs( dp1 ) / ( top_w * right_h );
   double cos2 = fabs( dp2 ) / ( bot_w * right_h );
   double cos3 = fabs( dp3 ) / ( bot_w * left_h );

   return ( cos0 + cos1 + cos2 + cos3 ) / 4.0;
}

// ============================================================================
// FUNÇÃO DE FITNESS GEOMÉTRICO (ATUALIZADA)
// ============================================================================
static double fitness_geometrico( const double *x, const GasPopulacao *elite, const int k ) {
   g_return_val_if_fail( x && elite && k >= 0 && k < 4, 0.0 );

   // 1. O PULO DO GATO: Simulação do polígono
   GasPopulacao simulacao[4];
   for ( int i = 0; i < 4; i++ ) {
      simulacao[i].x = ( i == k ) ? ( double * )x : elite[i].x;
   }

   // 2. Extrai as distâncias reais PRIMEIRO (Para resolver o "Ovo e a Galinha")
   double top_w   = gas_distancia( simulacao[0].x, simulacao[1].x );
   double bot_w   = gas_distancia( simulacao[3].x, simulacao[2].x );
   double left_h  = gas_distancia( simulacao[0].x, simulacao[3].x );
   double right_h = gas_distancia( simulacao[1].x, simulacao[2].x );

   double largura_real = ( top_w + bot_w ) / 2.0;
   double altura_real  = ( left_h + right_h ) / 2.0;

   // Barreira contra colapso geométrico (arestas muito pequenas)
   if ( largura_real < 50.0 || altura_real < 50.0 ) return 0.0;

   // 3. Detecção Automática da Direção da Página
   // Se o GA formou um retângulo mais largo que alto, assumimos gabarito Horizontal ('h')
   // Caso contrário, assumimos Vertical ('v')
   double proporcao_alvo;
   if ( largura_real > altura_real ) {
      proporcao_alvo = 14.0 / 11.0; // Horizontal
   } else {
      proporcao_alvo = 10.0 / 15.0; // Vertical
   }

   // 4. Calcula a área e as dimensões teóricas ideais
   double area = gas_calcular_area_ancoras( simulacao, 4 );

   // Como proporcao_alvo = Largura / Altura, deduzimos as dimensões ideais a partir da área:
   double largura_ideal = sqrt( area * proporcao_alvo );
   double altura_ideal  = sqrt( area / proporcao_alvo );

   // 5. Avaliação de Erros Geométricos
   double erro_w = fabs( largura_real - largura_ideal ) / largura_ideal;
   double erro_h = fabs( altura_real - altura_ideal ) / altura_ideal;

   // Cálculo do erro de ortogonalidade aproveitando as arestas já calculadas
   double erro_ortogonal = gas_erro_ortogonal( simulacao[0].x, simulacao[1].x,
                           simulacao[2].x, simulacao[3].x,
                           top_w, bot_w, left_h, right_h );

   // 6. Fitness (Erro Relativo Normalizado)
   // Os três erros gravitam de 0.0 a 1.0 (ou mais em deformações severas).
   double f_geo = 1.0 / ( 1.0 + erro_w + erro_h + erro_ortogonal );

   return f_geo;
}



// ============================================================================
// FUNÇÃO GLOBAL DE AVALIAÇÃO (COEVOLUÇÃO)
// ============================================================================

static double gas_fitness_coevolutivo( const double *x, const GasPopulacao *elite, const ImagemCinza *img,
      const double w1, const int limiar, const int k ) {
   g_return_val_if_fail( x && img, 0.0 );

   // Pesos da combinação linear para gerações > 0 (podem ser ajustados depois)
   // printf( "%lf\n", w1 ); getchar();
   const double w2 = 1.0 - w1;

   // 1. O fitness local sempre é calculado, independentemente da geração
   // Avalia o contraste/textura da imagem exatamente na coordenada 'x'
   double f_local = fitness_local( x, img, limiar, k );

   // 2. GERAÇÃO 0: Se a elite for NULL, não há como calcular a geometria
   if ( elite == NULL ) {
      return f_local;
   }

   // 3. GERAÇÕES > 0: A elite existe, ativando a pressão evolutiva geométrica
   // Avalia como a coordenada 'x' se comporta em relação às outras 3 âncoras
   double f_geo = fitness_geometrico( x, elite, k );

   // 4. FITNESS ATRIBUÍDO (Equilíbrio de Nash)
   double f_atribuido = ( w1 * f_local ) + ( w2 * f_geo );

   return f_atribuido;
}




// ============================================================================
// PIPELINE PRINCIPAL DE COEVOLUÇÃO
// ============================================================================
GasPopulacao *gas_pipeline( const ImagemCinza *img, const GasParametros *par, const GasLimites *lim ) {
   g_return_val_if_fail( img && par && lim, NULL );

   // Alocação da matriz de dispersão e populações
   double **coef_disp = g_new0( double*, par->n_obj );
   GasPopulacao **pop = g_new0( GasPopulacao*, par->n_obj );
   GasGenitores **gen = g_new0( GasGenitores*, par->n_obj );

   double disp_max = 0.0;
   g_autofree double *dispersao_media = g_new0( double, par->n_obj );

   // Inicialização e cálculo da Dispersão Máxima Teórica (Uniforme)
   for ( int k = 0; k < par->n_obj; k++ ) {
      coef_disp[k] = g_new0( double, lim[k].n_dim );

      for ( int j = 0; j < lim[k].n_dim; j++ ) {
         disp_max += par->peso_disp * ( lim[k].fim[j] - lim[k].ini[j] ) / sqrt( 12.0 );
      }

      pop[k] = gas_alocar_populacao( par->n_pop, lim[k].n_dim );
      gen[k] = gas_alocar_genitores( par->n_gen, lim[k].n_dim );

      gas_populacao_inicial_uniforme( pop[k], par, &lim[k] );

      // A dispersão inicial já é pré-calculada no setup
      gas_coeficiente_dispersao( pop[k], coef_disp[k], par, lim[k].n_dim );
      dispersao_media[k] = gas_mean( coef_disp[k], lim[k].n_dim );
   }

   disp_max /= ( lim[0].n_dim * par->n_obj );

   GasPopulacao *elite = gas_alocar_populacao( par->n_obj, lim[0].n_dim );
   int geracao = 0;
   double dispersao_media_global = 0.0;

   // =========================================================================
   // GERAÇÃO 0: AVALIAÇÃO EXPLORATÓRIA
   // =========================================================================

   // Passo 1: Calcular a dispersão global e o w1 oficial da Geração 0
   dispersao_media_global = gas_mean( dispersao_media, par->n_obj );
   double proporcao_inicial = dispersao_media_global / disp_max;
   double w1 = CLAMP( 0.90 * proporcao_inicial + 0.10, 0.0, 1.0 );

   // Passo 2: Avaliar todo mundo com o w1 perfeitamente sincronizado
   for ( int k = 0; k < par->n_obj; k++ ) {
      for ( int i = 0; i < par->n_pop; i++ ) {
         pop[k][i].fitness = gas_fitness_coevolutivo( pop[k][i].x, NULL, img, w1, par->limiar, k );
      }
      qsort( pop[k], par->n_pop, sizeof( GasPopulacao ), gas_comparar_objetivo_max );

      memcpy( elite[k].x, pop[k][par->n_pop - 1].x, lim[k].n_dim * sizeof( double ) );
      elite[k].fitness = pop[k][par->n_pop - 1].fitness;
   }

   // =========================================================================
   // LAÇO EVOLUTIVO CO-EVOLUTIVO (Equilíbrio de Nash)
   // =========================================================================
   do {
      geracao++;

      // ----------------------------------------------------------------------
      // ETAPA 1: DINÂMICA POPULACIONAL (Reprodução e Movimento)
      // Todas as âncoras avançam para o tempo t+1 antes de qualquer avaliação
      // ----------------------------------------------------------------------
      for ( int k = 0; k < par->n_obj; k++ ) {
         gas_torneio( pop[k], gen[k], lim[k].n_dim, par, gas_comparar_objetivo_max );
         gas_crossover_aritmetico( pop[k], gen[k], lim[k].n_dim, par );
         gas_mutacao_creep( pop[k], coef_disp[k], &lim[k], par );

         // Atualizou a população, calcula-se imediatamente o coeficiente de dispersão
         gas_coeficiente_dispersao( pop[k], coef_disp[k], par, lim[k].n_dim );
         dispersao_media[k] = gas_mean( coef_disp[k], lim[k].n_dim );
      }

      // ----------------------------------------------------------------------
      // ETAPA 2: SINCRONIZAÇÃO TÉRMICA GLOBAL (O W1 Universal da Geração)
      // Prepara um único w1 consolidado antes de iniciar a avaliação
      // ----------------------------------------------------------------------
      dispersao_media_global = gas_mean( dispersao_media, par->n_obj );
      double proporcao = dispersao_media_global / disp_max;
      w1 = CLAMP( 0.90 * proporcao + 0.10, 0.0, 1.0 );

      // ----------------------------------------------------------------------
      // ETAPA 3: AVALIAÇÃO E EQUILÍBRIO DE NASH (Via Gauss-Seidel)
      // Avaliação sob o mesmo W1, mas com atualização sequencial do Elite
      // ----------------------------------------------------------------------
      for ( int k = 0; k < par->n_obj; k++ ) {

         // OTIMIZAÇÃO: Avalia apenas os filhos gerados nesta rodada (n_gen)!
         for ( int i = 0; i < par->n_gen; i++ ) {
            pop[k][i].fitness = gas_fitness_coevolutivo( pop[k][i].x, elite, img, w1, par->limiar, k );
         }
         qsort( pop[k], par->n_pop, sizeof( GasPopulacao ), gas_comparar_objetivo_max );

         // Atualiza o Elite DESTE grupo para o próximo grupo k usar (Gauss-Seidel real)
         memcpy( elite[k].x, pop[k][par->n_pop - 1].x, lim[k].n_dim * sizeof( double ) );
         elite[k].fitness = pop[k][par->n_pop - 1].fitness;
      }

   } while ( dispersao_media_global > par->toleracia && geracao < par->max_geracoes );

   // =========================================================================
   // LIMPEZA DE RECURSOS DO PIPELINE
   // =========================================================================
   for ( int k = 0; k < par->n_obj; k++ ) {
      g_free( coef_disp[k] );
      gas_liberar_populacao( pop[k], par->n_pop );
      gas_liberar_genitores( gen[k], par->n_gen );
   }

   g_free( pop );
   g_free( gen );
   g_free( coef_disp );

   // Retorna as âncoras limpas e seguras
   return elite;
}



// GasPopulacao *gas_pipeline( const ImagemCinza *img, const GasParametros *par, const GasLimites *lim ) {
//    g_return_val_if_fail( img && par && lim, NULL );
//
//    // Alocação da matriz de dispersão
//    double **coef_disp = g_new0( double*, par->n_obj );
//
//    GasPopulacao **pop = g_new0( GasPopulacao*, par->n_obj );
//    GasGenitores **gen = g_new0( GasGenitores*, par->n_obj );
//
//    double disp_max = 0.0;
//    g_autofree double *dispersao_media = g_new0( double, par->n_obj );
//
//    // Inicialização e cálculo da Dispersão Máxima Teórica (Uniforme)
//    for ( int k = 0; k < par->n_obj; k++ ) {
//       coef_disp[k] = g_new0( double, lim[k].n_dim );
//
//       for ( int j = 0; j < lim[k].n_dim; j++ ) {
//          disp_max += par->peso_disp * ( lim[k].fim[j] - lim[k].ini[j] ) / sqrt( 12.0 );
//       }
//
//       pop[k] = gas_alocar_populacao( par->n_pop, lim[k].n_dim );
//       gen[k] = gas_alocar_genitores( par->n_gen, lim[k].n_dim );
//
//       gas_populacao_inicial_uniforme( pop[k], par, &lim[k] );
//
//       gas_coeficiente_dispersao( pop[k], coef_disp[k], par, lim[k].n_dim );
//       dispersao_media[k] = gas_mean( coef_disp[k], lim[k].n_dim );
//    }
//
//    disp_max /= ( lim[0].n_dim * par->n_obj );
//
//    GasPopulacao *elite = gas_alocar_populacao( par->n_obj, lim[0].n_dim );
//
//    int geracao = 0;
//
//    double dispersao_media_global = 0.0;
//
//    // =========================================================================
//    // GERAÇÃO 0: AVALIAÇÃO EXPLORATÓRIA
//    // =========================================================================
//
//    // Passo 2: Calcular a dispersão global e o w1 oficial da Geração 0
//    dispersao_media_global = gas_mean( dispersao_media, par->n_obj );
//    double proporcao_inicial = dispersao_media_global / disp_max;
//    double w1 = CLAMP( 0.95 * proporcao_inicial + 0.05, 0.0, 1.0 );
//
//    // Passo 3: Avaliar todo mundo com o w1 perfeitamente sincronizado
//    for ( int k = 0; k < par->n_obj; k++ ) {
//       for ( int i = 0; i < par->n_pop; i++ ) {
//          pop[k][i].fitness = gas_fitness_coevolutivo( pop[k][i].x, NULL, img, w1, par->limiar, k );
//       }
//       qsort( pop[k], par->n_pop, sizeof( GasPopulacao ), gas_comparar_objetivo_max );
//
//       memcpy( elite[k].x, pop[k][par->n_pop - 1].x, lim[k].n_dim * sizeof( double ) );
//       elite[k].fitness = pop[k][par->n_pop - 1].fitness;
//    }
//
//    // =========================================================================
//    // LAÇO EVOLUTIVO CO-EVOLUTIVO (Equilíbrio de Nash)
//    // =========================================================================
//    do {
//       geracao++;
//
//       for ( int k = 0; k < par->n_obj; k++ ) {
//
//          // ATUALIZAÇÃO DA POPULAÇÃO
//          gas_torneio( pop[k], gen[k], lim[k].n_dim, par, gas_comparar_objetivo_max );
//          gas_crossover_aritmetico( pop[k], gen[k], lim[k].n_dim, par );
//          gas_mutacao_creep( pop[k], coef_disp[k], &lim[k], par );
//
//          // Atualizou a população, imediatamente calcula-se o coeficiente de dispersão
//          gas_coeficiente_dispersao( pop[k], coef_disp[k], par, lim[k].n_dim );
//          dispersao_media[k] = gas_mean( coef_disp[k], lim[k].n_dim );
//       }
//
//       // Prepara um único w1 para cada geração antes da avaliação (fitness)
//       dispersao_media_global = gas_mean( dispersao_media, par->n_obj );
//       double proporcao = dispersao_media_global / disp_max;
//       w1 = CLAMP( 0.95 * proporcao + 0.05, 0.0, 1.0 );
//
//
//       for ( int k = 0; k < par->n_obj; k++ ) {
//
//          // OTIMIZAÇÃO APLICADA: Avalia apenas os recém-nascidos (n_gen)!
//          for ( int i = 0; i < par->n_gen; i++ ) {
//             pop[k][i].fitness = gas_fitness_coevolutivo( pop[k][i].x, elite, img, w1, par->limiar, k );
//          }
//          qsort( pop[k], par->n_pop, sizeof( GasPopulacao ), gas_comparar_objetivo_max );
//
//          // Atualiza o Elite DESTE grupo para o próximo grupo k usar (Gauss-Seidel)
//          memcpy( elite[k].x, pop[k][par->n_pop - 1].x, lim[k].n_dim * sizeof( double ) );
//          elite[k].fitness = pop[k][par->n_pop - 1].fitness;
//       }
//
//    } while ( dispersao_media_global > par->toleracia && geracao < par->max_geracoes );
//
//    // =========================================================================
//    // LIMPEZA DE RECURSOS DO PIPELINE
//    // =========================================================================
//    for ( int k = 0; k < par->n_obj; k++ ) {
//       g_free( coef_disp[k] );
//       gas_liberar_populacao( pop[k], par->n_pop );
//       gas_liberar_genitores( gen[k], par->n_gen );
//    }
//
//    g_free( pop );
//    g_free( gen );
//    g_free( coef_disp );
//
//    // Retorna as âncoras limpas e seguras
//    return elite;
// }

