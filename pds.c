/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "pds.h"
#include "imgcore.h"






// A Estrutura de Comunicação (O "Contrato" geométrico)
typedef struct {
   float x_origem;
   float y_origem;
   float ux; // Passo horizontal X
   float uy; // Passo horizontal Y
   float vx; // Passo vertical X
   float vy; // Passo vertical Y
   int largura_destino;
   int altura_destino;
} MapeamentoAfim;

// ====================================================================================
// MOTOR GRÁFICO (Única função que faz a interpolação pesada e usa o OpenMP)
// ====================================================================================
static void aplicar_transformacao_afim( const ImagemCinza *IMG, ImagemCinza *img, const MapeamentoAfim *mapa ) {
   if ( !IMG || !img || !mapa ) return;

   if ( mapa->largura_destino <= 0 || mapa->altura_destino <= 0 ) return;

   if ( img->image != NULL ) {
      liberar_matriz_pixels( img->image, img->nrow );
   }

   img->ncol = mapa->largura_destino;
   img->nrow = mapa->altura_destino;
   img->image = alocar_matriz_pixels( img->nrow, img->ncol );
   if ( !img->image ) return;

   g_strlcpy( img->key, IMG->key, sizeof( img->key ) );
   img->max = IMG->max;

   int x_lim = IMG->ncol - 1;
   int y_lim = IMG->nrow - 1;

   // #pragma omp parallel for schedule(static)
   for ( int y_novo = 0; y_novo < img->nrow; y_novo++ ) {
      for ( int x_novo = 0; x_novo < img->ncol; x_novo++ ) {

         float x_orig = mapa->x_origem + ( y_novo * mapa->vx ) + ( x_novo * mapa->ux );
         float y_orig = mapa->y_origem + ( y_novo * mapa->vy ) + ( x_novo * mapa->uy );

         int x_base = ( int )floorf( x_orig );
         int y_base = ( int )floorf( y_orig );

         float dx = x_orig - x_base;
         float dy = y_orig - y_base;

         int pA, pB, pC, pD;

         if ( x_base >= 0 && x_base < x_lim && y_base >= 0 && y_base < y_lim ) {
            pA = IMG->image[y_base][x_base];
            pB = IMG->image[y_base][x_base + 1];
            pC = IMG->image[y_base + 1][x_base];
            pD = IMG->image[y_base + 1][x_base + 1];
         } else {
            int x0 = ( x_base < 0 ) ? 0 : ( x_base > x_lim ) ? x_lim : x_base;
            int x1 = ( x_base + 1 < 0 ) ? 0 : ( x_base + 1 > x_lim ) ? x_lim : x_base + 1;
            int y0 = ( y_base < 0 ) ? 0 : ( y_base > y_lim ) ? y_lim : y_base;
            int y1 = ( y_base + 1 < 0 ) ? 0 : ( y_base + 1 > y_lim ) ? y_lim : y_base + 1;

            pA = ( x_base >= 0 && x_base <= x_lim && y_base >= 0 && y_base <= y_lim ) ? IMG->image[y0][x0] : 255;
            pB = ( x_base + 1 >= 0 && x_base + 1 <= x_lim && y_base >= 0 && y_base <= y_lim ) ? IMG->image[y0][x1] : 255;
            pC = ( x_base >= 0 && x_base <= x_lim && y_base + 1 >= 0 && y_base + 1 <= y_lim ) ? IMG->image[y1][x0] : 255;
            pD = ( x_base + 1 >= 0 && x_base + 1 <= x_lim && y_base + 1 >= 0 && y_base + 1 <= y_lim ) ? IMG->image[y1][x1] : 255;
         }

         float interpolado = pA * ( 1.0f - dx ) * ( 1.0f - dy ) +
                             pB * dx * ( 1.0f - dy ) +
                             pC * ( 1.0f - dx ) * dy +
                             pD * dx * dy;

         img->image[y_novo][x_novo] = ( int )( interpolado + 0.5f );
      }
   }
}




// ====================================================================================
// MOTOR GRÁFICO RGB (Apenas executa a matemática definida no MapeamentoAfim)
// ====================================================================================
static void aplicar_transformacao_afim_colorida( const ImagemColorida *IMG, ImagemColorida *img, const MapeamentoAfim *mapa ) {
   if ( !IMG || !img || !mapa ) return;

   if ( mapa->largura_destino <= 0 || mapa->altura_destino <= 0 ) return;

   if ( img->image != NULL ) {
      liberar_matriz_pixels_colorida( img->image, img->nrow );
   }

   img->ncol = mapa->largura_destino;
   img->nrow = mapa->altura_destino;
   img->image = alocar_matriz_pixels_colorida( img->nrow, img->ncol );
   if ( !img->image ) return;

   g_strlcpy( img->key, IMG->key, sizeof( img->key ) );
   img->max = IMG->max;

   int x_lim = IMG->ncol - 1;
   int y_lim = IMG->nrow - 1;

   // #pragma omp parallel for schedule(static)
   for ( int y_novo = 0; y_novo < img->nrow; y_novo++ ) {
      for ( int x_novo = 0; x_novo < img->ncol; x_novo++ ) {

         float x_orig = mapa->x_origem + ( y_novo * mapa->vx ) + ( x_novo * mapa->ux );
         float y_orig = mapa->y_origem + ( y_novo * mapa->vy ) + ( x_novo * mapa->uy );

         int x_base = ( int )floorf( x_orig );
         int y_base = ( int )floorf( y_orig );

         float dx = x_orig - x_base;
         float dy = y_orig - y_base;

         PixelRGB pA, pB, pC, pD;

         // [FAST PATH] - Interior da imagem
         if ( x_base >= 0 && x_base < x_lim && y_base >= 0 && y_base < y_lim ) {
            pA = IMG->image[y_base][x_base];
            pB = IMG->image[y_base][x_base + 1];
            pC = IMG->image[y_base + 1][x_base];
            pD = IMG->image[y_base + 1][x_base + 1];

         // [SLOW PATH] - Borda com fallback para branco
         } else {
            int x0 = ( x_base < 0 ) ? 0 : ( x_base > x_lim ) ? x_lim : x_base;
            int x1 = ( x_base + 1 < 0 ) ? 0 : ( x_base + 1 > x_lim ) ? x_lim : x_base + 1;
            int y0 = ( y_base < 0 ) ? 0 : ( y_base > y_lim ) ? y_lim : y_base;
            int y1 = ( y_base + 1 < 0 ) ? 0 : ( y_base + 1 > y_lim ) ? y_lim : y_base + 1;

            PixelRGB branco = {255, 255, 255};

            pA = ( x_base >= 0 && x_base <= x_lim && y_base >= 0 && y_base <= y_lim ) ? IMG->image[y0][x0] : branco;
            pB = ( x_base + 1 >= 0 && x_base + 1 <= x_lim && y_base >= 0 && y_base <= y_lim ) ? IMG->image[y0][x1] : branco;
            pC = ( x_base >= 0 && x_base <= x_lim && y_base + 1 >= 0 && y_base + 1 <= y_lim ) ? IMG->image[y1][x0] : branco;
            pD = ( x_base + 1 >= 0 && x_base + 1 <= x_lim && y_base + 1 >= 0 && y_base + 1 <= y_lim ) ? IMG->image[y1][x1] : branco;
         }

         // Cálculo dos pesos otimizado (calculado 1 vez, usado 3 vezes)
         float pesoA = ( 1.0f - dx ) * ( 1.0f - dy );
         float pesoB = dx * ( 1.0f - dy );
         float pesoC = ( 1.0f - dx ) * dy;
         float pesoD = dx * dy;

         // Aplicação nos 3 canais de cor
         float r_interp = pA.r * pesoA + pB.r * pesoB + pC.r * pesoC + pD.r * pesoD;
         float g_interp = pA.g * pesoA + pB.g * pesoB + pC.g * pesoC + pD.g * pesoD;
         float b_interp = pA.b * pesoA + pB.b * pesoB + pC.b * pesoC + pD.b * pesoD;

         img->image[y_novo][x_novo].r = ( unsigned char )( r_interp + 0.5f );
         img->image[y_novo][x_novo].g = ( unsigned char )( g_interp + 0.5f );
         img->image[y_novo][x_novo].b = ( unsigned char )( b_interp + 0.5f );
      }
   }
}





void extrair_regiao_por_ancoras( const ImagemCinza *IMG, ImagemCinza *img, const IndiceMatriz *ancora ) {
   if ( !IMG || !img || !ancora ) return;

   float x_B = ancora[1].j, y_B = ancora[1].i;
   float x_C = ancora[2].j, y_C = ancora[2].i;
   float x_D = ancora[3].j, y_D = ancora[3].i;

   float dist_DC_quad = ( x_C - x_D ) * ( x_C - x_D ) + ( y_C - y_D ) * ( y_C - y_D );
   float dist_BC_quad = ( x_C - x_B ) * ( x_C - x_B ) + ( y_C - y_B ) * ( y_C - y_B );

   MapeamentoAfim mapa;
   mapa.largura_destino = ( int )roundf( sqrtf( dist_DC_quad ) );
   mapa.altura_destino  = ( int )roundf( sqrtf( dist_BC_quad ) );

   if ( mapa.largura_destino <= 0 || mapa.altura_destino <= 0 ) return;

   mapa.x_origem = x_B + x_D - x_C;
   mapa.y_origem = y_B + y_D - y_C;

   mapa.ux = ( x_C - x_D ) / mapa.largura_destino;
   mapa.uy = ( y_C - y_D ) / mapa.largura_destino;
   mapa.vx = ( x_C - x_B ) / mapa.altura_destino;
   mapa.vy = ( y_C - y_B ) / mapa.altura_destino;

   aplicar_transformacao_afim( IMG, img, &mapa );
}




void extrair_regiao_colorida_por_ancoras( const ImagemColorida *IMG, ImagemColorida *img, const IndiceMatriz *ancora ) {
   if ( !IMG || !img || !ancora ) return;

   float x_B = ancora[1].j, y_B = ancora[1].i;
   float x_C = ancora[2].j, y_C = ancora[2].i;
   float x_D = ancora[3].j, y_D = ancora[3].i;

   float dist_DC_quad = ( x_C - x_D ) * ( x_C - x_D ) + ( y_C - y_D ) * ( y_C - y_D );
   float dist_BC_quad = ( x_C - x_B ) * ( x_C - x_B ) + ( y_C - y_B ) * ( y_C - y_B );

   MapeamentoAfim mapa;
   mapa.largura_destino = ( int )roundf( sqrtf( dist_DC_quad ) );
   mapa.altura_destino  = ( int )roundf( sqrtf( dist_BC_quad ) );

   if ( mapa.largura_destino <= 0 || mapa.altura_destino <= 0 ) return;

   mapa.x_origem = x_B + x_D - x_C;
   mapa.y_origem = y_B + y_D - y_C;

   mapa.ux = ( x_C - x_D ) / mapa.largura_destino;
   mapa.uy = ( y_C - y_D ) / mapa.largura_destino;
   mapa.vx = ( x_C - x_B ) / mapa.altura_destino;
   mapa.vy = ( y_C - y_B ) / mapa.altura_destino;

   aplicar_transformacao_afim_colorida( IMG, img, &mapa );
}




void rotacionar_imagem( const ImagemCinza *IMG, ImagemCinza *img, float angulo_graus ) {
   // Converte para radianos
   float rad = angulo_graus * ( G_PI / 180.0f );
   float cos_a = cosf( rad );
   float sin_a = sinf( rad );

   MapeamentoAfim mapa;
   // Em uma rotação pura, as dimensões da imagem destino mudam para acomodar as pontas
   mapa.largura_destino = (int)(fabs(IMG->ncol * cos_a) + fabs(IMG->nrow * sin_a));
   mapa.altura_destino  = (int)(fabs(IMG->nrow * cos_a) + fabs(IMG->ncol * sin_a));

   // Vetores diretores simples baseados no ângulo
   mapa.ux = cos_a;
   mapa.uy = -sin_a;
   mapa.vx = sin_a;
   mapa.vy = cos_a;

   // Calcula a origem para centralizar
   mapa.x_origem = (IMG->ncol / 2.0f) - (mapa.largura_destino / 2.0f) * mapa.ux - (mapa.altura_destino / 2.0f) * mapa.vx;
   mapa.y_origem = (IMG->nrow / 2.0f) - (mapa.largura_destino / 2.0f) * mapa.uy - (mapa.altura_destino / 2.0f) * mapa.vy;

   aplicar_transformacao_afim( IMG, img, &mapa );
}







void rotacionar_imagem_colorida( const ImagemColorida *IMG, ImagemColorida *img, float angulo_graus ) {
   float rad = angulo_graus * ( G_PI / 180.0f );
   float cos_a = cosf( rad );
   float sin_a = sinf( rad );

   MapeamentoAfim mapa;
   mapa.largura_destino = (int)(fabs(IMG->ncol * cos_a) + fabs(IMG->nrow * sin_a));
   mapa.altura_destino  = (int)(fabs(IMG->nrow * cos_a) + fabs(IMG->ncol * sin_a));

   mapa.ux = cos_a;
   mapa.uy = -sin_a;
   mapa.vx = sin_a;
   mapa.vy = cos_a;

   mapa.x_origem = (IMG->ncol / 2.0f) - (mapa.largura_destino / 2.0f) * mapa.ux - (mapa.altura_destino / 2.0f) * mapa.vx;
   mapa.y_origem = (IMG->nrow / 2.0f) - (mapa.largura_destino / 2.0f) * mapa.uy - (mapa.altura_destino / 2.0f) * mapa.vy;

   aplicar_transformacao_afim_colorida( IMG, img, &mapa );
}






void cortar_imagem_bilinear( const ImagemCinza *IMG, ImagemCinza *img, const IndiceMatriz *ancora ) {
   if ( !IMG || !img || !ancora ) return;

   // Coordenadas dos 3 pontos âncoras de base (B, C e D)
   float x_B = ancora[1].j, y_B = ancora[1].i;
   float x_C = ancora[2].j, y_C = ancora[2].i;
   float x_D = ancora[3].j, y_D = ancora[3].i;

   // 1. Dedução do Ponto A (Canto Superior Esquerdo)
   // Usado como ponto de origem inabalável para o mapeamento reverso
   float x_A = x_B + x_D - x_C;
   float y_A = y_B + y_D - y_C;

   // 2. Cálculo de dimensões (Baseado nas arestas opostas)
   // A largura é a distância entre D e C. A altura é a distância entre B e C.
   float dist_DC_quad = ( x_C - x_D ) * ( x_C - x_D ) + ( y_C - y_D ) * ( y_C - y_D );
   float dist_BC_quad = ( x_C - x_B ) * ( x_C - x_B ) + ( y_C - y_B ) * ( y_C - y_B );

   img->ncol = ( int )roundf( sqrtf( dist_DC_quad ) );
   img->nrow = ( int )roundf( sqrtf( dist_BC_quad ) );

   if ( img->ncol <= 0 || img->nrow <= 0 ) return;

   if ( img->image != NULL ) {
      liberar_matriz_pixels( img->image, img->nrow );
   }

   img->image = alocar_matriz_pixels( img->nrow, img->ncol );
   if ( !img->image ) return;

   g_strlcpy( img->key, IMG->key, sizeof( img->key ) );
   img->max = IMG->max;

   // 3. Vetores diretores de passo unitário
   // ux/uy (Passo Horizontal): Direção do vetor D->C
   float ux = ( x_C - x_D ) / img->ncol;
   float uy = ( y_C - y_D ) / img->ncol;

   // vx/vy (Passo Vertical): Direção do vetor B->C
   float vx = ( x_C - x_B ) / img->nrow;
   float vy = ( y_C - y_B ) / img->nrow;

   int x_lim = IMG->ncol - 1;
   int y_lim = IMG->nrow - 1;

   // 4. Mapeamento Reverso com Interpolação Bilinear
   // #pragma omp parallel for schedule(static)
   for ( int y_novo = 0; y_novo < img->nrow; y_novo++ ) {

      for ( int x_novo = 0; x_novo < img->ncol; x_novo++ ) {

         // O mapa reverso usa o ponto A deduzido matematicamente como origem
         float x_orig = x_A + ( y_novo * vx ) + ( x_novo * ux );
         float y_orig = y_A + ( y_novo * vy ) + ( x_novo * uy );

         int x_base = ( int )floorf( x_orig );
         int y_base = ( int )floorf( y_orig );

         float dx = x_orig - x_base;
         float dy = y_orig - y_base;

         int pA, pB, pC, pD;

         // [FAST PATH] - Caso ultra-comum
         if ( x_base >= 0 && x_base < x_lim && y_base >= 0 && y_base < y_lim ) {
            pA = IMG->image[y_base][x_base];
            pB = IMG->image[y_base][x_base + 1];
            pC = IMG->image[y_base + 1][x_base];
            pD = IMG->image[y_base + 1][x_base + 1];

            // [SLOW PATH] - Borda (Clamping com fallback para branco)
         } else {
            int x0 = ( x_base < 0 ) ? 0 : ( x_base > x_lim ) ? x_lim : x_base;
            int x1 = ( x_base + 1 < 0 ) ? 0 : ( x_base + 1 > x_lim ) ? x_lim : x_base + 1;
            int y0 = ( y_base < 0 ) ? 0 : ( y_base > y_lim ) ? y_lim : y_base;
            int y1 = ( y_base + 1 < 0 ) ? 0 : ( y_base + 1 > y_lim ) ? y_lim : y_base + 1;

            pA = ( x_base >= 0 && x_base <= x_lim && y_base >= 0 && y_base <= y_lim ) ? IMG->image[y0][x0] : 255;
            pB = ( x_base + 1 >= 0 && x_base + 1 <= x_lim && y_base >= 0 && y_base <= y_lim ) ? IMG->image[y0][x1] : 255;
            pC = ( x_base >= 0 && x_base <= x_lim && y_base + 1 >= 0 && y_base + 1 <= y_lim ) ? IMG->image[y1][x0] : 255;
            pD = ( x_base + 1 >= 0 && x_base + 1 <= x_lim && y_base + 1 >= 0 && y_base + 1 <= y_lim ) ? IMG->image[y1][x1] : 255;
         }

         // Cálculo dos pesos bilineares otimizado
         float interpolado = pA * ( 1.0f - dx ) * ( 1.0f - dy ) +
                             pB * dx * ( 1.0f - dy ) +
                             pC * ( 1.0f - dx ) * dy +
                             pD * dx * dy;

         img->image[y_novo][x_novo] = ( int )( interpolado + 0.5f );
      }
   }
}






void cortar_imagem_colorida_bilinear( const ImagemColorida *IMG, ImagemColorida *img, const IndiceMatriz *ancora ) {
   if ( !IMG || !img || !ancora ) return;

   float x_B = ancora[1].j, y_B = ancora[1].i;
   float x_C = ancora[2].j, y_C = ancora[2].i;
   float x_D = ancora[3].j, y_D = ancora[3].i;

   float x_A = x_B + x_D - x_C;
   float y_A = y_B + y_D - y_C;

   float dist_DC_quad = ( x_C - x_D ) * ( x_C - x_D ) + ( y_C - y_D ) * ( y_C - y_D );
   float dist_BC_quad = ( x_C - x_B ) * ( x_C - x_B ) + ( y_C - y_B ) * ( y_C - y_B );

   img->ncol = ( int )roundf( sqrtf( dist_DC_quad ) );
   img->nrow = ( int )roundf( sqrtf( dist_BC_quad ) );

   if ( img->ncol <= 0 || img->nrow <= 0 ) return;

   if ( img->image != NULL ) {
      liberar_matriz_pixels_colorida( img->image, img->nrow );
   }

   img->image = alocar_matriz_pixels_colorida( img->nrow, img->ncol );
   if ( !img->image ) return;

   g_strlcpy( img->key, IMG->key, sizeof( img->key ) );
   img->max = IMG->max;

   float ux = ( x_C - x_D ) / img->ncol;
   float uy = ( y_C - y_D ) / img->ncol;
   float vx = ( x_C - x_B ) / img->nrow;
   float vy = ( y_C - y_B ) / img->nrow;

   int x_lim = IMG->ncol - 1;
   int y_lim = IMG->nrow - 1;

   // #pragma omp parallel for schedule(static)
   for ( int y_novo = 0; y_novo < img->nrow; y_novo++ ) {
      for ( int x_novo = 0; x_novo < img->ncol; x_novo++ ) {

         float x_orig = x_A + ( y_novo * vx ) + ( x_novo * ux );
         float y_orig = y_A + ( y_novo * vy ) + ( x_novo * uy );

         int x_base = ( int )floorf( x_orig );
         int y_base = ( int )floorf( y_orig );

         float dx = x_orig - x_base;
         float dy = y_orig - y_base;

         PixelRGB pA, pB, pC, pD;

         if ( x_base >= 0 && x_base < x_lim && y_base >= 0 && y_base < y_lim ) {
            pA = IMG->image[y_base][x_base];
            pB = IMG->image[y_base][x_base + 1];
            pC = IMG->image[y_base + 1][x_base];
            pD = IMG->image[y_base + 1][x_base + 1];

         } else {
            int x0 = ( x_base < 0 ) ? 0 : ( x_base > x_lim ) ? x_lim : x_base;
            int x1 = ( x_base + 1 < 0 ) ? 0 : ( x_base + 1 > x_lim ) ? x_lim : x_base + 1;
            int y0 = ( y_base < 0 ) ? 0 : ( y_base > y_lim ) ? y_lim : y_base;
            int y1 = ( y_base + 1 < 0 ) ? 0 : ( y_base + 1 > y_lim ) ? y_lim : y_base + 1;

            PixelRGB branco = {255, 255, 255};

            pA = ( x_base >= 0 && x_base <= x_lim && y_base >= 0 && y_base <= y_lim ) ? IMG->image[y0][x0] : branco;
            pB = ( x_base + 1 >= 0 && x_base + 1 <= x_lim && y_base >= 0 && y_base <= y_lim ) ? IMG->image[y0][x1] : branco;
            pC = ( x_base >= 0 && x_base <= x_lim && y_base + 1 >= 0 && y_base + 1 <= y_lim ) ? IMG->image[y1][x0] : branco;
            pD = ( x_base + 1 >= 0 && x_base + 1 <= x_lim && y_base + 1 >= 0 && y_base + 1 <= y_lim ) ? IMG->image[y1][x1] : branco;
         }

         float pesoA = ( 1.0f - dx ) * ( 1.0f - dy );
         float pesoB = dx * ( 1.0f - dy );
         float pesoC = ( 1.0f - dx ) * dy;
         float pesoD = dx * dy;

         float r_interp = pA.r * pesoA + pB.r * pesoB + pC.r * pesoC + pD.r * pesoD;
         float g_interp = pA.g * pesoA + pB.g * pesoB + pC.g * pesoC + pD.g * pesoD;
         float b_interp = pA.b * pesoA + pB.b * pesoB + pC.b * pesoC + pD.b * pesoD;

         img->image[y_novo][x_novo].r = ( unsigned char )( r_interp + 0.5f );
         img->image[y_novo][x_novo].g = ( unsigned char )( g_interp + 0.5f );
         img->image[y_novo][x_novo].b = ( unsigned char )( b_interp + 0.5f );
      }
   }
}







/* Redução com Interpolação Bilinear Otimizada e Paralelizada */
void redimensionar_imagem_bilinear( ImagemCinza *origem, ImagemCinza *destino, int dim ) {
   if ( !origem || !destino || dim <= 0 ) return;

   gboolean deitada = ( origem->ncol > origem->nrow );
   destino->ncol = deitada ? dim : ( dim * origem->ncol ) / origem->nrow;
   destino->nrow = deitada ? ( dim * origem->nrow ) / origem->ncol : dim;

   g_strlcpy( destino->key, origem->key, sizeof( destino->key ) );
   destino->max = origem->max;

   if ( destino->image != NULL ) {
      liberar_matriz_pixels( destino->image, destino->nrow );
   }

   destino->image = alocar_matriz_pixels( destino->nrow, destino->ncol );
   if ( !destino->image ) return;

   // Fatores de proporção (mapeamento reverso alinhando os cantos)
   float x_ratio = ( ( float )( origem->ncol - 1 ) ) / ( destino->ncol > 1 ? destino->ncol - 1 : 1 );
   float y_ratio = ( ( float )( origem->nrow - 1 ) ) / ( destino->nrow > 1 ? destino->nrow - 1 : 1 );

   // Paralelização OpenMP ativada.
   // schedule(static) é perfeito aqui porque o custo computacional de cada linha é exatamente igual.
   // #pragma omp parallel for schedule(static)
   for ( int i = 0; i < destino->nrow; i++ ) {
      // OTIMIZAÇÃO 1: Variáveis declaradas aqui dentro são PRIVADAS para cada thread
      float src_y = y_ratio * i;
      int y = ( int )src_y;
      float y_diff = src_y - y;
      float y_inv = 1.0f - y_diff;

      int y_next = ( y + 1 < origem->nrow ) ? y + 1 : y;

      for ( int j = 0; j < destino->ncol; j++ ) {
         float src_x = x_ratio * j;
         int x = ( int )src_x;
         float x_diff = src_x - x;
         float x_inv = 1.0f - x_diff;

         int x_next = ( x + 1 < origem->ncol ) ? x + 1 : x;

         // OTIMIZAÇÃO 2: Captura limpa e independente
         int a = origem->image[y][x];
         int b = origem->image[y][x_next];
         int c = origem->image[y_next][x];
         int d = origem->image[y_next][x_next];

         float pixel_interpolado = a * x_inv * y_inv +
                                   b * x_diff * y_inv +
                                   c * x_inv * y_diff +
                                   d * x_diff * y_diff;

         // OTIMIZAÇÃO 3: Escrita sem colisão, cada thread escreve na sua própria linha 'i'
         destino->image[i][j] = ( int )( pixel_interpolado + 0.5f );
      }
   }
}






void redimensionar_imagem_colorida_bilinear( ImagemColorida *origem, ImagemColorida *destino, int dim ) {
   if ( !origem || !destino || dim <= 0 ) return;

   gboolean deitada = ( origem->ncol > origem->nrow );
   destino->ncol = deitada ? dim : ( dim * origem->ncol ) / origem->nrow;
   destino->nrow = deitada ? ( dim * origem->nrow ) / origem->ncol : dim;

   // 1. Cópia do Cabeçalho
   g_strlcpy( destino->key, origem->key, sizeof( destino->key ) );
   destino->max = origem->max;

   // 2. Barreira de Segurança (Memory Leak Prevention)
   if ( destino->image != NULL ) {
      liberar_matriz_pixels_colorida( destino->image, destino->nrow );
   }

   // 3. Alocação da Nova Matriz Colorida
   destino->image = alocar_matriz_pixels_colorida( destino->nrow, destino->ncol );
   if ( !destino->image ) return;

   // 4. Fatores de proporção
   float x_ratio = ( ( float )( origem->ncol - 1 ) ) / ( destino->ncol > 1 ? destino->ncol - 1 : 1 );
   float y_ratio = ( ( float )( origem->nrow - 1 ) ) / ( destino->nrow > 1 ? destino->nrow - 1 : 1 );

   // 5. Mapeamento Reverso Paralelizado
   // #pragma omp parallel for schedule(static)
   for ( int i = 0; i < destino->nrow; i++ ) {
      float src_y = y_ratio * i;
      int y = ( int )src_y;
      float y_diff = src_y - y;
      float y_inv = 1.0f - y_diff;

      int y_next = ( y + 1 < origem->nrow ) ? y + 1 : y;

      for ( int j = 0; j < destino->ncol; j++ ) {
         float src_x = x_ratio * j;
         int x = ( int )src_x;
         float x_diff = src_x - x;
         float x_inv = 1.0f - x_diff;

         int x_next = ( x + 1 < origem->ncol ) ? x + 1 : x;

         // OTIMIZAÇÃO DE CORES: Captura da estrutura PixelRGB inteira de uma vez
         PixelRGB pA = origem->image[y][x];
         PixelRGB pB = origem->image[y][x_next];
         PixelRGB pC = origem->image[y_next][x];
         PixelRGB pD = origem->image[y_next][x_next];

         // OTIMIZAÇÃO MATEMÁTICA: Pré-calculamos os pesos espaciais dos 4 vizinhos.
         // Isso economiza 8 multiplicações de ponto flutuante por cada pixel gerado!
         float pesoA = x_inv * y_inv;
         float pesoB = x_diff * y_inv;
         float pesoC = x_inv * y_diff;
         float pesoD = x_diff * y_diff;

         // Aplica os pesos aos 3 canais de forma independente
         float r_interp = pA.r * pesoA + pB.r * pesoB + pC.r * pesoC + pD.r * pesoD;
         float g_interp = pA.g * pesoA + pB.g * pesoB + pC.g * pesoC + pD.g * pesoD;
         float b_interp = pA.b * pesoA + pB.b * pesoB + pC.b * pesoC + pD.b * pesoD;

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
   if ( !IMG || !img || sigma <= 0.0f ) return;

   // 1. Preparação da Imagem de Destino
   if ( img->image != NULL ) {
      liberar_matriz_pixels( img->image, img->nrow );
   }

   img->ncol = IMG->ncol;
   img->nrow = IMG->nrow;
   img->image = alocar_matriz_pixels( img->nrow, img->ncol );
   if ( !img->image ) return;

   g_strlcpy( img->key, IMG->key, sizeof( img->key ) );
   img->max = IMG->max;

   // 2. Cálculo do tamanho do Kernel
   // A regra dos 3-sigmas garante que 99.7% da energia da curva gaussiana seja capturada
   int raio = ( int )ceilf( 3.0f * sigma );
   int diametro = 2 * raio + 1;

   // Alocamos o kernel como um vetor plano (1D) para maximizar o cache da CPU (L1/L2)
   g_autofree float *kernel = g_new0( float, diametro * diametro );

   // 3. Geração do Kernel Isotrópico Normalizado
   float soma_pesos = 0.0f;
   float divisor_expoente = 2.0f * sigma * sigma;
   int idx = 0;

   for ( int i = -raio; i <= raio; i++ ) {
      for ( int j = -raio; j <= raio; j++ ) {
         // Não precisamos da constante externa da fórmula, pois normalizaremos no final
         float peso = expf( -( ( i * i ) + ( j * j ) ) / divisor_expoente );
         kernel[idx++] = peso;
         soma_pesos += peso;
      }
   }

   // Normalização: garante que a soma de todos os pesos seja exatamente 1.0
   for ( int k = 0; k < diametro * diametro; k++ ) {
      kernel[k] /= soma_pesos;
   }

   // 4. Convolução 2D Paralelizada com Padding Virtual (Clamp to Edge)
   int x_lim = IMG->ncol - 1;
   int y_lim = IMG->nrow - 1;

   // #pragma omp parallel for schedule(static)
   for ( int y = 0; y < IMG->nrow; y++ ) {
      for ( int x = 0; x < IMG->ncol; x++ ) {

         float pixel_acumulado = 0.0f;
         int k_idx = 0;

         // [FAST PATH] - O pixel está seguro no centro da imagem (Nenhum if necessário)
         if ( x >= raio && x < IMG->ncol - raio && y >= raio && y < IMG->nrow - raio ) {
            for ( int ky = -raio; ky <= raio; ky++ ) {
               for ( int kx = -raio; kx <= raio; kx++ ) {
                  pixel_acumulado += IMG->image[y + ky][x + kx] * kernel[k_idx++];
               }
            }
         }
         // [SLOW PATH] - O pixel está na borda. Simulamos a "expansão" travando os índices.
         // Se a varredura tentar sair da imagem, ela repete a cor do pixel da beirada.
         else {
            for ( int ky = -raio; ky <= raio; ky++ ) {
               int coord_y = y + ky;
               coord_y = ( coord_y < 0 ) ? 0 : ( ( coord_y > y_lim ) ? y_lim : coord_y );

               for ( int kx = -raio; kx <= raio; kx++ ) {
                  int coord_x = x + kx;
                  coord_x = ( coord_x < 0 ) ? 0 : ( ( coord_x > x_lim ) ? x_lim : coord_x );

                  pixel_acumulado += IMG->image[coord_y][coord_x] * kernel[k_idx++];
               }
            }
         }

         // Arredondamento perfeito para inteiro
         img->image[y][x] = ( int )( pixel_acumulado + 0.5f );
      }
   }
}






void binarizar_pgm_metodo_otsu( ImagemCinza *IMG ) {
   if ( !IMG || !IMG->image ) return;

   int max_val = IMG->max;
   int num_bins = max_val + 1;

   // Alocação limpa com GLib. Para imagens de 12MP, um pixel count cabe no long,
   // mas usamos long long para garantir que nunca haverá overflow na variância.
   g_autofree long long *histograma = g_new0( long long, num_bins );

   // =================================================================================
   // 1. CONSTRUÇÃO DO HISTOGRAMA (Sequencial e Otimizado para Cache L1/L2)
   // =================================================================================
   for ( int i = 0; i < IMG->nrow; i++ ) {
      for ( int j = 0; j < IMG->ncol; j++ ) {
         int val = IMG->image[i][j];
         if ( val >= 0 && val <= max_val ) {
            histograma[val]++;
         }
      }
   }

   // =================================================================================
   // 2. MATEMÁTICA DO MÉTODO DE OTSU (Muito rápido, apenas 256 iterações)
   // =================================================================================
   long long total_pixels = ( long long )IMG->nrow * IMG->ncol;
   double soma_total = 0.0;

   for ( int i = 0; i < num_bins; i++ ) {
      soma_total += ( double )( i * histograma[i] );
   }

   double soma_b = 0.0;
   long long w_b = 0;
   long long w_f = 0;

   double variancia_maxima = 0.0;
   int limiar_otsu = 0;

   // Varre todos os limiares possíveis (0 a 255)
   for ( int t = 0; t < num_bins; t++ ) {
      w_b += histograma[t];              // Peso da classe "Fundo" (Background)
      if ( w_b == 0 ) continue;          // Evita divisão por zero

      w_f = total_pixels - w_b;          // Peso da classe "Frente" (Foreground)
      if ( w_f == 0 ) break;             // Fim dos dados úteis

      soma_b += ( double )( t * histograma[t] );

      // Médias das intensidades do fundo e da frente
      double media_b = soma_b / ( double )w_b;
      double media_f = ( soma_total - soma_b ) / ( double )w_f;

      // Variância Inter-Classes
      double diff = media_b - media_f;
      double variancia_inter = ( double )w_b * ( double )w_f * diff * diff;

      // Guarda o limiar que produziu a maior separação entre branco e preto
      if ( variancia_inter > variancia_maxima ) {
         variancia_maxima = variancia_inter;
         limiar_otsu = t;
      }
   }

   // =================================================================================
   // 3. APLICAÇÃO DO LIMIAR NA IMAGEM (Sequencial)
   // =================================================================================
   for ( int i = 0; i < IMG->nrow; i++ ) {
      for ( int j = 0; j < IMG->ncol; j++ ) {
         IMG->image[i][j] = ( IMG->image[i][j] > limiar_otsu ) ? max_val : 0;
      }
   }
}




