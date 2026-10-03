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

#include "comum.h"
#include "omr.h"
#include "basicas.h"
#include "interface.h"




//-----------------------------------------------------------------------------------------------------
// 1. Função auxiliar inline: Ordenação polar manual para 4 itens
// Refinada com 'const' correctness para evitar warnings do compilador.
void ordenar_polar_inline( const BlobInfo *candidatos[4], const BlobInfo *ordenados[4] ) {
   double cx = 0.0, cy = 0.0;
   for ( int i = 0; i < 4; i++ ) {
      cy += candidatos[i]->centro_i;
      cx += candidatos[i]->centro_j;
   }
   cy /= 4.0;
   cx /= 4.0;

   double angulos[4];
   for ( int i = 0; i < 4; i++ ) {
      angulos[i] = atan2( candidatos[i]->centro_i - cy, candidatos[i]->centro_j - cx );
   }

   // Selection sort otimizado para pequenos arrays
   int idx[4] = {0, 1, 2, 3};
   for ( int i = 0; i < 3; i++ ) {
      for ( int j = i + 1; j < 4; j++ ) {
         if ( angulos[idx[i]] > angulos[idx[j]] ) {
            int tmp = idx[i];
            idx[i] = idx[j];
            idx[j] = tmp;
         }
      }
   }

   for ( int i = 0; i < 4; i++ ) {
      ordenados[i] = candidatos[idx[i]];
   }
}

// 2. Ordenação por "solidez" (Fill Ratio) para priorizar quadrados verdadeiros
static int comparar_solidez_blob( const void *a, const void *b ) {
   const BlobInfo *ba = ( const BlobInfo * )a;
   const BlobInfo *bb = ( const BlobInfo * )b;

   double f_a = ( double )ba->area / ( ( ba->max_i - ba->min_i + 1 ) * ( ba->max_j - ba->min_j + 1 ) );
   double f_b = ( double )bb->area / ( ( bb->max_i - bb->min_i + 1 ) * ( bb->max_j - bb->min_j + 1 ) );

   return ( f_a < f_b ) - ( f_a > f_b ); // Ordem decrescente
}



// static double gas_calcular_area_poligono_blob( const BlobInfo *ord[4] ) {
//    g_return_val_if_fail( ord, 0.0 );
//
//    double soma = 0.0;
//    int n_ancoras = 4;
//
//    for ( int i = 0; i < n_ancoras; i++ ) {
//       // O operador modulo (%) garante que o próximo vértice após o último seja o primeiro (0)
//       int proximo = ( i + 1 ) % n_ancoras;
//
//       // Coordenadas do vértice atual (i) e do próximo (proximo) - X=centro_j, Y=centro_i
//       double x_atual   = ord[i]->centro_j;
//       double y_atual   = ord[i]->centro_i;
//
//       double x_proximo = ord[proximo]->centro_j;
//       double y_proximo = ord[proximo]->centro_i;
//
//       // Produto cruzado em 2D (Determinante da matriz 2x2)
//       soma += ( x_atual * y_proximo ) - ( x_proximo * y_atual );
//    }
//
//    // A área é a metade do módulo do determinante acumulado
//    return fabs( soma ) / 2.0;
// }

/**
 * Calcula o desvio ortogonal das 4 quinas do quadrilátero.
 * O retorno é a média dos cossenos absolutos (0.0 = Retângulo Perfeito, ângulos de 90º).
 */
static double gas_erro_ortogonal( const double p0[2], const double p1[2],
                                  const double p2[2], const double p3[2],
                                  const double top_w, const double bot_w,
                                  const double left_h, const double right_h ) {

   // Vetores partindo de cada vértice (Produto Escalar)
   // Canto 0 (Top-Esq): Vetor para P1 e Vetor para P3
   double dp0 = ( p1[0] - p0[0] ) * ( p3[0] - p0[0] ) + ( p1[1] - p0[1] ) * ( p3[1] - p0[1] );

   // Canto 1 (Top-Dir): Vetor para P0 e Vetor para P2
   double dp1 = ( p0[0] - p1[0] ) * ( p2[0] - p1[0] ) + ( p0[1] - p1[1] ) * ( p2[1] - p1[1] );

   // Canto 2 (Bot-Dir): Vetor para P1 e Vetor para P3
   double dp2 = ( p1[0] - p2[0] ) * ( p3[0] - p2[0] ) + ( p1[1] - p2[1] ) * ( p3[1] - p2[1] );

   // Canto 3 (Bot-Esq): Vetor para P2 e Vetor para P0
   double dp3 = ( p2[0] - p3[0] ) * ( p0[0] - p3[0] ) + ( p2[1] - p3[1] ) * ( p0[1] - p3[1] );

   // O erro é a média dos cossenos absolutos de cada quina.
   // Como a área na função principal é garantida > 100, não há risco de divisão por zero.
   double cos0 = fabs( dp0 ) / ( top_w * left_h );
   double cos1 = fabs( dp1 ) / ( top_w * right_h );
   double cos2 = fabs( dp2 ) / ( bot_w * right_h );
   double cos3 = fabs( dp3 ) / ( bot_w * left_h );

   return ( cos0 + cos1 + cos2 + cos3 ) / 4.0;
}


/**
 * Função auxiliar inteligente para separar a âncora quadrada de riscos de caneta.
 * Utiliza histogramas de projeção ortogonal (X e Y) para detectar "caudas" e amputá-las,
 * recalculando o Bounding Box estrito e o Centroide apenas com a massa sólida.
 */
static void aparar_riscos_blob( BlobInfo *blob, const int *fila_i, const int *fila_j, int num_pixels, int *proj_x, int *proj_y ) {
   int w = blob->max_j - blob->min_j + 1;
   int h = blob->max_i - blob->min_i + 1;

   if ( w <= 0 || h <= 0 || num_pixels <= 0 ) return;

   // Limpa apenas o trecho de memória que será usado para este blob (Alta performance)
   memset( proj_x, 0, w * sizeof( int ) );
   memset( proj_y, 0, h * sizeof( int ) );

   int max_px = 0, max_py = 0;

   // 1. Constrói o histograma de densidade do blob nos eixos X e Y
   for ( int k = 0; k < num_pixels; k++ ) {
      int x = fila_j[k] - blob->min_j;
      int y = fila_i[k] - blob->min_i;

      proj_x[x]++;
      proj_y[y]++;

      if ( proj_x[x] > max_px ) max_px = proj_x[x];
      if ( proj_y[y] > max_py ) max_py = proj_y[y];
   }

   // 2. Define o Limiar de Corte (50% da massa máxima projetada).
   // Um risco de caneta tem espessura muito menor que a largura do quadrado âncora.
   int limiar_x = ( int )( max_px * 0.5 );
   int limiar_y = ( int )( max_py * 0.5 );

   int novo_min_j = blob->min_j, novo_max_j = blob->max_j;
   int novo_min_i = blob->min_i, novo_max_i = blob->max_i;

   // 3. Amputa as extremidades de baixa densidade (Caudas/Riscos)
   for ( int j = 0; j < w; j++ ) {
      if ( proj_x[j] >= limiar_x ) { novo_min_j = blob->min_j + j; break; }
   }
   for ( int j = w - 1; j >= 0; j-- ) {
      if ( proj_x[j] >= limiar_x ) { novo_max_j = blob->min_j + j; break; }
   }
   for ( int i = 0; i < h; i++ ) {
      if ( proj_y[i] >= limiar_y ) { novo_min_i = blob->min_i + i; break; }
   }
   for ( int i = h - 1; i >= 0; i-- ) {
      if ( proj_y[i] >= limiar_y ) { novo_max_i = blob->min_i + i; break; }
   }

   // 4. Consolida o Bounding Box limpo e recalcula Área e Centroide exatos
   blob->min_i = novo_min_i;
   blob->max_i = novo_max_i;
   blob->min_j = novo_min_j;
   blob->max_j = novo_max_j;
   blob->area = 0;

   double soma_i = 0, soma_j = 0;

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
      blob->centro_i = soma_i / blob->area;
      blob->centro_j = soma_j / blob->area;
   }
}

/**
 * Varre a imagem, extrai candidatos OMR (com inteligência contra riscos)
 * e utiliza Avaliação Geométrica para encontrar os 4 quadrados formadores.
 */
int extrair_quadrados_pretos( const ImagemCinza *img_bin, BlobInfo ancora_final[4], char *direcao_inferida ) {
   int nrow = img_bin->nrow;
   int ncol = img_bin->ncol;

   g_autofree uint8_t *visitado = g_new0( uint8_t, nrow * ncol );
   g_autofree int *fila_i = g_new( int, nrow * ncol );
   g_autofree int *fila_j = g_new( int, nrow * ncol );

   // Alocação única para os arrays de histograma usados na poda de riscos (Sem gargalos de malloc)
   g_autofree int *proj_x = g_new( int, ncol );
   g_autofree int *proj_y = g_new( int, nrow );

   int limiar_binario = img_bin->max / 2;
   BlobInfo blobs[128];
   int num_blobs = 0;

   // Extração BFS (Flood Fill)
   for ( int i = 0; i < nrow; i++ ) {
      for ( int j = 0; j < ncol; j++ ) {
         if ( img_bin->image[i][j] < limiar_binario && !visitado[i * ncol + j] ) {
            int start_idx = 0, end_idx = 0;
            fila_i[end_idx] = i; fila_j[end_idx] = j; end_idx++; visitado[i * ncol + j] = 1;

            BlobInfo blob = { i, i, j, j, 0, 0.0, 0.0 };

            while ( start_idx < end_idx ) {
               int ci = fila_i[start_idx], cj = fila_j[start_idx]; start_idx++;

               if ( ci < blob.min_i ) blob.min_i = ci;
               if ( ci > blob.max_i ) blob.max_i = ci;
               if ( cj < blob.min_j ) blob.min_j = cj;
               if ( cj > blob.max_j ) blob.max_j = cj;

               for ( int di = -1; di <= 1; di++ ) {
                  for ( int dj = -1; dj <= 1; dj++ ) {
                     int ni = ci + di, nj = cj + dj;
                     if ( ni >= 0 && ni < nrow && nj >= 0 && nj < ncol ) {
                        if ( img_bin->image[ni][nj] < limiar_binario && !visitado[ni * ncol + nj] ) {
                           fila_i[end_idx] = ni; fila_j[end_idx] = nj; end_idx++; visitado[ni * ncol + nj] = 1;
                        }
                     }
                  }
               }
            } // Fim BFS

            // ===== INTELIGÊNCIA CONTRA RISCOS DE CANETA =====
            // Ao fim do BFS, a função apara falhas finas nas bordas do Bounding Box capturado
            aparar_riscos_blob( &blob, fila_i, fila_j, end_idx, proj_x, proj_y );

            int altura = blob.max_i - blob.min_i + 1;
            int largura = blob.max_j - blob.min_j + 1;
            double aspect_ratio = ( double )largura / ( double )altura;
            double fill_ratio = ( double )blob.area / ( (double)largura * altura );

            if ( blob.area > 50 && blob.area < 6000 && aspect_ratio >= 0.5 && aspect_ratio <= 2.0 && fill_ratio > 0.6 ) {
               if ( num_blobs < 128 ) blobs[num_blobs++] = blob;
            }
         }
      }
   }

   if ( num_blobs < 4 ) return 0;

   if ( num_blobs > 30 ) {
      qsort( blobs, num_blobs, sizeof( BlobInfo ), comparar_solidez_blob );
      num_blobs = 30;
   }

   // ... [Resto do código de Busca Combinatória mantém-se INTACTO] ...

   // =========================================================================
   // NOVA ARQUITETURA: BUSCA COMBINATÓRIA COM FITNESS GEOMÉTRICO (Agora com Ortogonalidade)
   // =========================================================================
   double menor_erro_global = 1e9;
   gboolean encontrou_padrao = FALSE;

   for ( int i = 0; i < num_blobs - 3; i++ ) {
      for ( int j = i + 1; j < num_blobs - 2; j++ ) {
         for ( int k = j + 1; k < num_blobs - 1; k++ ) {
            for ( int l = k + 1; l < num_blobs; l++ ) {

               // 1. Filtro Rápido de Área
               double a0 = blobs[i].area, a1 = blobs[j].area, a2 = blobs[k].area, a3 = blobs[l].area;
               double max_a = fmax( fmax( a0, a1 ), fmax( a2, a3 ) );
               double min_a = fmin( fmin( a0, a1 ), fmin( a2, a3 ) );
               if ( max_a / min_a > 2.0 ) continue;

               // 2. Ordenação Polar Rápida
               const BlobInfo *candidatos[4] = { &blobs[i], &blobs[j], &blobs[k], &blobs[l] };
               const BlobInfo *ord[4];
               ordenar_polar_inline( candidatos, ord );

               // 3. Medidas reais das arestas do polígono usando limites EXTERNOS
               double top_w   = hypot( ord[1]->max_j - ord[0]->min_j, ord[1]->min_i - ord[0]->min_i );
               double bot_w   = hypot( ord[2]->max_j - ord[3]->min_j, ord[2]->max_i - ord[3]->max_i );
               double left_h  = hypot( ord[3]->min_j - ord[0]->min_j, ord[3]->max_i - ord[0]->min_i );
               double right_h = hypot( ord[2]->max_j - ord[1]->max_j, ord[2]->max_i - ord[1]->min_i );

               double larg = ( top_w + bot_w ) / 2.0;
               double alt  = ( left_h + right_h ) / 2.0;

               if ( larg < 50.0 || alt < 50.0 ) continue;

               // 4. Erro de Paralelogramo (Lados opostos devem ser equivalentes)
               double erro_paralelogramo = ( fabs( top_w - bot_w ) / larg ) + ( fabs( left_h - right_h ) / alt );
               if ( erro_paralelogramo > 0.25 ) continue;

               // 4.1. Erro de Ortogonalidade (Garante que seja um Retângulo, não um Rombo inclinado)
               // Extração dos vértices externos (p[0] = X = coluna j, p[1] = Y = linha i)
               double p0[2] = { ord[0]->min_j, ord[0]->min_i }; // Top-Esq
               double p1[2] = { ord[1]->max_j, ord[1]->min_i }; // Top-Dir
               double p2[2] = { ord[2]->max_j, ord[2]->max_i }; // Bot-Dir
               double p3[2] = { ord[3]->min_j, ord[3]->max_i }; // Bot-Esq

               double erro_ortogonal = gas_erro_ortogonal( p0, p1, p2, p3, top_w, bot_w, left_h, right_h );
               if ( erro_ortogonal > 0.20 ) continue; // Rejeita se as quinas fugirem muito de 90 graus (cos 0.20 = ~78º ou 101º)

               // 5. Fórmula de Direção e Proporção
               char dir = ( - ord[0]->min_i - ord[1]->min_i + ord[2]->max_i + ord[3]->max_i <
                            - ord[0]->min_j + ord[1]->max_j + ord[2]->max_j - ord[3]->min_j ) ? 'h' : 'v';

               double proporcao_alvo = ( dir == 'h' ) ? ( 14.0 / 11.0 ) : ( 10.0 / 15.0 );
               double proporcao_real = larg / alt;
               double erro_proporcao = fabs( proporcao_real - proporcao_alvo ) / proporcao_alvo;

               // 6. Erro de Área das Âncoras
               double erro_area_ancoras = ( max_a - min_a ) / max_a;

               // 7. Estratégia dos Cantos Extremos
               double ci_A = ord[0]->centro_i, cj_A = ord[0]->centro_j;
               double ci_B = ord[1]->centro_i, cj_B = ord[1]->centro_j;
               double ci_C = ord[2]->centro_i, cj_C = ord[2]->centro_j;
               double ci_D = ord[3]->centro_i, cj_D = ord[3]->centro_j;

               double d_tl = (ci_A * ci_A) + (cj_A * cj_A);
               double d_tr = (ci_B * ci_B) + ((ncol - cj_B) * (ncol - cj_B));
               double d_br = ((nrow - ci_C) * (nrow - ci_C)) + ((ncol - cj_C) * (ncol - cj_C));
               double d_bl = ((nrow - ci_D) * (nrow - ci_D)) + (cj_D * cj_D);

               double max_dist_quad = (double)(nrow * nrow) + (double)(ncol * ncol);
               double erro_extremos = (d_tl + d_tr + d_br + d_bl) / max_dist_quad;

               // 8. Cálculo do Erro Total da Função de Aptidão (Fitness com blindagem ortogonal)
               double erro_total = erro_proporcao + erro_paralelogramo + erro_ortogonal + ( erro_area_ancoras * 0.5 ) + erro_extremos;

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

   return encontrou_padrao ? 4 : 0;
}

/**
 * Pipeline OMR Determinístico Atualizado.
 * O trabalho geométrico e de blindagem contra ruído é feito na extração.
 */
gboolean detectar_ancoras_omr( const ImagemCinza *img_bin, IndiceMatriz ancora[4], char *direcao_inferida ) {
   BlobInfo melhores_blobs[4];

   // Busca integrada: retorna as 4 âncoras validadas geometricamente e ordenadas
   if ( extrair_quadrados_pretos( img_bin, melhores_blobs, direcao_inferida ) < 4 ) {
      return FALSE;
   }

   // Construção do Quadrilátero Envolvente usando a orientação validada
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
//-----------------------------------------------------------------------------------------------------




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
gboolean decodificar_payload_matriz( MapeamentoGabarito *map, const LimitesFiltro *limite ) {
   // 1. Validação defensiva dos ponteiros de saída usando macros da GLib
   g_return_val_if_fail( map != NULL, FALSE );
   g_return_val_if_fail( limite != NULL, FALSE );

   // 2. Executa o teste de integridade via checksum/paridade
   if ( !verificar_paridade_matriz( map->payload ) ) {
      // g_warning("Falha na integridade do payload lido: paridade invalida.");
      return FALSE;
   }

   // 3. Desempacotamento (Bitwise Shift reverso e aplicação de máscaras)
   map->id    = ( uint8_t )( map->payload & 0x3F );      // Isola os bits 0 a 5
   map->turma = ( uint8_t )( ( map->payload >> 6 )  & 0xFF ); // Desloca 6 bits e isola 8 bits (6 a 13)
   map->disc  = ( uint8_t )( ( map->payload >> 14 ) & 0x0F ); // Desloca 14 bits e isola 4 bits (14 a 17)
   map->per   = ( uint8_t )( ( map->payload >> 18 ) & 0x07 ); // Desloca 18 bits e isola 3 bits (18 a 20)
   map->seq   = ( uint8_t )( ( map->payload >> 21 ) & 0x03 ); // Desloca 21 bits e isola 2 bits (21 a 22)

   // A sequência da prova é o único valor que não pode ser zero (valores 1, 2 e 3)
   // Pois a leitura do binário numa área branca retornava um payload válido onde todos os bits são zero
   // Mudando a sequência da prova de 0, 1, 2 para 1, 2, 3 elimina essa vulnerabilidade.
   if ( map->turma >= limite->turmas || map->per + 1 >= limite->periodos || map->seq == 0 ) {
      return FALSE;
   }

   return TRUE;
}







/**
 * Extrai o payload de 27 bits armazenado na grade 3x9 da imagem PGM binarizada.
 * Suporta orientações horizontais (14p x 11p) e verticais (10p x 15p).
 */
uint32_t extrair_payload_matriz( const ImagemCinza *IMG, char direcao ) {
   // 1. Validações defensivas robustas via GLib
   g_return_val_if_fail( IMG != NULL, 0 );
   g_return_val_if_fail( IMG->image != NULL, 0 );
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
   int limiar_binario = IMG->max / 2;

   // Kernel dinâmico ajustado para cobrir precisamente ~50% da área total do quadrado (raio = 35% do lado).
   // Isso equivale a uma varredura de 70% da largura e 70% da altura (0.7 * 0.7 ≈ 0.50).
   int raio_x = MAX( 1, ( int )( ( p_x / 3.0 ) * 0.8 ) );
   int raio_y = MAX( 1, ( int )( ( p_y / 3.0 ) * 0.8 ) );

   // 3. Varredura dos 27 bits do payload
   for ( int i = 0; i < 27; i++ ) {

      int linha  = ( direcao == 'h' )  ?  i / 3  :  i % 3;
      int coluna = ( direcao == 'h' )  ?  i % 3  :  i / 3;

      double x_centro_p, y_centro_p;
      double passo_grade = 2.0 / 3.0;

      x_centro_p = 1.0 + ( coluna * passo_grade ) + ( passo_grade / 2.0 );
      y_centro_p = 1.0 + ( linha  * passo_grade ) + ( passo_grade / 2.0 );

      // Converte a unidade fracionária 'p' para a coordenada real de pixels
      int px = ( int )( x_centro_p * p_x );
      int py = ( int )( y_centro_p * p_y );

      // Garante que o centro calculado está dentro dos limites físicos da imagem
      if ( px >= 0 && px < IMG->ncol && py >= 0 && py < IMG->nrow ) {
         int pixels_pretos = 0;
         int cont_pixels = 0;

         // 5. Votação por Maioria usando o Kernel Dinâmico Adaptativo
         for ( int dy = -raio_y; dy <= raio_y; dy++ ) {
            for ( int dx = -raio_x; dx <= raio_x; dx++ ) {
               int nx = px + dx;
               int ny = py + dy;

               if ( nx >= 0 && nx < IMG->ncol && ny >= 0 && ny < IMG->nrow ) {
                  if ( IMG->image[ny][nx] < limiar_binario ) {
                     pixels_pretos++;
                  }
                  cont_pixels++;
                  // IMG->image[ny][nx] = 0; // apenas para verificação visual
               }
            }
         }

         // Valida o bit se a densidade de tinta no miolo do quadrado for maior que 50%
         if ( pixels_pretos > ( cont_pixels / 3 ) ) {
            payload_lido |= ( 1U << i );
         }
      }
   }

   return payload_lido;
}




static float calcular_densidade_celula( const ImagemCinza *IMG, int i_celula, int j_celula,
                                        float p_x, float p_y, float raio, float raio_quad ) {
   float cx = ( j_celula + 0.5f ) * p_x;
   float cy = ( i_celula + 0.5f ) * p_y;

   int x_min = ( int )floorf( cx - raio );
   int x_max = ( int )ceilf( cx + raio );
   int y_min = ( int )floorf( cy - raio );
   int y_max = ( int )ceilf( cy + raio );

   if ( x_min < 0 ) x_min = 0;
   if ( x_max >= IMG->ncol ) x_max = IMG->ncol - 1;
   if ( y_min < 0 ) y_min = 0;
   if ( y_max >= IMG->nrow ) y_max = IMG->nrow - 1;

   int total_pixels = 0;
   int pixels_pretos = 0;

   int limiar_binario = IMG->max / 2;

   for ( int y = y_min; y <= y_max; y++ ) {
      for ( int x = x_min; x <= x_max; x++ ) {
         float dx = x - cx;
         float dy = y - cy;

         if ( ( dx * dx + dy * dy ) <= raio_quad ) {
            total_pixels++;
            if ( IMG->image[y][x] < limiar_binario ) {
               pixels_pretos++;
            }
         }
      }
   }

   return total_pixels > 0 ? ( float )pixels_pretos / total_pixels : 0.0f;
}





// Função para escanear a matriz de pixels e detectar a resposta assinalada
void ler_respostas_gabarito( const ImagemCinza *IMG, char direcao, char *respostas_out ) {
   if ( !IMG || !IMG->image || !respostas_out ) return;

   // 1. Definição da malha matricial com base na orientação
   int cols_grid = ( direcao == 'h' ) ? 14 : 10;
   int rows_grid = ( direcao == 'h' ) ? 11 : 15;

   // 2. Tamanho da unidade 'p' em pixels nos eixos X e Y
   float p_x = ( float )IMG->ncol / cols_grid;
   float p_y = ( float )IMG->nrow / rows_grid;

   // Para manter a área de busca perfeitamente circular, usamos o menor dos dois 'p'
   float p_min = ( p_x < p_y ) ? p_x : p_y;
   float raio = 0.35f * p_min;
   float raio_quad = raio * raio; // Evita usar sqrt() dentro dos laços

   // 3. Varredura das 10 questões
   for ( int q = 0; q < 10; q++ ) {

      float max_densidade = -1.0f;
      int alt_marcada = -1; // 0=A, 1=B, 2=C, 3=D, 4=E

      // Varredura das 5 alternativas para a questão atual
      for ( int alt = 0; alt < 5; alt++ ) {
         int i_celula, j_celula;

         if ( direcao == 'h' ) {
            j_celula = q + 3;     // Colunas: 3 a 12
            i_celula = alt + 2;   // Linhas:  2 a 6
         } else {
            // Transposição Matricial ('v')
            i_celula = q + 3;     // Linhas:  3 a 12
            j_celula = alt + 2;   // Colunas: 2 a 6
         }

         // Calcula a porcentagem de preenchimento da bolinha
         float densidade = calcular_densidade_celula( IMG, i_celula, j_celula, p_x, p_y, raio, raio_quad );

         // Registra a alternativa com a marcação mais forte
         if ( densidade > max_densidade ) {
            max_densidade = densidade;
            alt_marcada = alt;
         }
      }

      // 4. Critério de Aceitação (Tratamento de rasuras e questões em branco)
      // Exigimos no mínimo 50% de preenchimento para considerar a bolinha marcada.
      // Caso contrário, a questão foi deixada em branco (assinalada com '-').
      if ( max_densidade > 0.50f ) {
         respostas_out[q] = 'A' + alt_marcada;
      } else {
         respostas_out[q] = '-';
      }
   }

   // Finaliza a string C corretamente
   respostas_out[10] = '\0';
}






int ler_numero_aluno( const ImagemCinza *IMG, char direcao ) {
   if ( !IMG || !IMG->image ) return -1;

   int cols_grid = ( direcao == 'h' ) ? 14 : 10;
   int rows_grid = ( direcao == 'h' ) ? 11 : 15;

   float p_x = ( float )IMG->ncol / cols_grid;
   float p_y = ( float )IMG->nrow / rows_grid;

   float p_min = ( p_x < p_y ) ? p_x : p_y;
   float raio = 0.35f * p_min;
   float raio_quad = raio * raio;

   int dezena_marcada = -1;
   float max_den_dezena = -1.0f;

   // Varredura da DEZENA (0 a 6)
   for ( int val = 0; val <= 6; val++ ) {
      int i_cel = ( direcao == 'h' ) ? 7 : 3 + val;
      int j_cel = ( direcao == 'h' ) ? 3 + val : 7;

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
      int i_cel = ( direcao == 'h' ) ? 8 : 3 + val;
      int j_cel = ( direcao == 'h' ) ? 3 + val : 8;

      float densidade = calcular_densidade_celula( IMG, i_cel, j_cel, p_x, p_y, raio, raio_quad );

      if ( densidade > max_den_unidade ) {
         max_den_unidade = densidade;
         unidade_marcada = val;
      }
   }

   // Tratamento de segurança (Limiar de marcação em 50%)
   // Se o aluno deixou em branco ou a marcação for fraca demais, retornamos -1
   // para o programa principal acionar um alerta no painel de revisão.
   if ( max_den_dezena <= 0.50f || max_den_unidade <= 0.50f ) {
      return -1;
   }

   return ( dezena_marcada * 10 ) + unidade_marcada;
}









//========================================================================================================//
void mudar_numero_na_imagem( float l, float h, float pp, int rot, Ponto2D *C, int n0, int n, unsigned char t, char *img, const InterfaceDados *dados ) {

   char str[2000], sx[100], sy[100];

   if ( dados->periodo[0] == 'R' ) {
      sprintf( str, "cp ./dados/'Gabaritos'/'%s'/'%s'/'%s'/'Imagens Recuperação Final'/%s 'Relatórios e Provas'/", dados->ano, dados->disciplina, dados->escola, img );
      if ( system( str ) == -1 ) {
         fprintf( stderr, "Erro crítico: Não foi possível executar o comando: %s\n", str );
      }
   } else {
      sprintf( str, "cp ./dados/'Gabaritos'/'%s'/'%s'/'%s'/'Imagens %s Prova%d'/%s 'Relatórios e Provas'/", dados->ano, dados->disciplina, dados->escola, dados->periodo, dados->iprova, img );
      if ( system( str ) == -1 ) {
         fprintf( stderr, "Erro crítico: Não foi possível executar o comando: %s\n", str );
      }
   }

   FILE *p = fopen( "Relatórios e Provas/img.tex", "w+" );

   fprintf( p, "\\documentclass[11pt]{report}\n" );
   fprintf( p, "\\usepackage[latin1]{inputenc}\n" );
   fprintf( p, "\\usepackage[T1]{fontenc}\n" );
   fprintf( p, "\\usepackage[brazil]{babel}\n" );
   fprintf( p, "\\usepackage{cmbright,ifthen,setspace,tikz}\n" );
   fprintf( p, "\\usetikzlibrary{calc}\n" );
   fprintf( p, "\\usepackage{wallpaper}\n" );
   fprintf( p, "\\pagestyle{empty}\n" );
   fprintf( p, "\\usepackage[bottom=0cm,top=0cm,left=0cm,right=0cm]{geometry}\n" );

   sprintf( sx, "%10.6f", l );
   sx[3] = '.';
   sprintf( sy, "%10.6f", h );
   sy[3] = '.';
   fprintf( p, "\\geometry{paperwidth=%scm,paperheight=%scm}\n", sx, sy );

   fprintf( p, "\\pgfmathsetmacro{\\rot}{%d}\n", rot );
   sprintf( str, "%10.6f", pp );
   str[3] = '.';
   fprintf( p, "\\pgfmathsetmacro{\\p}{%s}\n", str );

   fprintf( p, "\\ThisULCornerWallPaper{1}{%s}\n", img );

   fprintf( p, "\\makeatletter\n" );
   fprintf( p, "\\newcommand{\\dist}[3]{\n" );
   fprintf( p, "\\tikz@scan@one@point\\pgfutil@firstofone($#2-#3$)\\relax\n" );
   fprintf( p, "\\pgfmathsetmacro{#1}{round(0.99626*veclen(\\the\\pgf@x,\\the\\pgf@y)/0.0283465)/1000}\n" );
   fprintf( p, "}\n" );
   fprintf( p, "\\makeatother\n" );

   fprintf( p, "\\begin{document}\n" );

   fprintf( p, "{\\noindent\\small\n" );
   fprintf( p, "\\begin{tikzpicture}[baseline=(current bounding box.center)]\n" );

   sprintf( sx, "%10.6f", C[0].x );
   sx[3] = '.';
   sprintf( sy, "%10.6f", C[0].y );
   sy[3] = '.';
   fprintf( p, "\\coordinate (A) at (%s,-%s);\n", sy, sx );
   sprintf( sx, "%10.6f", C[1].x );
   sx[3] = '.';
   sprintf( sy, "%10.6f", C[1].y );
   sy[3] = '.';
   fprintf( p, "\\coordinate (B) at (%s,-%s);\n", sy, sx );
   sprintf( sx, "%10.6f", C[2].x );
   sx[3] = '.';
   sprintf( sy, "%10.6f", C[2].y );
   sy[3] = '.';
   fprintf( p, "\\coordinate (C) at (%s,-%s);\n", sy, sx );
   sprintf( sx, "%10.6f", C[3].x );
   sx[3] = '.';
   sprintf( sy, "%10.6f", C[3].y );
   sy[3] = '.';
   fprintf( p, "\\coordinate (D) at (%s,-%s);\n", sy, sx );

   fprintf( p, "\\ifthenelse{\\rot=90}{\n" );
   fprintf( p, "\\coordinate (F) at (B);\n" );
   fprintf( p, "\\coordinate (B) at (D);\n" );
   fprintf( p, "\\coordinate (D) at (F);\n" );
   fprintf( p, "}{}\n" );

   fprintf( p, "\\dist{\\dAB}{(A)}{(B)};\n" );
   fprintf( p, "\\dist{\\dBC}{(B)}{(C)};\n" );
   fprintf( p, "\\dist{\\dCD}{(C)}{(D)};\n" );
   fprintf( p, "\\dist{\\dAD}{(A)}{(D)};\n" );

   fprintf( p, "\\fill (0,0) circle (0pt);\n" );

   // Muda a dezena do número
   fprintf( p, "\\pgfmathsetmacro{\\a}{0.75}\n" );
   fprintf( p, "\\pgfmathsetmacro{\\d}{\\dAD/(\\dBC/\\a+\\dAD-\\dBC)}\n" );
   fprintf( p, "\\coordinate (X) at ($(A)!\\d!(B)$);\n" );
   fprintf( p, "\\coordinate (Y) at ($(D)!\\d!(C)$);\n" );
   if ( n0 / 10 != n / 10 && t >> 6 & 1 ) {
      fprintf( p, "\\pgfmathsetmacro{\\a}{7/28+%d/14}\n", n0 / 10 );
      fprintf( p, "\\pgfmathsetmacro{\\d}{\\dAB/(\\dCD/\\a+\\dAB-\\dCD)}\n" );
      fprintf( p, "\\fill[color=white] ($(X)!\\d!(Y)$) circle (0.24*\\p);\n" );
      fprintf( p, "\\pgfmathsetmacro{\\a}{7/28+%d/14}\n", n / 10 );
      fprintf( p, "\\pgfmathsetmacro{\\d}{\\dAB/(\\dCD/\\a+\\dAB-\\dCD)}\n" );
      fprintf( p, "\\fill[color=black] ($(X)!\\d!(Y)$) circle (0.24*\\p);\n" );
   }

   // Insere a dezena do número quando este está ausente
   if ( !( t >> 6 & 1 ) ) {
      fprintf( p, "\\pgfmathsetmacro{\\a}{7/28+%d/14}\n", n / 10 );
      fprintf( p, "\\pgfmathsetmacro{\\d}{\\dAB/(\\dCD/\\a+\\dAB-\\dCD)}\n" );
      fprintf( p, "\\fill[color=black] ($(X)!\\d!(Y)$) circle (0.24*\\p);\n" );
   }

   // Muda a unidade do número
   fprintf( p, "\\pgfmathsetmacro{\\a}{0.85}\n" );
   fprintf( p, "\\pgfmathsetmacro{\\d}{\\dAD/(\\dBC/\\a+\\dAD-\\dBC)}\n" );
   fprintf( p, "\\coordinate (X) at ($(A)!\\d!(B)$);\n" );
   fprintf( p, "\\coordinate (Y) at ($(D)!\\d!(C)$);\n" );
   if ( n0 % 10 != n % 10 && t >> 7 & 1 ) {
      fprintf( p, "\\pgfmathsetmacro{\\a}{7/28+%d/14}\n", n0 % 10 );
      fprintf( p, "\\pgfmathsetmacro{\\d}{\\dAB/(\\dCD/\\a+\\dAB-\\dCD)}\n" );
      fprintf( p, "\\fill[color=white] ($(X)!\\d!(Y)$) circle (0.24*\\p);\n" );
      fprintf( p, "\\pgfmathsetmacro{\\a}{7/28+%d/14}\n", n % 10 );
      fprintf( p, "\\pgfmathsetmacro{\\d}{\\dAB/(\\dCD/\\a+\\dAB-\\dCD)}\n" );
      fprintf( p, "\\fill[color=black] ($(X)!\\d!(Y)$) circle (0.24*\\p);\n" );
   }

   // Insere a unidade do número quando este está ausente
   if ( !( t >> 7 & 1 ) ) {
      fprintf( p, "\\pgfmathsetmacro{\\a}{7/28+%d/14}\n", n % 10 );
      fprintf( p, "\\pgfmathsetmacro{\\d}{\\dAB/(\\dCD/\\a+\\dAB-\\dCD)}\n" );
      fprintf( p, "\\fill[color=black] ($(X)!\\d!(Y)$) circle (0.24*\\p);\n" );
   }

   fprintf( p, "\\end{tikzpicture}}\n" );
   fprintf( p, "\\end{document}\n" );

   fclose( p );

   if ( chdir( "Relatórios e Provas/" ) != 0 ) {
      perror( "Erro ao acessar diretório de Relatórios" );
   }
   if ( system( "pdflatex -synctex=1 -interaction=nonstopmode img.tex" ) == -1 ) {
      fprintf( stderr, "Erro crítico: Não foi possível executar o pdflatex\n" );
   }
   sprintf( str, "convert -density 300 img.pdf -quality 100 %s", img );
   if ( system( str ) == -1 ) {
      fprintf( stderr, "Erro crítico: Não foi possível executar o comando: %s\n", str );
   }
   if ( dados->periodo[0] == 'R' ) {
      sprintf( str, "cp %s ../dados/'Gabaritos'/'%s'/'%s'/'%s'/'Imagens Recuperação Final'/", img, dados->ano, dados->disciplina, dados->escola );
   } else {
      sprintf( str, "cp %s ../dados/'Gabaritos'/'%s'/'%s'/'%s'/'Imagens %s Prova%d'/", img, dados->ano, dados->disciplina, dados->escola, dados->periodo, dados->iprova );
   }

   if ( system( str ) == -1 ) {
      fprintf( stderr, "Erro crítico: Não foi possível executar o comando: %s\n", str );
   }
   if ( system( "rm -f *.aux *.log *.synctex.gz *.tex *.pdf" ) == -1 ) {
      fprintf( stderr, "Erro crítico: Não foi possível remover arquivos latex\n" );
   }
   if ( chdir( "../" ) != 0 ) {
      perror( "Erro ao acessar diretório de Relatórios" );
   }

}
//========================================================================================================//



