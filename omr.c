/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <omp.h>

#include "comum.h"
#include "omr.h"
#include "interface.h"

// ============================================================================
// CONSTANTES E CONFIGURAÇÕES
// ============================================================================
#define MAX_CANDIDATOS 128

// Estruturas de dados base
typedef struct {
   int min_i, max_i;
   int min_j, max_j;
   int area;
   double centro_i;
   double centro_j;
} BlobInfo;

typedef struct {
   BlobInfo blob;
   double score;
} MatchCandidate;

// Parâmetros de controle do Template
typedef struct {
   int core_size;      // Largura/Altura do quadrado preto interno
   int margin_size;    // Largura da margem branca ao redor
   int total_size;     // core_size + 2 * margin_size
   double max_energia; // Pré-calculado para otimização
} TemplateParams;

// Ordenação por "solidez" (Fill Ratio) para priorizar quadrados verdadeiros
static int blob_comparar_solidez( const void *a, const void *b ) {
   const BlobInfo *ba = ( const BlobInfo * )a;
   const BlobInfo *bb = ( const BlobInfo * )b;

   double area_box_a = ( double )( ( ba->max_i - ba->min_i + 1 ) * ( ba->max_j - ba->min_j + 1 ) );
   double area_box_b = ( double )( ( bb->max_i - bb->min_i + 1 ) * ( bb->max_j - bb->min_j + 1 ) );

   double f_a = ( double )ba->area / area_box_a;
   double f_b = ( double )bb->area / area_box_b;

   return ( f_a < f_b ) - ( f_a > f_b ); // Ordem decrescente
}

// ============================================================================
// MÓDULO 2: Correlação Cruzada Direta (Otimizada)
// ============================================================================
static double calcular_correcao_cruzada_direta( const ImagemCinza *img, int start_i, int start_j, const TemplateParams *tpl ) {
   if ( tpl->max_energia <= 0.0 ) {
      return 0.0;
   }

   int soma_margem = 0;
   int soma_core   = 0;

   const int ts         = tpl->total_size;
   const int ms         = tpl->margin_size;
   const int limit_core = ts - ms;

   // 1. Margem Superior (Apenas branco)
   for ( int i = 0; i < ms; i++ ) {
      const uint8_t *linha = img->image[start_i + i] + start_j;
      for ( int j = 0; j < ts; j++ ) {
         soma_margem += ( int )linha[j];
      }
   }

   // 2. Miolo (Margem Esquerda + Core Preto + Margem Direita)
   for ( int i = ms; i < limit_core; i++ ) {
      const uint8_t *linha = img->image[start_i + i] + start_j;

      for ( int j = 0; j < ms; j++ ) {
         soma_margem += ( int )linha[j];
      }
      for ( int j = ms; j < limit_core; j++ ) {
         soma_core += ( int )linha[j];
      }
      for ( int j = limit_core; j < ts; j++ ) {
         soma_margem += ( int )linha[j];
      }
   }

   // 3. Margem Inferior (Apenas branco)
   for ( int i = limit_core; i < ts; i++ ) {
      const uint8_t *linha = img->image[start_i + i] + start_j;
      for ( int j = 0; j < ts; j++ ) {
         soma_margem += ( int )linha[j];
      }
   }

   double correlacao = ( double )( soma_margem - soma_core );
   return correlacao / tpl->max_energia;
}

// ============================================================================
// MÓDULO 3: Non-Maximum Suppression (NMS)
// ============================================================================
static void adicionar_candidato_nms( MatchCandidate *candidatos, int *num_candidatos,
                                     BlobInfo nova_blob, double novo_score, int raio_supressao ) {

   double raio_sq = ( double )( raio_supressao * raio_supressao );

   for ( int k = 0; k < *num_candidatos; k++ ) {
      double dist_i  = candidatos[k].blob.centro_i - nova_blob.centro_i;
      double dist_j  = candidatos[k].blob.centro_j - nova_blob.centro_j;
      double dist_sq = ( dist_i * dist_i ) + ( dist_j * dist_j );

      if ( dist_sq < raio_sq ) {
         if ( novo_score > candidatos[k].score ) {
            candidatos[k].blob  = nova_blob;
            candidatos[k].score = novo_score;
         }
         return; // Supressão ocorreu (atualizando ou descartando)
      }
   }

   // Adiciona novo candidato se houver espaço no buffer
   if ( *num_candidatos < MAX_CANDIDATOS ) {
      candidatos[*num_candidatos].blob  = nova_blob;
      candidatos[*num_candidatos].score = novo_score;
      ( *num_candidatos )++;
   }
}

// ============================================================================
// MÓDULO 4: Função Principal de Extração por Template
// ============================================================================
static int blob_extrair_por_template( const ImagemCinza *img, BlobInfo *blobs, int core_size, int margin_size ) {
   const double limiar_correlacao = 0.5;
   const int stride_busca         = 1;
   const int max_blobs_saida      = MAX_CANDIDATOS;

   MatchCandidate candidatos[MAX_CANDIDATOS];
   int num_candidatos = 0;

   TemplateParams tpl;
   tpl.core_size   = core_size;
   tpl.margin_size = margin_size;
   tpl.total_size  = core_size + ( 2 * margin_size );

   // Pré-cálculo da energia máxima teórica para O(1) dentro do laço
   int pixels_margem = ( tpl.total_size * tpl.total_size ) - ( core_size * core_size );
   tpl.max_energia   = ( double )pixels_margem * 255.0;

   int raio_nms = ( tpl.total_size * 70 ) / 100;
   int limite_i = ( int )img->nrow - tpl.total_size;
   int limite_j = ( int )img->ncol - tpl.total_size;

   // Acumula diretamente no buffer NMS da imagem atual (sem re-fusão redundante por linha)
   for ( int i = 0; i <= limite_i; i += stride_busca ) {
      for ( int j = 0; j <= limite_j; j += stride_busca ) {

         double score = calcular_correcao_cruzada_direta( img, i, j, &tpl );

         if ( score > limiar_correlacao ) {
            BlobInfo blob;
            blob.min_i = i + tpl.margin_size;
            blob.max_i = i + tpl.margin_size + tpl.core_size - 1;
            blob.min_j = j + tpl.margin_size;
            blob.max_j = j + tpl.margin_size + tpl.core_size - 1;

            blob.area     = tpl.core_size * tpl.core_size;
            blob.centro_i = ( double )( blob.min_i + blob.max_i ) * 0.5;
            blob.centro_j = ( double )( blob.min_j + blob.max_j ) * 0.5;

            adicionar_candidato_nms( candidatos, &num_candidatos, blob, score, raio_nms );
         }
      }
   }

   for ( int k = 0; k < num_candidatos; k++ ) {
      blobs[k] = candidatos[k].blob;
   }

   if ( num_candidatos < 4 ) {
      return 0;
   }

   if ( num_candidatos > max_blobs_saida ) {
      qsort( blobs, ( gsize )num_candidatos, sizeof( BlobInfo ), blob_comparar_solidez );
      num_candidatos = max_blobs_saida;
   }

   return num_candidatos;
}

//-----------------------------------------------------------------------------------------------------
// 1. Função auxiliar inline: Ordenação polar manual para 4 itens
static void blob_ordenar_polar_inline( const BlobInfo *candidatos[4], const BlobInfo *ordenados[4] ) {
   double cx = 0.0;
   double cy = 0.0;

   for ( int i = 0; i < 4; i++ ) {
      cy += candidatos[i]->centro_i;
      cx += candidatos[i]->centro_j;
   }
   cy *= 0.25;
   cx *= 0.25;

   double angulos[4];
   for ( int i = 0; i < 4; i++ ) {
      angulos[i] = atan2( candidatos[i]->centro_i - cy, candidatos[i]->centro_j - cx );
   }

   int idx[4] = { 0, 1, 2, 3 };
   for ( int i = 0; i < 3; i++ ) {
      for ( int j = i + 1; j < 4; j++ ) {
         if ( angulos[idx[i]] > angulos[idx[j]] ) {
            int tmp = idx[i];
            idx[i]  = idx[j];
            idx[j]  = tmp;
         }
      }
   }

   for ( int i = 0; i < 4; i++ ) {
      ordenados[i] = candidatos[idx[i]];
   }
}

/**
 * Calcula o desvio ortogonal das 4 quinas do quadrilátero.
 * O retorno é a média dos cossenos absolutos (0.0 = Retângulo Perfeito, ângulos de 90º).
 */
static double blob_erro_ortogonal( const double p0[2], const double p1[2],
                                   const double p2[2], const double p3[2],
                                   const double top_w, const double bot_w,
                                   const double left_h, const double right_h ) {

   double dp0 = ( p1[0] - p0[0] ) * ( p3[0] - p0[0] ) + ( p1[1] - p0[1] ) * ( p3[1] - p0[1] );
   double dp1 = ( p0[0] - p1[0] ) * ( p2[0] - p1[0] ) + ( p0[1] - p1[1] ) * ( p2[1] - p1[1] );
   double dp2 = ( p1[0] - p2[0] ) * ( p3[0] - p2[0] ) + ( p1[1] - p2[1] ) * ( p3[1] - p2[1] );
   double dp3 = ( p2[0] - p3[0] ) * ( p0[0] - p3[0] ) + ( p2[1] - p3[1] ) * ( p0[1] - p3[1] );

   double cos0 = fabs( dp0 ) / ( top_w * left_h );
   double cos1 = fabs( dp1 ) / ( top_w * right_h );
   double cos2 = fabs( dp2 ) / ( bot_w * right_h );
   double cos3 = fabs( dp3 ) / ( bot_w * left_h );

   return ( cos0 + cos1 + cos2 + cos3 ) * 0.25;
}

/**
 * Separa a âncora quadrada de riscos de caneta através de projeção ortogonal.
 */
static void blob_aparar_riscos( BlobInfo *blob, const int *fila_i, const int *fila_j, int num_pixels,
                                int *proj_x, int *proj_y ) {
   int w = blob->max_j - blob->min_j + 1;
   int h = blob->max_i - blob->min_i + 1;

   if ( w <= 0 || h <= 0 || num_pixels <= 0 ) {
      return;
   }

   memset( proj_x, 0, ( gsize )w * sizeof( int ) );
   memset( proj_y, 0, ( gsize )h * sizeof( int ) );

   int max_px = 0;
   int max_py = 0;

   // 1. Constrói o histograma de densidade
   for ( int k = 0; k < num_pixels; k++ ) {
      int x = fila_j[k] - blob->min_j;
      int y = fila_i[k] - blob->min_i;

      proj_x[x]++;
      proj_y[y]++;

      if ( proj_x[x] > max_px ) {
         max_px = proj_x[x];
      }
      if ( proj_y[y] > max_py ) {
         max_py = proj_y[y];
      }
   }

   // 2. Limiar de Corte (50% da massa máxima projetada via divisão inteira exata)
   int limiar_x = max_px / 2;
   int limiar_y = max_py / 2;

   int novo_min_j = blob->min_j;
   int novo_max_j = blob->max_j;
   int novo_min_i = blob->min_i;
   int novo_max_i = blob->max_i;

   // 3. Amputa as extremidades (Riscos finos)
   for ( int j = 0; j < w; j++ ) {
      if ( proj_x[j] >= limiar_x ) {
         novo_min_j = blob->min_j + j;
         break;
      }
   }
   for ( int j = w - 1; j >= 0; j-- ) {
      if ( proj_x[j] >= limiar_x ) {
         novo_max_j = blob->min_j + j;
         break;
      }
   }
   for ( int i = 0; i < h; i++ ) {
      if ( proj_y[i] >= limiar_y ) {
         novo_min_i = blob->min_i + i;
         break;
      }
   }
   for ( int i = h - 1; i >= 0; i-- ) {
      if ( proj_y[i] >= limiar_y ) {
         novo_max_i = blob->min_i + i;
         break;
      }
   }

   // 4. Consolida o Bounding Box limpo e recalcula
   blob->min_i = novo_min_i;
   blob->max_i = novo_max_i;
   blob->min_j = novo_min_j;
   blob->max_j = novo_max_j;
   blob->area  = 0;

   int64_t soma_i = 0;
   int64_t soma_j = 0;

   for ( int k = 0; k < num_pixels; k++ ) {
      int ci = fila_i[k];
      int cj = fila_j[k];

      if ( ci >= novo_min_i && ci <= novo_max_i && cj >= novo_min_j && cj <= novo_max_j ) {
         blob->area++;
         soma_i += ci;
         soma_j += cj;
      }
   }

   if ( blob->area > 0 ) {
      blob->centro_i = ( double )soma_i / ( double )blob->area;
      blob->centro_j = ( double )soma_j / ( double )blob->area;
   }
}

/**
 * Varre a imagem e extrai candidatos OMR (com inteligência contra riscos)
 */
static int blob_extrair_quadrados_pretos( const ImagemCinza *img_bin, BlobInfo *blobs ) {
   int nrow = ( int )img_bin->nrow;
   int ncol = ( int )img_bin->ncol;
   const int max_blobs_saida = 30;

   gsize total_pixels = img_bin->nrow * img_bin->ncol;
   if ( total_pixels == 0 ) {
      return 0;
   }

   uint8_t *visitado = g_new0( uint8_t, total_pixels );
   int     *fila_i   = g_new( int, total_pixels );
   int     *fila_j   = g_new( int, total_pixels );
   int     *proj_x   = g_new( int, img_bin->ncol );
   int     *proj_y   = g_new( int, img_bin->nrow );

   uint8_t limiar_binario = ( uint8_t )( img_bin->max / 2 );
   int num_blobs = 0;

   for ( int i = 0; i < nrow; i++ ) {
      for ( int j = 0; j < ncol; j++ ) {
         gsize idx_raiz = ( ( gsize )i * img_bin->ncol ) + ( gsize )j;

         if ( img_bin->dados[idx_raiz] < limiar_binario && !visitado[idx_raiz] ) {
            int start_idx = 0;
            int end_idx   = 0;

            fila_i[end_idx]    = i;
            fila_j[end_idx]    = j;
            end_idx++;
            visitado[idx_raiz] = 1;

            BlobInfo blob = { i, i, j, j, 0, 0.0, 0.0 };

            while ( start_idx < end_idx ) {
               int ci = fila_i[start_idx];
               int cj = fila_j[start_idx];
               start_idx++;

               if ( ci < blob.min_i ) {
                  blob.min_i = ci;
               }
               if ( ci > blob.max_i ) {
                  blob.max_i = ci;
               }
               if ( cj < blob.min_j ) {
                  blob.min_j = cj;
               }
               if ( cj > blob.max_j ) {
                  blob.max_j = cj;
               }

               for ( int di = -1; di <= 1; di++ ) {
                  int ni = ci + di;
                  if ( ni < 0 || ni >= nrow ) {
                     continue;
                  }

                  for ( int dj = -1; dj <= 1; dj++ ) {
                     int nj = cj + dj;
                     if ( nj >= 0 && nj < ncol ) {
                        gsize idx_viz = ( ( gsize )ni * img_bin->ncol ) + ( gsize )nj;
                        if ( img_bin->dados[idx_viz] < limiar_binario && !visitado[idx_viz] ) {
                           fila_i[end_idx]   = ni;
                           fila_j[end_idx]   = nj;
                           end_idx++;
                           visitado[idx_viz] = 1;
                        }
                     }
                  }
               }
            }

            blob_aparar_riscos( &blob, fila_i, fila_j, end_idx, proj_x, proj_y );

            double altura       = ( double )( blob.max_i - blob.min_i + 1 );
            double largura      = ( double )( blob.max_j - blob.min_j + 1 );
            double aspect_ratio = largura / altura;
            double fill_ratio   = ( double )blob.area / ( largura * altura );

            // Filtro rigoroso: Apenas Quadrados Sólidos (Descarta bolhas de 6mm cujo limite natural é ~0.785)
            if ( blob.area > 50 && blob.area < 6000 &&
                 aspect_ratio >= 0.5 && aspect_ratio <= 2.0 && fill_ratio > 0.6 ) {
               if ( num_blobs < MAX_CANDIDATOS ) {
                  blobs[num_blobs++] = blob;
               }
            }
         }
      }
   }

   // Desalocação imediata após a varredura de pixels (evita falsos positivos do Dead Symbol Reaper no Clang)
   g_free( proj_y );
   g_free( proj_x );
   g_free( fila_j );
   g_free( fila_i );
   g_free( visitado );

   if ( num_blobs < 4 ) {
      return 0;
   }

   if ( num_blobs > max_blobs_saida ) {
      qsort( blobs, ( gsize )num_blobs, sizeof( BlobInfo ), blob_comparar_solidez );
      num_blobs = max_blobs_saida;
   }

   return num_blobs;
}

//------------------------------------------------------------------------------------------------------------
static gboolean blob_busca_combinatoria( const ImagemCinza *img_bin, BlobInfo ancora_final[4],
                                         char *direcao_inferida, gboolean resgate ) {
   double nrow_d        = ( double )img_bin->nrow;
   double ncol_d        = ( double )img_bin->ncol;
   double max_dist_quad = ( nrow_d * nrow_d ) + ( ncol_d * ncol_d );

   BlobInfo blobs[MAX_CANDIDATOS] = { 0 };
   int num_blobs = 0;

   if ( resgate ) {
      // Valores otimizados para quadradinhos de 6 milímetros de lado
      int core_size   = 28;
      int margin_size = 7;
      num_blobs       = blob_extrair_por_template( img_bin, blobs, core_size, margin_size );
   } else {
      num_blobs = blob_extrair_quadrados_pretos( img_bin, blobs );
   }

   double menor_erro_global = 1e9;
   gboolean encontrou_padrao = FALSE;

   for ( int i = 0; i < num_blobs - 3; i++ ) {
      for ( int j = i + 1; j < num_blobs - 2; j++ ) {
         for ( int k = j + 1; k < num_blobs - 1; k++ ) {
            for ( int l = k + 1; l < num_blobs; l++ ) {

               double a0 = ( double )blobs[i].area;
               double a1 = ( double )blobs[j].area;
               double a2 = ( double )blobs[k].area;
               double a3 = ( double )blobs[l].area;

               double max_a = fmax( fmax( a0, a1 ), fmax( a2, a3 ) );
               double min_a = fmin( fmin( a0, a1 ), fmin( a2, a3 ) );
               if ( max_a / min_a > 2.0 ) {
                  continue;
               }

               const BlobInfo *candidatos[4] = { &blobs[i], &blobs[j], &blobs[k], &blobs[l] };
               const BlobInfo *ord[4];
               blob_ordenar_polar_inline( candidatos, ord );

               // Trava de Rotação (Máx 10 graus)
               double dx_top           = ord[1]->centro_j - ord[0]->centro_j;
               double dy_top           = ord[1]->centro_i - ord[0]->centro_i;
               double angulo_top_graus = fabs( atan2( dy_top, dx_top ) * ( 180.0 / G_PI ) );

               if ( angulo_top_graus > 10.0 && fabs( angulo_top_graus - 90.0 ) > 10.0 ) {
                  continue;
               }

               double p0[2] = { ( double )ord[0]->min_j, ( double )ord[0]->min_i };
               double p1[2] = { ( double )ord[1]->max_j, ( double )ord[1]->min_i };
               double p2[2] = { ( double )ord[2]->max_j, ( double )ord[2]->max_i };
               double p3[2] = { ( double )ord[3]->min_j, ( double )ord[3]->max_i };

               double top_w   = hypot( p1[0] - p0[0], p1[1] - p0[1] );
               double bot_w   = hypot( p2[0] - p3[0], p2[1] - p3[1] );
               double left_h  = hypot( p3[0] - p0[0], p3[1] - p0[1] );
               double right_h = hypot( p2[0] - p1[0], p2[1] - p1[1] );

               double larg = ( top_w + bot_w ) * 0.5;
               double alt  = ( left_h + right_h ) * 0.5;

               if ( larg < 50.0 || alt < 50.0 ) {
                  continue;
               }

               double erro_paralelogramo = ( fabs( top_w - bot_w ) / larg ) + ( fabs( left_h - right_h ) / alt );
               if ( erro_paralelogramo > 0.25 ) {
                  continue;
               }

               double erro_ortogonal = blob_erro_ortogonal( p0, p1, p2, p3, top_w, bot_w, left_h, right_h );
               if ( erro_ortogonal > 0.20 ) {
                  continue;
               }

               int soma_alt  = -ord[0]->min_i - ord[1]->min_i + ord[2]->max_i + ord[3]->max_i;
               int soma_larg = -ord[0]->min_j + ord[1]->max_j + ord[2]->max_j - ord[3]->min_j;
               char dir      = ( soma_alt < soma_larg ) ? 'h' : 'v';

               double proporcao_alvo = ( dir == 'h' ) ? ( 14.0 / 11.0 ) : ( 10.0 / 15.0 );
               double proporcao_real = larg / alt;
               double erro_proporcao = fabs( proporcao_real - proporcao_alvo ) / proporcao_alvo;

               double erro_area_ancoras = ( max_a - min_a ) / max_a;

               double ci_A = ord[0]->centro_i, cj_A = ord[0]->centro_j;
               double ci_B = ord[1]->centro_i, cj_B = ord[1]->centro_j;
               double ci_C = ord[2]->centro_i, cj_C = ord[2]->centro_j;
               double ci_D = ord[3]->centro_i, cj_D = ord[3]->centro_j;

               double d_tl = ( ci_A * ci_A ) + ( cj_A * cj_A );
               double d_tr = ( ci_B * ci_B ) + ( ( ncol_d - cj_B ) * ( ncol_d - cj_B ) );
               double d_br = ( ( nrow_d - ci_C ) * ( nrow_d - ci_C ) ) + ( ( ncol_d - cj_C ) * ( ncol_d - cj_C ) );
               double d_bl = ( ( nrow_d - ci_D ) * ( nrow_d - ci_D ) ) + ( cj_D * cj_D );

               double erro_extremos = ( d_tl + d_tr + d_br + d_bl ) / max_dist_quad;

               double erro_total = erro_proporcao + erro_paralelogramo + erro_ortogonal +
                                   ( erro_area_ancoras * 0.5 ) + erro_extremos;

               if ( erro_total < menor_erro_global ) {
                  menor_erro_global = erro_total;
                  encontrou_padrao  = TRUE;
                  *direcao_inferida = dir;

                  ancora_final[0] = *ord[0];
                  ancora_final[1] = *ord[1];
                  ancora_final[2] = *ord[2];
                  ancora_final[3] = *ord[3];
               }
            }
         }
      }
   }
   return encontrou_padrao;
}

/**
 * Pipeline OMR Determinístico Atualizado.
 */
gboolean detectar_ancoras_omr( const ImagemCinza *img_bin, IndiceMatriz ancora[4], char *direcao_inferida, gboolean resgate ) {
   g_return_val_if_fail( img_bin != NULL, FALSE );
   g_return_val_if_fail( img_bin->dados != NULL, FALSE );
   g_return_val_if_fail( ancora != NULL && direcao_inferida != NULL, FALSE );

   BlobInfo melhores_blobs[4];

   if ( !blob_busca_combinatoria( img_bin, melhores_blobs, direcao_inferida, resgate ) ) {
      return FALSE;
   }

   // Construção do Quadrilátero Envolvente (Limites externos da âncora já limpos do ruído de caneta)
   ancora[0].i = melhores_blobs[0].min_i;
   ancora[0].j = melhores_blobs[0].min_j;

   ancora[1].i = melhores_blobs[1].min_i;
   ancora[1].j = melhores_blobs[1].max_j;

   ancora[2].i = melhores_blobs[2].max_i;
   ancora[2].j = melhores_blobs[2].max_j;

   ancora[3].i = melhores_blobs[3].max_i;
   ancora[3].j = melhores_blobs[3].min_j;

   return TRUE;
}




/*
* Função para verificar a integridade dos 27 bits extraídos da imagem.
* Retorna TRUE se a leitura estiver íntegra, FALSE se houve rasura/ruído.
*/
gboolean verificar_paridade_matriz( uint32_t payload_lido ) {
   // Isola os 23 bits de dados (Máscara 0x7FFFFF)
   uint32_t dados = payload_lido & 0x7FFFFF;

   // Isola os 4 bits de paridade que estão nas posições 23 a 26
   uint32_t paridade_esperada = ( payload_lido >> 23 ) & 0x0F;

   uint32_t paridade_calculada = 0;

   // Recalcula o XOR em blocos de 4 bits
   while ( dados > 0 ) {
      paridade_calculada ^= ( dados & 0x0F );
      dados >>= 4;
   }

   return ( paridade_calculada == paridade_esperada );
}

/*
* Verifica a integridade e extrai os identificadores originais a partir do payload lido.
* Retorna TRUE se a imagem estava íntegra e os dados foram extraídos com sucesso,
* ou FALSE se a verificação de paridade falhar.
*/
gboolean decodificar_payload_matriz( MapeamentoGabarito *map, const InterfaceCabecalho *cabecalho ) {
   // 1. Validação defensiva dos ponteiros de saída usando macros da GLib
   g_return_val_if_fail( map != NULL, FALSE );
   g_return_val_if_fail( cabecalho != NULL, FALSE );

   // 2. Executa o teste de integridade via checksum/paridade
   if ( !verificar_paridade_matriz( map->payload ) ) {
      // g_warning("Falha na integridade do payload lido: paridade invalida.");
      return FALSE;
   }

   // 3. Desempacotamento (Bitwise Shift reverso e aplicação de máscaras)
   map->id    = ( guint8 )( map->payload & 0x3F );      // Isola os bits 0 a 5
   map->turma = ( guint8 )( ( map->payload >> 6 )  & 0xFF ); // Desloca 6 bits e isola 8 bits (6 a 13)
   map->disc  = ( guint8 )( ( map->payload >> 14 ) & 0x0F ); // Desloca 14 bits e isola 4 bits (14 a 17)
   map->per   = ( guint8 )( ( map->payload >> 18 ) & 0x07 ); // Desloca 18 bits e isola 3 bits (18 a 20)
   map->seq   = ( guint8 )( ( map->payload >> 21 ) & 0x03 ); // Desloca 21 bits e isola 2 bits (21 a 22)

   // A sequência da prova é o único valor que não pode ser zero (valores 1, 2 e 3)
   // Pois a leitura do binário numa área branca retornava um payload válido onde todos os bits são zero
   // Mudando a sequência da prova de 0, 1, 2 para 1, 2, 3 elimina essa vulnerabilidade.
   if ( map->turma >= cabecalho->limite_turmas || map->per + 1 >= 5 || map->seq == 0 ) {
      return FALSE;
   }

   return TRUE;
}







// ============================================================================
// MÓDULO 5: Extração do Payload Binário (27 bits - Grade 3x9)
// ============================================================================
/**
 * Extrai o payload de 27 bits armazenado na grade 3x9 da imagem PGM binarizada.
 * Suporta orientações horizontais (14p x 11p) e verticais (10p x 15p).
 */
uint32_t extrair_payload_matriz( const ImagemCinza *IMG, char direcao ) {
   // 1. Validações defensivas robustas via GLib
   g_return_val_if_fail( IMG != NULL && IMG->dados != NULL && IMG->image != NULL, 0 );
   g_return_val_if_fail( direcao == 'h' || direcao == 'v', 0 );

   uint32_t payload_lido = 0;
   double p_x, p_y;

   // 2. Define dinamicamente as proporções de 'p' com base na orientação
   if ( direcao == 'h' ) {
      p_x = ( double )IMG->ncol / 14.0;
      p_y = ( double )IMG->nrow / 11.0;
   } else { // 'v'
      p_x = ( double )IMG->ncol / 10.0;
      p_y = ( double )IMG->nrow / 15.0;
   }

   // Ponto de corte para separar branco e preto
   uint8_t limiar_binario = ( uint8_t )( IMG->max / 2 );

   // Kernel dinâmico ajustado para cobrir o miolo seguro de cada célula
   gssize raio_x = MAX( 1, ( gssize )( ( p_x / 3.0 ) * 0.8 ) );
   gssize raio_y = MAX( 1, ( gssize )( ( p_y / 3.0 ) * 0.8 ) );

   gssize ncol_s = ( gssize )IMG->ncol;
   gssize nrow_s = ( gssize )IMG->nrow;
   const double passo_grade = 2.0 / 3.0;

   // 3. Varredura dos 27 bits do payload (guint evita -Wsign-conversion no bit shift)
   for ( guint i = 0; i < 27U; i++ ) {

      guint linha  = ( direcao == 'h' ) ? ( i / 3U ) : ( i % 3U );
      guint coluna = ( direcao == 'h' ) ? ( i % 3U ) : ( i / 3U );

      double x_centro_p = 1.0 + ( ( double )coluna * passo_grade ) + ( passo_grade * 0.5 );
      double y_centro_p = 1.0 + ( ( double )linha  * passo_grade ) + ( passo_grade * 0.5 );

      // Converte a unidade fracionária 'p' para a coordenada real de pixels
      gssize px = ( gssize )( x_centro_p * p_x );
      gssize py = ( gssize )( y_centro_p * p_y );

      // Garante que o centro calculado está dentro dos limites físicos da imagem
      if ( px >= 0 && px < ncol_s && py >= 0 && py < nrow_s ) {
         int pixels_pretos = 0;
         int cont_pixels = 0;

         // 4. Votação usando o Kernel Dinâmico com ponteiro de linha em cache
         for ( gssize dy = -raio_y; dy <= raio_y; dy++ ) {
            gssize ny = py + dy;
            if ( ny < 0 || ny >= nrow_s ) {
               continue;
            }

            const uint8_t *linha_img = IMG->image[ny];

            for ( gssize dx = -raio_x; dx <= raio_x; dx++ ) {
               gssize nx = px + dx;

               if ( nx >= 0 && nx < ncol_s ) {
                  if ( linha_img[nx] < limiar_binario ) {
                     pixels_pretos++;
                  }
                  cont_pixels++;
               }
            }
         }

         // Valida o bit se a densidade de tinta no miolo atingir o limiar
         if ( pixels_pretos > ( cont_pixels / 3 ) ) {
            payload_lido |= ( 1U << i );
         }
      }
   }

   return payload_lido;
}

// ============================================================================
// MÓDULO 6: Leitura das Bolhas de Resposta e Identificação do Aluno
// ============================================================================
static float calcular_densidade_celula( const ImagemCinza *IMG, int i_celula, int j_celula,
                                        float p_x, float p_y, float raio, float raio_quad ) {
   float cx = ( ( float )j_celula + 0.5f ) * p_x;
   float cy = ( ( float )i_celula + 0.5f ) * p_y;

   gssize ncol_s = ( gssize )IMG->ncol;
   gssize nrow_s = ( gssize )IMG->nrow;

   gssize x_min = ( gssize )floorf( cx - raio );
   gssize x_max = ( gssize )ceilf( cx + raio );
   gssize y_min = ( gssize )floorf( cy - raio );
   gssize y_max = ( gssize )ceilf( cy + raio );

   if ( x_min < 0 ) {
      x_min = 0;
   }
   if ( x_max >= ncol_s ) {
      x_max = ncol_s - 1;
   }
   if ( y_min < 0 ) {
      y_min = 0;
   }
   if ( y_max >= nrow_s ) {
      y_max = nrow_s - 1;
   }

   int total_pixels = 0;
   int pixels_pretos = 0;
   uint8_t limiar_binario = ( uint8_t )( IMG->max / 2 );

   for ( gssize y = y_min; y <= y_max; y++ ) {
      float dy = ( float )y - cy;
      float dy_quad = dy * dy;
      const uint8_t *linha_img = IMG->image[y];

      for ( gssize x = x_min; x <= x_max; x++ ) {
         float dx = ( float )x - cx;

         if ( ( ( dx * dx ) + dy_quad ) <= raio_quad ) {
            total_pixels++;
            if ( linha_img[x] < limiar_binario ) {
               pixels_pretos++;
            }
         }
      }
   }

   return ( total_pixels > 0 ) ? ( ( float )pixels_pretos / ( float )total_pixels ) : 0.0f;
}

/**
 * Escaneia a matriz de pixels e detecta a alternativa assinalada em cada questão.
 */
void ler_respostas_gabarito( const ImagemCinza *IMG, char direcao, char *respostas_out ) {
   g_return_if_fail( IMG != NULL && IMG->dados != NULL && IMG->image != NULL );
   g_return_if_fail( respostas_out != NULL );
   g_return_if_fail( direcao == 'h' || direcao == 'v' );

   // 1. Definição da malha matricial com base na orientação
   int cols_grid = ( direcao == 'h' ) ? 14 : 10;
   int rows_grid = ( direcao == 'h' ) ? 11 : 15;

   // 2. Tamanho da unidade 'p' em pixels nos eixos X e Y
   float p_x = ( float )IMG->ncol / ( float )cols_grid;
   float p_y = ( float )IMG->nrow / ( float )rows_grid;

   // Para manter a área de busca perfeitamente circular, usamos o menor dos dois 'p'
   float p_min = ( p_x < p_y ) ? p_x : p_y;
   float raio = 0.35f * p_min;
   float raio_quad = raio * raio;

   // 3. Varredura das 10 questões
   for ( int q = 0; q < 10; q++ ) {

      float max_densidade = -1.0f;
      int alt_marcada = -1; // 0=A, 1=B, 2=C, 3=D, 4=E

      // Varredura das 5 alternativas para a questão atual
      for ( int alt = 0; alt < 5; alt++ ) {
         int i_celula = ( direcao == 'h' ) ? ( alt + 2 ) : ( q + 3 );
         int j_celula = ( direcao == 'h' ) ? ( q + 3 )   : ( alt + 2 );

         float densidade = calcular_densidade_celula( IMG, i_celula, j_celula, p_x, p_y, raio, raio_quad );

         if ( densidade > max_densidade ) {
            max_densidade = densidade;
            alt_marcada = alt;
         }
      }

      // 4. Critério de Aceitação (Mínimo de 50% de preenchimento)
      if ( max_densidade > 0.50f && alt_marcada >= 0 ) {
         respostas_out[q] = ( char )( 'A' + alt_marcada );
      } else {
         respostas_out[q] = '-';
      }
   }

   respostas_out[10] = '\0';
}

/**
 * Lê o número de chamada do aluno (dezena 0..6 e unidade 0..9) na matriz OMR.
 */
int ler_numero_aluno( const ImagemCinza *IMG, char direcao ) {
   g_return_val_if_fail( IMG != NULL && IMG->dados != NULL && IMG->image != NULL, -1 );
   g_return_val_if_fail( direcao == 'h' || direcao == 'v', -1 );

   int cols_grid = ( direcao == 'h' ) ? 14 : 10;
   int rows_grid = ( direcao == 'h' ) ? 11 : 15;

   float p_x = ( float )IMG->ncol / ( float )cols_grid;
   float p_y = ( float )IMG->nrow / ( float )rows_grid;

   float p_min = ( p_x < p_y ) ? p_x : p_y;
   float raio = 0.35f * p_min;
   float raio_quad = raio * raio;

   int dezena_marcada = -1;
   float max_den_dezena = -1.0f;

   // Varredura da DEZENA (0 a 6)
   for ( int val = 0; val <= 6; val++ ) {
      int i_cel = ( direcao == 'h' ) ? 7 : ( 3 + val );
      int j_cel = ( direcao == 'h' ) ? ( 3 + val ) : 7;

      float densidade = calcular_densidade_celula( IMG, i_cel, j_cel, p_x, p_y, raio, raio_quad );

      if ( densidade > max_den_dezena ) {
         max_den_dezena = densidade;
         dezena_marcada = val;
      }
   }

   int unidade_marcada = -1;
   float max_den_unidade = -1.0f;

   // Varredura da UNIDADE (0 a 9)
   for ( int val = 0; val <= 9; val++ ) {
      int i_cel = ( direcao == 'h' ) ? 8 : ( 3 + val );
      int j_cel = ( direcao == 'h' ) ? ( 3 + val ) : 8;

      float densidade = calcular_densidade_celula( IMG, i_cel, j_cel, p_x, p_y, raio, raio_quad );

      if ( densidade > max_den_unidade ) {
         max_den_unidade = densidade;
         unidade_marcada = val;
      }
   }

   // Limiar de segurança em 50%: aciona alerta de revisão se houver falha ou rasura fraca
   if ( max_den_dezena <= 0.50f || max_den_unidade <= 0.50f ) {
      return -1;
   }

   return ( dezena_marcada * 10 ) + unidade_marcada;
}

