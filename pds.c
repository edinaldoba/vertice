/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdio.h>
#include <math.h>
#include <omp.h>

#include "glib.h"
#include "glibconfig.h"
#include "matriz.h"
#include "pds.h"
#include "imgcore.h"






// ====================================================================================
// ESTRUTURA DE COMUNICAÇÃO (O "Contrato" Geométrico)
// ====================================================================================
typedef struct {
   float x_origem;
   float y_origem;
   float ux; // Passo horizontal X
   float uy; // Passo horizontal Y
   float vx; // Passo vertical X
   float vy; // Passo vertical Y
   gsize largura_destino;
   gsize altura_destino;
} MapeamentoAfim;

// ====================================================================================
// HELPERS GEOMÉTRICOS (Evitam duplicação entre os motores Cinza e RGB)
// ====================================================================================
static inline gboolean calcular_mapa_afim_ancoras( const IndiceMatriz *ancora, MapeamentoAfim *mapa ) {
   g_return_val_if_fail( ancora != NULL && mapa != NULL, FALSE );

   float x_B = ( float )ancora[1].j, y_B = ( float )ancora[1].i;
   float x_C = ( float )ancora[2].j, y_C = ( float )ancora[2].i;
   float x_D = ( float )ancora[3].j, y_D = ( float )ancora[3].i;

   float dx_DC = x_C - x_D;
   float dy_DC = y_C - y_D;
   float dx_BC = x_C - x_B;
   float dy_BC = y_C - y_B;

   mapa->largura_destino = ( gsize )roundf( hypotf( dx_DC, dy_DC ) );
   mapa->altura_destino  = ( gsize )roundf( hypotf( dx_BC, dy_BC ) );

   if ( mapa->largura_destino == 0 || mapa->altura_destino == 0 ) {
      return FALSE;
   }

   float inv_larg = 1.0f / ( float )mapa->largura_destino;
   float inv_alt  = 1.0f / ( float )mapa->altura_destino;

   mapa->x_origem = x_B + x_D - x_C;
   mapa->y_origem = y_B + y_D - y_C;

   mapa->ux = dx_DC * inv_larg;
   mapa->uy = dy_DC * inv_larg;
   mapa->vx = dx_BC * inv_alt;
   mapa->vy = dy_BC * inv_alt;

   return TRUE;
}

static inline gboolean calcular_mapa_afim_rotacao( gsize ncol, gsize nrow, float angulo_graus, MapeamentoAfim *mapa ) {
   g_return_val_if_fail( ncol > 0 && nrow > 0 && mapa != NULL, FALSE );

   // Matemática 100% em precisão simples (float) para evitar -Wdouble-promotion
   float rad   = angulo_graus * ( ( float )G_PI / 180.0f );
   float cos_a = cosf( rad );
   float sin_a = sinf( rad );

   float ncol_f = ( float )ncol;
   float nrow_f = ( float )nrow;

   mapa->largura_destino = ( gsize )roundf( ( fabsf( ncol_f * cos_a ) + fabsf( nrow_f * sin_a ) ) );
   mapa->altura_destino  = ( gsize )roundf( ( fabsf( nrow_f * cos_a ) + fabsf( ncol_f * sin_a ) ) );

   if ( mapa->largura_destino == 0 || mapa->altura_destino == 0 ) {
      return FALSE;
   }

   mapa->ux =  cos_a;
   mapa->uy = -sin_a;
   mapa->vx =  sin_a;
   mapa->vy =  cos_a;

   float meia_larg_dst = ( float )mapa->largura_destino * 0.5f;
   float meia_alt_dst  = ( float )mapa->altura_destino  * 0.5f;

   mapa->x_origem = ( ncol_f * 0.5f ) - ( meia_larg_dst * mapa->ux ) - ( meia_alt_dst * mapa->vx );
   mapa->y_origem = ( nrow_f * 0.5f ) - ( meia_larg_dst * mapa->uy ) - ( meia_alt_dst * mapa->vy );

   return TRUE;
}

// ====================================================================================
// MOTOR GRÁFICO EM TONS DE CINZA (Interpolação Bilinear Paralelizada)
// ====================================================================================
static void aplicar_transformacao_afim( const ImagemCinza *IMG, ImagemCinza *img, const MapeamentoAfim *mapa ) {
   g_return_if_fail( IMG != NULL && IMG->dados != NULL && IMG->image != NULL );
   g_return_if_fail( img != NULL && mapa != NULL );
   g_return_if_fail( mapa->largura_destino > 0 && mapa->altura_destino > 0 );

   // 1. Limpeza e Realocação no Padrão Contíguo (1D real + 2D pointers)
   if ( img->dados ) {
      g_free( img->dados );
      g_free( img->image );
   } else if ( img->image ) {
      liberar_matriz_pixels( img->image, img->nrow );
   }

   img->ncol = mapa->largura_destino;
   img->nrow = mapa->altura_destino;

   img->dados = g_malloc( img->nrow * img->ncol * sizeof( uint8_t ) );
   img->image = g_malloc( img->nrow * sizeof( uint8_t * ) );
   for ( gsize i = 0; i < img->nrow; i++ ) {
      img->image[i] = img->dados + ( i * img->ncol );
   }

   g_strlcpy( img->key, IMG->key, sizeof( img->key ) );
   img->max = IMG->max;

   gssize x_lim = ( gssize )IMG->ncol - 1;
   gssize y_lim = ( gssize )IMG->nrow - 1;

   #pragma omp parallel for schedule(static) if( !omp_in_parallel() )
   for ( gsize y_novo = 0; y_novo < img->nrow; y_novo++ ) {
      // OTIMIZAÇÃO: Coordenadas base da linha e ponteiro de escrita içados para fora do laço X
      float y_f = ( float )y_novo;
      float x_linha_base = mapa->x_origem + ( y_f * mapa->vx );
      float y_linha_base = mapa->y_origem + ( y_f * mapa->vy );
      uint8_t *linha_dest = img->image[y_novo];

      for ( gsize x_novo = 0; x_novo < img->ncol; x_novo++ ) {
         float x_f = ( float )x_novo;
         float x_orig = x_linha_base + ( x_f * mapa->ux );
         float y_orig = y_linha_base + ( x_f * mapa->uy );

         gssize x_base = ( gssize )floorf( x_orig );
         gssize y_base = ( gssize )floorf( y_orig );

         float dx = x_orig - ( float )x_base;
         float dy = y_orig - ( float )y_base;
         float dx_inv = 1.0f - dx;
         float dy_inv = 1.0f - dy;

         uint8_t pA = 255, pB = 255, pC = 255, pD = 255;

         // [FAST PATH] - Pixel totalmente dentro da imagem de origem
         if ( x_base >= 0 && x_base < x_lim && y_base >= 0 && y_base < y_lim ) {
            const uint8_t *linha_y0 = IMG->image[y_base];
            const uint8_t *linha_y1 = IMG->image[y_base + 1];

            pA = linha_y0[x_base];
            pB = linha_y0[x_base + 1];
            pC = linha_y1[x_base];
            pD = linha_y1[x_base + 1];
         } else {
            // [SLOW PATH] - Tratamento individual das bordas
            if ( x_base >= 0 && x_base <= x_lim && y_base >= 0 && y_base <= y_lim ) {
               pA = IMG->image[y_base][x_base];
            }
            if ( x_base + 1 >= 0 && x_base + 1 <= x_lim && y_base >= 0 && y_base <= y_lim ) {
               pB = IMG->image[y_base][x_base + 1];
            }
            if ( x_base >= 0 && x_base <= x_lim && y_base + 1 >= 0 && y_base + 1 <= y_lim ) {
               pC = IMG->image[y_base + 1][x_base];
            }
            if ( x_base + 1 >= 0 && x_base + 1 <= x_lim && y_base + 1 >= 0 && y_base + 1 <= y_lim ) {
               pD = IMG->image[y_base + 1][x_base + 1];
            }
         }

         float interpolado = ( ( float )pA * dx_inv * dy_inv ) +
                             ( ( float )pB * dx     * dy_inv ) +
                             ( ( float )pC * dx_inv * dy ) +
                             ( ( float )pD * dx     * dy );

         linha_dest[x_novo] = ( uint8_t )( interpolado + 0.5f );
      }
   }
}

// ====================================================================================
// MOTOR GRÁFICO RGB COLORIDO (Interpolação Bilinear Paralelizada)
// ====================================================================================
static void aplicar_transformacao_afim_colorida( const ImagemColorida *IMG, ImagemColorida *img, const MapeamentoAfim *mapa ) {
   g_return_if_fail( IMG != NULL && IMG->dados != NULL && IMG->image != NULL );
   g_return_if_fail( img != NULL && mapa != NULL );
   g_return_if_fail( mapa->largura_destino > 0 && mapa->altura_destino > 0 );

   // 1. Limpeza e Realocação no Padrão Contíguo (1D real + 2D pointers)
   if ( img->dados ) {
      g_free( img->dados );
      g_free( img->image );
   } else if ( img->image ) {
      liberar_matriz_pixels_colorida( img->image, img->nrow );
   }

   img->ncol = mapa->largura_destino;
   img->nrow = mapa->altura_destino;

   img->dados = g_malloc( img->nrow * img->ncol * sizeof( PixelRGB ) );
   img->image = g_malloc( img->nrow * sizeof( PixelRGB * ) );
   for ( gsize i = 0; i < img->nrow; i++ ) {
      img->image[i] = img->dados + ( i * img->ncol );
   }

   g_strlcpy( img->key, IMG->key, sizeof( img->key ) );
   img->max = IMG->max;

   gssize x_lim = ( gssize )IMG->ncol - 1;
   gssize y_lim = ( gssize )IMG->nrow - 1;

   #pragma omp parallel for schedule(static) if( !omp_in_parallel() )
   for ( gsize y_novo = 0; y_novo < img->nrow; y_novo++ ) {
      float y_f = ( float )y_novo;
      float x_linha_base = mapa->x_origem + ( y_f * mapa->vx );
      float y_linha_base = mapa->y_origem + ( y_f * mapa->vy );
      PixelRGB *linha_dest = img->image[y_novo];

      for ( gsize x_novo = 0; x_novo < img->ncol; x_novo++ ) {
         float x_f = ( float )x_novo;
         float x_orig = x_linha_base + ( x_f * mapa->ux );
         float y_orig = y_linha_base + ( x_f * mapa->uy );

         gssize x_base = ( gssize )floorf( x_orig );
         gssize y_base = ( gssize )floorf( y_orig );

         float dx = x_orig - ( float )x_base;
         float dy = y_orig - ( float )y_base;
         float dx_inv = 1.0f - dx;
         float dy_inv = 1.0f - dy;

         PixelRGB pA = { 255, 255, 255 };
         PixelRGB pB = { 255, 255, 255 };
         PixelRGB pC = { 255, 255, 255 };
         PixelRGB pD = { 255, 255, 255 };

         // [FAST PATH] - Interior da imagem
         if ( x_base >= 0 && x_base < x_lim && y_base >= 0 && y_base < y_lim ) {
            const PixelRGB *linha_y0 = IMG->image[y_base];
            const PixelRGB *linha_y1 = IMG->image[y_base + 1];

            pA = linha_y0[x_base];
            pB = linha_y0[x_base + 1];
            pC = linha_y1[x_base];
            pD = linha_y1[x_base + 1];
         } else {
            // [SLOW PATH] - Borda
            if ( x_base >= 0 && x_base <= x_lim && y_base >= 0 && y_base <= y_lim ) {
               pA = IMG->image[y_base][x_base];
            }
            if ( x_base + 1 >= 0 && x_base + 1 <= x_lim && y_base >= 0 && y_base <= y_lim ) {
               pB = IMG->image[y_base][x_base + 1];
            }
            if ( x_base >= 0 && x_base <= x_lim && y_base + 1 >= 0 && y_base + 1 <= y_lim ) {
               pC = IMG->image[y_base + 1][x_base];
            }
            if ( x_base + 1 >= 0 && x_base + 1 <= x_lim && y_base + 1 >= 0 && y_base + 1 <= y_lim ) {
               pD = IMG->image[y_base + 1][x_base + 1];
            }
         }

         float pesoA = dx_inv * dy_inv;
         float pesoB = dx     * dy_inv;
         float pesoC = dx_inv * dy;
         float pesoD = dx     * dy;

         float r_interp = ( ( float )pA.r * pesoA ) + ( ( float )pB.r * pesoB ) +
                          ( ( float )pC.r * pesoC ) + ( ( float )pD.r * pesoD );
         float g_interp = ( ( float )pA.g * pesoA ) + ( ( float )pB.g * pesoB ) +
                          ( ( float )pC.g * pesoC ) + ( ( float )pD.g * pesoD );
         float b_interp = ( ( float )pA.b * pesoA ) + ( ( float )pB.b * pesoB ) +
                          ( ( float )pC.b * pesoC ) + ( ( float )pD.b * pesoD );

         linha_dest[x_novo].r = ( uint8_t )( r_interp + 0.5f );
         linha_dest[x_novo].g = ( uint8_t )( g_interp + 0.5f );
         linha_dest[x_novo].b = ( uint8_t )( b_interp + 0.5f );
      }
   }
}

// ====================================================================================
// INTERFACES PÚBLICAS DE RECORTE E ROTAÇÃO AFIM
// ====================================================================================
void extrair_regiao_por_ancoras( const ImagemCinza *IMG, ImagemCinza *img, const IndiceMatriz *ancora ) {
   g_return_if_fail( IMG != NULL && img != NULL && ancora != NULL );

   MapeamentoAfim mapa;
   if ( calcular_mapa_afim_ancoras( ancora, &mapa ) ) {
      aplicar_transformacao_afim( IMG, img, &mapa );
   }
}

void extrair_regiao_colorida_por_ancoras( const ImagemColorida *IMG, ImagemColorida *img, const IndiceMatriz *ancora ) {
   g_return_if_fail( IMG != NULL && img != NULL && ancora != NULL );

   MapeamentoAfim mapa;
   if ( calcular_mapa_afim_ancoras( ancora, &mapa ) ) {
      aplicar_transformacao_afim_colorida( IMG, img, &mapa );
   }
}

void rotacionar_imagem( const ImagemCinza *IMG, ImagemCinza *img, float angulo_graus ) {
   g_return_if_fail( IMG != NULL && img != NULL );

   MapeamentoAfim mapa;
   if ( calcular_mapa_afim_rotacao( IMG->ncol, IMG->nrow, angulo_graus, &mapa ) ) {
      aplicar_transformacao_afim( IMG, img, &mapa );
   }
}

void rotacionar_imagem_colorida( const ImagemColorida *IMG, ImagemColorida *img, float angulo_graus ) {
   g_return_if_fail( IMG != NULL && img != NULL );

   MapeamentoAfim mapa;
   if ( calcular_mapa_afim_rotacao( IMG->ncol, IMG->nrow, angulo_graus, &mapa ) ) {
      aplicar_transformacao_afim_colorida( IMG, img, &mapa );
   }
}






void cortar_imagem_bilinear( const ImagemCinza *IMG, ImagemCinza *img, const IndiceMatriz *ancora ) {
   g_return_if_fail( IMG != NULL && img != NULL && ancora != NULL );

   // Coordenadas dos 3 pontos âncoras de base (B, C e D)
   float x_B = ( float )ancora[1].j, y_B = ( float )ancora[1].i;
   float x_C = ( float )ancora[2].j, y_C = ( float )ancora[2].i;
   float x_D = ( float )ancora[3].j, y_D = ( float )ancora[3].i;

   // 1. Dedução do Ponto A (Canto Superior Esquerdo)
   float x_A = x_B + x_D - x_C;
   float y_A = y_B + y_D - y_C;

   // 2. Cálculo de dimensões
   float dist_DC_quad = ( x_C - x_D ) * ( x_C - x_D ) + ( y_C - y_D ) * ( y_C - y_D );
   float dist_BC_quad = ( x_C - x_B ) * ( x_C - x_B ) + ( y_C - y_B ) * ( y_C - y_B );

   img->ncol = ( gsize )roundf( sqrtf( dist_DC_quad ) );
   img->nrow = ( gsize )roundf( sqrtf( dist_BC_quad ) );

   if ( img->ncol == 0 || img->nrow == 0 ) {
      return;
   }

   // Limpeza e Realocação no Padrão Contíguo (1D real + 2D pointers)
   if ( img->dados ) {
      g_free( img->dados );
      g_free( img->image );
   }

   img->dados = g_malloc( img->nrow * img->ncol * sizeof( uint8_t ) );
   img->image = g_malloc( img->nrow * sizeof( uint8_t * ) );
   for ( gsize i = 0; i < img->nrow; i++ ) {
      img->image[i] = img->dados + ( i * img->ncol );
   }

   g_strlcpy( img->key, IMG->key, sizeof( img->key ) );
   img->max = IMG->max;

   // 3. Vetores diretores de passo unitário
   float ux = ( x_C - x_D ) / ( float )img->ncol;
   float uy = ( y_C - y_D ) / ( float )img->ncol;

   float vx = ( x_C - x_B ) / ( float )img->nrow;
   float vy = ( y_C - y_B ) / ( float )img->nrow;

   // Limites espaciais (usando gssize para lidar com mapeamentos negativos seguros)
   gssize x_lim = ( gssize )IMG->ncol - 1;
   gssize y_lim = ( gssize )IMG->nrow - 1;

   // 4. Mapeamento Reverso com Interpolação Bilinear
   #pragma omp parallel for schedule(static) if( !omp_in_parallel() )
   for ( gsize y_novo = 0; y_novo < img->nrow; y_novo++ ) {
      for ( gsize x_novo = 0; x_novo < img->ncol; x_novo++ ) {

         float x_orig = x_A + ( ( float )y_novo * vx ) + ( ( float )x_novo * ux );
         float y_orig = y_A + ( ( float )y_novo * vy ) + ( ( float )x_novo * uy );

         gssize x_base = ( gssize )floorf( x_orig );
         gssize y_base = ( gssize )floorf( y_orig );

         float dx = x_orig - ( float )x_base;
         float dy = y_orig - ( float )y_base;

         int pA = 255, pB = 255, pC = 255, pD = 255;

         // [FAST PATH] - Totalmente dentro da imagem
         if ( x_base >= 0 && x_base < x_lim && y_base >= 0 && y_base < y_lim ) {
            pA = IMG->image[y_base][x_base];
            pB = IMG->image[y_base][x_base + 1];
            pC = IMG->image[y_base + 1][x_base];
            pD = IMG->image[y_base + 1][x_base + 1];

            // [SLOW PATH] - Borda com fallback para branco
         } else {
            if ( x_base >= 0 && x_base <= x_lim && y_base >= 0 && y_base <= y_lim ) {
               pA = IMG->image[y_base][x_base];
            }

            if ( x_base + 1 >= 0 && x_base + 1 <= x_lim && y_base >= 0 && y_base <= y_lim ) {
               pB = IMG->image[y_base][x_base + 1];
            }

            if ( x_base >= 0 && x_base <= x_lim && y_base + 1 >= 0 && y_base + 1 <= y_lim ) {
               pC = IMG->image[y_base + 1][x_base];
            }

            if ( x_base + 1 >= 0 && x_base + 1 <= x_lim && y_base + 1 >= 0 && y_base + 1 <= y_lim ) {
               pD = IMG->image[y_base + 1][x_base + 1];
            }
         }

         float interpolado = ( float )pA * ( 1.0f - dx ) * ( 1.0f - dy ) +
                             ( float )pB * dx * ( 1.0f - dy ) +
                             ( float )pC * ( 1.0f - dx ) * dy +
                             ( float )pD * dx * dy;

         img->image[y_novo][x_novo] = ( uint8_t )( interpolado + 0.5f );
      }
   }
}






void cortar_imagem_colorida_bilinear( const ImagemColorida *IMG, ImagemColorida *img, const IndiceMatriz *ancora ) {
   g_return_if_fail( IMG != NULL && img != NULL && ancora != NULL );

   // Coordenadas dos 3 pontos âncoras de base (B, C e D)
   float x_B = ( float )ancora[1].j, y_B = ( float )ancora[1].i;
   float x_C = ( float )ancora[2].j, y_C = ( float )ancora[2].i;
   float x_D = ( float )ancora[3].j, y_D = ( float )ancora[3].i;

   // 1. Dedução do Ponto A (Canto Superior Esquerdo)
   float x_A = x_B + x_D - x_C;
   float y_A = y_B + y_D - y_C;

   // 2. Cálculo de dimensões (Baseado nas arestas opostas)
   float dist_DC_quad = ( x_C - x_D ) * ( x_C - x_D ) + ( y_C - y_D ) * ( y_C - y_D );
   float dist_BC_quad = ( x_C - x_B ) * ( x_C - x_B ) + ( y_C - y_B ) * ( y_C - y_B );

   img->ncol = ( gsize )roundf( sqrtf( dist_DC_quad ) );
   img->nrow = ( gsize )roundf( sqrtf( dist_BC_quad ) );

   if ( img->ncol == 0 || img->nrow == 0 ) {
      return;
   }

   // Limpeza e Realocação no Padrão Contíguo (1D real + 2D pointers)
   if ( img->dados ) {
      g_free( img->dados );
      g_free( img->image );
   }

   img->dados = g_malloc( img->nrow * img->ncol * sizeof( PixelRGB ) );
   img->image = g_malloc( img->nrow * sizeof( PixelRGB * ) );
   for ( gsize i = 0; i < img->nrow; i++ ) {
      img->image[i] = img->dados + ( i * img->ncol );
   }

   g_strlcpy( img->key, IMG->key, sizeof( img->key ) );
   img->max = IMG->max;

   // 3. Vetores diretores de passo unitário
   float ux = ( x_C - x_D ) / ( float )img->ncol;
   float uy = ( y_C - y_D ) / ( float )img->ncol;

   float vx = ( x_C - x_B ) / ( float )img->nrow;
   float vy = ( y_C - y_B ) / ( float )img->nrow;

   // Limites espaciais (usando gssize para lidar com mapeamentos negativos seguros)
   gssize x_lim = ( gssize )IMG->ncol - 1;
   gssize y_lim = ( gssize )IMG->nrow - 1;

   // 4. Mapeamento Reverso com Interpolação Bilinear RGB
   #pragma omp parallel for schedule(static) if( !omp_in_parallel() )
   for ( gsize y_novo = 0; y_novo < img->nrow; y_novo++ ) {
      for ( gsize x_novo = 0; x_novo < img->ncol; x_novo++ ) {

         // O mapa reverso usa o ponto A deduzido matematicamente como origem
         float x_orig = x_A + ( ( float )y_novo * vx ) + ( ( float )x_novo * ux );
         float y_orig = y_A + ( ( float )y_novo * vy ) + ( ( float )x_novo * uy );

         gssize x_base = ( gssize )floorf( x_orig );
         gssize y_base = ( gssize )floorf( y_orig );

         float dx = x_orig - ( float )x_base;
         float dy = y_orig - ( float )y_base;

         // Inicialização segura com fallback para fundo branco
         PixelRGB pA = {255, 255, 255};
         PixelRGB pB = {255, 255, 255};
         PixelRGB pC = {255, 255, 255};
         PixelRGB pD = {255, 255, 255};

         // [FAST PATH] - Totalmente dentro da imagem (Caso ultra-comum)
         if ( x_base >= 0 && x_base < x_lim && y_base >= 0 && y_base < y_lim ) {
            pA = IMG->image[y_base][x_base];
            pB = IMG->image[y_base][x_base + 1];
            pC = IMG->image[y_base + 1][x_base];
            pD = IMG->image[y_base + 1][x_base + 1];

            // [SLOW PATH] - Borda com clamping individual (elimina ternários confusos)
         } else {
            if ( x_base >= 0 && x_base <= x_lim && y_base >= 0 && y_base <= y_lim ) {
               pA = IMG->image[y_base][x_base];
            }

            if ( x_base + 1 >= 0 && x_base + 1 <= x_lim && y_base >= 0 && y_base <= y_lim ) {
               pB = IMG->image[y_base][x_base + 1];
            }

            if ( x_base >= 0 && x_base <= x_lim && y_base + 1 >= 0 && y_base + 1 <= y_lim ) {
               pC = IMG->image[y_base + 1][x_base];
            }

            if ( x_base + 1 >= 0 && x_base + 1 <= x_lim && y_base + 1 >= 0 && y_base + 1 <= y_lim ) {
               pD = IMG->image[y_base + 1][x_base + 1];
            }
         }

         // Cálculo dos pesos bilineares otimizado (calculado 1 vez, usado nos 3 canais)
         float pesoA = ( 1.0f - dx ) * ( 1.0f - dy );
         float pesoB = dx * ( 1.0f - dy );
         float pesoC = ( 1.0f - dx ) * dy;
         float pesoD = dx * dy;

         float r_interp = ( float )pA.r * pesoA + ( float )pB.r * pesoB + ( float )pC.r * pesoC + ( float )pD.r * pesoD;
         float g_interp = ( float )pA.g * pesoA + ( float )pB.g * pesoB + ( float )pC.g * pesoC + ( float )pD.g * pesoD;
         float b_interp = ( float )pA.b * pesoA + ( float )pB.b * pesoB + ( float )pC.b * pesoC + ( float )pD.b * pesoD;

         img->image[y_novo][x_novo].r = ( uint8_t )( r_interp + 0.5f );
         img->image[y_novo][x_novo].g = ( uint8_t )( g_interp + 0.5f );
         img->image[y_novo][x_novo].b = ( uint8_t )( b_interp + 0.5f );
      }
   }
}







// ====================================================================================
// REDIMENSIONAMENTO BILINEAR - ESCALA DE CINZA (Multithread / Contíguo)
// ====================================================================================
void redimensionar_imagem_bilinear( const ImagemCinza *origem, ImagemCinza *destino, gsize dim ) {
   g_return_if_fail( origem != NULL && origem->dados != NULL && origem->image != NULL );
   g_return_if_fail( destino != NULL );
   g_return_if_fail( dim > 0 );

   gboolean deitada = ( origem->ncol > origem->nrow );
   destino->ncol = deitada ? dim : ( dim * origem->ncol ) / origem->nrow;
   destino->nrow = deitada ? ( dim * origem->nrow ) / origem->ncol : dim;

   g_strlcpy( destino->key, origem->key, sizeof( destino->key ) );
   destino->max = origem->max;

   // 1. Limpeza e Realocação no Padrão Contíguo (1D real + 2D pointers)
   if ( destino->dados ) {
      g_free( destino->dados );
      g_free( destino->image );
   } else if ( destino->image ) {
      liberar_matriz_pixels( destino->image, destino->nrow );
   }

   // Alocação em bloco único para vetorização SIMD e aderência perfeita ao cache
   destino->dados = g_malloc( destino->nrow * destino->ncol * sizeof( uint8_t ) );
   destino->image = g_malloc( destino->nrow * sizeof( uint8_t * ) );
   for ( gsize i = 0; i < destino->nrow; i++ ) {
      destino->image[i] = destino->dados + ( i * destino->ncol );
   }

   // Fatores de proporção (mapeamento reverso alinhando os cantos)
   float x_ratio = ( float )( origem->ncol - 1 ) / ( float )( destino->ncol > 1 ? destino->ncol - 1 : 1 );
   float y_ratio = ( float )( origem->nrow - 1 ) / ( float )( destino->nrow > 1 ? destino->nrow - 1 : 1 );

   // 2. Mapeamento Reverso Paralelizado
   // schedule(static) é perfeito pois não há divergência de ramificação (branching)
   #pragma omp parallel for schedule(static) if( !omp_in_parallel() )
   for ( gsize i = 0; i < destino->nrow; i++ ) {
      float src_y = y_ratio * ( float )i;
      gsize y = ( gsize )src_y;
      float y_diff = src_y - ( float )y;
      float y_inv = 1.0f - y_diff;

      gsize y_next = ( y + 1 < origem->nrow ) ? y + 1 : y;

      for ( gsize j = 0; j < destino->ncol; j++ ) {
         float src_x = x_ratio * ( float )j;
         gsize x = ( gsize )src_x;
         float x_diff = src_x - ( float )x;
         float x_inv = 1.0f - x_diff;

         gsize x_next = ( x + 1 < origem->ncol ) ? x + 1 : x;

         // OTIMIZAÇÃO 1: Pré-cálculo dos pesos espaciais (reaproveitados)
         float pesoA = x_inv * y_inv;
         float pesoB = x_diff * y_inv;
         float pesoC = x_inv * y_diff;
         float pesoD = x_diff * y_diff;

         // OTIMIZAÇÃO 2: Captura limpa via ponteiros diretos
         uint8_t a = origem->image[y][x];
         uint8_t b = origem->image[y][x_next];
         uint8_t c = origem->image[y_next][x];
         uint8_t d = origem->image[y_next][x_next];

         float pixel_interpolado = ( ( float )a * pesoA ) +
                                   ( ( float )b * pesoB ) +
                                   ( ( float )c * pesoC ) +
                                   ( ( float )d * pesoD );

         // Escrita livre de colisão garantida pelo índice da thread 'i'
         destino->image[i][j] = ( uint8_t )( pixel_interpolado + 0.5f );
      }
   }
}


// ====================================================================================
// REDIMENSIONAMENTO BILINEAR - RGB COLORIDO (Multithread / Contíguo)
// ====================================================================================
void redimensionar_imagem_colorida_bilinear( const ImagemColorida *origem, ImagemColorida *destino, gsize dim ) {
   g_return_if_fail( origem != NULL && origem->dados != NULL && origem->image != NULL );
   g_return_if_fail( destino != NULL );
   g_return_if_fail( dim > 0 );

   gboolean deitada = ( origem->ncol > origem->nrow );
   destino->ncol = deitada ? dim : ( dim * origem->ncol ) / origem->nrow;
   destino->nrow = deitada ? ( dim * origem->nrow ) / origem->ncol : dim;

   g_strlcpy( destino->key, origem->key, sizeof( destino->key ) );
   destino->max = origem->max;

   // 1. Barreira de Segurança e Limpeza no Padrão Contíguo
   if ( destino->dados ) {
      g_free( destino->dados );
      g_free( destino->image );
   } else if ( destino->image ) {
      liberar_matriz_pixels_colorida( destino->image, destino->nrow );
   }

   // Alocação da Nova Matriz Colorida em Bloco Único (Vetorização SIMD)
   destino->dados = g_malloc( destino->nrow * destino->ncol * sizeof( PixelRGB ) );
   destino->image = g_malloc( destino->nrow * sizeof( PixelRGB * ) );
   for ( gsize i = 0; i < destino->nrow; i++ ) {
      destino->image[i] = destino->dados + ( i * destino->ncol );
   }

   // Fatores de proporção
   float x_ratio = ( float )( origem->ncol - 1 ) / ( float )( destino->ncol > 1 ? destino->ncol - 1 : 1 );
   float y_ratio = ( float )( origem->nrow - 1 ) / ( float )( destino->nrow > 1 ? destino->nrow - 1 : 1 );

   // 2. Mapeamento Reverso Paralelizado
   #pragma omp parallel for schedule(static) if( !omp_in_parallel() )
   for ( gsize i = 0; i < destino->nrow; i++ ) {
      float src_y = y_ratio * ( float )i;
      gsize y = ( gsize )src_y;
      float y_diff = src_y - ( float )y;
      float y_inv = 1.0f - y_diff;

      gsize y_next = ( y + 1 < origem->nrow ) ? y + 1 : y;

      for ( gsize j = 0; j < destino->ncol; j++ ) {
         float src_x = x_ratio * ( float )j;
         gsize x = ( gsize )src_x;
         float x_diff = src_x - ( float )x;
         float x_inv = 1.0f - x_diff;

         gsize x_next = ( x + 1 < origem->ncol ) ? x + 1 : x;

         // OTIMIZAÇÃO MATEMÁTICA: Pré-calculamos os pesos espaciais dos 4 vizinhos.
         // Isso economiza 8 multiplicações de ponto flutuante por cada pixel gerado!
         float pesoA = x_inv * y_inv;
         float pesoB = x_diff * y_inv;
         float pesoC = x_inv * y_diff;
         float pesoD = x_diff * y_diff;

         // OTIMIZAÇÃO DE CORES: Captura da estrutura PixelRGB inteira de uma vez
         PixelRGB pA = origem->image[y][x];
         PixelRGB pB = origem->image[y][x_next];
         PixelRGB pC = origem->image[y_next][x];
         PixelRGB pD = origem->image[y_next][x_next];

         // Aplica os pesos aos 3 canais de forma independente
         float r_interp = ( ( float )pA.r * pesoA ) + ( ( float )pB.r * pesoB ) +
                          ( ( float )pC.r * pesoC ) + ( ( float )pD.r * pesoD );

         float g_interp = ( ( float )pA.g * pesoA ) + ( ( float )pB.g * pesoB ) +
                          ( ( float )pC.g * pesoC ) + ( ( float )pD.g * pesoD );

         float b_interp = ( ( float )pA.b * pesoA ) + ( ( float )pB.b * pesoB ) +
                          ( ( float )pC.b * pesoC ) + ( ( float )pD.b * pesoD );

         // Escrita limpa e sem colisão arredondando de volta para uint8_t
         destino->image[i][j].r = ( uint8_t )( r_interp + 0.5f );
         destino->image[i][j].g = ( uint8_t )( g_interp + 0.5f );
         destino->image[i][j].b = ( uint8_t )( b_interp + 0.5f );
      }
   }
}





// ====================================================================================
// FILTRO GAUSSIANO ISOTRÓPICO 2D (Purista em C)
// ====================================================================================
void aplicar_filtro_gaussiano_2d( const ImagemCinza *IMG, ImagemCinza *img, float sigma ) {
   g_return_if_fail( IMG != NULL && img != NULL );
   g_return_if_fail( IMG->nrow > 0 && IMG->ncol > 0 && sigma > 0.0f );

   // 1. Limpeza e Realocação no Novo Padrão Contíguo (1D real + 2D pointers)
   if ( img->dados ) {
      g_free( img->dados );
      g_free( img->image );
   }

   img->ncol = IMG->ncol;
   img->nrow = IMG->nrow;

   img->dados = g_malloc( img->nrow * img->ncol * sizeof( uint8_t ) );
   img->image = g_malloc( img->nrow * sizeof( uint8_t * ) );
   for ( gsize i = 0; i < img->nrow; i++ ) {
      img->image[i] = img->dados + ( i * img->ncol );
   }

   g_strlcpy( img->key, IMG->key, sizeof( img->key ) );
   img->max = IMG->max;

   // 2. Cálculo do tamanho do Kernel
   gsize raio = ( gsize )ceilf( 3.0f * sigma );
   gsize diametro = 2 * raio + 1;

   g_autofree float *kernel = g_new0( float, diametro * diametro );

   // 3. Geração do Kernel Isotrópico Normalizado
   float soma_pesos = 0.0f;
   float divisor_expoente = 2.0f * sigma * sigma;
   gsize idx = 0;

   // CRÍTICO: Índices que vão para o negativo DEVEM ser gssize (com sinal)
   gssize limite_raio = ( gssize )raio;

   for ( gssize i = -limite_raio; i <= limite_raio; i++ ) {
      for ( gssize j = -limite_raio; j <= limite_raio; j++ ) {
         float peso = expf( ( float ) - ( ( i * i ) + ( j * j ) ) / divisor_expoente );
         kernel[idx++] = peso;
         soma_pesos += peso;
      }
   }

   for ( gsize k = 0; k < diametro * diametro; k++ ) {
      kernel[k] /= soma_pesos;
   }

   // 4. Convolução 2D Paralelizada com Padding Virtual (Clamp to Edge)
   gssize x_lim = ( gssize )IMG->ncol - 1;
   gssize y_lim = ( gssize )IMG->nrow - 1;

   #pragma omp parallel for schedule(static) if( !omp_in_parallel() )
   for ( gsize y = 0; y < IMG->nrow; y++ ) {
      for ( gsize x = 0; x < IMG->ncol; x++ ) {

         float pixel_acumulado = 0.0f;
         gsize k_idx = 0;

         // [FAST PATH] - O pixel está seguro no centro da imagem
         if ( x >= raio && x < IMG->ncol - raio && y >= raio && y < IMG->nrow - raio ) {
            for ( gssize ky = -limite_raio; ky <= limite_raio; ky++ ) {
               for ( gssize kx = -limite_raio; kx <= limite_raio; kx++ ) {
                  pixel_acumulado += ( float )IMG->image[y + ( gsize )ky][x + ( gsize )kx] * kernel[k_idx++];
               }
            }
         }
         // [SLOW PATH] - O pixel está na borda. Simulamos a "expansão" travando os índices.
         else {
            for ( gssize ky = -limite_raio; ky <= limite_raio; ky++ ) {
               gssize coord_y = ( gssize )y + ky;
               coord_y = ( coord_y < 0 ) ? 0 : ( ( coord_y > y_lim ) ? y_lim : coord_y );

               for ( gssize kx = -limite_raio; kx <= limite_raio; kx++ ) {
                  gssize coord_x = ( gssize )x + kx;
                  coord_x = ( coord_x < 0 ) ? 0 : ( ( coord_x > x_lim ) ? x_lim : coord_x );

                  pixel_acumulado += ( float )IMG->image[coord_y][coord_x] * kernel[k_idx++];
               }
            }
         }

         img->image[y][x] = ( uint8_t )( pixel_acumulado + 0.5f );
      }
   }
}




// ====================================================================================
// FILTRO GAUSSIANO ISOTRÓPICO 2D RGB
// ====================================================================================
void aplicar_filtro_gaussiano_2d_rgb( const ImagemColorida *IMG, ImagemColorida *img, float sigma ) {
   g_return_if_fail( IMG != NULL && img != NULL );
   g_return_if_fail( IMG->nrow > 0 && IMG->ncol > 0 && sigma > 0.0f );

   // 1. Limpeza e Realocação no Padrão Contíguo (1D real + 2D pointers)
   if ( img->dados ) {
      g_free( img->dados );
      g_free( img->image );
   }

   img->ncol = IMG->ncol;
   img->nrow = IMG->nrow;

   img->dados = g_malloc( img->nrow * img->ncol * sizeof( PixelRGB ) );
   img->image = g_malloc( img->nrow * sizeof( PixelRGB * ) );
   for ( gsize i = 0; i < img->nrow; i++ ) {
      img->image[i] = img->dados + ( i * img->ncol );
   }

   g_strlcpy( img->key, IMG->key, sizeof( img->key ) );
   img->max = IMG->max;

   // 2. Cálculo do tamanho do Kernel
   gsize raio = ( gsize )ceilf( 3.0f * sigma );
   gsize diametro = 2 * raio + 1;

   g_autofree float *kernel = g_new0( float, diametro * diametro );

   // 3. Geração do Kernel Isotrópico Normalizado
   float soma_pesos = 0.0f;
   float divisor_expoente = 2.0f * sigma * sigma;
   gsize idx = 0;

   // CRÍTICO: Índices que vão para o negativo DEVEM ser gssize (com sinal)
   gssize limite_raio = ( gssize )raio;

   for ( gssize i = -limite_raio; i <= limite_raio; i++ ) {
      for ( gssize j = -limite_raio; j <= limite_raio; j++ ) {
         float peso = expf( ( float ) - ( ( i * i ) + ( j * j ) ) / divisor_expoente );
         kernel[idx++] = peso;
         soma_pesos += peso;
      }
   }

   for ( gsize k = 0; k < diametro * diametro; k++ ) {
      kernel[k] /= soma_pesos;
   }

   // 4. Convolução 2D Paralelizada com Padding Virtual (Clamp to Edge)
   gssize x_lim = ( gssize )IMG->ncol - 1;
   gssize y_lim = ( gssize )IMG->nrow - 1;

   // Diretiva OpenMP reativada: tira proveito total da nova memória contígua
   #pragma omp parallel for schedule(static) if( !omp_in_parallel() )
   for ( gsize y = 0; y < IMG->nrow; y++ ) {
      for ( gsize x = 0; x < IMG->ncol; x++ ) {

         float pixel_r = 0.0f;
         float pixel_g = 0.0f;
         float pixel_b = 0.0f;
         gsize k_idx = 0;

         // [FAST PATH] - O pixel está seguro no centro da imagem
         if ( x >= raio && x < IMG->ncol - raio && y >= raio && y < IMG->nrow - raio ) {
            for ( gssize ky = -limite_raio; ky <= limite_raio; ky++ ) {
               for ( gssize kx = -limite_raio; kx <= limite_raio; kx++ ) {
                  float peso = kernel[k_idx++];
                  const PixelRGB *p_src = &IMG->image[y + ( gsize )ky][x + ( gsize )kx];

                  pixel_r += ( float )p_src->r * peso;
                  pixel_g += ( float )p_src->g * peso;
                  pixel_b += ( float )p_src->b * peso;
               }
            }
         }
         // [SLOW PATH] - O pixel está na borda. Simulamos a "expansão" travando os índices.
         else {
            for ( gssize ky = -limite_raio; ky <= limite_raio; ky++ ) {
               gssize coord_y = ( gssize )y + ky;
               coord_y = ( coord_y < 0 ) ? 0 : ( ( coord_y > y_lim ) ? y_lim : coord_y );

               for ( gssize kx = -limite_raio; kx <= limite_raio; kx++ ) {
                  gssize coord_x = ( gssize )x + kx;
                  coord_x = ( coord_x < 0 ) ? 0 : ( ( coord_x > x_lim ) ? x_lim : coord_x );

                  float peso = kernel[k_idx++];
                  const PixelRGB *p_src = &IMG->image[coord_y][coord_x];

                  pixel_r += ( float )p_src->r * peso;
                  pixel_g += ( float )p_src->g * peso;
                  pixel_b += ( float )p_src->b * peso;
               }
            }
         }

         // Arredondamento perfeito para inteiro limitando a 255 (max) para evitar overflow
         img->image[y][x].r = ( uint8_t )fminf( pixel_r + 0.5f, 255.0f );
         img->image[y][x].g = ( uint8_t )fminf( pixel_g + 0.5f, 255.0f );
         img->image[y][x].b = ( uint8_t )fminf( pixel_b + 0.5f, 255.0f );
      }
   }
}





// ====================================================================================
// BINARIZAÇÃO GLOBAL: MÉTODO DE OTSU (Arquitetura Contígua 1D + Vetorização SIMD)
// ====================================================================================
void binarizar_pgm_metodo_otsu( ImagemCinza *IMG ) {
   g_return_if_fail( IMG != NULL && IMG->dados != NULL && IMG->image != NULL );
   g_return_if_fail( IMG->nrow > 0 && IMG->ncol > 0 && IMG->max == 255 );

   // 256 bins fixos na Stack (2 KB): Zero alocação na Heap e máxima localidade de cache L1
   enum { NUM_BINS = 256 };
   gsize histograma[NUM_BINS] = { 0 };

   const gsize total_pixels = IMG->nrow * IMG->ncol;
   uint8_t * restrict dados = IMG->dados;

   // =================================================================================
   // 1. CONSTRUÇÃO DO HISTOGRAMA (Varredura Linear 1D)
   // =================================================================================
   for ( gsize i = 0; i < total_pixels; i++ ) {
      histograma[dados[i]]++;
   }

   // =================================================================================
   // 2. MATEMÁTICA DO MÉTODO DE OTSU
   // =================================================================================
   double soma_total = 0.0;
   for ( gsize i = 0; i < NUM_BINS; i++ ) {
      soma_total += ( double )i * ( double )histograma[i];
   }

   double soma_b           = 0.0;
   double variancia_maxima = -1.0;
   gsize  w_b              = 0;
   uint8_t limiar_otsu     = 0;

   for ( gsize t = 0; t < NUM_BINS; t++ ) {
      w_b += histograma[t]; // Peso da classe "Fundo" (Background)
      if ( w_b == 0 ) {
         continue;
      }

      gsize w_f = total_pixels - w_b; // Peso da classe "Frente" (Foreground)
      if ( w_f == 0 ) {
         break; // Todos os pixels restantes pertencem ao fundo
      }

      soma_b += ( double )t * ( double )histograma[t];

      double media_b = soma_b / ( double )w_b;
      double media_f = ( soma_total - soma_b ) / ( double )w_f;
      double diff    = media_b - media_f;

      // Variância Inter-Classes: w_b * w_f * (media_b - media_f)^2
      double variancia_inter = ( double )w_b * ( double )w_f * diff * diff;

      if ( variancia_inter > variancia_maxima ) {
         variancia_maxima = variancia_inter;
         limiar_otsu      = ( uint8_t )t;
      }
   }

   // =================================================================================
   // 3. APLICAÇÃO DO LIMIAR NA IMAGEM (Vetorizada via SIMD sem Overhead de Threads)
   // =================================================================================
   const uint8_t max_val = ( uint8_t )IMG->max;

   #pragma omp simd
   for ( gsize i = 0; i < total_pixels; i++ ) {
      dados[i] = ( dados[i] > limiar_otsu ) ? max_val : 0U;
   }
}






// ====================================================================================
// TRANSFORMADA HOMOGRÁFICA COM INTERPOLAÇÃO BILINEAR (CORREÇÃO DE PERSPECTIVA)
// ====================================================================================
void transformada_homografica( ImagemCinza *img, ImagemCinza *img_crop, IndiceMatriz *ancora, char direcao ) {
   g_return_if_fail( img != NULL && img->image != NULL && img_crop != NULL && ancora != NULL );

   gsize largura, altura;
   gsize fator_de_proporcionalidade = 50;

   if ( direcao == 'h' ) {
      largura = 14 * fator_de_proporcionalidade; // 700 px
      altura = 11 * fator_de_proporcionalidade;  // 550 px
   } else {
      largura = 10 * fator_de_proporcionalidade; // 500 px
      altura = 15 * fator_de_proporcionalidade;  // 750 px
   }

   // =========================================================================
   // 1. PREPARAÇÃO DOS PONTOS (TARGET -> SOURCE)
   // =========================================================================
   Matrix dst_pts = mat_new( 4, 2 ); // O Gabarito Perfeito Virtual
   Matrix src_pts = mat_new( 4, 2 ); // As coordenadas reais tortas da foto

   // Âncora A (Top-Left)
   dst_pts.data[0] = 0.0;
   dst_pts.data[1] = 0.0;
   // Âncora B (Top-Right)
   dst_pts.data[2] = ( double )( largura - 1 );
   dst_pts.data[3] = 0.0;
   // Âncora C (Bottom-Right)
   dst_pts.data[4] = ( double )( largura - 1 );
   dst_pts.data[5] = ( double )( altura - 1 );
   // Âncora D (Bottom-Left)
   dst_pts.data[6] = 0.0;
   dst_pts.data[7] = ( double )( altura - 1 );

   // Assumindo IndiceMatriz: j = x (coluna) e i = y (linha)
   for ( gsize k = 0; k < 4; k++ ) {
      src_pts.data[k * 2 + 0] = ( double )ancora[k].j;
      src_pts.data[k * 2 + 1] = ( double )ancora[k].i;
   }

   // =========================================================================
   // 2. RESOLUÇÃO DA HOMOGRAFIA INVERSA VIA LAPACKE
   // =========================================================================
   Matrix H_inv = mat_new( 3, 3 );

   // Passamos de DST para SRC para podermos fazer o Backward Warping
   if ( mat_homography_dlt( dst_pts, src_pts, H_inv ) != 0 ) {
      g_printerr( "[ERRO] Falha no LAPACKE ao calcular a homografia!\n" );
      mat_free( src_pts );
      mat_free( dst_pts );
      mat_free( H_inv );
      return;
   }

   // =========================================================================
   // 3. ALOCAÇÃO DO NOVO BUFFER DE PIXELS (Arquitetura Contígua)
   // =========================================================================
   // Limpa dados velhos se estiver reutilizando a mesma estrutura img_crop
   if ( img_crop->dados ) {
      g_free( img_crop->dados );
      g_free( img_crop->image );
   }

   img_crop->ncol = largura;
   img_crop->nrow = altura;
   img_crop->max = img->max;
   g_strlcpy( img_crop->key, img->key, sizeof( img_crop->key ) );

   // Aloca a matriz do crop em bloco único para vetorização SIMD
   img_crop->dados = g_malloc( img_crop->nrow * img_crop->ncol * sizeof( uint8_t ) );
   img_crop->image = g_malloc( img_crop->nrow * sizeof( uint8_t * ) );
   for ( gsize i = 0; i < img_crop->nrow; i++ ) {
      img_crop->image[i] = img_crop->dados + ( i * img_crop->ncol );
   }

   // Limites espaciais base convertidos para gssize para lidar com mapeamentos negativos
   gssize orig_w = ( gssize )img->ncol;
   gssize orig_h = ( gssize )img->nrow;
   uint8_t fundo_branco = ( img->max > 0 ) ? ( uint8_t )img->max : 255;

   // =========================================================================
   // 4. BACKWARD WARPING COM INTERPOLAÇÃO BILINEAR (MULTITHREAD)
   // =========================================================================
   // Diretiva OpenMP reativada: agendamento estático para blocos de memória contíguos
   #pragma omp parallel for schedule(static) if( !omp_in_parallel() )
   for ( gsize y_dst = 0; y_dst < altura; y_dst++ ) {
      for ( gsize x_dst = 0; x_dst < largura; x_dst++ ) {

         // Cast explícito para double para silenciar alertas de precisão implícita (-Wconversion)
         double dx_dst = ( double )x_dst;
         double dy_dst = ( double )y_dst;

         // 4.1 Projeção 3D (Coordenadas Homogêneas)
         double w = H_inv.data[6] * dx_dst + H_inv.data[7] * dy_dst + H_inv.data[8];
         if ( fabs( w ) < 1.0e-8 ) {
            w = 1.0e-8;   // Evita divisão por zero matemática
         }

         // Resgate da coordenada exata subpixel na imagem original
         double x_src = ( H_inv.data[0] * dx_dst + H_inv.data[1] * dy_dst + H_inv.data[2] ) / w;
         double y_src = ( H_inv.data[3] * dx_dst + H_inv.data[4] * dy_dst + H_inv.data[5] ) / w;

         // Ancoragem com sinal (gssize) para evitar wrap-around em projeções fora da imagem
         gssize x1 = ( gssize )floor( x_src );
         gssize y1 = ( gssize )floor( y_src );

         // 4.2 Verifica se o ponto rastreado existe estritamente dentro da foto original
         if ( x1 >= 0 && x1 < orig_w - 1 && y1 >= 0 && y1 < orig_h - 1 ) {

            // 4.3 Interpolação Bilinear C Puro
            gssize x2 = x1 + 1;
            gssize y2 = y1 + 1;

            double dx = x_src - ( double )x1;
            double dy = y_src - ( double )y1;

            // Coleta as 4 intensidades vizinhas da imagem de origem
            double p11 = ( double )img->image[y1][x1];
            double p12 = ( double )img->image[y2][x1];
            double p21 = ( double )img->image[y1][x2];
            double p22 = ( double )img->image[y2][x2];

            // Mistura topológica
            double pixel_val = ( p11 * ( 1.0 - dx ) * ( 1.0 - dy ) ) +
                               ( p21 * dx * ( 1.0 - dy ) ) +
                               ( p12 * ( 1.0 - dx ) * dy ) +
                               ( p22 * dx * dy );

            // Grava diretamente no crop arredondando de forma segura
            img_crop->image[y_dst][x_dst] = ( uint8_t )( pixel_val + 0.5 );
         } else {
            img_crop->image[y_dst][x_dst] = fundo_branco;
         }
      }
   }

   // =========================================================================
   // 5. LIMPEZA DOS OBJETOS MATEMÁTICOS LOCAIS
   // =========================================================================
   mat_free( src_pts );
   mat_free( dst_pts );
   mat_free( H_inv );
}






// ====================================================================================
// TRANSFORMADA HOMOGRÁFICA RGB COM INTERPOLAÇÃO BILINEAR (CORREÇÃO DE PERSPECTIVA)
// ====================================================================================
void transformada_homografica_colorida( ImagemColorida *img, ImagemColorida *img_crop, IndiceMatriz *ancora, char direcao ) {
   g_return_if_fail( img != NULL && img->image != NULL && img_crop != NULL && ancora != NULL );

   gsize largura, altura;
   gsize fator_de_proporcionalidade = 60;

   if ( direcao == 'h' ) {
      largura = 14 * fator_de_proporcionalidade; // 840 px
      altura = 11 * fator_de_proporcionalidade;  // 660 px
   } else {
      largura = 10 * fator_de_proporcionalidade; // 600 px
      altura = 15 * fator_de_proporcionalidade;  // 900 px
   }

   // =========================================================================
   // 1. PREPARAÇÃO DOS PONTOS (TARGET -> SOURCE)
   // =========================================================================
   Matrix dst_pts = mat_new( 4, 2 ); // O Gabarito Perfeito Virtual (Proporcional)
   Matrix src_pts = mat_new( 4, 2 ); // As coordenadas reais tortas da foto original

   // Âncora A (Top-Left)
   dst_pts.data[0] = 0.0;
   dst_pts.data[1] = 0.0;
   // Âncora B (Top-Right)
   dst_pts.data[2] = ( double )( largura - 1 );
   dst_pts.data[3] = 0.0;
   // Âncora C (Bottom-Right)
   dst_pts.data[4] = ( double )( largura - 1 );
   dst_pts.data[5] = ( double )( altura - 1 );
   // Âncora D (Bottom-Left)
   dst_pts.data[6] = 0.0;
   dst_pts.data[7] = ( double )( altura - 1 );

   // Assumindo IndiceMatriz: j = x (coluna) e i = y (linha)
   for ( gsize k = 0; k < 4; k++ ) {
      src_pts.data[k * 2 + 0] = ( double )ancora[k].j;
      src_pts.data[k * 2 + 1] = ( double )ancora[k].i;
   }

   // =========================================================================
   // 2. RESOLUÇÃO DA HOMOGRAFIA INVERSA VIA LAPACKE
   // =========================================================================
   Matrix H_inv = mat_new( 3, 3 );

   if ( mat_homography_dlt( dst_pts, src_pts, H_inv ) != 0 ) {
      g_printerr( "[ERRO] Falha no LAPACKE ao calcular a homografia colorida!\n" );
      mat_free( src_pts );
      mat_free( dst_pts );
      mat_free( H_inv );
      return;
   }

   // =========================================================================
   // 3. ALOCAÇÃO DO NOVO BUFFER DE PIXELS RGB (Arquitetura Contígua)
   // =========================================================================
   if ( img_crop->dados ) {
      g_free( img_crop->dados );
      g_free( img_crop->image );
   }

   img_crop->ncol = largura;
   img_crop->nrow = altura;
   img_crop->max = img->max;
   g_strlcpy( img_crop->key, img->key, sizeof( img_crop->key ) );

   // Aloca a matriz do crop em bloco único de structs PixelRGB para vetorização SIMD
   img_crop->dados = g_malloc( img_crop->nrow * img_crop->ncol * sizeof( PixelRGB ) );
   img_crop->image = g_malloc( img_crop->nrow * sizeof( PixelRGB * ) );
   for ( gsize i = 0; i < img_crop->nrow; i++ ) {
      img_crop->image[i] = img_crop->dados + ( i * img_crop->ncol );
   }

   gssize orig_w = ( gssize )img->ncol;
   gssize orig_h = ( gssize )img->nrow;
   uint8_t fundo_branco = ( img->max > 0 ) ? ( uint8_t )img->max : 255;

   // =========================================================================
   // 4. BACKWARD WARPING COM INTERPOLAÇÃO BILINEAR DE 3 CANAIS (MULTITHREAD)
   // =========================================================================
   // Diretiva OpenMP reativada: agendamento estático tira total proveito do L1/L2
   #pragma omp parallel for schedule(static) if( !omp_in_parallel() )
   for ( gsize y_dst = 0; y_dst < altura; y_dst++ ) {
      for ( gsize x_dst = 0; x_dst < largura; x_dst++ ) {

         // Cast explícito para double para silenciar alertas de precisão -Wconversion
         double dx_dst = ( double )x_dst;
         double dy_dst = ( double )y_dst;

         // 4.1 Projeção 3D (Coordenadas Homogêneas)
         double w = H_inv.data[6] * dx_dst + H_inv.data[7] * dy_dst + H_inv.data[8];
         if ( fabs( w ) < 1.0e-8 ) {
            w = 1.0e-8;   // Evita divisão por zero matemática
         }

         // Resgate da coordenada exata subpixel na imagem original
         double x_src = ( H_inv.data[0] * dx_dst + H_inv.data[1] * dy_dst + H_inv.data[2] ) / w;
         double y_src = ( H_inv.data[3] * dx_dst + H_inv.data[4] * dy_dst + H_inv.data[5] ) / w;

         // Ancoragem com sinal (gssize) para evitar wrap-around e corrupção de memória
         gssize x1 = ( gssize )floor( x_src );
         gssize y1 = ( gssize )floor( y_src );

         // 4.2 Verifica se o ponto rastreado existe estritamente dentro da foto original
         if ( x1 >= 0 && x1 < orig_w - 1 && y1 >= 0 && y1 < orig_h - 1 ) {

            // 4.3 Interpolação Bilinear - Extração dos Vizinhos
            gssize x2 = x1 + 1;
            gssize y2 = y1 + 1;

            double dx = x_src - ( double )x1;
            double dy = y_src - ( double )y1;

            // OTIMIZAÇÃO: Cálculo único dos 4 pesos topológicos (Reaproveitados no R, G e B)
            double pesoA = ( 1.0 - dx ) * ( 1.0 - dy );
            double pesoB = dx * ( 1.0 - dy );
            double pesoC = ( 1.0 - dx ) * dy;
            double pesoD = dx * dy;

            // Coleta as structs de cor dos 4 pixels vizinhos da imagem de origem
            PixelRGB p11 = img->image[y1][x1];
            PixelRGB p21 = img->image[y1][x2];
            PixelRGB p12 = img->image[y2][x1];
            PixelRGB p22 = img->image[y2][x2];

            // Mistura topológica por canal (R, G, B)
            double r_val = ( ( double )p11.r * pesoA ) + ( ( double )p21.r * pesoB ) +
                           ( ( double )p12.r * pesoC ) + ( ( double )p22.r * pesoD );

            double g_val = ( ( double )p11.g * pesoA ) + ( ( double )p21.g * pesoB ) +
                           ( ( double )p12.g * pesoC ) + ( ( double )p22.g * pesoD );

            double b_val = ( ( double )p11.b * pesoA ) + ( ( double )p21.b * pesoB ) +
                           ( ( double )p12.b * pesoC ) + ( ( double )p22.b * pesoD );

            // Grava diretamente no crop arredondando de forma segura
            img_crop->image[y_dst][x_dst].r = ( uint8_t )( r_val + 0.5 );
            img_crop->image[y_dst][x_dst].g = ( uint8_t )( g_val + 0.5 );
            img_crop->image[y_dst][x_dst].b = ( uint8_t )( b_val + 0.5 );

         } else {
            img_crop->image[y_dst][x_dst].r = fundo_branco;
            img_crop->image[y_dst][x_dst].g = fundo_branco;
            img_crop->image[y_dst][x_dst].b = fundo_branco;
         }
      }
   }

   // =========================================================================
   // 5. LIMPEZA DOS OBJETOS MATEMÁTICOS LOCAIS
   // =========================================================================
   mat_free( src_pts );
   mat_free( dst_pts );
   mat_free( H_inv );
}






//===================================================================================================
// FUNÇÃO: Filtragem Flat-Field Mágica para Imagens em Tons de Cinza (Otimizada)
//===================================================================================================
void filtrar_fundo_magico_cinza( const ImagemCinza *orig, ImagemCinza *dest, gsize raio_blur ) {
   if ( !orig || !orig->image || !dest || orig->nrow <= 0 || orig->ncol <= 0 || raio_blur <= 5 ) {
      return;
   }

   gsize rows = orig->nrow;
   gsize cols = orig->ncol;

   // 1. Aloca a estrutura da imagem de destino se necessário
   if ( dest->image == NULL ) {
      dest->nrow = orig->nrow;
      dest->ncol = orig->ncol;
      dest->max = orig->max;
      g_strlcpy( dest->key, orig->key, sizeof( dest->key ) );
      dest->image = alocar_matriz_pixels( rows, cols );
   }

   // 2. Passo A: Desfoque Separável
   ImagemCinza fundo_h = {0};
   ImagemCinza fundo   = {0};
   fundo_h.image = alocar_matriz_pixels( rows, cols );
   fundo.image   = alocar_matriz_pixels( rows, cols );

   // Pré-calcula a quantidade de vizinhos
   int qtd_vizinhos = 0;
   for ( gsize d = -raio_blur; d <= raio_blur; d += 2 ) {
      qtd_vizinhos++;
   }

   // 2.1 Desfoque Horizontal (Linhas)
   #pragma omp parallel for schedule(dynamic) if( !omp_in_parallel() )
   for ( gsize r = 0; r < rows; r++ ) {
      for ( gsize c = 0; c < cols; c++ ) {
         long soma = 0;

         for ( gsize dc = -raio_blur; dc <= raio_blur; dc += 2 ) {
            gsize nc = c + dc;
            if ( nc >= cols ) {
               nc = cols - 1;
            }

            soma += orig->image[r][nc];
         }
         long razao = ( qtd_vizinhos > 0 ) ? soma / qtd_vizinhos : soma;
         fundo_h.image[r][c] = ( qtd_vizinhos == 0 ) ? 0 : ( guint8 )razao;
      }
   }

   // 2.2 Desfoque Vertical (Colunas) - Lê da fundo_h e escreve na fundo
   #pragma omp parallel for schedule(dynamic) if( !omp_in_parallel() )
   for ( gsize r = 0; r < rows; r++ ) {
      for ( gsize c = 0; c < cols; c++ ) {
         long soma = 0;

         for ( gsize dr = -raio_blur; dr <= raio_blur; dr += 2 ) {
            gsize nr = r + dr;
            if ( nr >= rows ) {
               nr = rows - 1;
            }

            soma += fundo_h.image[nr][c];
         }
         long razao = ( qtd_vizinhos > 0 ) ? soma / qtd_vizinhos : soma;
         fundo.image[r][c] = ( qtd_vizinhos == 0 ) ? 0 : ( guint8 )razao;
      }
   }

   // 3. Passo B: Normalização Flat-Field Otimizada
   // Calcula o limiar de branco (equivalente a 230 em uma escala de 255)
   int limiar_branco = ( orig->max * 230 ) / 255;

   #pragma omp parallel for schedule(static) if( !omp_in_parallel() )
   for ( gsize r = 0; r < rows; r++ ) {
      for ( gsize c = 0; c < cols; c++ ) {

         // Acesso direto ao valor do pixel em cinza
         int po = orig->image[r][c];
         int pf = fundo.image[r][c];

         // Evita divisão por zero
         int f = ( pf < 1 ) ? 1 : pf;

         // Divisão Flat-Field usando inteiros escalados para o 'max' da imagem original
         int n = ( po * orig->max ) / f;

         // Realce de Fundo Branco:
         // Se passar do limiar, estoura para o valor máximo para limpar o fundo.
         dest->image[r][c] = ( n > limiar_branco ) ? ( guint8 )orig->max : ( guint8 )n;
      }
   }

   // 4. Limpeza das matrizes temporárias
   liberar_matriz_pixels( fundo_h.image, rows );
   liberar_matriz_pixels( fundo.image, rows );
}





/**
 * Aplica filtro de remoção de sombras e iluminação de fundo (Magic Color/Flat-Field)
 * mantendo as cores originais da caneta e marcações para geração de PDFs limpos.
 */
void filtrar_fundo_magico_colorido( const ImagemColorida *orig, ImagemColorida *dest, gsize raio_blur ) {
   if ( !orig || !orig->image || !dest || orig->nrow <= 0 || orig->ncol <= 0 || raio_blur <= 5 ) {
      return;
   }

   gsize rows = orig->nrow;
   gsize cols = orig->ncol;

   // 1. Aloca a estrutura da imagem de destino se necessário
   if ( dest->image == NULL ) {
      dest->nrow = orig->nrow;
      dest->ncol = orig->ncol;
      dest->max = orig->max;
      g_strlcpy( dest->key, orig->key, sizeof( dest->key ) );
      dest->image = alocar_matriz_pixels_colorida( rows, cols );
   }

   // 2. Passo A: Desfoque Separável (O Segredo da Velocidade)
   // Precisamos de duas matrizes: uma intermediária para o desfoque horizontal
   // e a matriz final de fundo para o vertical.
   ImagemColorida fundo_h = {0};
   ImagemColorida fundo   = {0};
   fundo_h.image = alocar_matriz_pixels_colorida( rows, cols );
   fundo.image   = alocar_matriz_pixels_colorida( rows, cols );

   // Pré-calcula a quantidade de vizinhos para evitar o "conta++" no laço interno
   int qtd_vizinhos = 0;
   for ( gsize d = -raio_blur; d <= raio_blur; d += 2 ) {
      qtd_vizinhos++;
   }

   // 2.1 Desfoque Horizontal (Linhas)
   #pragma omp parallel for schedule(dynamic) if( !omp_in_parallel() )
   for ( gsize r = 0; r < rows; r++ ) {
      for ( gsize c = 0; c < cols; c++ ) {
         long sr = 0, sg = 0, sb = 0;

         for ( gsize dc = -raio_blur; dc <= raio_blur; dc += 2 ) {
            gsize nc = c + dc;
            // Clamp otimizado inline sem macros genéricas
            if ( nc >= cols ) {
               nc = cols - 1;
            }

            sr += orig->image[r][nc].r;
            sg += orig->image[r][nc].g;
            sb += orig->image[r][nc].b;
         }
         qtd_vizinhos = ( qtd_vizinhos == 0 ) ? 1 : qtd_vizinhos;
         fundo_h.image[r][c].r = ( unsigned char )( sr / qtd_vizinhos );
         fundo_h.image[r][c].g = ( unsigned char )( sg / qtd_vizinhos );
         fundo_h.image[r][c].b = ( unsigned char )( sb / qtd_vizinhos );
      }
   }

   // 2.2 Desfoque Vertical (Colunas) - Lê da fundo_h e escreve na fundo
   #pragma omp parallel for schedule(dynamic) if( !omp_in_parallel() )
   for ( gsize r = 0; r < rows; r++ ) {
      for ( gsize c = 0; c < cols; c++ ) {
         long sr = 0, sg = 0, sb = 0;

         for ( gsize dr = -raio_blur; dr <= raio_blur; dr += 2 ) {
            gsize nr = r + dr;
            if ( nr >= rows ) {
               nr = rows - 1;
            }

            sr += fundo_h.image[nr][c].r;
            sg += fundo_h.image[nr][c].g;
            sb += fundo_h.image[nr][c].b;
         }
         fundo.image[r][c].r = ( unsigned char )( sr / qtd_vizinhos );
         fundo.image[r][c].g = ( unsigned char )( sg / qtd_vizinhos );
         fundo.image[r][c].b = ( unsigned char )( sb / qtd_vizinhos );
      }
   }

   // 3. Passo B: Normalização Flat-Field Otimizada
   #pragma omp parallel for schedule(static) if( !omp_in_parallel() )
   for ( gsize r = 0; r < rows; r++ ) {
      for ( gsize c = 0; c < cols; c++ ) {

         // Acesso direto e linear em memória
         PixelRGB po = orig->image[r][c];
         PixelRGB pf = fundo.image[r][c];

         // Evita divisão por zero (sem saltos condicionais caros)
         int fr = ( pf.r < 1 ) ? 1 : pf.r;
         int fg = ( pf.g < 1 ) ? 1 : pf.g;
         int fb = ( pf.b < 1 ) ? 1 : pf.b;

         // Divisão Flat-Field usando INTEIROS (Extremamente mais rápido que double)
         int nr = ( po.r * 255 ) / fr;
         int ng = ( po.g * 255 ) / fg;
         int nb = ( po.b * 255 ) / fb;

         // Realce de Fundo Branco Otimizado!
         // Se for maior que 230, vira 255. Isso elimina o perigo de passar de 255
         // e remove completamente a necessidade da macro CLAMP e de números negativos.
         dest->image[r][c].r = ( nr > 230 ) ? 255 : ( unsigned char )nr;
         dest->image[r][c].g = ( ng > 230 ) ? 255 : ( unsigned char )ng;
         dest->image[r][c].b = ( nb > 230 ) ? 255 : ( unsigned char )nb;
      }
   }

   // 4. Limpeza das matrizes temporárias
   liberar_matriz_pixels_colorida( fundo_h.image, rows );
   liberar_matriz_pixels_colorida( fundo.image, rows );
}



/**
 * Realça o contraste e a saturação da imagem in-place.
 * @param img Matriz de pixels a ser alterada.
 * @param contraste Fator de contraste (1.0 = original, > 1.0 aumenta. Ex: 1.2)
 * @param saturacao Fator de saturação (1.0 = original, > 1.0 aumenta. Ex: 1.5)
 */
void realcar_cores_in_place( ImagemColorida *img, float contraste, float saturacao ) {
   if ( !img || !img->image ) {
      return;
   }

   gsize nrow = img->nrow;
   gsize ncol = img->ncol;

   for ( gsize i = 0; i < nrow; i++ ) {
      for ( gsize j = 0; j < ncol; j++ ) {
         PixelRGB *p = &img->image[i][j];

         float r = ( float )p->r;
         float g = ( float )p->g;
         float b = ( float )p->b;

         // 1. Aplicação de Contraste (afasta do ponto central 128)
         r = 128.0f + ( r - 128.0f ) * contraste;
         g = 128.0f + ( g - 128.0f ) * contraste;
         b = 128.0f + ( b - 128.0f ) * contraste;

         // 2. Aplicação de Saturação
         // Calcula a luminância (brilho percebido do pixel) usando os pesos standard da ITU-R
         float luma = 0.299f * r + 0.587f * g + 0.114f * b;

         // Afasta os canais individuais da sua versão em tons de cinza
         r = luma + ( r - luma ) * saturacao;
         g = luma + ( g - luma ) * saturacao;
         b = luma + ( b - luma ) * saturacao;

         // 3. Clamping seguro (impede que os valores ultrapassem 0 e 255)
         r = fmaxf( 0.0f, fminf( r, 255.0f ) );
         g = fmaxf( 0.0f, fminf( g, 255.0f ) );
         b = fmaxf( 0.0f, fminf( b, 255.0f ) );

         // 4. Arredondamento e devolução ao pixel
         p->r = ( guint8 )( r + 0.5f );
         p->g = ( guint8 )( g + 0.5f );
         p->b = ( guint8 )( b + 0.5f );
      }
   }
}
