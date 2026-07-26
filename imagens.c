/*
 * Copyright (C) 2026 Edinaldo Barbosa de Alencar
 * Este programa é software livre; você pode redistribuí-lo e/ou
 * modificá-lo sob os termos da Licença Pública Geral GNU...
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <glib/gstdio.h> // Necessário para g_remove e g_mkdir_with_parents

#include "comum.h"
#include "imagens.h"
#include "imgcore.h"
#include "basicas.h"   // Para nFile e Files
#include "gabaritos.h" // Para mudar_numero_na_imagem, gabaritos e imagens_corrigidas
#include "mensagens.h"
#include "glibrary.h"
#include "latex.h"
#include "pds.h"






//========================================================================================================//
static void nome_aleatorio( char *nome, size_t tam ) {
   const char alfanumerico[] = "0123456789abcdefghijklmnopqrstuvwxyz";
   size_t len = tam - 1;

   for ( size_t j = 0; j < len; j++ ) {
      int i = g_random_int_range( 0, 36 );
      nome[j] = alfanumerico[i];
   }
   nome[len] = '\0';
}

static int converter_e_copiar_imagens( const char *origem, const char *destino, ItemTextoCurto **imgs_png ) {
   if ( !origem || !destino || !imgs_png ) return 0;

   GError *error = NULL;

   GDir *dir = g_dir_open( origem, 0, &error );
   if ( dir == NULL ) {
      g_printerr( "Aviso: Não foi possível abrir a pasta de origem: %s (%s)\n", origem, error->message );
      g_clear_error( &error );
      return 0;
   }

   // 1. FASE DE COLETA: Vetor dinâmico da GLib para guardar os nomes válidos
   GPtrArray *arquivos_validos = g_ptr_array_new_with_free_func( g_free );
   const char *filename;

   while ( ( filename = g_dir_read_name( dir ) ) != NULL ) {
      char *filename_lc = g_utf8_strdown( filename, -1 );

      gboolean e_imagem = ( g_str_has_suffix( filename_lc, ".png" )  ||
                            g_str_has_suffix( filename_lc, ".jpg" )  ||
                            g_str_has_suffix( filename_lc, ".jpeg" ) ||
                            g_str_has_suffix( filename_lc, ".bmp" ) );
      g_free( filename_lc );

      if ( e_imagem ) {
         // Guarda uma cópia da string no vetor
         g_ptr_array_add( arquivos_validos, g_strdup( filename ) );
      }
   }
   g_dir_close( dir );

   *imgs_png = g_new0( ItemTextoCurto, arquivos_validos->len );

   // 2. FASE DE PROCESSAMENTO PARALELO
   // OTIMIZAÇÃO: Usamos 'schedule(dynamic)' em vez de 'static'.
   // Como arquivos de imagem têm tamanhos diferentes, algumas conversões demoram mais que outras.
   // O 'dynamic' garante que threads rápidas peguem novos arquivos, evitando tempo ocioso.
   #pragma omp parallel for schedule(dynamic)
   for ( guint i = 0; i < arquivos_validos->len; i++ ) {

      // Resgata o nome do arquivo da lista
      const char *file_atual = ( const char * ) g_ptr_array_index( arquivos_validos, i );

      char nome_img[17];
      nome_aleatorio( nome_img, sizeof( nome_img ) );
      snprintf( ( *imgs_png )[i].str, sizeof( ( *imgs_png )[i].str ), "%s.png", nome_img );

      // Montagem de caminhos independente para cada thread
      char *path_origem = g_build_filename( origem, file_atual, NULL );
      char *path_destino = g_build_filename( destino, ( *imgs_png )[i].str, NULL );

      // O processamento interno pesado
      if ( converter_para_png_otimizado( path_origem, path_destino ) ) {
         // g_remove(path_origem);
      } else {
         // g_printerr é thread-safe no Linux, não corrompe o terminal
         g_printerr( "Falha no processamento da imagem: %s\n", file_atual );
      }

      g_free( path_origem );
      g_free( path_destino );
   }

   // 3. LIMPEZA FINAL
   // Libera o vetor e, automaticamente, todas as strings copiadas lá dentro
   int qtd_img = arquivos_validos->len;
   g_ptr_array_unref( arquivos_validos );

   return qtd_img;
}
//========================================================================================================//











// Função unificada: dir_i e dir_j ditam para qual lado o "L-shape" vai crescer (1 ou -1)
static int ponto_de_referencia( ImagemCinza *IMG, IndiceMatriz p, int dir_i, int dir_j, int tamanho_maximo ) {
   if ( !IMG || !IMG->image ) return 0;

   int limiar = 10;

   int soma = 0;
   float taxa_aux = 0.0f;

   // Variáveis para validar a integridade da "quina" (linhas/colunas de origem)
   int soma_linha_origem = 0;
   int soma_coluna_origem = 0;

   // 1. Proteção Direcional de Borda precisa usar (tamanho_maximo - 1)
   int limite_i = p.i + ( ( tamanho_maximo - 1 ) * dir_i );
   int limite_j = p.j + ( ( tamanho_maximo - 1 ) * dir_j );

   if ( limite_i < 0 || limite_i >= IMG->nrow || limite_j < 0 || limite_j >= IMG->ncol ) {
      return 0;
   }

   // 2. Crescimento direcional do quadrado
   for ( int i = 0; i < tamanho_maximo; i++ ) {

      // OTIMIZAÇÃO: Verifica a saúde das bordas da quina em O(1) por camada
      if ( IMG->image[ p.i ][ p.j + ( i * dir_j ) ] < limiar ) soma_linha_origem++;
      if ( IMG->image[ p.i + ( i * dir_i ) ][ p.j ] < limiar ) soma_coluna_origem++;

      // Varre a linha "horizontal" da nova camada L
      for ( int j = 0; j <= i; j++ ) {
         if ( IMG->image[ p.i + ( i * dir_i ) ][ p.j + ( j * dir_j ) ] < limiar ) {
            soma++;
         }
      }

      // Varre a coluna "vertical" da nova camada L
      for ( int r = 0; r < i; r++ ) {
         if ( IMG->image[ p.i + ( r * dir_i ) ][ p.j + ( i * dir_j ) ] < limiar ) {
            soma++;
         }
      }

      int area_total = ( i + 1 ) * ( i + 1 );
      float taxa = ( float )soma / area_total;

      // ====================================================================
      // 3. BLINDAGEM DE DESLIZAMENTO (A Lógica que você propôs)
      // ====================================================================
      float taxa_linha_origem  = ( float )soma_linha_origem / ( i + 1 );
      float taxa_coluna_origem = ( float )soma_coluna_origem / ( i + 1 );

      // Se a quina estiver vazando para a margem branca, ignoramos a taxa
      // global de 83% e simplesmente pulamos o critério de aceite.
      if ( taxa_linha_origem < 0.8f || taxa_coluna_origem < 0.8f ) {
         continue;
      }

      // 4. Critério de aceite e detecção de queda (Peak Detection)
      if ( ( i > 3 ) && ( ( taxa > 0.8f ) || ( taxa_aux > 0.0f ) ) ) {

         // Aceita maior OU igual (>=) para tolerar o preenchimento total
         if ( taxa >= taxa_aux ) {
            taxa_aux = taxa;
         } else {
            // A densidade caiu! Significa que a camada anterior (i-1) era o limite.
            return i;
         }
      }
   }

   return 0;
}




// =====================================================================
// Funções de Varredura dos 4 Cantos (Corrigidas)
// =====================================================================

static IndiceMatriz quadradinho_A( ImagemCinza *IMG, int tam_max ) {
   if ( !IMG ) return ( IndiceMatriz ) {
      -1, -1
      };

   // Canto Superior Esquerdo: cresce para Baixo (+1) e Direita (+1)
   for ( int i = 0; i < IMG->nrow / 2; i++ ) {
      for ( int j = 0; j < IMG->ncol / 2; j++ ) {
         IndiceMatriz p = { .i = i, .j = j };
         int lado_quadradinho = ponto_de_referencia( IMG, p, 1, 1, tam_max );
         if ( lado_quadradinho > 0 ) {
            return p;
         }
      }
   }
   return ( IndiceMatriz ) {
      -1, -1
      };
}

static IndiceMatriz quadradinho_B( ImagemCinza *IMG, int tam_max ) {
   if ( !IMG ) return ( IndiceMatriz ) {
      -1, -1
      };

   // Canto Superior Direito: cresce para Baixo (+1) e Esquerda (-1)
   // CORRIGIDO: j começa em IMG->ncol - 1
   for ( int j = IMG->ncol - 1; j > IMG->ncol / 2; j-- ) {
      for ( int i = 0; i < IMG->nrow / 2; i++ ) {
         IndiceMatriz p = { .i = i, .j = j };
         int lado_quadradinho = ponto_de_referencia( IMG, p, 1, -1, tam_max );
         if ( lado_quadradinho > 0 ) {
            return p;
         }
      }
   }
   return ( IndiceMatriz ) {
      -1, -1
      };
}

static IndiceMatriz quadradinho_C( ImagemCinza *IMG, int tam_max ) {
   if ( !IMG ) return ( IndiceMatriz ) {
      -1, -1
      };

   // Canto Inferior Direito: cresce para Cima (-1) e Esquerda (-1)
   // CORRIGIDO: i e j começam no tamanho máximo - 1
   for ( int i = IMG->nrow - 1; i > IMG->nrow / 2; i-- ) {
      for ( int j = IMG->ncol - 1; j > IMG->ncol / 2; j-- ) {
         IndiceMatriz p = { .i = i, .j = j };
         int lado_quadradinho = ponto_de_referencia( IMG, p, -1, -1, tam_max );
         if ( lado_quadradinho > 0 ) {
            return p;
         }
      }
   }
   return ( IndiceMatriz ) {
      -1, -1
      };
}

static IndiceMatriz quadradinho_D( ImagemCinza *IMG, int tam_max ) {
   if ( !IMG ) return ( IndiceMatriz ) {
      -1, -1
      };

   // Canto Inferior Esquerdo: cresce para Cima (-1) e Direita (+1)
   // CORRIGIDO: i começa em IMG->nrow - 1
   for ( int i = IMG->nrow - 1; i > IMG->nrow / 2; i-- ) {
      for ( int j = 0; j < IMG->ncol / 2; j++ ) {
         IndiceMatriz p = { .i = i, .j = j };
         int lado_quadradinho = ponto_de_referencia( IMG, p, -1, 1, tam_max );
         if ( lado_quadradinho > 0 ) {
            return p;
         }
      }
   }
   return ( IndiceMatriz ) {
      -1, -1
      };
}



typedef struct {
   int distorcido;
   int culpado;
   double grau_erro;
   char direcao;     // Agora a função retorna a direção correta!
} AnaliseAncoras;

static AnaliseAncoras verificar_ancoras( const IndiceMatriz *p ) {
   AnaliseAncoras resultado = {0, -1, 0.0, 'x'};
   if ( !p ) return resultado;

   // =======================================================================
   // 1. ACHAR A "QUINA IMACULADA" E SALVAR TODOS OS ÂNGULOS
   // =======================================================================
   double menor_cos_sq = 2.0;
   int melhor_vertice = 0;

   // double cos_sq_array[4];

   for ( int k = 0; k < 4; k++ ) {
      int prev = ( k + 3 ) % 4;
      int next = ( k + 1 ) % 4;

      long long v1_i = p[prev].i - p[k].i;
      long long v1_j = p[prev].j - p[k].j;
      long long v2_i = p[next].i - p[k].i;
      long long v2_j = p[next].j - p[k].j;

      long long dot = ( v1_i * v2_i ) + ( v1_j * v2_j );
      long long mag1 = ( v1_i * v1_i ) + ( v1_j * v1_j );
      long long mag2 = ( v2_i * v2_i ) + ( v2_j * v2_j );

      double cos_sq = 1.0;
      if ( mag1 != 0 && mag2 != 0 ) {
         cos_sq = ( double )( dot * dot ) / ( double )( mag1 * mag2 );
      }

      // cos_sq_array[k] = cos_sq;

      if ( cos_sq < menor_cos_sq ) {
         menor_cos_sq = cos_sq;
         melhor_vertice = k;
      }
   }

   // =======================================================================
   // 2. EXTRAIR A DIREÇÃO DAS ARESTAS IMACULADAS
   // =======================================================================
   int k = melhor_vertice;
   double edge_prev = hypot( p[( k + 3 ) % 4].i - p[k].i, p[( k + 3 ) % 4].j - p[k].j );
   double edge_next = hypot( p[( k + 1 ) % 4].i - p[k].i, p[( k + 1 ) % 4].j - p[k].j );

   double good_w, good_h;
   if ( k == 0 ) {
      good_w = edge_next;
      good_h = edge_prev;
   } else if ( k == 1 ) {
      good_w = edge_prev;
      good_h = edge_next;
   } else if ( k == 2 ) {
      good_w = edge_next;
      good_h = edge_prev;
   } else             {
      good_w = edge_prev;
      good_h = edge_next;
   }

   resultado.direcao = ( good_h < good_w ) ? 'h' : 'v';

   // =======================================================================
   // 3. ANÁLISE DE DISTORÇÃO DO PARALELOGRAMO
   // =======================================================================
   long long err_i = ( long long )p[0].i - p[1].i + p[2].i - p[3].i;
   long long err_j = ( long long )p[0].j - p[1].j + p[2].j - p[3].j;
   long long distorcao_sq = ( err_i * err_i ) + ( err_j * err_j );

   long long diag_sq = ( long long )( p[2].i - p[0].i ) * ( p[2].i - p[0].i ) +
                       ( long long )( p[2].j - p[0].j ) * ( p[2].j - p[0].j );

   if ( diag_sq == 0 ) {
      resultado.distorcido = 1;
      return resultado;
   }

   double threshold_sq = 0.03 * 0.03;
   double razao_sq = ( double )distorcao_sq / ( double )diag_sq;
   resultado.grau_erro = sqrt( razao_sq );

   // =======================================================================
   // 4. VEREDITO FINAL E DEFESA CONTRA O INSCRITÍVEL
   // =======================================================================
   if ( razao_sq > threshold_sq ) {
      resultado.distorcido = 1; // Faltava sinalizar a distorção!

      // Usa o melhor_vertice que o laço inicial (passo 1) já encontrou
      int suposto_culpado = ( melhor_vertice + 2 ) % 4;

      // Consulta no vetor se o ângulo do oposto também é suspeitamente bom
      // if (cos_sq_array[suposto_culpado] < 0.03) {
      //    resultado.culpado = -1; // Aborta a restauração
      // } else {
      resultado.culpado = suposto_culpado; // Restaura com segurança
      // }
   } else {
      resultado.distorcido = 0;
      resultado.culpado = -1;
   }

   return resultado;
}


static int validar_proporcao_final( const IndiceMatriz *ancora, char direcao ) {
   double w_top = hypot( ancora[1].i - ancora[0].i, ancora[1].j - ancora[0].j );
   double w_bot = hypot( ancora[2].i - ancora[3].i, ancora[2].j - ancora[3].j );
   double h_left  = hypot( ancora[3].i - ancora[0].i, ancora[3].j - ancora[0].j );
   double h_right = hypot( ancora[2].i - ancora[1].i, ancora[2].j - ancora[1].j );

   double w_avg = ( w_top + w_bot ) / 2.0;
   double h_avg = ( h_left + h_right ) / 2.0;

   if ( w_avg < 1.0 || h_avg < 1.0 ) return 0; // Proteção contra zeros

   double proporcao_real = h_avg / w_avg;
   double proporcao_esperada = ( direcao == 'h' ) ? ( 11.0 / 14.0 ) : ( 15.0 / 10.0 );

   double erro = fabs( proporcao_real - proporcao_esperada ) / proporcao_esperada;

   // 6% de tolerância para o papel amassado no scanner
   return ( erro <= 0.06 );
}



static void restaurar_ancora_culpada( IndiceMatriz *ancora, int culpado ) {
   if ( !ancora || culpado < 0 || culpado > 3 ) return;

   // Usamos variáveis locais para deixar a equação exatamente igual à teoria matemática
   int i0 = ancora[0].i, j0 = ancora[0].j;
   int i1 = ancora[1].i, j1 = ancora[1].j;
   int i2 = ancora[2].i, j2 = ancora[2].j;
   int i3 = ancora[3].i, j3 = ancora[3].j;

   switch ( culpado ) {
   case 0:
      ancora[0].i = i1 + i3 - i2;
      ancora[0].j = j1 + j3 - j2;
      break;
   case 1:
      ancora[1].i = i0 + i2 - i3;
      ancora[1].j = j0 + j2 - j3;
      break;
   case 2:
      ancora[2].i = i1 + i3 - i0;
      ancora[2].j = j1 + j3 - j0;
      break;
   case 3:
      ancora[3].i = i0 + i2 - i1;
      ancora[3].j = j0 + j2 - j1;
      break;
   }

}



static gboolean referencias_e_direcao( ImagemCinza *IMG, MapeamentoGabarito *info, IndiceMatriz *ancora ) {
   if ( !IMG || !info ) return FALSE;
   int tam_max = MAX( IMG->ncol, IMG->nrow ) / 20;

   // 1. Threads buscam os 4 cantos
   #pragma omp parallel sections
   {
      #pragma omp section
      {
         ancora[0] = quadradinho_A( IMG, tam_max );
      }
      #pragma omp section
      {
         ancora[1] = quadradinho_B( IMG, tam_max );
      }
      #pragma omp section
      {
         ancora[2] = quadradinho_C( IMG, tam_max );
      }
      #pragma omp section
      {
         ancora[3] = quadradinho_D( IMG, tam_max );
      }
   }

   // 2. Análise geométrica profunda
   AnaliseAncoras analise = verificar_ancoras( ancora );

   // 3. A direção foi deduzida com segurança cirúrgica ignorando o erro!
   info->direcao = analise.direcao;

   // 4. Salva a prova caso tenha um falso positivo esticando ela
   if ( analise.distorcido ) {
      if ( analise.culpado == -1 ) return FALSE; // Distorção inexplicável (ex: encontrou uma linha)

      g_printerr( "Aviso: Distorção detectada (%.1f%%). Restaurando a quina %d...\n",
                  analise.grau_erro * 100.0, analise.culpado );

      restaurar_ancora_culpada( ancora, analise.culpado );
   }

   // 5. O Veredito: A malha (original ou restaurada) possui a escala que esperamos?
   if ( !validar_proporcao_final( ancora, info->direcao ) ) {
      g_printerr( "Erro Crítico: Proporção da malha não bate com a direção %c. Enviando para Quarentena.\n", info->direcao );
      return FALSE;
   }

   printf( "\ndireção = %c\nA( %3d, %3d )         B( %3d, %3d )\nD( %3d, %3d )         C( %3d, %3d )\n\n", info->direcao,
           ancora[0].i, ancora[0].j, ancora[1].i, ancora[1].j,
           ancora[3].i, ancora[3].j, ancora[2].i, ancora[2].j );

   return TRUE; // Sucesso absoluto! A imagem pode ser recortada perfeitamente.
}



// Atualize a assinatura no .h e no .c para receber o angulo_aplicado
static void normalizar_ancora( const ImagemColorida *img_rgb, const ImagemCinza *img_gray_rot,
                               double angulo_aplicado, IndiceMatriz *ancora )
{
   if ( !img_rgb || !img_gray_rot || !ancora ) return;

   // 1. O ângulo usado para DESENTORTAR a miniatura foi -angulo_aplicado
   double rad = -angulo_aplicado * ( G_PI / 180.0 );
   double cos_a = cos( rad );
   double sin_a = sin( rad );

   // 2. Dimensões reais da imagem RGB Original (que continua torta no disco/memória)
   double W_orig = img_rgb->ncol;
   double H_orig = img_rgb->nrow;

   // 3. Calculamos qual seria o tamanho do Bounding Box da RGB se ela fosse desentortada
   double W_rot_rgb = fabs(W_orig * cos_a) + fabs(H_orig * sin_a);
   double H_rot_rgb = fabs(H_orig * cos_a) + fabs(W_orig * sin_a);

   // 4. Fatores de escala exatos (Cinza Reta -> RGB Reta)
   double scale_x = W_rot_rgb / (double)img_gray_rot->ncol;
   double scale_y = H_rot_rgb / (double)img_gray_rot->nrow;

   // 5. Parâmetros da transformação afim inversa (Para voltar para o espaço torto)
   double ux = cos_a;
   double uy = -sin_a;
   double vx = sin_a;
   double vy = cos_a;

   // Origem do centro de rotação mapeado (idêntico ao cálculo da rotacionar_imagem)
   double x_origem = (W_orig / 2.0) - (W_rot_rgb / 2.0) * ux - (H_rot_rgb / 2.0) * vx;
   double y_origem = (H_orig / 2.0) - (W_rot_rgb / 2.0) * uy - (H_rot_rgb / 2.0) * vy;

   for ( int ii = 0; ii < 4; ii++ ) {
      // Passo A: Escala o ponto para o mundo RGB Reto
      double x_sh = ancora[ii].j * scale_x;
      double y_sh = ancora[ii].i * scale_y;

      // Passo B: Aplica a Rotação Inversa para achar a coordenada na RGB Original (torta)
      double x_skewed = x_origem + ( y_sh * vx ) + ( x_sh * ux );
      double y_skewed = y_origem + ( y_sh * vy ) + ( x_sh * uy );

      // Atualiza a âncora com precisão sub-pixel convertida para inteiro
      ancora[ii].j = ( int )round( x_skewed );
      ancora[ii].i = ( int )round( y_skewed );
   }
}


//========================================================================================================//
int processar_imagens( const InterfaceDados *dados, const LimitesFiltro *limite ) {
   if ( !dados || !limite ) return -1;

   const char *home = g_get_home_dir();
   if ( home == NULL ) home = ".";

   g_autofree char *origem = g_build_filename( home, "Downloads", "imagens", NULL );
   g_autofree char *destino = g_build_filename( ".", "dados", "gabaritos", dados->ano, dados->escola, "imagens", NULL );
   g_autofree char *respostas = g_build_filename( ".", "dados", "gabaritos", dados->ano, dados->escola, "respostas", NULL );
   g_autofree char *dir_rejeitadas = g_build_filename( destino, "rejeitadas", NULL );

   if ( g_mkdir_with_parents( destino, 0755 ) != 0 ||
        g_mkdir_with_parents( dir_rejeitadas, 0755 ) != 0 ||
        g_mkdir_with_parents( respostas, 0755 ) != 0 )
   {
      g_printerr( "Erro crítico: Não foi possível criar os diretórios de destino.\n" );
      return -1;
   }

   g_autofree char *gabaritos = g_build_filename( ".", "dados", "gabaritos", dados->ano, dados->escola, "gabaritos", NULL );
   int qtd_bin = quantidade_arquivos_por_extensao( gabaritos, ".bin" );
   if ( qtd_bin <= 0 ) {
      g_printerr( "[AVISO] Nenhuma prova foi gerada até o momento.\n" );
      return -1;
   }
   ItemTextoCurto *resp_bin = carregar_arquivos_por_extensao( gabaritos, ".bin", qtd_bin );
   qsort( resp_bin, qtd_bin, sizeof( ItemTextoCurto ), comparar_item_texto_curto );

   FILE **f = ( FILE ** ) g_malloc0( qtd_bin * sizeof( FILE * ) );
   for ( int i = 0; i < qtd_bin; i++ ) {
      g_autofree char *arquivo = g_build_filename( respostas, resp_bin[i].str, NULL );
      f[i] = fopen( arquivo, "ab" );
   }

   ItemTextoCurto *imgs_png = NULL;
   int qtd_img = converter_e_copiar_imagens( origem, destino, &imgs_png );

   int n_rejeitadas = 0;

   // =========================================================================
   // LAÇO PARALELO (O OpenMP processará múltiplas imagens simultaneamente)
   // =========================================================================
   #pragma omp parallel for schedule(dynamic) reduction(+:n_rejeitadas)
   for ( int i = 0; i < qtd_img; i++ ) {

      gboolean sucesso = TRUE; // Substitui o goto!

      MapeamentoGabarito info = {0};
      IndiceMatriz ancora[4] = {0};

      ImagemColorida img_rgb_orig  = {0};
      ImagemColorida img_rgb_crop  = {0};
      // ImagemColorida img_rgb_rot  = {0};

      ImagemCinza img_gray_bin   = {0};
      ImagemCinza img_gray_crop  = {0};
      ImagemCinza img_gray_alloc = {0};
      ImagemCinza img_gray_rot  = {0};

      g_autofree char *path_png = g_build_filename( destino, imgs_png[i].str, NULL );
      g_autofree char *path_ppm = trocar_extensao( path_png, "ppm" );

      converter_para_ppm( path_png, path_ppm );

      // FASE 1: Carregamento
      imread( &img_rgb_orig, path_ppm );
      rgb2gray( &img_rgb_orig, &img_gray_bin );
      // binarizar_pgm_metodo_otsu( &img_gray_bin );
      g_remove( path_ppm );

      // FASE 2: Normalização e Deskewing (Em Cinza para Visão)
      int dim = 640;
      ImagemCinza *ptr_gray_work = reduzir_imagem_bilinear( &img_gray_bin, &img_gray_alloc, dim );
      salvar_imagem_pgm( ptr_gray_work, path_ppm ); // Teste

      double angulo_erro = detectar_angulo_inclinacao_cv( ptr_gray_work );
      double angulo_aplicado = 0.0; // <--- NOVA VARIÁVEL

      if ( fabs( angulo_erro ) > 0.2 ) {
         g_printerr( "Gabarito rotacionado em %.2f graus. Corrigindo o prumo...\n", angulo_erro );
         rotacionar_imagem( ptr_gray_work, &img_gray_rot, -angulo_erro );
         ptr_gray_work = &img_gray_rot;
         angulo_aplicado = angulo_erro; // <--- REGISTRAMOS QUE ROTACIONAMOS DE FATO
      }

      // FASE 3: Visão Computacional
      if ( !referencias_e_direcao( ptr_gray_work, &info, ancora ) ) {
         fprintf( stderr, "[ERRO] Falha de âncoras na imagem: %s\n", imgs_png[i].str );
         sucesso = FALSE;
      }

      if ( sucesso ) {
         // FASE 4: Correção Geométrica Dupla
         cortar_imagem_bilinear( ptr_gray_work, &img_gray_crop, ancora );
         // salvar_imagem_pgm( &img_gray_crop, path_ppm ); // Teste

         normalizar_ancora( &img_rgb_orig, ptr_gray_work, angulo_aplicado, ancora );
         cortar_imagem_colorida_bilinear( &img_rgb_orig, &img_rgb_crop, ancora );

         salvar_imagem_png( &img_rgb_crop, path_png );

         // Binarização maravilhosa usando o Método de Otsu
         binarizar_pgm_metodo_otsu( &img_gray_crop );

         // FASE 5: Leitura do Payload
         info.payload = extrair_payload_matriz( &img_gray_crop, info.direcao );

         if ( !decodificar_payload_matriz( &info, limite ) ) {
            fprintf( stderr, "[FALHA] Payload inválido. Imagem: %s\n", imgs_png[i].str );
            sucesso = FALSE;
         }
      }

      // FASE 6: Identificação e Escrita (Se tudo deu certo até aqui)
      if ( sucesso ) {
         ItemTextoCurto chave;
         nome_base_gabaritos_bin( chave.str, sizeof( chave.str ), info.turma, info.disc, info.per, info.seq );
         int j = buscar_indice_bsearch( &chave, resp_bin, qtd_bin, sizeof( chave ), comparar_item_texto_curto );

         if ( j >= 0 && f[j] != NULL ) {
            ler_respostas_gabarito( &img_gray_crop, info.direcao, info.resp );
            info.num = ler_numero_aluno( &img_gray_crop, info.direcao );
            g_strlcpy( info.nome_img, imgs_png[i].str, sizeof( info.nome_img ) );

            // PROTEÇÃO CRÍTICA: Múltiplas threads não podem escrever no mesmo arquivo juntas!
            #pragma omp critical(escrita_binario)
            {
               if ( fwrite( &info, sizeof( MapeamentoGabarito ), 1, f[j] ) != 1 ) {
                  g_printerr( "[ERRO] O registro da imagem %s não foi salvo.\n", imgs_png[i].str );
               }
               fflush( f[j] ); // Garante que o dado vá fisicamente para o disco
            }

            printf( "%3d %3d %3d %3d %3d | Número: %2d | Respostas: %s\n",
                    info.id, info.turma, info.disc, info.per, info.seq, info.num, info.resp );
         } else {
            g_printerr( "[ALERTA] Binário '%s' não encontrado para: %s\n", chave.str, imgs_png[i].str );
            sucesso = FALSE;
         }
      }

      // ====================================================================
      // TRATAMENTO DE ERROS (Quarentena)
      // ====================================================================
      if ( !sucesso ) {
         g_autofree char *path_erro = g_build_filename( dir_rejeitadas, imgs_png[i].str, NULL );
         salvar_imagem_png( &img_rgb_orig, path_erro );
         g_remove( path_png );
         n_rejeitadas++;
      }

      // ====================================================================
      // LIMPEZA SEGURA DE MEMÓRIA
      // ====================================================================
      // Não precisamos mais rastrear o `ptr_gray_work`. Basta checar se a
      // estrutura tem um ponteiro alocado e liberar. Simples, direto e blindado.
      if ( img_gray_crop.image )  liberar_matriz_pixels( img_gray_crop.image, img_gray_crop.nrow );
      if ( img_gray_bin.image )   liberar_matriz_pixels( img_gray_bin.image, img_gray_bin.nrow );
      if ( img_gray_alloc.image ) liberar_matriz_pixels( img_gray_alloc.image, img_gray_alloc.nrow );
      if ( img_gray_rot.image )   liberar_matriz_pixels( img_gray_rot.image, img_gray_rot.nrow );
      if ( img_rgb_crop.image )   liberar_matriz_pixels_colorida( img_rgb_crop.image, img_rgb_crop.nrow );

      liberar_imagem_imread( &img_rgb_orig );
   }

   // Limpeza final de arquivos e arrays
   for ( int i = 0; i < qtd_bin; i++ ) {
      if ( f[i] != NULL ) fclose( f[i] );
   }
   g_free( f );
   g_free( imgs_png );
   free( resp_bin );

   puts( "Processamento das imagens concluído com sucesso!" );

   return n_rejeitadas;
}
//========================================================================================================//









static void copiar_arquivos_correcao_externamente( const InterfaceDados *dados, const CaminhoDiretorio *caminho,
                                                   const char *arquivo_saida )
{
   g_autofree char *nome_arquivo_escola = NULL;
   if ( dados->periodo[0] == 'R' ) {
      nome_arquivo_escola = g_strdup_printf( "Correção Recuperação Final - %s - %s - %s.pdf",
                                             dados->ano, dados->turma, dados->disciplina );
   } else {
      nome_arquivo_escola = g_strdup_printf( "Correção %s Prova - %s_%c - %s - %s.pdf",
                                             dados->prova_sequencia, dados->ano, dados->periodo[0],
                                             dados->turma, dados->disciplina );
   }

   g_autofree char *pasta_provas_escola = g_build_filename( caminho->externo_escola, "Correções", NULL );
   g_autofree char *destino_escola      = g_build_filename( pasta_provas_escola, nome_arquivo_escola, NULL );

   // Garante que a pasta "Provas" exista lá no drive/nuvem da escola
   g_mkdir_with_parents( pasta_provas_escola, 0777 );

   if ( !gio_copiar_arquivo( arquivo_saida, destino_escola ) ) {
      g_printerr( "Erro ao salvar a cópia institucional na pasta Provas da Escola!\n" );
   }
}


static void copiar_arquivos_correcao_nao_presencial( const MapeamentoGabarito *info, const int qtd_linhas,
                                                     const FichaAluno *diario, const InterfaceDados *dados,
                                                     const CaminhoDiretorio *caminho )
{
   g_autofree char *diretorio_imagens = NULL;

   // 1. Construção simplificada: Não precisamos criar o diretório "Notas" separadamente.
   // O g_mkdir_with_parents já cria toda a árvore genealógica de pastas se não existirem.
   if ( dados->periodo[0] == 'R' ) {
      diretorio_imagens = g_build_filename( caminho->externo, "Notas", "Recuperação Final Imagens Corrigidas", NULL );

   } else {
      g_autofree char *pasta_imagens = g_strdup_printf( "%s Prova Imagens Corrigidas", dados->prova_sequencia );
      diretorio_imagens = g_build_filename( caminho->externo, "Notas", pasta_imagens, NULL );
   }

   // 2. Apenas uma chamada de criação de pasta resolve tudo
   if ( g_mkdir_with_parents( diretorio_imagens, 0777 ) != 0 ) {
      g_printerr( "ERRO CRÍTICO: Falha ao criar a hierarquia externa de pastas: %s\n", diretorio_imagens );
      return;
   }

   // 3. Libere o poder do OpenMP! A conversão de PDF para PNG consome muita CPU.
   // Fazer isso em paralelo para 40 alunos economiza dezenas de segundos.
   #pragma omp parallel for schedule(static)
   for ( int i = 0; i < qtd_linhas; i++ ) {

      if ( info[i].status & ( STATUS_PROVA_OK | AVISO_ALUNO_INATIVO ) ) {
         int num_aluno = info[i].num;

         g_autofree char *thread_caminho_pdf = g_strdup_printf( "./dados/temporarios/%.2d.pdf", i );
         g_autofree char *nome_arquivo_png   = g_strdup_printf( "%.2d - %s.png", num_aluno, diario[num_aluno-1].aluno );
         g_autofree char *thread_caminho_png = g_build_filename( diretorio_imagens, nome_arquivo_png, NULL );

         if ( !pdf2png( thread_caminho_pdf, thread_caminho_png, 1.5 ) ) {
            // Em laços OpenMP, evite GTK, mas g_printerr é seguro.
            g_printerr( "[AVISO] Falha ao converter e mover imagem %s\n", thread_caminho_png );
         }
      }
   }

}


static StatusMapeamento validar_prova_escaneada( const MapeamentoGabarito *info, const InterfaceDados *dados,
      const FichaAluno *diario ) {

   // 1º TESTE (Crítico): O número da folha protege o array 'diario'
   if ( info->num <= 0 || info->num > dados->qtd_alunos_total ) {
      return ERRO_NUMERO_ALUNO_INVALIDO;
   }

   // 2º TESTE (Crítico): O ID protege o array 'G' (Gabaritos)
   if ( info->id >= dados->qtd_alunos_ativos ) {
      return ERRO_ID_GABARITO_INVALIDO;
   }

   // 3º TESTE (Regra de Negócio): Se chegou aqui, a memória está segura!
   if ( diario[info->num - 1].ativo == false ) {
      return STATUS_PROVA_OK | AVISO_ALUNO_INATIVO; // Acumula os estados perfeitamente
   }

   return STATUS_PROVA_OK;
}


void corrigir_prova( InterfacePainel *painel, const AppContext *ctx ) {
   if ( !painel || !ctx ) return;

   const InterfaceDados   *dados   = &ctx->dados;
   const FocoCoordenadas  *foco    = &ctx->cascata.foco;
   const CaminhoDiretorio *caminho = &ctx->caminho;
   const FichaAluno       *diario  =  ctx->diario;

   char nome_bin[64];
   nome_base_gabaritos_bin( nome_bin, sizeof( nome_bin ), foco->turma, foco->disciplina, foco->periodo, dados->iprova - 1 );

   g_autofree char *path_resp = g_build_filename( ".", "dados", "gabaritos", dados->ano, dados->escola,
                                "respostas", nome_bin, NULL );

   g_autofree char *path_gab  = g_build_filename( ".", "dados", "gabaritos", dados->ano, dados->escola,
                                "gabaritos", nome_bin, NULL );

   // ====================================================================================
   // 1. VALIDAÇÃO DE SEGURANÇA
   // ====================================================================================
   FILE *fr = g_fopen( path_resp, "rb" );
   if ( !fr ) {
      painel->format_titulo    = meu_gerador_variadico( "⚠ Respostas Não Encontradas" );
      painel->format_subtitulo = meu_gerador_variadico( "O arquivo de escaneamento %s não existe.", nome_bin );
      painel->format_instrucao = meu_gerador_variadico( "Realize o processamento das folhas da turma antes de corrigir." );
      criar_mensagem_painel( AVISO, painel );
      return;
   }

   FILE *fg = g_fopen( path_gab, "rb" );
   if ( !fg ) {
      painel->format_titulo    = meu_gerador_variadico( "⚠ Gabarito Mestre Ausente" );
      painel->format_subtitulo = meu_gerador_variadico( "Nenhum gabarito foi gerado para esta turma ainda." );
      painel->format_instrucao = meu_gerador_variadico( "Acesse a aba de geração de provas e crie o arquivo de gabaritos." );
      criar_mensagem_painel( ERRO, painel );
      fclose( fr );
      return;
   }

   // ====================================================================================
   // 2. LEITURA BINÁRIA
   // ====================================================================================
   int qtd_linhas = contar_registros_binarios( path_resp, sizeof( MapeamentoGabarito ) );
   if ( qtd_linhas <= 0 ) {
      g_printerr( "Aviso: Arquivo de respostas vazio ou corrompido.\n" );
      fclose( fr );
      fclose( fg );
      return;
   }

   g_autofree MapeamentoGabarito *info = g_new0( MapeamentoGabarito, qtd_linhas );
   size_t lidos_resp = fread( info, sizeof( MapeamentoGabarito ), qtd_linhas, fr );
   fclose( fr );

   qsort( info, qtd_linhas, sizeof( MapeamentoGabarito ), comparar_mapeamento_gabarito );

   g_autofree ItemTextoCurto *G = g_new0( ItemTextoCurto, dados->qtd_alunos_ativos );
   size_t lidos_gab = fread( G, sizeof( ItemTextoCurto ), dados->qtd_alunos_ativos, fg );
   fclose( fg );

   if ( lidos_resp != ( size_t )qtd_linhas || lidos_gab != ( size_t )dados->qtd_alunos_ativos ) {
      g_printerr( "Aviso de I/O: Tamanhos lidos não batem perfeitamente com os registrados.\n" );
   }

   // ====================================================================================
   // 3. LAÇO PARALELO BLINDADO
   // ====================================================================================
   #pragma omp parallel for schedule(static)
   for ( int i = 0; i < qtd_linhas; i++ ) {

      StatusMapeamento status = validar_prova_escaneada( &info[i], dados, diario );
      info[i].status = status;

      // Se o status POSSUI (OK) OU POSSUI (INATIVO)
      if ( status & ( STATUS_PROVA_OK | AVISO_ALUNO_INATIVO ) ) {

         // O caminho feliz! Tudo perfeito.
         g_autofree gchar *nome_base = g_strdup_printf( "%.2d", i );
         const char *gab = G[ info[i].id ].str;

         int nota = imagens_corrigidas( gab, &info[i], ctx, nome_base );
         info[i].nota = nota;

      } else {
         // O caminho de tratamento de erros
         int thread_id = omp_get_thread_num();

         if ( status & ERRO_NUMERO_ALUNO_INVALIDO ) {
            g_printerr( "Alerta [Thread %d]: Número da prova (%d) corrompido ou fora dos limites.\n",
                        thread_id, info[i].num );

         } else if ( status & ERRO_ID_GABARITO_INVALIDO ) {
            g_printerr( "Alerta [Thread %d]: O ID lido (%d) na prova Nº %d não possui gabarito correspondente.\n",
                        thread_id, info[i].id, info[i].num );
         }
      }

   }

   // ====================================================================================
   // 4. COMPILAÇÃO PARALELA (LaTeX Multi-Core)
   // ====================================================================================
   g_pdflatex_parallel( "./dados/temporarios" );

   if ( dados->naopresencial ) {
      copiar_arquivos_correcao_nao_presencial( info, qtd_linhas, diario, dados, caminho );
   }

   // ====================================================================================
   // 5. UNIFICAÇÃO DOS PDFS E LIMPEZA NATIVA
   // ====================================================================================
   g_auto(GStrv) arquivos_pdf = g_new0( char *, qtd_linhas + 1 );
   int qtd_sucessos = 0;

   for ( int i = 0; i < qtd_linhas; i++ ) {
      if ( info[i].status & ( STATUS_PROVA_OK | AVISO_ALUNO_INATIVO ) ) {
         arquivos_pdf[qtd_sucessos] = g_strdup_printf( "%.2d.pdf", i );
         qtd_sucessos++;
      }
   }

   g_autofree char *nome_arquivo  = g_strdup_printf( "Correção_%d.pdf", dados->iprova );
   g_autofree char *arquivo_saida = g_build_filename( caminho->relatorios, nome_arquivo, NULL );

   // ---------------------------------------------------------------------------------
   // BLINDAGEM CONTRA FALSOS POSITIVOS (O seu toque de mestre de volta)
   // Removemos o arquivo anterior preventivamente. Se o pdfunite falhar,
   // nada será criado no lugar, e o passo 6 capturará o erro perfeitamente.
   // ---------------------------------------------------------------------------------
   g_remove( arquivo_saida );

   // Unifica os PDFs enviando DIRETAMENTE para o destino final
   g_pdfunite( "./dados/temporarios/", (const char **)arquivos_pdf, qtd_sucessos, arquivo_saida );

   // Limpeza nativa paralela
   #pragma omp parallel for schedule(static)
   for ( int i = 0; i < qtd_linhas; i++ ) {
      if ( info[i].status & ( STATUS_PROVA_OK | AVISO_ALUNO_INATIVO ) ) {
         g_autofree gchar *nome_base = g_strdup_printf( "%.2d", i );
         apagar_arquivos_temporarios_latex_nativamente( "./dados/temporarios/", nome_base, 5 );
      }
   }

   // ====================================================================================
   // 6. EXIBIÇÃO AUTOMÁTICA E FEEDBACK NA INTERFACE
   // ====================================================================================
   // Como apagamos o arquivo velho no passo 5, se este arquivo existe agora,
   // temos 100% de certeza de que ele acabou de ser gerado pelo g_pdfunite!
   if ( g_file_test( arquivo_saida, G_FILE_TEST_EXISTS ) ) {

      // Fluxo adicional: Se precisar exportar, copiamos o arquivo recém-criado
      if ( dados->expor ) {
         copiar_arquivos_correcao_externamente( dados, caminho, arquivo_saida );
      }

      // Abre o PDF APENAS se temos 100% de certeza do sucesso!
      g_xdg_open( arquivo_saida );

      painel->format_titulo    = meu_gerador_variadico( "✔ Correção Finalizada" );
      painel->format_subtitulo = meu_gerador_variadico( "%d provas unificadas com sucesso.", qtd_sucessos );
      painel->format_instrucao = meu_gerador_variadico( "As imagens corrigidas estão prontas no diretório base." );
      criar_mensagem_painel( SUCESSO, painel );

   } else {
      painel->format_titulo    = meu_gerador_variadico( "✘ Falha na Geração do PDF" );
      painel->format_subtitulo = meu_gerador_variadico( "Ocorreu um erro ao unificar os arquivos." );
      painel->format_instrucao = meu_gerador_variadico( "Verifique se o LaTeX apresentou erros ou se o pdfunite falhou." );
      criar_mensagem_painel( ERRO, painel );
   }

}






